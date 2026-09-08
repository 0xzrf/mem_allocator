#ifndef mem_alloc_bins_h
#define mem_alloc_bins_h

#include "types.h"
#include "chunk.h"
#include <stdint.h>

// this is the MIN_SIZE for the mem_chunk struct
#define MIN_SIZE          32
#define MALLOC_ALIGN      (SIZE_T * 2)
#define MALLOC_ALIGN_MASK (MALLOC_ALIGN - 1)

#define MAX_FAST_BIN_SIZE 80
#define NBINS_SMALL       32
#define NBINS_LARGE       64
#define NBINS             (NBINS_SMALL + NBINS_LARGE)
#define MIN_LARGE_SIZE    (NBINS_SMALL * MALLOC_ALIGN)
#define MAX_FASTBIN_SIZE  80
#define NFASTBIN          ((MAX_FASTBIN_SIZE >> 4) + 1)
#define UNSORTED_BIN_IDX  0
#define LARGE_SHIFT       9
#define SUBBINS_LOG       2
#define SUBBINS           (1u << SUBBINS_LOG)
#define BINMAP_BITS       64
#define BINMAP_WORDS      ((NBINS + BINMAP_BITS - 1) / BINMAP_BITS)

typedef struct bin {
    struct bin *next;
    struct bin *back;
} bin;

typedef struct bin *binptr;

typedef uint64_t binmap_word;

#define bm_word(i)           ((unsigned) (i) / BINMAP_BITS)
#define bm_bit(i)            (((binmap_word) 1) << ((unsigned) (i) % BINMAP_BITS))
#define bm_mark(map, i)      ((map)[bm_word(i)] |= bm_bit(i))
#define bm_clear(map, i)     ((map)[bm_word(i)] &= ~bm_bit(i))
#define bm_is_marked(map, i) (((map)[bm_word(i)] & bm_bit(i)) != 0)

#define BIN_NONE ((unsigned) -1)

static inline void bm_init(binmap_word *map) {
    for (unsigned w = 0; w < BINMAP_WORDS; w++)
        map[w] = 0;
}

static inline unsigned bin_find_next_nonempty(const bin *arr, binmap_word *map, unsigned from) {
    unsigned i = from;

    while (i < NBINS) {
        unsigned w = bm_word(i);

        /* Keep only the bits at or above i -- never look backwards.
         * bm_bit(i) - 1 is all ones BELOW i; ~ flips it to all ones AT/ABOVE. */
        binmap_word word = map[w] & ~(bm_bit(i) - 1);

        if (word == 0) { /* 64 bins ruled out by one compare */
            i = (w + 1) * BINMAP_BITS;
            continue;
        }

        /* Jump straight to the lowest set bit instead of testing bit by bit. */
        i = w * BINMAP_BITS + (unsigned) __builtin_ctzll(word);
        if (i >= NBINS)
            return BIN_NONE; /* bits past the last real bin */

        if (arr[i].next == &arr[i]) { /* marked, but actually empty */
            bm_clear(map, i);         /* THE ONLY place clearing happens */
            i++;
            continue;
        }
        return i;
    }
    return BIN_NONE;
}

static inline unsigned highest_set_bit(size_t x) {
    return (unsigned) (63 - __builtin_clzll((unsigned long long) x));
}

static inline unsigned largebin_ix(size_t size) {
    unsigned m = highest_set_bit(size);
    unsigned octave = m - LARGE_SHIFT;
    unsigned subbin = (unsigned) ((size >> (m - SUBBINS_LOG)) & (SUBBINS - 1));
    unsigned i = octave * SUBBINS + subbin;

    if (i >= NBINS_LARGE)
        i = NBINS_LARGE - 1; /* catch-all top bin */
    return NBINS_SMALL + i;
}

// macros
/* Small sizes get an exact-size bin; large sizes get a log-spaced range bin.
 * Bin 0 is the unsorted bin and bin 1 is unused, which is why the smallest
 * possible chunk (MIN_SIZE = 32) lands at index 2. */
#define bin_ix(size) ((size) < MIN_LARGE_SIZE ? (unsigned) ((size) / MALLOC_ALIGN) : largebin_ix((size)))

/* Fastbins are a SEPARATE array with its own, smaller index space.
 * Reusing bin_ix here would index past the end: bin_ix(80) == 5 while the
 * array is NFASTBIN long. */
#define fastbin_ix(size) ((unsigned) ((size) / MALLOC_ALIGN))

#define bin_at_size(bin, size) (state_ptr->bin[bin_ix((size))])
#define is_bin_empty(bin, i)   (state_ptr->bin[(i)].next == &state_ptr->bin[(i)])
#define unsorted_bins()        (&state_ptr->bins[UNSORTED_BIN_IDX])

/* Can `c` be cut into a chunk of payload `s` plus a legal leftover chunk?
 * The leftover needs its own header, so it costs s + CHUNK_OVERHEAD out of c,
 * and what remains must still be at least MIN_PAYLOAD. */
#define can_split(c, s) (chunk_size(c) >= (s) + CHUNK_OVERHEAD + MIN_PAYLOAD)

/* Shrink `c` to payload `s` and hand back the leftover chunk.
 * Does NO list surgery -- the caller unlinks `c` first and files the
 * remainder wherever it belongs (the unsorted bin). */
#define split_chunk(c, s, out_rem)                                                                 \
    do {                                                                                           \
        size_t size_before_ = chunk_size((c));                                                     \
        set_size((c), (s));                                                                        \
        mchunkptr rem_ = next_chunk(c);                                                            \
        /* what is left after taking s payload AND the remainder's header */                       \
        set_size(rem_, size_before_ - (s) - CHUNK_OVERHEAD);                                       \
        set_prev_in_use(rem_); /* c is in use now */                                               \
        set_foot(rem_, chunk_size(rem_));                                                          \
        (out_rem) = rem_;                                                                          \
    } while (0)

#define insert_at_head(bin, i, c)                                                                  \
    do {                                                                                           \
        binptr head = &state_ptr->bin[(i)];                                                        \
        (c)->back = head;                                                                          \
        (c)->next = head->next;                                                                    \
        head->next->back = (binptr) (chunk2mem((c)));                                              \
        head->next = (binptr) (chunk2mem((c)));                                                    \
    } while (0)

#define init_bin(bin, i)                                                                           \
    (state_ptr->bin[(i)].next = state_ptr->bin[(i)].back = &state_ptr->bin[(i)])

#define remove_from_bin(c)                                                                         \
    do {                                                                                           \
        (c)->next->back = (c)->back;                                                               \
        (c)->back->next = (c)->next;                                                               \
    } while (0)

#define request2size(req)                                                                          \
    ((((req) + SIZE_T + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK) < MIN_SIZE                        \
         ? MIN_SIZE                                                                                \
         : ((req) + SIZE_T + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

#endif

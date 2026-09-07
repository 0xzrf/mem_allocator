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

typedef uint64_t binmap_word;

#define bm_word(i)           ((unsigned) (i) / BINMAP_BITS)
#define bm_bit(i)            (((binmap_word) 1) << ((unsigned) (i) % BINMAP_BITS))
#define bm_mark(map, i)      ((map)[bm_word(i)] |= bm_bit(i))
#define bm_clear(map, i)     ((map)[bm_word(i)] &= ~bm_bit(i))
#define bm_is_marked(map, i) (((map)[bm_word(i)] & bm_bit(i)) != 0)

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

typedef struct bin {
    struct bin *next;
    struct bin *back;
} bin;

typedef struct bin *binptr;

// macros
#define bin_ix(size) ((size) < MIN_LARGE_SIZE ? (size) / MALLOC_ALIGN : large_bin_ix((size)))

#define bin_at_size(bin, size) (state_ptr->bin[bin_ix((size))])
#define is_bin_empty(bin, i)   (state_ptr->bin[(i)].next == &state_ptr->bin[(i)])
#define unsorted_bins()        (&state_ptr->bins[1])

#define split_chunk(c, s)                                                                          \
    do {                                                                                           \
        size_t size_before = chunk_size((c));                                                      \
        size_t new_chunk_size = size_before - (s);                                                 \
        set_size((c), s);                                                                          \
        mchunkptr new_chunk = next_chunk(c);                                                       \
        set_size(new_chunk, new_chunk_size);                                                       \
        set_prev_in_use(new_chunk);                                                                \
        new_chunk->back = (c)->back;                                                               \
        new_chunk->next = (c)->next;                                                               \
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

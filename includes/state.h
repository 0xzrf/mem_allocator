#ifndef mem_alloc_state_h
#define mem_alloc_state_h

#include "stddef.h"
#include "chunk.h"
#include "bins.h"

/* `addr` is a HINT, not a guarantee -- the kernel may place the mapping
 * anywhere. Callers MUST compare the result against what they asked for. */
#define mmap_at_offset(addr)                                                                       \
    mmap((addr), PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#define mmap_bytes_at(addr, len)                                                                   \
    mmap((addr), (len), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)

/* Whole pages needed to hold `bytes`. */
#define pages_for(bytes) ((((bytes) + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE)

#define MAX_FREE_BIT             0x1
#define any_bin_free()           (state_ptr->max_free_bin & MAX_FREE_BIT)
#define set_any_bin_free()       (state_ptr->max_free_bin = state_ptr->max_free_bin | MAX_FREE_BIT)
#define top_empty()              (state_ptr->top_allocation == NULL)
#define set_chunk_free()         (state_ptr->max_free_bin = state_ptr->max_free_bin | MAX_FREE_BIT)
#define bump_top_to_offset(t, s) ((t) = (mchunkptr) ((char *) (t) + (s) + (2 * SIZE_T)))

typedef struct {
    size_t max_free_bin; // the lower bit is there to signal the presence of any free chunks in bin
    mchunkptr top_allocation; // used to fetch data and merge freed data when nothing else is free
    bin bins[NBINS];
    bin fastbins[NFASTBIN];
    binmap_word binmap[BINMAP_WORDS];
} mstate;

#define mark_bin(i)                                                                                \
    do {                                                                                           \
        if ((i) >= 2)                                                                              \
            bm_mark(state_ptr->binmap, (i));                                                       \
    } while (0)
#define unmark_bin(i)           bm_clear(state_ptr->binmap, (i))
#define bin_is_marked(i)        bm_is_marked(state_ptr->binmap, (i))
#define next_nonempty_bin(from) bin_find_next_nonempty(state_ptr->bins, state_ptr->binmap, (from))

typedef mstate *mstateptr;
#endif

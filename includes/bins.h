#ifndef mem_alloc_bins_h
#define mem_alloc_bins_h

#include "types.h"
#include "chunk.h"
#define MIN_SIZE          32
#define MALLOC_ALIGN      (SIZE_T * 2)
#define MALLOC_ALIGN_MASK (MALLOC_ALIGN - 1)

#define MAX_FAST_BIN_SIZE 80
#define NBINS_SMALL       32
#define NBINS_LARGE       40
#define NBINS             (NBINS_SMALL + NBINS_LARGE)
#define MIN_LARGE_SIZE    (NBINS_SMALL * MALLOC_ALIGN)
#define MAX_FASTBIN_SIZE  80

typedef struct bin {
    struct bin *next;
    struct bin *back;
} bin;

typedef struct bin *binptr;

// macros
// large bin not supported yet
#define bin_ix(size) ((size) < MIN_LARGE_SIZE ? (size) / MALLOC_ALIGN : 0)

#define bin_at_size(bin, size) (state_ptr->bin[bin_ix((size))])
#define is_bin_empty(i)        (state_ptr->bins[i].next == state_ptr->bins[i].back)
#define unsorted_bins()        (&state_ptr->bins[1])

#define insert_at_head(bin, i, c)                                                                  \
    do {                                                                                           \
        binptr head = &state_ptr->bin[(i)];                                                        \
        (c)->back = head;                                                                          \
        (c)->next = head->next;                                                                    \
        head->back = (c)->next;                                                                    \
        head->next = (c)->next;                                                                    \
    } while (0)

#define init_bin(bin, i) (state_ptr->bin[(i)].next = state_ptr->bin[(i)].back)

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

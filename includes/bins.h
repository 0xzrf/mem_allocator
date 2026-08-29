#ifndef mem_alloc_bins_h
#define mem_alloc_bins_h

#include "types.h"
#include "chunk.h"
#define MIN_SIZE 32
#define MALLOC_ALIGN (SIZE_T * 2)
#define MALLOC_ALIGN_MASK (MALLOC_ALIGN - 1)

#define MAX_FAST_BIN_SIZE 80
#define NBINS_SMALL 32
#define NBINS_LARGE 40
#define NBINS (NBINS_SMALL + NBINS_LARGE)
#define MIN_LARGE_SIZE (NBINS_SMALL * MALLOC_ALIGN)

typedef struct bin {
    mchunkptr fd;
    mchunkptr bk;
};

// macros
// large bin not supported yet
#define bin_ix(size) ((size) < MIN_LARGE_SIZE ? (size) / MALLOC_ALIGN : 0)

#define bin_at_size(size) (state_ptr->bins[bin_ix((size))])
#define is_bin_empty(i) (state_ptr->bins[i].fd == state_ptr->bins[i].bk)
#define insert_at_head(i, c) (do {  \
    head = &state_ptr->bins[i];     \
    c.bk = head                     \
    c.fd = head.fd                  \
    head.fd.bk = c                  \
    head.fd = c                     \ 
})

#define request2size(req)  \
    ( (((req) + SIZE_T + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK) < MIN_SIZE  \
    ? MIN_SIZE \
    : ((req) + SIZE_T + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK )

#endif
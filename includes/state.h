#ifndef mem_alloc_state_h
#define mem_alloc_state_h

#include "stddef.h"
#include "chunk.h"
#include "bins.h"

#define mmap_at_offset(size)                                                                       \
    mmap((size), PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)

#define MAX_FREE_BIT     0x1
#define any_bin_free()   (state_ptr->max_free_bin & MAX_FREE_BIT)
#define top_empty()      (state_ptr->top_allocation == NULL)
#define set_chunk_free() (state_ptr->max_free_bin = state_ptr->max_free_bin | MAX_FREE_BIT)

typedef struct {
    size_t max_free_bin; // the lower bit is there to signal the presence of any free chunks in bin
    mchunkptr top_allocation; // used to fetch data and merge freed data when nothing else is free
    bin bins[NBINS];
    bin fastbins[MAX_FAST_BIN >> 4]
} mstate;

typedef mstate *mstateptr;
#endif

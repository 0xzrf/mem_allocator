#ifndef mem_alloc_state_h
#define mem_alloc_state_h

#include "stddef.h"
#include "chunk.h"

typedef mstate *mstateptr;

#define MAX_FREE_BIT 0x1
#define any_bin_free() (state_ptr->max_free_bin & MAX_FREE_BIT)
#define top_empty() (state_ptr->top_allocations == NULL)

typedef struct {
    size_t max_free_bin; // the lower bit is there to signal the presence of any free chunks in bin
    mchunkptr *top_allocation; // used to fetch data and merge freed data when nothing else is free
} mstate;

#endif
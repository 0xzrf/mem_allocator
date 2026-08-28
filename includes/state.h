#ifndef mem_alloc_state_h
#define mem_alloc_state_h

#include "stddef.h"
#include "chunk.h"

typedef struct mstate *mstateptr;

typedef struct {
    size_t max_free_bin; // the lower bit is there to signal the presence of any free chunks in bin
    mchunkptr top_allocation; // used to fetch data and merge freed data when nothing else is free
} mstate;

#endif
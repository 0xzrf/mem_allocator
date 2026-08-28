#ifndef mem_alloc_bins_h
#define mem_alloc_bins_h

#include "types.h"

#define MIN_SIZE 32
#define MALLOC_ALIGN (SIZE_T * 2)
#define MALLOC_ALIGN_MASK (MALLOC_ALIGN - 1)

#define request2size(req)  \
    ( (((req) + SIZE_T + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK) < MIN_SIZE  \
    ? MIN_SIZE \
    : ((req) + SIZE_T + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK )

#endif
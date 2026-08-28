#ifndef mem_alloc_chunk_h
#define mem_alloc_chunk_h

#include "./types.h"

typedef struct mem_chunk *mchunkptr;

// macros
#define chunk2mem(c) ((void *)((char *)c + 2 * INTERNAL_SIZE_T))
#define mem2chunk(m) ((void *) ((char *)m - 2 * INTERNAL_SIZE_T))

#define PREV_IN_USE_BIT 0x1
#define MMAPED_BIT 0x2
#define FLAG_BITS 0xf

// setters
#define set_size(p, s) ((p)->size = (s))

// state_ptr->top_allocation specific
#define bump_top_to_offset(s) (state_ptr->top_allocation = (mchunkptr)((char *)state_ptr->top_allocation + s + 2 * SIZE_T))

#define prev_in_use(c) (c->size & PREV_IN_USE_BIT)
// This is fine as long as we don't compare it like is_mmaped == 1. truthy is when > 0
#define is_mmaped(c) (c->size & MMAPED_BIT) 
#define chunk_size(c) ((c)->size & ~FLAG_BITS)

struct mem_chunk {
   INTERNAL_SIZE_T prev_size;
   // the lower 4 bits are free to be used, since the size is a multiple of 16
   // hence, we use the following as flags: [mmaped] [prev_in_use]
   INTERNAL_SIZE_T size;
   mchunkptr fd;
   mchunkptr bk;
};

#endif
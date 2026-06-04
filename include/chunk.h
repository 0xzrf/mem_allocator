#ifndef alloc_chunk_h
#define alloc_chunk_h

#include "common.h"
#include "bins.h"
#include <stdbool.h>

// Helper macros
#define mem_2_chunk(mem) ((chunk_ptr)((BYTE_PTR)(mem) - 2 * SIZE_SZ))
#define chunk_2_mem(mem) ((chunk_ptr)((BYTE_PTR)(mem) + 2 * SIZE_SZ))
/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */
#define PREV_INUSE 0x1
#define IS_MMAPPED 0x2

#define SIZE_BITS (PREV_INUSE | IS_MMAPPED)

/* extract inuse bit of previous chunk */
#define prev_inuse(p) ((p)->size & PREV_INUSE)

#define prev_chunk(p) ((chunk_ptr)((BYTE_PTR)(p) - ((p)->prev_size) ))

#define chunksize(p) ((p)->size & ~(SIZE_BITS))

/*
 * This is a chunk header, which is mainly used for double-linked list
 * We use knuth's technique to write to prev_size, when the prev. chunk
 * is freed, and manipulate the first bit of the `size` field, which is mainly
 * used to see if the prev. chunk is in use or not
 *
 * The `data` field is only used when the chunk is free
 */
struct chunk_header {
    INTERNAL_SIZE_T prev_size;
    INTERNAL_SIZE_T size;

    struct chunk_header* data;
    struct chunk_header* next_chunk;
};

typedef struct chunk_header *chunk_ptr;

#define has_any_chunk(malloc_state) ((malloc_state)->has_chunk)

struct malloc_state {
    chunk_ptr top; // this is the location which will be allocated memory(and given back to the user) if bins are empty
    bool has_chunk;

    INTERNAL_SIZE_T max_fast;

    chunk_ptr bins[NBINS];
    chunk_ptr fast_bins[NFASTBINS]
};

typedef struct malloc_state *mstate;

#endif

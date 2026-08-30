#ifndef mem_alloc_chunk_h
#define mem_alloc_chunk_h

#include "./types.h"
#include "bins.h"
typedef struct mem_chunk *mchunkptr;

// macros
#define chunk2mem(c) ((void *) ((char *) c + 2 * SIZE_T))
#define mem2chunk(m) ((void *) ((char *) m - 2 * SIZE_T))

#define PREV_IN_USE_BIT 0x1
#define MMAPED_BIT      0x2
#define FLAG_BITS       (PREV_IN_USE_BIT | MMAPED_BIT)

// setters
#define set_size(p, s)     ((p)->size = (s))
#define set_prev_in_use(p) ((p)->size = (p)->size | PREV_IN_USE_BIT)
#define set_mmaped(p)      ((p)->size = (p)->size | MMAPED_BIT)
#define set_foot(c, s)     (((mchunkptr) ((char *) (c) + s + (2 * SIZE_T)))->prev_size = (s))

// helper
#define is_mmaped(c)   ((c)->size & MMAPED_BIT)
#define prev_in_use(c) ((c)->size & PREV_IN_USE_BIT)
#define chunk_size(c)  ((c)->size & ~FLAG_BITS)
#define next_chunk(c)  ((mchunkptr) ((char *) c + 2 * SIZE_T))
#define prev_chunk(c)  ((mchunkptr) ((char *) (c) - ((c)->prev_size + 2 * SIZE_T)))

#define bump_top_to_offset(t, s) ((t) = (mchunkptr) ((char *) (t) + (s) + (2 * SIZE_T)))

#define coalece(prev_chunk, join_chunk)                                                            \
    do {                                                                                           \
        prev_chunk->next = join_chunk->next;                                                       \
        prev_chunk->size = join_chunk->size + prev_chunk->size;                                    \
    } while (0)

struct mem_chunk {
    INTERNAL_SIZE_T prev_size;
    // the lower 4 bits are free to be used, since the size is a multiple of 16
    // hence, we use the following as flags: [mmaped] [prev_in_use]
    INTERNAL_SIZE_T size;
    binptr next;
    binptr back;
};

#endif

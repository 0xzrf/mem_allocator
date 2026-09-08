#ifndef mem_alloc_chunk_h
#define mem_alloc_chunk_h

#include "./types.h"
#include "bins.h"
typedef struct mem_chunk *mchunkptr;

// macros
#define chunk2mem(c) ((void *) ((char *) (c) + 2 * SIZE_T))
/* mem2chunk yields a CHUNK pointer, so it can be used inline:
 *     chunk_size(mem2chunk(p))
 * Returning void* forced a temporary variable at every call site. */
#define mem2chunk(m) ((mchunkptr) ((char *) (m) - 2 * SIZE_T))

/* THIS ALLOCATOR'S SIZE CONVENTION
 *   chunk->size  = PAYLOAD bytes (what the user can use)
 *   stride to the next chunk = size + CHUNK_OVERHEAD
 *
 * So a chunk of `size` occupies size + 16 bytes in total. Every place that
 * walks or merges chunks has to add CHUNK_OVERHEAD; forgetting it is a
 * 16-byte-per-chunk drift that shows up as overlapping chunks.
 */
#define CHUNK_OVERHEAD (2 * SIZE_T)

/* A FREE chunk stores next/back in its payload area, so the payload can never
 * be smaller than two pointers. MIN_SIZE (32) is the whole struct; the payload
 * floor is that minus the header. */
#define MIN_PAYLOAD (MIN_SIZE - CHUNK_OVERHEAD)

#define PREV_IN_USE_BIT 0x1
#define MMAPED_BIT      0x2
#define FLAG_BITS       (PREV_IN_USE_BIT | MMAPED_BIT)

// setters
#define set_size(p, s)       ((p)->size = ((p)->size & FLAG_BITS) | (s))
#define set_prev_in_use(p)   ((p)->size = (p)->size | PREV_IN_USE_BIT)
#define unset_prev_in_use(p) ((p)->size = (p)->size & ~PREV_IN_USE_BIT)
#define set_foot(c, s)       (((mchunkptr) ((char *) (c) + (s) + CHUNK_OVERHEAD))->prev_size = (s))
#define set_mmaped(c)        ((c)->size = (c)->size | MMAPED_BIT)
#define unset_mmaped(c)      ((c)->size = (c)->size & ~MMAPED_BIT)

// helper
#define is_mmaped(c)       ((c)->size & MMAPED_BIT)
#define prev_in_use(c)     ((c)->size & PREV_IN_USE_BIT)
#define chunk_size(c)      ((c)->size & ~FLAG_BITS)
#define next_chunk(c)      ((mchunkptr) ((char *) (c) + chunk_size(c) + CHUNK_OVERHEAD))
#define prev_chunk(c)      ((mchunkptr) ((char *) (c) - (c)->prev_size - CHUNK_OVERHEAD))
#define next_chunk_free(c) (!prev_in_use(next_chunk(next_chunk(c))))

/* Merge join_chunk (the HIGHER one) into prev_chunk (the LOWER one).
 *
 * The merged payload is  a + b + CHUNK_OVERHEAD, not a + b: join_chunk's
 * 16-byte header stops being a header and becomes payload of the merged chunk.
 *
 * chunk_size() masks the flags off both operands -- adding raw ->size words
 * would sum the flag bits into the size. prev_chunk keeps its OWN flags.
 *
 * The CALLER must already have unlinked join_chunk from its bin, or that bin
 * keeps a pointer into the middle of the merged chunk. */
#define coalece(prev_chunk, join_chunk)                                                            \
    do {                                                                                           \
        size_t merged_ = chunk_size(prev_chunk) + chunk_size(join_chunk) + CHUNK_OVERHEAD;         \
        set_size((prev_chunk), merged_);                                                           \
    } while (0)

struct mem_chunk {
    INTERNAL_SIZE_T prev_size;
    // the lower 4 bits are free to be used, since the size is a multiple of 16
    // hence, we use the following as flags: [mmaped] [prev_in_use]
    INTERNAL_SIZE_T size;
    struct bin *next;
    struct bin *back;
};

#endif

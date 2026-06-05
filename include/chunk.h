#ifndef alloc_chunk_h
#define alloc_chunk_h

#include "bins.h"
#include "common.h"

// Helper macros
#define mem_2_chunk(mem) ((chunk_ptr)((BYTE_PTR)(mem) - 2 * SIZE_SZ))
#define chunk_2_mem(mem) ((void *)((BYTE_PTR)(mem) + 2 * SIZE_SZ))
/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */
#define PREV_INUSE 0x1
#define IS_MMAPED 0x2

#define SIZE_BITS (PREV_INUSE | IS_MMAPED)

/* extract inuse bit of previous chunk */
#define prev_inuse(p) ((p)->size & PREV_INUSE)
#define is_mmapd(p) ((p)->size & IS_MMAPED)

#define set_mmapd(p) ((p)->size |= IS_MMAPED)

#define prev_chunk(p) ((chunk_ptr)((BYTE_PTR)(p) - ((p)->prev_size)))

#define chunksize(p) ((p)->size & ~(SIZE_BITS))
// set every bit and preserve the last flag bit
#define set_chunksize(p, s) ((p)->size = s | ((p)->size & ~ANYCHUNK_BIT))
#define chunk_at_offset(p, s) ((chunk_ptr)(((char *)(p)) + (s)))

#define unlink(p, new_size, next_chunk)                                        \
  do {                                                                         \
    p->size = new_size;                                                        \
    p->next_chunk = next_chunk;                                                \
  } while (0)

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

  void *data; // pointer to the actual data
  struct chunk_header *next_chunk;
};

typedef struct chunk_header *chunk_ptr;

#define ANYCHUNK_BIT (1U)
// fetch and set the last bit of malloc_state->max_fast
#define has_any_chunk(ms) ((ms)->max_fast & ANYCHUNK_BIT)
#define set_any_chunk(ms) ((ms)->max_fast |= ANYCHUNK_BIT)

#define FASTCHUNK_BIT (2U)
#define LAST_2_BITS_SET (ANYCHUNK_BIT | FASTCHUNK_BIT)
// fetch the 2nd last bit of malloc_state->max_fast
#define have_fastchunk(ms) ((ms)->max_fast & FASTCHUNK_BIT)
// sets the last 2 bits of malloc_state->max_fast to 1
#define set_fastchunk(ms) ((ms)->max_fast |= LAST_2_BITS_SET)
// opposite of set_fast_chunk
#define clear_fastchunk(ms) ((ms)->max_fast &= ~(FASTCHUNK_BIT))

// set every value, except the last 2 bits to the rounded off value of size(s)
// (using request_2_size)
#define set_max_fast(ms, s)                                                    \
  (ms)->max_fast =                                                             \
      ((s) == 0) ? SMALLBIN_WIDTH                                              \
                 : request_2_size(s) | ((ms)->max_fast & ~LAST_2_BITS_SET)
// fetch the actual value of get_max_fast(excluding the first 3 bits)
#define get_max_fast(ms) ((ms)->max_fast & ~LAST_2_BITS_SET)

/*
 * This struct contains all the necessary informations needed for the allocation
 * for malloc
 */
struct malloc_state {
  chunk_ptr top; // this is the location which will be allocated memory(and
                 // given back to the user) if bins are empty

  INTERNAL_SIZE_T max_fast;

  chunk_ptr bins[NBINS];

  chunk_ptr fast_bins[NFASTBINS];
};

typedef struct malloc_state *mstate;

#endif

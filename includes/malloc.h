#ifndef mem_alloc_malloc_h
#define mem_alloc_malloc_h

#include "types.h"
#include "state.h"
#include "bins.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"

#define panic(fmt, ...)                                       \
    do {                                                      \
        fprintf(stderr, "panic: %s:%d: %s: " fmt "\n",        \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        abort();                                              \
    } while (0)

#define PAGE_SIZE (1 << 15)

void * dl_malloc(size_t);
void dl_free(void *);

static void init_state(void);
static void *fetch_mem_from_top(size_t);

#ifdef TEST
// Test-only hooks. Compiled in by `make test` (which passes -DTEST to BOTH the
// test binary and the objects in src/), never in a normal or release build.
void reset_mem_state(void);

/* Read-only probes so tests can inspect bin/binmap state without the
 * allocator having to expose state_ptr. */
unsigned test_bin_ix(size_t size);
int      test_bin_is_marked(unsigned i);
unsigned test_next_nonempty_bin(unsigned from);
int      test_bin_is_empty(unsigned i);
size_t   test_bin_count(unsigned i);
size_t   test_largest_free(void);
#endif

#endif
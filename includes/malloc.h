#ifndef mem_alloc_malloc_h
#define mem_alloc_malloc_h

#include "types.h"
#include "state.h"
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

#define PAGE_SIZE 1 << 15

void * dl_malloc(size_t);
void dl_free(void *);

static void init_state();
static void *fetch_mem_from_top();

#endif
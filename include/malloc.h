#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "chunk.h"

#define bin_at(i) (malloc_state->bins[i])

void *malloc(size_t);

static void init_malloc_state();
static void *use_top(size_t);
static void *get_mem_from_os(size_t);

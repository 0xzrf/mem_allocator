#include "chunk.h"
#include "common.h"
#include "debug.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#define bin_at(m, i)                                                           \
  ((chunk_ptr)((char *)&((m)->bins[(i) << 1]) - (SIZE_SZ << 1)))

#define get_malloc_state() (&(malloc_state))
#define initial_top(ms) (unsorted_bin(ms))
void *malloc(size_t);
void free(void *);

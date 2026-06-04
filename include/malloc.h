#include "chunk.h"
#include "common.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define bin_at(ms, i) (ms->bins[i])
#define get_malloc_state() (&(malloc_state))

void *malloc(size_t);

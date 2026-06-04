#include "chunk.h"
#include "common.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define bin_at(i) (malloc_state->bins[i])

void *malloc(size_t);

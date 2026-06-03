#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "chunk.h"

void *malloc(size_t);

static void init_malloc_state();
static void *get_mem_from_os(size_t);

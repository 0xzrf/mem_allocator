#ifndef malloc_h
#define malloc_h

#include <sys/mman.h>
#include <stdio.h>
#include "common.h"

void *malloc(size_t);

static void *get_mem_from_os(size_t);

#endif

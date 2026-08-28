#ifndef mem_alloc_malloc_h
#define mem_alloc_malloc_h

#include "types.h"

static void * dl_malloc(usize_t);
static void dl_free(void *);

#endif
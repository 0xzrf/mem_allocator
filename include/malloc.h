#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"

void *malloc(size_t);

static void *get_mem_from_os(size_t);

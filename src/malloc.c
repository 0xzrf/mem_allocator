#include "malloc.h"
#include "state.h"

static mstate memory_state;
static mstateptr state_ptr = NULL;

static void * dl_malloc(size_t size) {}

static void dl_free(void *) {}
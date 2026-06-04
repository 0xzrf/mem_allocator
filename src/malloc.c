#include "../include/malloc.h"
#include <string.h>

static mstate malloc_state;

/* dlmalloc-style empty-bin sentinels: each bin list head lives here */
static struct chunk_header bin_heads[NBINS];

static void init_malloc_state(void);
static void *use_top(size_t size);
static void *get_mem_from_os(size_t size);

void *malloc(size_t size) {
  printf("Allocating memory\n");

  size_t normalized_size = request_2_size(size);

  /*
   * we need to check to see if the bins are populated or not(from malloc_state)
   * If not, the path would be to check the top(wilderness) chunk, which will
   * have additional memory allocated to it
   *
   * This will be called when the malloc is first called, and later populate the
   * bins when the program frees the memory allocated by this malloc
   */

  if (malloc_state == NULL) {
    static struct malloc_state m;
    memset(&m, 0, sizeof(m));
    malloc_state = &m;
  }

  if (!has_any_chunk(malloc_state)) {
    if (malloc_state->max_fast == 0) {
      init_malloc_state();
    }
    return use_top(normalized_size);
  }

  // TODO: Get mem from bins
  return get_mem_from_os(size);
}

static void init_malloc_state(void) {
  chunk_ptr bin;
  void *arena;

  for (size_t i = 0; i < NBINS; i++) {
    /* point bin slot at its sentinel header, then circular empty list */
    malloc_state->bins[i] = &bin_heads[i];
    bin = bin_at(i);
    bin->prev_size = 0;
    bin->size = 0;
    bin->data = bin;
    bin->next_chunk = bin; // bin == bin.data == bin.next_chunk => empty bin
  }

  /* wilderness (top) chunk: one mmap'd arena with header at the base */
  arena = get_mem_from_os(SYS_ALLOC_PAGE_SIZE);
  malloc_state->top = (chunk_ptr)arena;
  malloc_state->top->prev_size = 0;
  malloc_state->top->size = SYS_ALLOC_PAGE_SIZE - MIN_CHUNK_SIZE;
  malloc_state->top->data = (void *)((BYTE_PTR)arena + MIN_CHUNK_SIZE);
  malloc_state->top->next_chunk = NULL;
}

static void *use_top(size_t size) {
  void *mem = NULL;
  int new_size;
  if (chunksize(malloc_state->top) < size) {
    mem = get_mem_from_os(SYS_ALLOC_PAGE_SIZE);
    new_size = SYS_ALLOC_PAGE_SIZE;
  } else {
    mem = malloc_state->top->data;
    new_size = (int)(chunksize(malloc_state->top) - size);
  }

  malloc_state->top->data =
      mem + size; // bumps up the pointer of top by size(since it's being
                  // allocated to the program)
  malloc_state->top->size = new_size;

  return mem;
}

static void *get_mem_from_os(size_t size) {
  void *mem =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

  if (mem == MAP_FAILED) {
    printf("Couldn't allocate memory from the OS\nerrno value: %d\n", errno);
    exit(1);
  }
  return mem;
}

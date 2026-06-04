#include "../include/malloc.h"

static struct malloc_state malloc_state;

static void init_malloc_state(mstate);
static void *use_top(mstate, size_t size);
static void *get_mem_from_os(size_t size);

void *malloc(size_t size) {
  printf("Allocating memory\n");

  size_t normalized_size = request_2_size(size);

  mstate ms = get_malloc_state();
  /*
   * we need to check to see if the bins are populated or not(from malloc_state)
   * If not, the path would be to check the top(wilderness) chunk, which will
   * have additional memory allocated to it
   *
   * This will be called when the malloc is first called, and later populate the
   * bins when the program frees the memory allocated by this malloc
   */

  if (!has_any_chunk(ms)) {
    if (ms->max_fast == 0) {
      init_malloc_state(ms);
    }
    return use_top(ms, normalized_size);
  }

  // TODO: Get mem from bins
  return get_mem_from_os(size);
}

static void init_malloc_state(mstate ms) {
  chunk_ptr bin;
  for (size_t i = 0; i < NBINS; i++) {
    bin = bin_at(ms, i);
    bin->data = bin->next_chunk = bin; // bin == bin.data == bin.next_chunk is
                                       // how we know that a bin is empty
  }

  ms->top->size = 0;
  ms->top->data = ms->top->next_chunk = NULL;
}

static void *use_top(mstate ms, size_t size) {
  printf("running use_top\n");
  void *mem = NULL;
  int new_size;
  if (chunksize(ms->top) < size) {
    mem = get_mem_from_os(SYS_ALLOC_PAGE_SIZE);
    new_size = SYS_ALLOC_PAGE_SIZE;
  } else {
    mem = ms->top->data;
    new_size = ms->top->prev_size - size;
  }

  ms->top->data = mem + size; // bumps up the pointer of top by size(since it's
                              // being allocated to the program)
  ms->top->size = new_size;

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

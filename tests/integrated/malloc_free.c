#include "../include/malloc_free.h"

#define KB 1024

Test(malloc_free, small_chunk_allocation) {
  size_t request = 16;
  size_t expected_size = request_2_size(request);

  void *mem = dl_malloc(request);
  cr_assert_not_null(mem);

  chunk_ptr chunk = mem_2_chunk(mem);
  cr_assert_eq(chunksize(chunk), expected_size);
  cr_assert(prev_inuse(chunk));
  cr_assert_not(is_mmapd(chunk));

  dl_free(mem);
}

Test(malloc_free, fastbin_allocation_reuses_freed_chunk) {
  size_t request = 24;

  void *first = dl_malloc(request);
  cr_assert_not_null(first);
  cr_assert_leq(chunksize(mem_2_chunk(first)), DEFAULT_MAX_FASTBIN_SIZE);

  dl_free(first);

  void *second = dl_malloc(request);
  cr_assert_eq(second, first);
  cr_assert_eq(chunksize(mem_2_chunk(second)), request_2_size(request));

  dl_free(second);
}

Test(malloc_free, heap_allocation_from_mmap_backed_top) {
  size_t request = KB;
  size_t expected_size = request_2_size(request);

  void *mem = dl_malloc(request);
  cr_assert_not_null(mem);

  chunk_ptr chunk = mem_2_chunk(mem);
  cr_assert_eq(chunksize(chunk), expected_size);
  cr_assert(prev_inuse(chunk));
  cr_assert_not(is_mmapd(chunk));

  dl_free(mem);
}

Test(malloc_free, malloc_free_for_different_sizes) {
  for (int i = 0; i < 4; i++) {
    dl_free(dl_malloc(KB - (i * 256)));
  }
}

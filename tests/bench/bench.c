#include "../../include/malloc.h"
#include "../include/test_commons.h"
#include <time.h>

#define BENCH_ITERS 10000
#define KB 1024

static uint64_t now_ns(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

static void print_result(const char *name, uint64_t elapsed_ns,
                         size_t iterations) {
  double avg_ns = (double)elapsed_ns / (double)iterations;
  double ops_per_sec = 1000000000.0 / avg_ns;

  printf("\n[bench] %s\n", name);
  printf("  iterations: %zu\n", iterations);
  printf("  total:      %llu ns\n", (unsigned long long)elapsed_ns);
  printf("  avg:        %.2f ns/op\n", avg_ns);
  printf("  throughput: %.2f ops/sec\n", ops_per_sec);
}

/*
 * Put a chunk into a smallbin without measuring the setup:
 * 1. Free `small` into unsorted.
 * 2. Ask for `large`, causing the too-small unsorted chunk to be rebinned.
 * 3. Free `large` back to unsorted so the allocator remains healthy.
 */
static void prepare_smallbin_chunk(void) {
  void *small = dl_malloc(80);
  cr_assert_not_null(small);
  dl_free(small);

  void *large = dl_malloc(160);
  cr_assert_not_null(large);
  dl_free(large);
}

Test(bench, fastbin_allocation_speed) {
  void *seed = dl_malloc(24);
  cr_assert_not_null(seed);
  dl_free(seed);

  uint64_t start = now_ns();
  for (size_t i = 0; i < BENCH_ITERS; i++) {
    void *p = dl_malloc(24);
    cr_assert_not_null(p);
    cr_assert_leq(chunksize(mem_2_chunk(p)), DEFAULT_MAX_FASTBIN_SIZE);
    dl_free(p);
  }
  uint64_t elapsed = now_ns() - start;

  print_result("fastbin malloc/free pair (24-byte request)", elapsed,
               BENCH_ITERS);
}

Test(bench, smallbin_allocation_speed) {
  uint64_t elapsed = 0;

  for (size_t i = 0; i < BENCH_ITERS; i++) {
    prepare_smallbin_chunk();

    uint64_t start = now_ns();
    void *p = dl_malloc(80);
    elapsed += now_ns() - start;

    cr_assert_not_null(p);
    cr_assert_gt(chunksize(mem_2_chunk(p)), DEFAULT_MAX_FASTBIN_SIZE);
    cr_assert_lt(chunksize(mem_2_chunk(p)), MIN_LARGE_SIZE);
    dl_free(p);
  }

  print_result("smallbin allocation only (80-byte request)", elapsed,
               BENCH_ITERS);
}

Test(bench, unsorted_bin_allocation_speed) {
  void *seed = dl_malloc(KB);
  cr_assert_not_null(seed);
  dl_free(seed);

  uint64_t start = now_ns();
  for (size_t i = 0; i < BENCH_ITERS; i++) {
    void *p = dl_malloc(KB);
    cr_assert_not_null(p);
    cr_assert_geq(chunksize(mem_2_chunk(p)), request_2_size(KB));
    dl_free(p);
  }
  uint64_t elapsed = now_ns() - start;

  print_result("unsorted-bin malloc/free pair (1KB request)", elapsed,
               BENCH_ITERS);
}

Test(bench, top_allocation_speed) {
  void *ptrs[BENCH_ITERS];

  uint64_t start = now_ns();
  for (size_t i = 0; i < BENCH_ITERS; i++) {
    ptrs[i] = dl_malloc(128);
    cr_assert_not_null(ptrs[i]);
    cr_assert_eq(chunksize(mem_2_chunk(ptrs[i])), request_2_size(128));
  }
  uint64_t elapsed = now_ns() - start;

  for (size_t i = 0; i < BENCH_ITERS; i++) {
    dl_free(ptrs[i]);
  }

  print_result("top allocation only (128-byte request)", elapsed, BENCH_ITERS);
}

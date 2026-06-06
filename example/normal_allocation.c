#include "../include/malloc.h"

#define KB 1024
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static void print_chunk(const char *label, void *ptr) {
  if (ptr == NULL) {
    printf("%s -> NULL\n", label);
    return;
  }

  chunk_ptr chunk = mem_2_chunk(ptr);

  printf("%s\n", label);
  printf("  user ptr:      %p\n", ptr);
  printf("  chunk ptr:     %p\n", (void *)chunk);
  printf("  chunk size:    %zu\n", chunksize(chunk));
  printf("  prev in use:   %s\n", prev_inuse(chunk) ? "yes" : "no");
  printf("  mmap tagged:   %s\n", is_mmapd(chunk) ? "yes" : "no");
}

static void small_allocations(void) {
  printf("\n== Small allocations ==\n");

  void *a = dl_malloc(16);
  void *b = dl_malloc(32);
  void *c = dl_malloc(48);

  print_chunk("small 16-byte request", a);
  print_chunk("small 32-byte request", b);
  print_chunk("small 48-byte request", c);

  dl_free(a);
  dl_free(b);
  dl_free(c);
}

static void fastbin_reuse(void) {
  printf("\n== Fastbin reuse ==\n");

  void *first = dl_malloc(24);
  print_chunk("first 24-byte request", first);
  dl_free(first);

  void *second = dl_malloc(24);
  print_chunk("second 24-byte request after free", second);
  printf("  reused pointer: %s\n", first == second ? "yes" : "no");

  dl_free(second);
}

static void heap_allocations(void) {
  printf("\n== Heap/top allocations ==\n");

  size_t requests[] = {KB / 2, KB, KB + 256, 2 * KB};
  void *ptrs[ARRAY_LEN(requests)];

  for (size_t i = 0; i < ARRAY_LEN(requests); i++) {
    char label[64];
    ptrs[i] = dl_malloc(requests[i]);
    snprintf(label, sizeof(label), "%zu-byte heap request", requests[i]);
    print_chunk(label, ptrs[i]);
  }

  for (size_t i = 0; i < ARRAY_LEN(ptrs); i++) {
    dl_free(ptrs[i]);
  }
}

static void interleaved_alloc_free(void) {
  printf("\n== Interleaved malloc/free ==\n");

  void *a = dl_malloc(128);
  void *b = dl_malloc(512);
  void *c = dl_malloc(64);

  print_chunk("allocated a = 128", a);
  print_chunk("allocated b = 512", b);
  print_chunk("allocated c = 64", c);

  dl_free(b);
  printf("freed b first\n");

  void *d = dl_malloc(256);
  print_chunk("allocated d = 256 after freeing b", d);

  dl_free(a);
  dl_free(c);
  dl_free(d);
}

static void edge_cases(void) {
  printf("\n== Edge cases ==\n");

  void *zero = dl_malloc(0);
  print_chunk("zero-byte request", zero);

  printf("freeing NULL\n");
  dl_free(NULL);
}

int main(void) {
  small_allocations();
  fastbin_reuse();
  heap_allocations();
  interleaved_alloc_free();
  edge_cases();

  return 0;
}

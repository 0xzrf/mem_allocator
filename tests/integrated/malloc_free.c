#include "../include/malloc_free.h"

#define KB 1024
Test(malloc_free, test_malloc_for_different_sizes) {
  for (int i = 0; i < 4; i++) {
    free(malloc(KB - (i * 256)));
  }

  for (int i = 0; i < 4; i++) {
    free(malloc(KB + (i * 256)));
  }
}

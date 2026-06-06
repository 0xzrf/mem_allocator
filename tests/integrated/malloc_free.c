#include "../include/malloc_free.h"

#define KB 1024
Test(malloc_free, test_malloc_for_different_sizes) {
  for (int i = 0; i < 4; i++) {
    dl_free(dl_malloc(KB - (i * 256)));
  }
}

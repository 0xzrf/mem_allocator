#include "../include/test_chunk.h"

Test(chunk, prev_inuse_bit) {
  struct chunk_header x;

  x.size = 1U;
  cr_assert_eq(prev_inuse(&x), 1);
}

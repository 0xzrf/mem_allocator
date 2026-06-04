#include "../include/test_chunk.h"

void setUp() {}
void tearDown() {}

void test_chunk_helpers() {
    struct chunk_header x;

    x.size = 1U; // first bit set
    TEST_ASSERT_EQUAL(prev_inuse(&x), 1);
}

void run_chunk_tests() {
    RUN_TEST(test_chunk_helpers);
}

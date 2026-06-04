#include "../include/test_commons.h"
#include "../../include/malloc.h"

Test(bench, malloc_small) {
    const size_t iterations = 1000;

    for (size_t i = 0; i < iterations; i++) {
        void *p = malloc(64);
        cr_assert_not_null(p);
    }
}

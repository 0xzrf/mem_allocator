
#include <criterion/criterion.h>
#include <stddef.h>

#include "malloc.h"

Test(free_correctness, free_to_fastbin) {
    void *ptr = dl_malloc(8);

    dl_free(ptr);
}

Test(free_correctness, multiple_malloc_and_free_pair_works) {
    for (size_t i = 16; i < MAX_FASTBIN_SIZE; i *= 2) {
        void *ptr = dl_malloc(i);
        dl_free(ptr);
    }
}

#include <criterion/criterion.h>
#include <stddef.h>

#include "malloc.h"
#include "stdio.h"

Test(malloc_correctness, malloc_works) {
    void *ret_ptr = dl_malloc(8);
}

Test(malloc_correctness, malloc_chunk_size_correct_after_alloc) {
    size_t request_size = 8;
    size_t aligned_req = request2size(request_size);
    void *ret_ptr = dl_malloc(request_size);

    mchunkptr chunk = mem2chunk(ret_ptr);
    size_t actual_chunk_size = chunk_size(chunk);
    printf("actual: %zu\n expected: %zu", actual_chunk_size, aligned_req);
    cr_assert_eq(aligned_req, actual_chunk_size);
}
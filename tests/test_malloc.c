#include <criterion/criterion.h>
#include <stddef.h>

#include "malloc.h"
#include "stdio.h"

Test(malloc_correctness, malloc_chunk_size_correct_after_alloc) {
    size_t request_size = 8;
    size_t aligned_req = request2size(request_size);
    void *ret_ptr = dl_malloc(request_size);

    mchunkptr chunk = mem2chunk(ret_ptr);
    size_t actual_chunk_size = chunk_size(chunk);

    cr_assert_eq(aligned_req, actual_chunk_size);

    cr_assert(is_mmaped(chunk));
    cr_assert(prev_in_use(chunk));
}


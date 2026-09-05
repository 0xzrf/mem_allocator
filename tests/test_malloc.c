#include <criterion/criterion.h>
#include <stddef.h>
#include "stdio.h"
#include "malloc.h"

// Test(free_correctness, free_to_fastbin) {
//     void *ptr = dl_malloc(8);

//     dl_free(ptr);
// }

// Test(malloc_correctness, malloc_chunk_size_correct_after_alloc) {
//     size_t request_size = 8;
//     size_t aligned_req = request2size(request_size);
//     void *ret_ptr = dl_malloc(request_size);

//     mchunkptr chunk = mem2chunk(ret_ptr);
//     size_t actual_chunk_size = chunk_size(chunk);

//     cr_assert_eq(aligned_req, actual_chunk_size);
//     cr_assert(prev_in_use(chunk));
// }

// Test(malloc_free_correctness, multiple_malloc_and_free_pair_works_for_fastbins) {
//     for (size_t i = 16; i <= MAX_FASTBIN_SIZE; i *= 2) {
//         void *ptr = dl_malloc(i);
//         dl_free(ptr);
//     }
// }

// Test(malloc_free_correctness, multiple_malloc_and_free_pair_works_for_smallbins) {
//     for (size_t i = MAX_FASTBIN_SIZE + MALLOC_ALIGN; i <= MIN_LARGE_SIZE; i *= 2) {
//         void *ptr = dl_malloc(i);
//         dl_free(ptr);
//     }
// }

Test(malloc_free_correctness, allocation_after_first_free_should_alloc_from_bins) {
    size_t big_alloc = 150;
    size_t smaller_alloc = 100;

    void *big_alloc_ptr = dl_malloc(big_alloc);

    mchunkptr big_alloc_chunk = mem2chunk(big_alloc_ptr);

    // freeing big_alloc(which is within small_bin size) should put it in
    dl_free(big_alloc_ptr); // puts to unsorted_bin

    void *small_alloc_ptr = dl_malloc(smaller_alloc); // should pick from small_bin set

    mchunkptr small_alloc_chunk = mem2chunk(small_alloc_ptr);
    cr_assert_eq(big_alloc_ptr, small_alloc_ptr); // if the allocation happened to the same chunk,
                                                  // the pointers should be the same
}

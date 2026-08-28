#include <criterion/criterion.h>
#include <stddef.h>

#include "chunk.h"

_Static_assert(sizeof(struct mem_chunk) == 32,
               "mem_chunk must be exactly 4 words with no padding");

            
Test(chunk_layout, chunk_size_correct) {
    cr_assert_eq(sizeof(struct mem_chunk), 32);
}
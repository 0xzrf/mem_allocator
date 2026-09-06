#include <criterion/criterion.h>
#include <stddef.h>

#include "chunk.h"

_Static_assert(sizeof(struct mem_chunk) == 32, "mem_chunk must be exactly 4 words with no padding");

Test(chunk_layout, chunk_size_correct) {
    cr_assert_eq(sizeof(struct mem_chunk), 32);
}

Test(chunk_ops, chunk_size_preserves_actual_size_value_ignoring_flags) {
    size_t expected_size = 80;
    struct mem_chunk c = {prev_size : 0, size : expected_size, next : NULL, back : NULL};

    set_prev_in_use(&c);

    cr_assert_eq(chunk_size(&c), expected_size);
    cr_assert(c.size != expected_size);
    cr_assert(prev_in_use(&c));
}

Test(chunk_ops, next_chunk_identifies_prev_chunk_free) {
    struct mem_chunk c[2] = {{
                                 prev_size : 0,
                                 size : 16,
                                 next : NULL,
                                 back : NULL
                             }, // cannot put more then 16, coz then will have to allocate that
                             {prev_size : 0, size : 80, next : NULL, back : NULL}};

    mchunkptr chunk1 = &c[0];
    mchunkptr chunk2 = &c[1];

    set_foot(chunk1, chunk1->size);
    set_prev_in_use(next_chunk(chunk1)); // this is done in the actual code, so it has to fetch the
    // next chunk, even tho we have chunk2

    cr_assert_eq(next_chunk(chunk1), chunk2);
    cr_assert_eq(prev_chunk(chunk2), chunk1);
    cr_assert(prev_in_use(chunk2));
    cr_assert_eq(chunk2->prev_size, 16);
}

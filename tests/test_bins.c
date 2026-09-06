
#include <criterion/criterion.h>
#include <stddef.h>

#include "bins.h"

Test(bin_correctness, request2size_generates_malloc_aligned_output) {
    cr_assert_eq(request2size(8), MIN_SIZE);
    cr_assert_eq(request2size(32), 48);
    cr_assert_eq(request2size(48), 48 + 16);
}

Test(bin_correctness, bins_point_at_the_right_free_chunks) {
    // init bin(s)
    bin fastbins[MAX_FASTBIN_SIZE >> 4] = {0};

    struct dummy_mem_stat {
        bin fastbins[MAX_FASTBIN_SIZE >> 4];
    };

    struct dummy_mem_stat mem_state = {fastbins : fastbins};
    struct dummy_mem_stat *state_ptr = &mem_state;

    for (size_t i = 0; i <= MAX_FASTBIN_SIZE >> 4; i++) {
        init_bin(fastbins, i);
    }

    // populate the bin(s)
    struct mem_chunk chunks[4] = {};

    for (size_t i = 0; i < 4; i++) {
        mchunkptr c = &chunks[i];
        c->size = MALLOC_ALIGN;
        // every second chunk free(2nd and 4th)
        if ((i + 1) % 2 == 0) {
            set_foot(c, MALLOC_ALIGN);
            unset_prev_in_use(next_chunk(c));
            set_prev_in_use(c); // set the 1st and 3rd as allocated

            insert_at_head(fastbins, bin_ix(MALLOC_ALIGN), c);
        }
    }

    // check if the doubly-linked bins is poiting at the right bin
    binptr bin_at_m_align = &bin_at_size(fastbins, MALLOC_ALIGN);

    binptr next_chunk = bin_at_m_align->next;

    for (size_t i = 1; i < 5; i *= 2) {
        cr_assert_eq(&chunks[i], mem2chunk(next_chunk));
        next_chunk = next_chunk->next;
    }
}

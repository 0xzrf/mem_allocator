
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
            set_prev_in_use(c); // set the 1st and 3rd as allocated

            insert_at_head(fastbins, bin_ix(MALLOC_ALIGN), c);
        }
    }

    // check if the doubly-linked bins is poiting at the right bin
    binptr bin_at_m_align = &bin_at_size(fastbins, MALLOC_ALIGN);

    binptr next_chunk = bin_at_m_align->next;

    cr_assert_eq(&chunks[3], mem2chunk(next_chunk));
    next_chunk = next_chunk->next;
    cr_assert_eq(&chunks[1], mem2chunk(next_chunk));
    next_chunk = next_chunk->next;
}

struct test_state {
    bin bins[NBINS];
    bin fastbins[NFASTBIN];
};

#define SETUP_BINS()                                                                               \
    struct test_state mem_state_ = {0};                                                            \
    struct test_state *state_ptr = &mem_state_;                                                    \
    for (size_t i_ = 0; i_ < NBINS; i_++)                                                          \
        init_bin(bins, i_);                                                                        \
    for (size_t i_ = 0; i_ < NFASTBIN; i_++)                                                       \
    init_bin(fastbins, i_)

static void make_chunks(struct mem_chunk *c, size_t n, size_t size) {
    for (size_t i = 0; i < n; i++) {
        c[i].prev_size = 0;
        c[i].size = size;
        c[i].next = c[i].back = NULL;
    }
}

Test(bin_correctness, fresh_bins_are_empty_rings) {
    SETUP_BINS();

    for (size_t i = 0; i < NBINS; i++) {
        cr_assert(is_bin_empty(bins, i), "bins[%zu] not empty after init", i);
        cr_assert_eq(state_ptr->bins[i].next, &state_ptr->bins[i],
                     "bins[%zu].next must point at itself", i);
        cr_assert_eq(state_ptr->bins[i].back, &state_ptr->bins[i],
                     "bins[%zu].back must point at itself", i);
    }
    for (size_t i = 0; i < NFASTBIN; i++)
        cr_assert(is_bin_empty(fastbins, i), "fastbins[%zu] not empty after init", i);
}

Test(bin_correctness, single_insert_links_both_directions) {
    SETUP_BINS();
    struct mem_chunk chunks[1];
    make_chunks(chunks, 1, MIN_SIZE);

    binptr head = &bin_at_size(fastbins, MIN_SIZE);
    binptr c = (binptr) chunk2mem(&chunks[0]);

    insert_at_head(fastbins, bin_ix(MIN_SIZE), &chunks[0]);

    cr_assert(!is_bin_empty(fastbins, bin_ix(MIN_SIZE)));
    cr_assert_eq(head->next, c, "head->next must be the inserted chunk");
    cr_assert_eq(head->back, c, "with one chunk, head->back must also be it");
    cr_assert_eq(c->next, head, "the only chunk's next must close the ring");
    cr_assert_eq(c->back, head, "the only chunk's back must close the ring");
}

Test(bin_correctness, forward_and_backward_walks_agree) {
    SETUP_BINS();
    enum { N = 3 };
    struct mem_chunk chunks[N];
    make_chunks(chunks, N, MIN_SIZE);

    for (size_t i = 0; i < N; i++)
        insert_at_head(fastbins, bin_ix(MIN_SIZE), &chunks[i]);

    binptr head = &bin_at_size(fastbins, MIN_SIZE);

    // head insertion is LIFO: last in comes out first
    mchunkptr fwd[N];
    size_t n = 0;
    for (binptr p = head->next; p != head; p = p->next)
        fwd[n++] = mem2chunk(p);
    cr_assert_eq(n, N, "forward walk found %zu chunks, expected %d", n, N);
    cr_assert_eq(fwd[0], &chunks[2]);
    cr_assert_eq(fwd[1], &chunks[1]);
    cr_assert_eq(fwd[2], &chunks[0]);

    // walking back from the head must produce the exact reverse
    mchunkptr bwd[N];
    n = 0;
    for (binptr p = head->back; p != head; p = p->back)
        bwd[n++] = mem2chunk(p);
    cr_assert_eq(n, N, "backward walk found %zu chunks, expected %d", n, N);
    for (size_t i = 0; i < N; i++)
        cr_assert_eq(bwd[i], fwd[N - 1 - i], "backward[%zu] should equal forward[%d]", i,
                     N - 1 - (int) i);
}

Test(bin_correctness, insert_then_remove_restores_empty_bin) {
    SETUP_BINS();
    struct mem_chunk chunks[1];
    make_chunks(chunks, 1, MIN_SIZE);

    size_t ix = bin_ix(MIN_SIZE);
    insert_at_head(fastbins, ix, &chunks[0]);
    cr_assert(!is_bin_empty(fastbins, ix));

    remove_from_bin(&chunks[0]);

    cr_assert(is_bin_empty(fastbins, ix), "bin should be empty again");
    cr_assert_eq(state_ptr->fastbins[ix].next, &state_ptr->fastbins[ix]);
    cr_assert_eq(state_ptr->fastbins[ix].back, &state_ptr->fastbins[ix]);
}

Test(bin_correctness, remove_from_middle_keeps_ring_intact) {
    SETUP_BINS();
    enum { N = 3 };
    struct mem_chunk chunks[N];
    make_chunks(chunks, N, MIN_SIZE);

    size_t ix = bin_ix(MIN_SIZE);
    for (size_t i = 0; i < N; i++)
        insert_at_head(fastbins, ix, &chunks[i]);
    // list is now: head -> c2 -> c1 -> c0 -> head

    remove_from_bin(&chunks[1]); // yank the middle one

    binptr head = &state_ptr->fastbins[ix];
    mchunkptr seen[N];
    size_t n = 0;
    for (binptr p = head->next; p != head && n < N; p = p->next)
        seen[n++] = mem2chunk(p);

    cr_assert_eq(n, 2, "expected 2 chunks left, walked %zu", n);
    cr_assert_eq(seen[0], &chunks[2]);
    cr_assert_eq(seen[1], &chunks[0]);

    // and the ring must still close backwards
    cr_assert_eq(head->back, (binptr) chunk2mem(&chunks[0]),
                 "head->back must be the last remaining chunk");
    cr_assert_eq(((binptr) chunk2mem(&chunks[0]))->next, head);
}

Test(bin_correctness, bin_ix_is_in_range_and_monotonic) {
    size_t prev = 0;
    for (size_t sz = MIN_SIZE; sz < MIN_LARGE_SIZE; sz += MALLOC_ALIGN) {
        size_t ix = bin_ix(sz);
        cr_assert_lt(ix, NBINS, "bin_ix(%zu) = %zu is out of range", sz, ix);
        cr_assert_geq(ix, 2, "bin_ix(%zu) = %zu collides with bin 0/1 (unsorted)", sz, ix);
        cr_assert_gt(ix, prev, "bin_ix must be strictly increasing; %zu gave %zu", sz, ix);
        prev = ix;
    }
    for (size_t sz = MIN_SIZE; sz <= MAX_FASTBIN_SIZE; sz += MALLOC_ALIGN)
        cr_assert_lt(bin_ix(sz), NFASTBIN, "bin_ix(%zu) overflows fastbins[]", sz);
}

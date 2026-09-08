#include <criterion/criterion.h>
#include <stddef.h>
#include <string.h>

#include "malloc.h"

/* Large bins hold a size RANGE, so they need a search -- and the binmap is
 * what makes "my bin is empty, try a bigger one" cheap. */

#define SPACER 100 /* small, keeps large chunks from coalescing */

Test(largebins, index_is_in_range_and_monotonic) {
    for (size_t nb = MIN_LARGE_SIZE; nb <= (size_t) 1 << 20; nb += MALLOC_ALIGN) {
        unsigned ix = test_bin_ix(nb);
        cr_assert_geq(ix, NBINS_SMALL, "size %zu -> bin %u, below the large range", nb, ix);
        cr_assert_lt(ix, NBINS, "size %zu -> bin %u, out of range", nb, ix);
    }
    /* monotonic: a bigger chunk never lands in a lower bin */
    unsigned prev = 0;
    for (size_t nb = MIN_LARGE_SIZE; nb <= (size_t) 1 << 20; nb += MALLOC_ALIGN) {
        unsigned ix = test_bin_ix(nb);
        cr_assert_geq(ix, prev, "index went backwards at size %zu", nb);
        prev = ix;
    }
}

Test(largebins, four_sub_bins_per_power_of_two) {
    /* This is the whole point of the log spacing: each doubling of size gets
     * exactly SUBBINS bins, so relative waste stays bounded at every scale. */
    for (unsigned oct = 0; oct < 8; oct++) {
        size_t base = MIN_LARGE_SIZE << oct;
        unsigned first = test_bin_ix(base);
        unsigned last = test_bin_ix((base * 2) - MALLOC_ALIGN);
        cr_assert_eq(last - first + 1, SUBBINS,
                     "octave at %zu spans %u bins, expected %u", base, last - first + 1,
                     (unsigned) SUBBINS);
    }
}

Test(largebins, same_size_reuse_comes_back) {
    reset_mem_state();
    void *a = dl_malloc(1000);
    mchunkptr ca = mem2chunk(a);
    dl_free(a);
    void *b = dl_malloc(1000);
    cr_assert_eq(mem2chunk(b), ca);
    reset_mem_state();
}

Test(largebins, freed_large_chunk_is_filed_into_its_bin_by_the_drain) {
    reset_mem_state();
    void *guard = dl_malloc(SPACER);
    void *a = dl_malloc(1000); /* payload 1008 -> bin 35 */
    void *hi = dl_malloc(SPACER);
    unsigned ix = test_bin_ix(chunk_size(mem2chunk(a)));

    dl_free(a);
    cr_assert(!test_bin_is_marked(ix), "free() must not mark");

    void *other = dl_malloc(3000); /* different size -> forces the drain */
    cr_assert(test_bin_is_marked(ix), "the drain should have filed and marked it");
    cr_assert_eq(test_bin_count(ix), 1);

    dl_free(other);
    dl_free(hi);
    dl_free(guard);
    reset_mem_state();
}

Test(largebins, binmap_scan_finds_a_bigger_bin_and_splits_it) {
    reset_mem_state();
    void *lo = dl_malloc(SPACER);
    void *big = dl_malloc(8000); /* payload 8016 -> a high bin */
    void *hi = dl_malloc(SPACER);
    size_t bigsz = chunk_size(mem2chunk(big));
    unsigned bigix = test_bin_ix(bigsz);

    dl_free(big);
    void *t = dl_malloc(20000); /* forces the drain, files `big` */
    cr_assert(test_bin_is_marked(bigix));

    /* Ask for something far smaller. Its own bin is empty, so the binmap scan
     * must find `bigix` and split the chunk. */
    void *mid = dl_malloc(600); /* payload 608 -> bin 32 */
    cr_assert_eq(mem2chunk(mid), mem2chunk(big),
                 "the request should have been carved from the 8016 chunk");
    cr_assert_eq(chunk_size(mem2chunk(mid)), request2size(600), "should be exactly sized");
    cr_assert_lt(chunk_size(mem2chunk(mid)), bigsz, "should have been split");

    dl_free(mid);
    dl_free(t);
    dl_free(hi);
    dl_free(lo);
    reset_mem_state();
}

Test(largebins, best_fit_picks_the_smallest_chunk_that_works) {
    reset_mem_state();
    /* three large chunks in the SAME bin range, kept apart by live spacers */
    void *s0 = dl_malloc(SPACER);
    void *a = dl_malloc(1000); /* 1008 */
    void *s1 = dl_malloc(SPACER);
    void *b = dl_malloc(900); /* 912 */
    void *s2 = dl_malloc(SPACER);
    void *c = dl_malloc(1100); /* 1104 */
    void *s3 = dl_malloc(SPACER);

    mchunkptr want = mem2chunk(b); /* 912 is the smallest of the three */
    dl_free(a);
    dl_free(b);
    dl_free(c);

    void *t = dl_malloc(30000); /* force the drain */

    /* 900 fits in all three; best fit must choose the 912 one */
    void *got = dl_malloc(900);
    cr_assert_eq(mem2chunk(got), want, "best fit should pick the smallest that fits");

    dl_free(got);
    dl_free(t);
    dl_free(s3); dl_free(s2); dl_free(s1); dl_free(s0);
    reset_mem_state();
}

Test(largebins, malloc_free_pairs_across_a_wide_large_range) {
    reset_mem_state();
    for (size_t req = MIN_LARGE_SIZE; req <= 64u * 1024; req *= 2) {
        void *p = dl_malloc(req);
        cr_assert_neq(p, NULL, "malloc(%zu) returned NULL", req);
        cr_assert_geq(chunk_size(mem2chunk(p)), req, "payload too small for %zu", req);
        memset(p, 0x5A, req);
        dl_free(p);
    }
    reset_mem_state();
}

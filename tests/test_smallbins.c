#include <criterion/criterion.h>
#include <stddef.h>
#include <string.h>

#include "malloc.h"

/* Small bins hold ONE exact size each, so a hit is always an exact fit with no
 * search and no split. These go through dl_malloc / dl_free, not the macros. */

#define SMALL_REQ 100 /* -> payload 112, bin 7, above MAX_FASTBIN_SIZE */

Test(smallbins, free_then_malloc_reuses_the_same_chunk) {
    reset_mem_state();
    void *a = dl_malloc(SMALL_REQ);
    mchunkptr ca = mem2chunk(a);

    dl_free(a);
    void *b = dl_malloc(SMALL_REQ);

    cr_assert_eq(mem2chunk(b), ca, "a same-size request should reuse the chunk");
    reset_mem_state();
}

Test(smallbins, exact_size_reuse_does_not_split) {
    reset_mem_state();
    void *a = dl_malloc(SMALL_REQ);
    size_t sz = chunk_size(mem2chunk(a));
    dl_free(a);

    void *b = dl_malloc(SMALL_REQ);
    cr_assert_eq(chunk_size(mem2chunk(b)), sz,
                 "exact fit must hand back the whole chunk, unsplit");
    reset_mem_state();
}

Test(smallbins, freed_chunk_reaches_a_sized_bin_only_via_malloc) {
    reset_mem_state();
    void *guard = dl_malloc(SMALL_REQ); /* keeps the next chunk off top */
    void *a = dl_malloc(SMALL_REQ);
    unsigned ix = test_bin_ix(chunk_size(mem2chunk(a)));

    dl_free(a);
    /* free() puts it in UNSORTED, never a sized bin, and never marks. */
    cr_assert(test_bin_is_empty(ix), "free() must not file into a sized bin");
    cr_assert(!test_bin_is_marked(ix), "free() must not touch the binmap");

    /* A request of a DIFFERENT size forces the drain, which files it. */
    void *other = dl_malloc(300);
    cr_assert(!test_bin_is_empty(ix), "the drain should have filed the chunk");
    cr_assert(test_bin_is_marked(ix), "the drain should have marked the bin");

    dl_free(other);
    dl_free(guard);
    reset_mem_state();
}

Test(smallbins, many_distinct_sizes_land_in_distinct_bins) {
    reset_mem_state();
    enum { N = 8 };
    void *p[N], *spacer[N];
    size_t reqs[N] = {88, 104, 120, 136, 152, 168, 184, 200};

    /* Interleave LIVE spacers. Without them the 8 chunks are physically
     * adjacent, and freeing them coalesces the lot into one chunk -- correct
     * allocator behaviour, but then there is nothing per-size left to check. */
    for (int i = 0; i < N; i++) {
        p[i] = dl_malloc(reqs[i]);
        spacer[i] = dl_malloc(SMALL_REQ);
    }
    for (int i = 0; i < N; i++)
        dl_free(p[i]);

    /* force the drain so everything gets filed into its sized bin */
    void *trigger = dl_malloc(2000);

    for (int i = 0; i < N; i++) {
        unsigned ix = test_bin_ix(request2size(reqs[i]));
        cr_assert(test_bin_is_marked(ix), "bin %u for req %zu not marked", ix, reqs[i]);
        cr_assert_eq(test_bin_count(ix), 1, "bin %u should hold exactly 1 chunk", ix);
    }

    dl_free(trigger);
    for (int i = 0; i < N; i++)
        dl_free(spacer[i]);
    reset_mem_state();
}

Test(smallbins, malloc_free_pairs_across_the_whole_small_range) {
    reset_mem_state();
    for (size_t req = MAX_FASTBIN_SIZE + 8; req + 8 < MIN_LARGE_SIZE; req += MALLOC_ALIGN) {
        void *p = dl_malloc(req);
        cr_assert_neq(p, NULL, "malloc(%zu) returned NULL", req);
        cr_assert_eq(chunk_size(mem2chunk(p)), request2size(req),
                     "wrong payload for req %zu", req);
        memset(p, 0xAB, req); /* must not corrupt metadata */
        dl_free(p);
    }
    reset_mem_state();
}

Test(smallbins, a_bigger_small_chunk_is_split_for_a_smaller_request) {
    reset_mem_state();
    void *guard_lo = dl_malloc(SMALL_REQ);
    void *big = dl_malloc(400); /* payload 416, bin 26 */
    void *guard_hi = dl_malloc(SMALL_REQ);
    size_t bigsz = chunk_size(mem2chunk(big));

    dl_free(big);
    /* force the drain so the 416 chunk lands in bin 26 and gets marked */
    void *t = dl_malloc(2000);

    /* now ask for something much smaller: no bin 7 chunk exists, so the
     * binmap scan should find bin 26 and split it */
    void *small = dl_malloc(SMALL_REQ);
    cr_assert_eq(mem2chunk(small), mem2chunk(big),
                 "the small request should have been carved out of the 416 chunk");
    cr_assert_lt(chunk_size(mem2chunk(small)), bigsz, "it should have been split");

    dl_free(small);
    dl_free(t);
    dl_free(guard_hi);
    dl_free(guard_lo);
    reset_mem_state();
}

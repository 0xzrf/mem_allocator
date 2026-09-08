#include <criterion/criterion.h>
#include <stddef.h>
#include <string.h>

#include "malloc.h"

/* Coalescing is what keeps the heap from turning into a field of unusable
 * holes. These tests are all about the boundary-tag protocol:
 *   free() writes its footer and clears the NEXT chunk's PREV_IN_USE.
 *   Anyone standing at a chunk can then safely jump backwards.
 *
 * Sizes here are all ABOVE MAX_FASTBIN_SIZE, because a fastbin free
 * deliberately skips the protocol -- that is tested separately. */

#define BIG 200 /* payload 208: above MAX_FASTBIN_SIZE (80), below MIN_LARGE_SIZE */

Test(coalesce, free_publishes_footer_and_clears_neighbour_flag) {
    reset_mem_state();
    void *a = dl_malloc(BIG);
    void *b = dl_malloc(BIG); /* physically after a */
    mchunkptr ca = mem2chunk(a), cb = mem2chunk(b);

    cr_assert(prev_in_use(cb), "while a is allocated, b must see it as in use");

    dl_free(a);

    cr_assert(!prev_in_use(cb), "after free(a), b must see it as free");
    cr_assert_eq(cb->prev_size, chunk_size(ca), "footer must equal a's size");
    cr_assert_eq(prev_chunk(cb), ca, "prev_chunk must jump back to a");

    dl_free(b);
    reset_mem_state();
}

Test(coalesce, forward_merge) {
    reset_mem_state();
    void *lo = dl_malloc(BIG); /* anchor below */
    void *a = dl_malloc(BIG);
    void *b = dl_malloc(BIG);
    void *hi = dl_malloc(BIG); /* anchor above, keeps b off top */

    size_t sa = chunk_size(mem2chunk(a)), sb = chunk_size(mem2chunk(b));
    mchunkptr ca = mem2chunk(a);

    dl_free(b); /* b free, a still allocated */
    dl_free(a); /* a should swallow b going forward */

    /* merged payload = sa + sb + CHUNK_OVERHEAD (b's header becomes payload) */
    cr_assert_eq(chunk_size(ca), sa + sb + CHUNK_OVERHEAD,
                 "forward merge: expected %zu, got %zu", sa + sb + CHUNK_OVERHEAD,
                 chunk_size(ca));

    dl_free(hi);
    dl_free(lo);
    reset_mem_state();
}

Test(coalesce, backward_merge) {
    reset_mem_state();
    void *lo = dl_malloc(BIG);
    void *a = dl_malloc(BIG);
    void *b = dl_malloc(BIG);
    void *hi = dl_malloc(BIG);

    size_t sa = chunk_size(mem2chunk(a)), sb = chunk_size(mem2chunk(b));
    mchunkptr ca = mem2chunk(a);

    dl_free(a); /* a free first */
    dl_free(b); /* b must merge BACKWARD into a */

    cr_assert_eq(chunk_size(ca), sa + sb + CHUNK_OVERHEAD,
                 "backward merge: expected %zu, got %zu", sa + sb + CHUNK_OVERHEAD,
                 chunk_size(ca));

    dl_free(hi);
    dl_free(lo);
    reset_mem_state();
}

Test(coalesce, both_sides_in_one_free) {
    reset_mem_state();
    void *lo = dl_malloc(BIG);
    void *a = dl_malloc(BIG);
    void *b = dl_malloc(BIG);
    void *c = dl_malloc(BIG);
    void *hi = dl_malloc(BIG);

    size_t sa = chunk_size(mem2chunk(a));
    size_t sb = chunk_size(mem2chunk(b));
    size_t sc = chunk_size(mem2chunk(c));
    mchunkptr ca = mem2chunk(a);

    dl_free(a); /* free the outer two first */
    dl_free(c);
    dl_free(b); /* freeing the middle must merge BOTH ways at once */

    size_t want = sa + sb + sc + 2 * CHUNK_OVERHEAD;
    cr_assert_eq(chunk_size(ca), want, "three-way merge: expected %zu, got %zu", want,
                 chunk_size(ca));

    dl_free(hi);
    dl_free(lo);
    reset_mem_state();
}

Test(coalesce, checkerboard_fully_collapses) {
    reset_mem_state();
    enum { N = 16 };
    void *v[N];
    void *lo = dl_malloc(BIG);
    for (int i = 0; i < N; i++)
        v[i] = dl_malloc(BIG);
    void *hi = dl_malloc(BIG); /* keeps v[N-1] off top */
    size_t one = chunk_size(mem2chunk(v[0]));

    /* free every other one -> a real checkerboard of separate holes */
    for (int i = 0; i < N; i += 2)
        dl_free(v[i]);
    void *t = dl_malloc(20000); /* force the drain so the holes get filed */

    /* ASSERT THE PRECONDITION: a "fragmentation" test whose holes silently
     * merged would measure nothing at all. */
    cr_assert_eq(test_largest_free(), one,
                 "expected %zu-byte holes, largest is %zu -- not a checkerboard", one,
                 test_largest_free());

    /* now fill in the gaps: everything must coalesce into one run */
    for (int i = 1; i < N; i += 2)
        dl_free(v[i]);
    void *t2 = dl_malloc(20000); /* drain again */

    size_t want = (size_t) N * one + (size_t) (N - 1) * CHUNK_OVERHEAD;
    cr_assert_geq(test_largest_free(), want,
                  "did NOT collapse: expected >= %zu contiguous, largest is %zu", want,
                  test_largest_free());

    dl_free(t2);
    dl_free(t);
    dl_free(hi);
    dl_free(lo);
    reset_mem_state();
}

Test(coalesce, chunk_next_to_top_does_not_merge_across_it) {
    reset_mem_state();
    void *a = dl_malloc(BIG); /* a's next chunk IS top */
    mchunkptr ca = mem2chunk(a);
    size_t sa = chunk_size(ca);

    cr_assert(is_mmaped(next_chunk(ca)), "the chunk after a should be top");

    dl_free(a);
    /* free() must skip the forward merge when the neighbour is top -- reading
     * past top would walk off the end of the mapping. */
    cr_assert_eq(chunk_size(ca), sa, "a must not have absorbed top");
    reset_mem_state();
}

Test(coalesce, fastbin_free_deliberately_skips_the_protocol) {
    reset_mem_state();
    void *a = dl_malloc(24); /* payload 32 -> fastbin */
    void *b = dl_malloc(24);
    mchunkptr ca = mem2chunk(a), cb = mem2chunk(b);

    dl_free(a);

    /* A fastbin chunk keeps claiming to be in use, so its neighbour will not
     * try to coalesce with it. That is what makes fastbin free ~4 instructions
     * -- and it is why those chunks stay frozen until consolidation. */
    cr_assert(prev_in_use(cb), "b must STILL believe a is in use");
    cr_assert_eq(chunk_size(ca), request2size(24), "size unchanged");

    dl_free(b);
    reset_mem_state();
}

Test(coalesce, stress_random_alloc_free_preserves_contents) {
    reset_mem_state();
    enum { LIVE = 200, ITER = 20000 };
    static void *p[LIVE];
    static size_t sz[LIVE];
    static unsigned char pat[LIVE];
    unsigned st = 12345;
    int n = 0;

    for (long it = 0; it < ITER; it++) {
        st = st * 1103515245u + 12345u;
        if (n == 0 || (n < LIVE && (st >> 16) % 100 < 55)) {
            size_t s = MAX_FASTBIN_SIZE + 8 + (st >> 8) % 400;
            p[n] = dl_malloc(s);
            cr_assert_neq(p[n], NULL, "malloc(%zu) failed", s);
            sz[n] = s;
            pat[n] = (unsigned char) (st >> 3);
            memset(p[n], pat[n], s); /* fill the WHOLE request */
            n++;
        } else {
            int i = (int) ((st >> 16) % (unsigned) n);
            unsigned char *b = p[i];
            for (size_t k = 0; k < sz[i]; k++) {
                if (b[k] != pat[i]) {
                    cr_assert_fail("object %d corrupted at byte %zu -- the allocator "
                                   "wrote into live user data",
                                   i, k);
                }
            }
            dl_free(p[i]);
            p[i] = p[n - 1];
            sz[i] = sz[n - 1];
            pat[i] = pat[n - 1];
            n--;
        }
    }
    for (int i = 0; i < n; i++)
        dl_free(p[i]);
    reset_mem_state();
}

#include <criterion/criterion.h>
#include <stddef.h>

#include "bins.h"
#include "chunk.h"

/* Pure binmap logic -- no allocator state, just a stack-local bin array and
 * bitmap. That keeps these tests independent of malloc.c entirely. */

static bin         arr[NBINS];
static binmap_word map[BINMAP_WORDS];
static struct mem_chunk pool[8];

static void fresh(void) {
    for (unsigned i = 0; i < NBINS; i++)
        arr[i].next = arr[i].back = &arr[i];
    bm_init(map);
}

/* Put one chunk in bin `ix` and mark it. */
static void occupy(unsigned ix, unsigned slot) {
    mchunkptr c = &pool[slot];
    c->prev_size = 0;
    c->size      = MIN_SIZE;
    binptr cm    = (binptr) chunk2mem(c);
    c->next = &arr[ix];
    c->back = &arr[ix];
    arr[ix].next = cm;
    arr[ix].back = cm;
    bm_mark(map, ix);
}

Test(binmap, word_count_is_a_true_ceiling) {
    /* 96 bins need 2 words. Plain NBINS/64 truncates to 1 and would leave
     * bins 64..95 with no bit at all. */
    cr_assert_eq(BINMAP_WORDS, (NBINS + 63) / 64);
    cr_assert_geq(BINMAP_WORDS * 64, NBINS, "binmap cannot address every bin");
}

Test(binmap, bit_addressing_round_trips_for_every_bin) {
    fresh();
    for (unsigned i = 0; i < NBINS; i++) {
        cr_assert(!bm_is_marked(map, i), "bin %u marked before we touched it", i);
        bm_mark(map, i);
        cr_assert(bm_is_marked(map, i), "mark(%u) did not take", i);
        bm_clear(map, i);
        cr_assert(!bm_is_marked(map, i), "clear(%u) did not take", i);
    }
}

Test(binmap, marking_one_bin_does_not_disturb_its_neighbours) {
    fresh();
    bm_mark(map, 64); /* first bin of the SECOND word -- the boundary case */
    for (unsigned i = 0; i < NBINS; i++)
        cr_assert_eq(bm_is_marked(map, i) != 0, i == 64, "bin %u wrong", i);
}

Test(binmap, scan_finds_the_lowest_marked_bin_at_or_above_from) {
    fresh();
    occupy(36, 0);
    occupy(90, 1); /* deliberately in the second binmap word */

    cr_assert_eq(bin_find_next_nonempty(arr, map, 0), 36u);
    cr_assert_eq(bin_find_next_nonempty(arr, map, 36), 36u, "must include `from` itself");
    cr_assert_eq(bin_find_next_nonempty(arr, map, 37), 90u);
    cr_assert_eq(bin_find_next_nonempty(arr, map, 91), BIN_NONE);
}

Test(binmap, empty_map_reports_nothing) {
    fresh();
    cr_assert_eq(bin_find_next_nonempty(arr, map, 0), BIN_NONE);
}

Test(binmap, stale_bits_do_not_fool_the_scan_and_self_heal) {
    fresh();
    occupy(90, 0);
    bm_mark(map, 45); /* marked but EMPTY -- legal, this is the lazy-clear design */
    bm_mark(map, 70); /* another one, in the second word */

    /* A stale bit must not be reported as a usable bin... */
    cr_assert_eq(bin_find_next_nonempty(arr, map, 40), 90u);
    /* ...and the scan must have cleaned both up on the way past. */
    cr_assert(!bm_is_marked(map, 45), "scan left a stale bit on 45");
    cr_assert(!bm_is_marked(map, 70), "scan left a stale bit on 70");
    /* A bin that really has a chunk keeps its bit. */
    cr_assert(bm_is_marked(map, 90));
}

Test(binmap, an_all_stale_map_scans_to_nothing_and_ends_clean) {
    fresh();
    for (unsigned i = 2; i < NBINS; i++)
        bm_mark(map, i); /* every bin marked, every bin empty */

    cr_assert_eq(bin_find_next_nonempty(arr, map, 0), BIN_NONE);
    for (unsigned i = 0; i < NBINS; i++)
        cr_assert(!bm_is_marked(map, i), "bin %u still marked after a full scan", i);
}

Test(binmap, scan_never_looks_backwards) {
    fresh();
    occupy(10, 0);
    /* bin 10 has a chunk, but we start the scan above it */
    cr_assert_eq(bin_find_next_nonempty(arr, map, 11), BIN_NONE);
    /* and it is still there, untouched */
    cr_assert(bm_is_marked(map, 10));
}

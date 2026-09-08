#include "malloc.h"
#include "stdio.h"
#include <sys/mman.h>

static mstate memory_state;
static mstateptr state_ptr = &memory_state;

/* ------------------------------------------------------------------ helpers */

/* Hand `v` to the caller, returning any usable leftover to the unsorted bin.
 * `v` must already be unlinked from whatever bin held it. */
static void *take_chunk(mchunkptr v, size_t nb) {
    if (can_split(v, nb)) {
        mchunkptr rem;
        split_chunk(v, nb, rem);
        /* Remainder goes to UNSORTED, never straight to a sized bin: the next
         * malloc may want it, in which case computing its bin index would have
         * been wasted work. */
        insert_at_head(bins, UNSORTED_BIN_IDX, rem);
        set_chunk_free();
    } else {
        /* EXHAUST: the leftover would be too small to be a legal chunk, so the
         * whole chunk goes to the caller. Do NOT shrink v -- its size field is
         * the physical distance to the next chunk and must stay true. The extra
         * bytes are internal fragmentation and come back on free. */
    }
    set_prev_in_use(next_chunk(v)); /* v is in use: tell the neighbour */
    return chunk2mem(v);
}

/* ------------------------------------------------------------------- malloc */

void *dl_malloc(size_t req) {
    size_t aligned_req = request2size(req);

    if (top_empty()) {
        init_state();
    }

    /* Nothing has ever been freed: skip every bin, the drain and the bitmap. */
    if (!any_bin_free()) {
        return fetch_mem_from_top(aligned_req);
    }

    /* ---- 1. fastbin: exact size class, LIFO, no split ------------------- */
    if (aligned_req <= MAX_FASTBIN_SIZE && !is_bin_empty(fastbins, fastbin_ix(aligned_req))) {
        binptr head = &state_ptr->fastbins[fastbin_ix(aligned_req)];
        mchunkptr victim = mem2chunk(head->next); /* LIFO: still cache-hot */
        remove_from_bin(victim);
        set_prev_in_use(next_chunk(victim));
        return chunk2mem(victim);
    }

    /* ---- 2. drain the unsorted bin --------------------------------------
     * Every chunk gets ONE chance at an exact-fit reuse before being filed.
     * This is the only place a chunk enters a sized bin, and the only place
     * the binmap is marked.
     *
     * It is also a PREREQUISITE for steps 3 and 4: a chunk sitting unsorted is
     * invisible to the bin lookups below. */
    binptr unsorted = unsorted_bins();
    binptr p = unsorted->next;
    while (p != unsorted) {
        binptr nxt = p->next; /* save before we unlink */
        mchunkptr c = mem2chunk(p);
        size_t csize = chunk_size(c);

        if (csize == aligned_req) {
            remove_from_bin(c); /* must leave the bin before being handed out */
            set_prev_in_use(next_chunk(c));
            return chunk2mem(c);
        }

        remove_from_bin(c);
        unsigned j = bin_ix(csize);
        insert_at_head(bins, j, c);
        mark_bin(j); /* the ONLY place a binmap bit is set */

        p = nxt;
    }

    unsigned ix = bin_ix(aligned_req);

    /* ---- 3. this size's own bin -----------------------------------------
     * A small bin holds ONE exact size, so a non-empty bin is a guaranteed
     * exact fit -- no search, no split.
     * A large bin holds a RANGE, so walk it and take the smallest that fits. */
    if (aligned_req < MIN_LARGE_SIZE) {
        if (!is_bin_empty(bins, ix)) {
            binptr head = &state_ptr->bins[ix];
            mchunkptr victim = mem2chunk(head->back); /* FIFO: take the tail */
            remove_from_bin(victim);
            set_prev_in_use(next_chunk(victim));
            return chunk2mem(victim);
        }
    } else {
        binptr head = &state_ptr->bins[ix];
        for (binptr q = head->back; q != head; q = q->back) {
            mchunkptr c = mem2chunk(q);
            if (chunk_size(c) >= aligned_req) {
                remove_from_bin(c);
                return take_chunk(c, aligned_req);
            }
        }
    }

    /* ---- 4. binmap scan: the next non-empty bin ABOVE ours --------------
     * Every chunk in a larger bin is big enough by construction, so take the
     * smallest one with no search at all. */
    unsigned j = next_nonempty_bin(ix + 1);
    if (j != BIN_NONE) {
        binptr head = &state_ptr->bins[j];
        mchunkptr victim = mem2chunk(head->back); /* smallest in that bin */
        remove_from_bin(victim);
        return take_chunk(victim, aligned_req);
    }

    /* ---- 5. nothing reusable anywhere: grow ------------------------------ */
    return fetch_mem_from_top(aligned_req);
}

/* --------------------------------------------------------------------- free */

void dl_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    set_chunk_free();
    mchunkptr chunk = mem2chunk(ptr);
    size_t size = chunk_size(chunk);

    /* ---- fastbin: the short path.
     * Deliberately does NOT publish the chunk as free -- it keeps claiming to
     * be in use so its neighbours will not try to coalesce with it. That is
     * what makes a singly-reachable list safe and free this cheap. */
    if (size <= MAX_FASTBIN_SIZE) {
        insert_at_head(fastbins, fastbin_ix(size), chunk);
        return;
    }

    /* ---- coalesce backward.
     * Legal only when PREV_IN_USE is clear -- that bit is what says the footer
     * below us is a real size rather than the previous chunk's user data. */
    if (!prev_in_use(chunk)) {
        mchunkptr prev = prev_chunk(chunk);
        remove_from_bin(prev); /* it must leave its bin before being absorbed */
        coalece(prev, chunk);
        chunk = prev;
    }

    /* ---- coalesce forward. Skip if the neighbour is top (mmaped flag). */
    mchunkptr next = next_chunk(chunk);
    if (!is_mmaped(next) && next_chunk_free(chunk)) {
        remove_from_bin(next);
        coalece(chunk, next);
    }

    /* ---- publish: footer + clear the neighbour's PREV_IN_USE.
     * Use the MERGED size, not the size we were called with. */
    size_t merged = chunk_size(chunk);
    set_foot(chunk, merged);
    unset_prev_in_use(next_chunk(chunk));

    insert_at_head(bins, UNSORTED_BIN_IDX, chunk);
}

/* ----------------------------------------------------------------- from top */

static void *fetch_mem_from_top(size_t req) {
    mchunkptr ta = state_ptr->top_allocation;
    INTERNAL_SIZE_T ts;

    /* A chunk of payload `req` needs req + CHUNK_OVERHEAD bytes, and top must
     * survive with at least a legal chunk's worth left over. */
    size_t need = req + CHUNK_OVERHEAD + MIN_PAYLOAD;

    if (top_empty()) {
        /* Ask for as many whole pages as `need` requires -- NOT a single page.
         * One page silently fails for any request bigger than PAGE_SIZE. */
        size_t bytes = pages_for(need + CHUNK_OVERHEAD);
        void *m = mmap_bytes_at(NULL, bytes);
        if (m == MAP_FAILED) {
            panic("out of memory");
        }
        ta = m;
        ts = bytes - CHUNK_OVERHEAD;
    } else if (chunk_size(ta) < need) {
        size_t deficit = need - chunk_size(ta);
        size_t bytes = pages_for(deficit);
        char *want = (char *) ta + chunk_size(ta) + CHUNK_OVERHEAD;
        void *got = mmap_bytes_at(want, bytes);
        if (got == MAP_FAILED) {
            panic("out of memory");
        }
        if (got == want) {
            /* Contiguous: top simply got bigger. */
            ts = chunk_size(ta) + bytes;
        } else {
            /* The hint was ignored. We have no segment list or fenceposts, so
             * the old top cannot be linked to the new region -- abandon it and
             * start fresh. (A real allocator keeps segments + fenceposts here.) */
            size_t fresh = pages_for(need + CHUNK_OVERHEAD);
            if (bytes < fresh) {
                munmap(got, bytes);
                got = mmap_bytes_at(NULL, fresh);
                if (got == MAP_FAILED) {
                    panic("out of memory");
                }
                bytes = fresh;
            }
            ta = got;
            ts = bytes - CHUNK_OVERHEAD;
        }
    } else {
        ts = chunk_size(ta);
    }

    mchunkptr user_data = ta;

    set_size(user_data, req);
    set_prev_in_use(user_data);
    unset_mmaped(user_data);

    /* top moves past the chunk we just carved */
    bump_top_to_offset(ta, req);
    set_size(ta, ts - req - CHUNK_OVERHEAD);
    set_prev_in_use(ta); /* the chunk below top is in use */
    set_mmaped(ta);      /* marks "this is top" for free()'s forward check */

    state_ptr->top_allocation = ta;
    return chunk2mem(user_data);
}

/* ----------------------------------------------------------------- init */

static void init_state() {
    state_ptr->max_free_bin = 0; /* nothing freed yet */

    /* from 0: bin 0 is the UNSORTED bin and must be initialised too */
    for (unsigned i = 0; i < NBINS; i++) {
        init_bin(bins, i);
    }
    for (unsigned i = 0; i < NFASTBIN; i++) {
        init_bin(fastbins, i);
    }
    bm_init(state_ptr->binmap);
}

#ifdef TEST

void reset_mem_state() {
    state_ptr->top_allocation = NULL;
    init_state();
}

unsigned test_bin_ix(size_t size) { return bin_ix(size); }
int test_bin_is_marked(unsigned i) { return bin_is_marked(i); }
unsigned test_next_nonempty_bin(unsigned from) { return next_nonempty_bin(from); }
int test_bin_is_empty(unsigned i) { return is_bin_empty(bins, i); }
size_t test_bin_count(unsigned i) {
    size_t n = 0;
    binptr head = &state_ptr->bins[i];
    for (binptr p = head->next; p != head; p = p->next) {
        n++;
        if (n > 1024) {
            break;
        }
    }
    return n;
}
size_t test_largest_free(void) {
    size_t big = 0;
    for (unsigned i = 0; i < NBINS; i++) {
        binptr head = &state_ptr->bins[i];
        for (binptr p = head->next; p != head; p = p->next) {
            size_t s = chunk_size((mchunkptr) mem2chunk(p));
            if (s > big) {
                big = s;
            }
        }
    }
    return big;
}

#endif

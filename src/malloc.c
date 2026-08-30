#include "malloc.h"
#include "stdio.h"
static mstate memory_state;
static mstateptr state_ptr = &memory_state;

void *dl_malloc(size_t req) {
    size_t aligned_req = request2size(req);

    // this needs to be called whenever no free bin
    if (!any_bin_free()) {
        if (top_empty()) {
            init_state();
        }
        return fetch_mem_from_top(aligned_req);
    }

    // check fastbin, unsorted bin, small bins and then large bins. If no fit, fetch from top
}

void dl_free(void *ptr) {
    mchunkptr chunk = mem2chunk(ptr);

    size_t size = chunk_size(chunk);

    // if < MAX_FASTBIN_SIZE, put to fastbin
    if (size <= MAX_FASTBIN_SIZE) {
        insert_at_head(fastbins, bin_ix(size), chunk);
        set_foot(chunk, size);
        set_prev_in_use(next_chunk(chunk));
        return;
    }

    // else, put it in unsorted list
    bin *unsorted_bins = unsorted_bins();
}

static void *fetch_mem_from_top(size_t req) {
    void *return_mem;
    mchunkptr ta = state_ptr->top_allocation;
    INTERNAL_SIZE_T ts;

    if (top_empty()) {
        return_mem = mmap_at_offset(NULL);

        ts = PAGE_SIZE;
        if (return_mem == MAP_FAILED) {
            panic("invalid memory");
        }
        ta = return_mem;
    } else if (chunk_size(ta) < req) {
        return_mem = mmap_at_offset((char *) ta + (chunk_size(ta) + 2 * SIZE_T));

        ts = ta->size + PAGE_SIZE;

        if (return_mem == MAP_FAILED) {
            panic("invalid memory");
        }
    }

    mchunkptr user_data = (mchunkptr) return_mem;

    set_size(user_data, req);
    set_prev_in_use(user_data);
    set_mmaped(user_data);

    bump_top_to_offset(ta, req);
    set_size(ta, ts - req);

    state_ptr->top_allocation = ta;

    return chunk2mem(user_data);
}

static void init_state() {
    int i = 1;

    for (; i < NBINS; i++) {
        init_bin(bins, i);
    }
    for (i = 1; i < MAX_FASTBIN_SIZE >> 4; i++) {
        init_bin(fastbins, i);
    }
}

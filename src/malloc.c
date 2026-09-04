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
    if (aligned_req < MAX_FASTBIN_SIZE && !is_bin_empty(bins, bin_ix(aligned_req))) {
        binptr bin_head = &bin_at_size(fastbins, aligned_req);
        mchunkptr next_free_chunk = mem2chunk(bin_head->next);
        remove_from_bin(next_free_chunk);
        return chunk2mem(next_free_chunk);
    }

    /*
        when chunks are freed, they're put on unsorted bin unconditionally(except chunks with size <
       `MAX_FASTBIN_SIZE`), so they can be allocated directly from unsorted bin before sending them
       back to their original bin. This is an optimization to make frequently requested allocation
       faster. If the list doesn't have a chunk with the right size, it'll e send back(and
       potentially coalece with )
    */

    binptr unsorted_bin = unsorted_bins();

    /*
     since every new value is put right after head
     the last value's next chunk's next will be equal
     to head's back
    */
    if (!is_bin_empty(bins, 1)) {
        for (binptr next_chunk = unsorted_bin->next; next_chunk->next != unsorted_bin->back;
             next_chunk = next_chunk->next) {
            mchunkptr chunk = mem2chunk(next_chunk);
            size_t chunk_size = chunk_size(chunk);

            // if the chunk is big enough for it, return
            if (chunk_size > aligned_req) {
                split_chunk(chunk, aligned_req);
                return chunk2mem(chunk);
            }
            if (chunk_size == aligned_req) {
                return chunk2mem(chunk);
            }
            // else put it back to the appropriate bin and continue searching
            insert_at_head(bins, bin_ix(chunk_size), chunk);
        }
    }

    /*
        look at small bins based on `aliged_req`. If it's empty, we will move to
        large bins, which will be set at a logrithmic distance(where we will use bin_map)
    */
    if (!is_bin_empty(bins, bin_ix(aligned_req))) {
        binptr small_bin_head = &bin_at_size(bins, aligned_req);
        mchunkptr next_free_chunk = mem2chunk(small_bin_head->next);
        remove_from_bin(next_free_chunk);
        return chunk2mem(next_free_chunk);
    }

    /*
        if nothing found on the bins, get it from top
    */
    return fetch_mem_from_top(aligned_req);
}

void dl_free(void *ptr) {
    set_chunk_free();
    mchunkptr chunk = mem2chunk(ptr);

    size_t size = chunk_size(chunk);

    // if < MAX_FASTBIN_SIZE, put to fastbin
    if (size <= MAX_FASTBIN_SIZE) {
        insert_at_head(fastbins, bin_ix(size), chunk);
        set_foot(chunk, size);
        set_prev_in_use(next_chunk(chunk));
        return;
    }

    // coalece front and back if free
    if (!prev_in_use(chunk)) {
        mchunkptr prev_chunk = prev_chunk(chunk);
        coalece(prev_chunk, chunk);
        chunk = prev_chunk;
    }
    if (next_chunk_free(chunk)) {
        mchunkptr next_chunk = next_chunk(chunk);
        coalece(chunk, next_chunk);
    }

    insert_at_head(bins, 1, chunk);
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

    state_ptr->max_free_bin = MAX_FASTBIN_SIZE;

    for (; i < NBINS; i++) {
        init_bin(bins, i);
    }
    for (i = 1; i < MAX_FASTBIN_SIZE >> 4; i++) {
        init_bin(fastbins, i);
    }
}

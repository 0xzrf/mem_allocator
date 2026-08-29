#include "malloc.h"
#include "stdio.h"
static mstate memory_state;
static mstateptr state_ptr = &memory_state;

void * dl_malloc(size_t req) {
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

void dl_free(void *) {}

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
        return_mem = mmap_at_offset((char *)ta + (chunk_size(ta) + 2 * SIZE_T));

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
}



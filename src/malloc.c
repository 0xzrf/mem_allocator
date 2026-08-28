#include "malloc.h"

static mstate memory_state;
static mstateptr state_ptr = NULL;

void * dl_malloc(size_t req) {
    size_t aligned_req = request2size(req);

    // this needs to be called whenever no free bin
    if (!any_bin_free()) {
        if (top_empty()) {
            init_state();
        }
    }
}

static void *fetch_mem_from_os(size_t req) {
    void *return_mem;
    mchunkptr ta = state_ptr->top_allocatioin;
    INTERNAL_SIZE_T ts = ta->size;

    if (ta == NULL) {
        return_mem = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);

        ts = PAGE_SIZE;
         if (return_mem == MAP_FAILED) {
             panic("invalid memory");
        }
        ta = return_mem;
    } else if (chunk_size(ta) < req) {
        return_mem = mmap((char *)ta + (chunk_size(ta) + 2 * SIZE_T), PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);

        ts = ts + PAGE_SIZE;

         if (return_mem == MAP_FAILED) {
             panic("invalid memory");
        } 
    }

    mchunkptr user_data = (mchunkptr) return_mem;

    set_size(user_data, req);
    bump_top_to_offset(req);

    // fd is unused by us if it's 
    return (void *) user_data->fd;
}

static void init_state() {

}


void dl_free(void *) {}
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
    void * return_mem = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);

    if (return_mem == NULL) {
        panic("invalid memory");
    }

    return return_mem;
}

static void init_state() {

}


void dl_free(void *) {}
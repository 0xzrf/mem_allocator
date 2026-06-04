#include "../include/malloc.h"

static mstate malloc_state;

void *malloc(size_t size) {
    printf("Allocating memory\n");

    size_t normalized_size = request_2_size(size);

    /*
     * we need to check to see if the bins are populated or not(from malloc_state)
     * If not, the path would be to check the top(wilderness) chunk, which will have
     * additional memory allocated to it
     *
     * This will be called when the malloc is first called, and later populate the bins
     * when the program frees the memory allocated by this malloc
     */
    if (!has_any_chunk(malloc_state)) {
        if (malloc_state->max_fast == 0) {
            init_malloc_state();
        }
        return use_top(size);
    }

    return get_mem_from_os(size);
}

void init_malloc_state() {
    chunk_ptr bin;
    for (size_t i = 0; i < NBINS; i++) {
        bin = bin_at(i);
        bin->data = bin; // TODO: We might later add the `bk` field, which will need to be initialized to bin here
    }

    malloc_state->top->size = 0;
    malloc_state->top->data = NULL;
}

void *use_top(size_t size) {
    if (chunksize(malloc_state->top) < size) {

    }
}

void *get_mem_from_os(size_t size) {
    void *mem =  mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

    if (mem == MAP_FAILED) {
        printf("Couldn't allocate memory from the OS\nerrno value: %d\n", errno);
        exit(1);
    }
    return mem;
}

#include "../include/malloc.h"

static mstate malloc_state;

void *malloc(size_t size) {
    printf("Allocating memory\n");

    size_t normalized_size = request_2_size(size);

    return get_mem_from_os(size);
}

void *get_mem_from_os(size_t size) {
    void *mem =  mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

    if (mem == MAP_FAILED) {
        printf("Couldn't allocate memory from the OS\nerrno value: %d\n", errno);
        exit(1);
    }
    return mem;
}

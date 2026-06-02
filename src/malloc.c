#include "../include/malloc.h"
#include <stdlib.h>

void *malloc(size_t size) {
    printf("allocating memory\n");
   return get_mem_from_os(size);
}

void *get_mem_from_os(size_t size) {
    void *mem =  mmap(NULL, size, PROT_WRITE, MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        printf("Couldn't allocate memory from the OS\n");
        exit(1);
    }
    return mem;
}

#include "../include/malloc.h"

#define PAGE 4096

int main() {
    void *p = malloc(PAGE); // allocate ~ 1 page
    printf("Allocated memory: %p\n", p);
    return 0;
}

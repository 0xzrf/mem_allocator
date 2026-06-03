#include "../include/malloc.h"

int main() {
    void *p = malloc(4096); // allocate ~ 1 page
    printf("Allocated memory: %p\n", p);
    return 0;
}

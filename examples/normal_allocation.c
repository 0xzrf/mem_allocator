#include "../include/malloc.h"

int main() {
    void *p = malloc(1 >> 10);
    printf("Allocated memory: %p", p);
    return 0;
}

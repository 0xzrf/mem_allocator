#include "../include/malloc.h"

int main() {
    printf("Allocating memory");
    void *p = malloc(1);
    printf("Allocated memory: %p", p);
    return 0;
}

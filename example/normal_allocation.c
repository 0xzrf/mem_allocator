#include "../include/malloc.h"

#define PAGE 4096 / 2

int main() {
  void *p = malloc(PAGE); // allocate ~ 1 page
  printf("Allocated memory: %p\n", p);

  free(p);
  return 0;
}

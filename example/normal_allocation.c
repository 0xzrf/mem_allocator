#include "../include/malloc.h"

#define KB 1024

int main() {
  for (int i = 0; i < 4; i++) {
    dl_free(dl_malloc(KB - (i * 256)));
  }
}

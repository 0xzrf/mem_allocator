#include <criterion/criterion.h>
#include <stddef.h>

#include "malloc.h"

Test(malloc_correctness, malloc_works) {
    void *ret_ptr = dl_malloc(8);
}
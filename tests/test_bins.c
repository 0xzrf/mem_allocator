
#include <criterion/criterion.h>
#include <stddef.h>

#include "bins.h"

Test(bin_correctness, request2size_generates_malloc_aligned_output) {
    cr_assert_eq(request2size(8), MIN_SIZE);
    cr_assert_eq(request2size(32), 48);
    cr_assert_eq(request2size(48), 48 + 16);
}
/* Layout tests for struct mem_chunk.
 *
 * The allocator's whole addressing scheme assumes a fixed header shape:
 *   4 words of size_t, tightly packed, no padding.
 * On any LP64 target that is 32 bytes, which is also the smallest chunk we
 * can ever hand out. If this file fails, pointer arithmetic elsewhere lies.
 */

#include <criterion/criterion.h>
#include <stddef.h>

#include "chunk.h"

/* Caught at compile time as well as run time -- a bad layout should never
 * even produce a test binary. */
_Static_assert(sizeof(struct mem_chunk) == 32,
               "mem_chunk must be exactly 4 words with no padding");

            
Test(chunk_layout, chunk_size_correct) {
    cr_assert_eq(sizeof(struct mem_chunk), 32);
}
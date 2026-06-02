#ifndef alloc_common_h
#define alloc_common_h

#include <stddef.h>
#include <stdint.h>

#define INTERNAL_SIZE_T size_t
#define BYTE_PTR uint8_t *
#define SIZE_SZ (sizeof(INTERNAL_SIZE_T))

#define chunk_ptr chunk_header *

#endif

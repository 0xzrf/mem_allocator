#include <stddef.h>
#include <stdint.h>

#define INTERNAL_SIZE_T size_t
#define BYTE_PTR uint8_t *
#define SIZE_SZ (sizeof(INTERNAL_SIZE_T))
#define MALLOC_ALIGNMENT (2 * SIZE_SZ)
#define MALLOC_ALIGN_MASK (MALLOC_ALIGNMENT - 1)
#define CHUNK_SIZE_T unsigned long
#define MIN_CHUNK_SIZE (sizeof(struct chunk_header))
#define MIN_REQ_SIZE                                \
  (CHUNK_SIZE_T)(((MIN_CHUNK_SIZE + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))
#define SYS_ALLOC_PAGE_SIZE 4096

#define request_2_size(req) \
    ((req) + SIZE_SZ + MALLOC_ALIGN_MASK > MIN_REQ_SIZE) ?  \
    MIN_REQ_SIZE :                                          \
    ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK

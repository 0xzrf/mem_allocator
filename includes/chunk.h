#ifndef mem_alloc_chunk_h
#define mem_alloc_chunk_h

#include "./types.h"

typedef struct mem_chunk *mchunkptr;

struct mem_chunk {
   INTERNAL_SIZE_T prev_size;
   INTERNAL_SIZE_T size;
   mchunkptr fd;
   mchunkptr bk;
};

#endif

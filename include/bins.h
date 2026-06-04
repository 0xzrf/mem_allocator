#define NBINS              96 // total bins
#define NSMALLBINS         32 // small bins seperated by 8 bytes each(8..=248)
#define SMALLBIN_WIDTH      8
#define MIN_LARGE_SIZE    256

#define MAX_FAST_SIZE 80
#define NFASTBINS (fastbin_index(request_2_size(MAX_FAST_SIZE)) + 1)

#define in_smallbin_range(sz)  \
  ((CHUNK_SIZE_T)(sz) < (CHUNK_SIZE_T)MIN_LARGE_SIZE)

#define smallbin_index(sz)     (((unsigned)(sz)) >> 3)
#define fastbin_index(sz)      (((sz) >> 3) - 2)

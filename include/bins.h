#define MAX_FAST_SIZE 80

#define DEFAULT_MAX_FASTBIN_SIZE 64

#define NBINS              96 // total bins
#define NSMALLBINS         32 // small bins seperated by 8 bytes each(8..=248)
#define NFASTBINS (fastbin_index(request_2_size(MAX_FAST_SIZE)) + 1)

#define SMALLBIN_WIDTH      8
#define MIN_LARGE_SIZE    256 // the size of the starting large bin

#define in_smallbin_range(sz)  \
  ((CHUNK_SIZE_T)(sz) < (CHUNK_SIZE_T)MIN_LARGE_SIZE)

#define smallbin_index(sz)     (((unsigned)(sz)) >> 3)
#define fastbin_index(sz)      (((sz) >> 3) - 2)
#define is_bin_empty(bin)      ((bin) == bin->data && bin == bin->next_chunk)

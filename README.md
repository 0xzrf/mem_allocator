# UNDER DEVELOPMENT, AND README's DESIGN IS SUBJECT TO CHANGES

## Inspiration
This Memory allocator is inspired by Doug lea's memory allocator.

Why did I choose this? Well, it's pretty simple, and gets you going with memory allocator design
it has a simple tags for free/inuse memory, and reasonable algorithm to allocate memory, prioritizing locality
and small sized allocations.

The design and flow is inspired by the following repo: https://github.com/ennorehling/dlmalloc/blob/master/malloc.c
I'm just trying to implement it on my own to understand different kinds of memory allocators implementations

## Design

### Chunks
Chunks are the atoms of this whole allocator business, and are present continguosely in memory

The basic structure(at least, to have a good mental model) is this:

```C
struct malloc_chunk {

  INTERNAL_SIZE_T      prev_size;  /* Size of previous chunk (if free).  */
  INTERNAL_SIZE_T      size;       /* Size in bytes, including overhead. */

  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;
};
```

In memory, it looks something like this(if in-use by program):
```
chunk ptr ->  +----------------------------+
              | prev_size (or prev's data) |   <- 4 bytes
              +----------------------------+
              | size                 |A|M|P|   <- 4 bytes, low 3 bits = flags, P=InUse, M=Is mmaped
   mem ptr -> +----------------------------+   <- THIS is what malloc returns to you
              |                            |
              |   your usable data         |
              |                            |
              +----------------------------+
              | (next chunk's prev_size)   |
```

the user's data(fd) can be retrieved by skipping over the 2 size headers

Notice the lower 3 bits of `size`, since the allocation is a multiple of 8, the lower 3 bits are always zero(fun fact)
though we only care about the lowest bit, it is set to 0 if it's not in use, and 1 otherwise

now, the prev size is interesting, as it allows to go back to the prev. chunk
When a chunk is freed, it writes it's size to it's *bk pointer(of size INTERNAL_SIZE_T), which writes to the next chunk's prev_size field

So, when we need to merge, we can just look at the in_use flag in size, if it is free, we just offset backward, the pointer to the chunk
by the prev_size bytes

### Bins
There are 4 categories of bins
1. **Fast Bins**: this typically contains recently freed chunk(by default <= 64 bytes), this is a LIFO(like a stack). Index = (size >> 3) - 2
2. **Small Bins**: This contains chunks of sizes 16..=248 bytes. Each bin is seperated linearly by 8 bytes, and the size of each bin's chunk is 8*i. Near instant allocation(assuming the bin has free chunks), but optimizations like bitmap(discussed below) can optimize it's speed. This is a FIFO. Bin index for a specific size allocation(16..=248 bytes, size % 8 == 0) can be calculated using size >> 3
3. **Large Bins**: Not going to support in the first versions, but it will be >= 256 bytes, with logarithmic spacing instead of linear.

### Binmap
An optimization technique to skip over empty bins(will edit more based on what I learn)

## Malloc flow:
1. Normalize the request → chunk size nb (the request2size math above).
2. Fastbin hit? If the size is small enough and that fastbin has a chunk, pop it off the front and return. `O(1)`. This is the hot path for small short-lived allocations.
3. Exact smallbin hit? If small and that smallbin is non-empty, take the last chunk — it's guaranteed an exact fit. `O(1)`.
4. (Large request) consolidate fastbins first. Before doing heavy work for a big request, dump all the pinned fastbin chunks back into the real free pool (they might merge into something big enough). Avoids fragmentation.
5. Process the unsorted bin(put there right after free). Walk the unsorted chunks; either grab one that fits, or file each into its proper small/large bin as you pass.
6. Search largebins by best-fit, using the binmap to skip empty bins, walking up to bigger and bigger bins until something fits.
7. Carve from top. If a bin chunk was bigger than needed, split it: return the front, put the remainder back (in the unsorted bin). If nothing in any bin worked, slice off the top chunk.
8. Ask the OS (mmap) if even top is too small.

## Free flow:
1. `free_size <= malloc_state->max_fast`? push it to the fastbin list
2. if not, then check for the `IS_MMAPD` bit in the chunk, if it is, then return it back to the OS
3. Else, start coelescing that chunk with other neibouring chunks(forward and backword)
4. Push it to the `malloc_state->unsorted_list` (always)

It might come as a surprise here that Free doesn't put back the chunk to the bin of appropriate size.
this memory allocator lazily puts the chunks back to the bins after putting it in `malloc_state->unsorted_list`, giving it one last chance
to be directly allocated before being set to bin again
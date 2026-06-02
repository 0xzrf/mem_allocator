## Inspiration
This Memory allocator is inspired by Doug lea's memory allocator.

Why did I choose this? Well, it's pretty simple, and gets you going with memory allocator design
it has a simple tags for free/inuse memory, and reasonable algorithm to allocate memory, prioritizing locality
and small sized allocations.

## Design
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
chunk ptr ->  +----------------------------+
              | prev_size (or prev's data) |   <- 4 bytes
              +----------------------------+
              | size                 |A|M|P|   <- 4 bytes, low 3 bits = flags
   mem ptr -> +----------------------------+   <- THIS is what malloc returns to you
              |                            |
              |   your usable data         |
              |                            |
              +----------------------------+
              | (next chunk's prev_size)   |

the user's data(fd) can be retrieved by skipping over the 2 size headers

Notice the lower 3 bits of `size`, since the allocation is a multiple of 8, the lower 3 bits are always zero(fun fact)
though we only care about the lowest bit, it is set to 0 if it's not in use, and 1 otherwise

#include "../include/malloc.h"

static struct malloc_state malloc_state;

static void init_malloc_state(mstate);
static void *use_top(mstate, size_t size);
static void *get_mem_from_os(size_t size);
static void return_mem_to_os(void *, size_t);
static void link_unsorted(mstate ms, chunk_ptr c);
static void *finish_allocation(mstate ms, chunk_ptr c, size_t nb, size_t cs);
static void rebin_chunk(mstate ms, chunk_ptr c, size_t cs);
static void *malloc_smallbins(mstate ms, size_t nb);
static void *malloc_unsorted(mstate ms, size_t nb);

void *dl_malloc(size_t size) {
  printf("Allocating memory\n");

  if (size == 0) {
    OUT_ERR("dl_malloc called with size 0");
    return NULL;
  }

  size_t normalized_size = request_2_size(size);

  mstate ms = get_malloc_state();
  /*
   * we need to check to see if the bins are populated or not(from malloc_state)
   * If not, the path would be to check the top(wilderness) chunk, which will
   * have additional memory allocated to it
   *
   * This will be called when the malloc is first called, and later populate the
   * bins when the program frees the memory allocated by this malloc
   */

  if (has_fastchunk(ms) &&
      (CHUNK_SIZE_T)normalized_size <= (CHUNK_SIZE_T)ms->max_fast) {
    size_t fb = fastbin_index(normalized_size);
    chunk_ptr head = ms->fast_bins[fb];
    if (head != NULL) {
      ms->fast_bins[fb] = head->next_chunk;
      head->size |= PREV_INUSE;
      return chunk_2_mem(head);
    }
  }

  if (!has_any_chunk(ms)) {
    if (ms->max_fast == 0) {
      init_malloc_state(ms);
    }
    return use_top(ms, normalized_size);
  }

  if (has_any_chunk(ms) &&
      (CHUNK_SIZE_T)normalized_size > (CHUNK_SIZE_T)ms->max_fast &&
      (CHUNK_SIZE_T)normalized_size <= (CHUNK_SIZE_T)MIN_LARGE_SIZE) {
    void *mem = malloc_smallbins(ms, normalized_size);
    if (mem != NULL) {
      return mem;
    }
    OUT_ERR("smallbin miss for normalized_size=%zu", normalized_size);
  }

  if (!is_bin_empty(unsorted_bin(ms))) {
    void *mem = malloc_unsorted(ms, normalized_size);
    if (mem != NULL) {
      return mem;
    }
    OUT_ERR("unsorted bin miss for normalized_size=%zu", normalized_size);
  }

  return use_top(ms, normalized_size);
}

void dl_free(void *ptr) {
  if (ptr == NULL) {
    OUT_ERR("dl_free called with NULL pointer");
    return;
  }

  chunk_ptr cptr = mem_2_chunk(ptr);
  mstate ms = get_malloc_state();
  INTERNAL_SIZE_T size = chunksize(cptr);

  // free is a no-op when size == 0
  if (size != 0) {
    // Check if the size is <= ms->max_fast. if true, push it to
    // fastbins
    if (size <= ms->max_fast) {
      set_fastchunk(ms);
      chunk_ptr *head = &(ms->fast_bins[fastbin_index(size)]);
      cptr->next_chunk = *head;
      head = &cptr;
      return;
    }
    // Check if the memory being freed is was allocated from OS
    if (is_mmapd(cptr)) {
      return_mem_to_os(cptr, size + 2 * SIZE_SZ);
      return;
    }
    // coalesce and put to unsorted list since this is a value of size >
    // max_fast && < MMAP_TAG_THRESHOLD
    set_any_chunk(ms);

    chunk_ptr next_chunk = chunk_at_offset(cptr, size);
    size_t next_size = chunksize(next_chunk);

    // STEP 1: Check for coalescing possibility from prev chunk
    if (!prev_inuse(cptr)) {
      size_t prevsize = cptr->prev_size;
      size_t new_size = prevsize;
      chunk_ptr next_chunk = cptr->next_chunk;
      cptr = chunk_at_offset(cptr, -((long)prevsize));
      unlink_chunks(cptr, new_size, next_chunk);
    }

    // STEP 2: Check for forward coalescing
    if (cptr->next_chunk != ms->top) {
      size_t nextinuse = inuse_bit_at_offset(next_chunk, next_size);
      set_head(next_chunk, next_size);

      if (!nextinuse) {
        size_t new_size = cptr->size + next_size;
        chunk_ptr new_next_chunk = next_chunk->next_chunk;
        unlink_chunks(cptr, new_size, new_next_chunk);
      }
    }

    // STEP 3: Put the new cptr to the unsorted list
    link_unsorted(ms, cptr);

    return;
  }

  OUT_ERR("dl_free chunk has size 0 at ptr=%p", ptr);
}

static void init_malloc_state(mstate ms) {
  chunk_ptr bin;
  for (size_t i = 1; i < NBINS; i++) {
    bin = bin_at(ms, i);
    bin->data = bin->next_chunk = bin; // bin == bin.data == bin.next_chunk is
                                       // how we know that a bin is empty
  }

  ms->max_fast = DEFAULT_MAX_FASTBIN_SIZE;
  ms->top = initial_top(ms);
}

static void *use_top(mstate ms, size_t size) {
  printf("using top\n");
  void *mem;
  int new_size;
  INTERNAL_SIZE_T top_size = chunksize(ms->top);
  if (top_size < size) {
    PRINT_LD_2(top_size, size);
    mem = get_mem_from_os(SYS_ALLOC_PAGE_SIZE);
    if (mem == NULL) {
      OUT_ERR("use_top failed to obtain memory from OS");
      return NULL;
    }
    new_size = SYS_ALLOC_PAGE_SIZE - size;
  } else {
    PRINT_LD_2(top_size, size);
    mem = ms->top->data;
    new_size = ms->top->prev_size - size;
  }

  /*
  Since we're cutting off the front of the memory allocated from mmap
  We need to also make sure that we're storing it in a chunk and returning it to
  the user since when they return this ptr, we need to know it's size
  */

  // bump up the mem to reserve some space for the header
  printf("Assigning user_data = (char *)mem + 2 * SIZE_SZ;\n");
  char *user_data = chunk_2_mem(mem);

  printf("Updating ms->top->data = (char *)user_data + size;\n");
  ms->top->data = user_data + size; // bumps up the pointer of top by size(since
                                    // it's being allocated to the program)

  printf("Updating ms->top->size = new_size;\n");
  ms->top->size = new_size;

  printf("Assigning ch = mem_2_chunk(user_data);\n");
  chunk_ptr ch = mem_2_chunk(user_data);

  printf("Setting ch->size = size;\n");
  ch->size = size;

  printf("Checking if size >= MMAP_TAG_THRESHOLD;\n");
  if (size >= MMAP_TAG_THRESHOLD) {
    printf("Calling set_mmapd(ch);\n");
    set_mmapd(
        ch); // sets the second last bit to be 1, specifying that this chunk
             // is mmap allocated, and should be returned back to the OS
  }

  printf("Setting ch->prev_size = 0;\n");
  ch->prev_size = 0;

  printf("Setting ch->data = user_data;\n");
  ch->data = user_data;

  printf("Setting ch->next_chunk = ms->top->data;\n");
  ch->next_chunk = ms->top->data;

  return ch->data;
}

static void *get_mem_from_os(size_t size) {
  void *mem =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

  if (mem == MAP_FAILED) {
    OUT_ERR("mmap failed for size=%zu errno=%d", size, errno);
    exit(1);
  }
  return mem;
}

static void return_mem_to_os(void *p, size_t size) {
  if (munmap(p, size) == -1) {
    OUT_ERR("munmap failed for p=%p size=%zu errno=%d", p, size, errno);
    exit(1);
  }
}

static void link_unsorted(mstate ms, chunk_ptr c) {
  chunk_ptr b = unsorted_bin(ms);

  (void)ms;
  c->next_chunk = b->data;
  b->data = c;
  if (c->next_chunk == b) {
    b->next_chunk = c;
  }
}

static void link_smallbin(mstate ms, chunk_ptr c, size_t cs) {
  size_t idx = smallbin_index(cs);
  chunk_ptr b = bin_at(ms, idx);

  c->next_chunk = b->data;
  b->data = c;
  if (c->next_chunk == b) {
    b->next_chunk = c;
  }
}

static void *finish_allocation(mstate ms, chunk_ptr c, size_t nb, size_t cs) {
  chunk_ptr next;
  size_t used = nb;

  if (cs < nb) {
    OUT_ERR("finish_allocation: chunk size %zu smaller than request %zu", cs,
            nb);
    return NULL;
  }

  if (cs > nb + MIN_CHUNK_SIZE) {
    set_head(c, nb | PREV_INUSE | (c->size & IS_MMAPED));
    chunk_ptr rem = chunk_at_offset(c, nb);
    set_head(rem, cs - nb);
    set_foot(rem, cs - nb);
    next = chunk_at_offset(rem, cs - nb);
    if (next != ms->top) {
      next->size |= PREV_INUSE;
    }
    link_unsorted(ms, rem);
  } else {
    set_head(c, cs | PREV_INUSE | (c->size & IS_MMAPED));
    used = cs;
  }

  next = chunk_at_offset(c, used);
  if (next != ms->top) {
    next->size |= PREV_INUSE;
  }

  return chunk_2_mem(c);
}

static void *malloc_smallbins(mstate ms, size_t nb) {
  size_t start = smallbin_index(nb);

  for (size_t i = start; i < NSMALLBINS; i++) {
    chunk_ptr b = bin_at(ms, i);
    chunk_ptr c;

    if (is_bin_empty(b)) {
      continue;
    }

    c = b->data;
    b->data = c->next_chunk;
    if (b->data == b) {
      b->next_chunk = b;
    }

    return finish_allocation(ms, c, nb, chunksize(c));
  }

  OUT_ERR("malloc_smallbins: no suitable bin for nb=%zu (searched %zu..%zu)",
          nb, start, (size_t)(NSMALLBINS - 1));
  return NULL;
}

static void *malloc_unsorted(mstate ms, size_t nb) {
  chunk_ptr b = unsorted_bin(ms);
  chunk_ptr c;

  while (!is_bin_empty(b)) {
    c = b->data;
    size_t cs = chunksize(c);

    b->data = c->next_chunk;
    if (b->data == b) {
      b->next_chunk = b;
    }

    if (cs >= nb) {
      return finish_allocation(ms, c, nb, cs);
    }

    rebin_chunk(ms, c, cs);
  }

  OUT_ERR("malloc_unsorted: no chunk large enough for nb=%zu", nb);
  return NULL;
}
static void rebin_chunk(mstate ms, chunk_ptr c, size_t cs) {
  if (cs <= ms->max_fast) {
    size_t idx = fastbin_index(cs);
    set_fastchunk(ms);
    c->next_chunk = ms->fast_bins[idx];
    ms->fast_bins[idx] = c;
  } else if (in_smallbin_range(cs)) {
    link_smallbin(ms, c, cs);
  } else {
    link_unsorted(ms, c);
  }
}
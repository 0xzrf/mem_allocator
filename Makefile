# mem_allocator
#
#   make            build the library (debug: invariant checks ON)
#   make test       build and run the M1 acceptance tests
#   make release    NDEBUG build (invariant checks compiled out,
#                   Tier-0 hardening still in)
#   make bench      run the tools/ benchmarks against this allocator
#   make clean

CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion -g -O1
CPPFLAGS += -Iincludes
LDFLAGS  ?=

SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:src/%.c=build/%.o)
LIB      := build/libmem.a

.PHONY: all test release bench clean
all: $(LIB)

build:
	@mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJ)
	@$(AR) rcs $@ $^
	@echo "built $@"

build/test_%: tests/test_%.c $(LIB) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LIB)

TESTS := build/test_m1 build/test_m2 build/test_m4 build/test_m5 build/test_m6

test: $(TESTS)
	@for t in $(TESTS); do echo "== $$t"; ./$$t || exit 1; echo; done

release: CFLAGS += -O2 -DNDEBUG
release: clean $(LIB)

bench:
	@$(MAKE) -C tools ALLOC=mine all
	@./tools/bin/mine/bench_phases ramp   # M1 has no free(); ramp only

clean:
	@rm -rf build

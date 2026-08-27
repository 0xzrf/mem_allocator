# mem_allocator
#
#   make            build the library (debug: invariant checks ON)
#   make test       build and run the Criterion unit tests in tests/
#   make release    NDEBUG build (invariant checks compiled out,
#                   Tier-0 hardening still in)
#   make bench      run the tools/ benchmarks against this allocator
#   make clean

CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion -g -O1
CPPFLAGS += -Iincludes
LDFLAGS  ?=

# Criterion (unit test framework, `brew install criterion`).
# Override CRITERION_CFLAGS / CRITERION_LIBS if pkg-config can't see it.
PKG_CONFIG       ?= pkg-config
CRITERION_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags criterion 2>/dev/null)
CRITERION_LIBS   ?= $(shell $(PKG_CONFIG) --libs criterion 2>/dev/null || echo -lcriterion)

SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:src/%.c=build/%.o)
LIB      := build/libmem.a

TEST_SRC := $(wildcard tests/test_*.c)
TESTS    := $(TEST_SRC:tests/%.c=build/%)

.PHONY: all test release bench clean
all: $(LIB)

build:
	@mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(LIB): $(OBJ) | build
ifeq ($(strip $(SRC)),)
	@echo "src/ is empty -- nothing to archive into $@ yet"
else
	@$(AR) rcs $@ $^
	@echo "built $@"
endif

# Tests link the objects directly so they work before src/ has anything in it.
build/test_%: tests/test_%.c $(OBJ) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CRITERION_CFLAGS) -o $@ $< $(OBJ) $(LDFLAGS) $(CRITERION_LIBS)

test: $(TESTS)
ifeq ($(strip $(TEST_SRC)),)
	@echo "no tests/test_*.c found"
else
	@for t in $(TESTS); do echo "== $$t"; ./$$t || exit 1; echo; done
endif

release: CFLAGS += -O2 -DNDEBUG
release: clean $(LIB)

bench:
	@$(MAKE) -C tools ALLOC=mine all
	@./tools/bin/mine/bench_phases ramp   # M1 has no free(); ramp only

clean:
	@rm -rf build

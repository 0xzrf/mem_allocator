# mem_allocator
#
#   make            build the library
#   make test       build and run the Criterion unit tests (compiled with -DTEST)
#   make release    NDEBUG build
#   make run_script build and run test_scripts/script.c
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
TEST_SRC := $(wildcard tests/test_*.c)

# Objects live in a per-variant directory. Without this, `make` and `make test`
# would share build/*.o -- and since make only compares timestamps, switching
# between them would silently reuse objects built with the WRONG flags.
OBJDIR   := build/obj
TOBJDIR  := build/obj-test
OBJ      := $(SRC:src/%.c=$(OBJDIR)/%.o)
TOBJ     := $(SRC:src/%.c=$(TOBJDIR)/%.o)
LIB      := build/libmem.a
TESTS    := $(TEST_SRC:tests/%.c=build/%)

TESTFLAGS := -DTEST

.PHONY: all test release run_script clean

# Objects reached only through a pattern-rule chain are treated as intermediate
# files and deleted after the build, forcing a full recompile every time.
.SECONDARY: $(OBJ) $(TOBJ)

all: $(LIB)

build $(OBJDIR) $(TOBJDIR):
	@mkdir -p $@

# ---- normal objects -------------------------------------------------------
$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# ---- test objects: same sources, built with -DTEST ------------------------
$(TOBJDIR)/%.o: src/%.c | $(TOBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TESTFLAGS) -c -o $@ $<

$(LIB): $(OBJ) | build
ifeq ($(strip $(SRC)),)
	@echo "src/ is empty -- nothing to archive into $@ yet"
else
	@$(AR) rcs $@ $^
	@echo "built $@"
endif

# The test binary AND the objects it links are both built with -DTEST, so
# test-only hooks like reset_mem_state() are visible on both sides.
build/test_%: tests/test_%.c $(TOBJ) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TESTFLAGS) $(CRITERION_CFLAGS) \
	      -o $@ $< $(TOBJ) $(LDFLAGS) $(CRITERION_LIBS)

test: $(TESTS)
ifeq ($(strip $(TEST_SRC)),)
	@echo "no tests/test_*.c found"
else
	@for t in $(TESTS); do echo "== $$t"; ./$$t || exit 1; echo; done
endif

release: CFLAGS += -O2 -DNDEBUG
release: clean $(LIB)

run_script: | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -o build/script test_scripts/script.c $(SRC)
	@./build/script

clean:
	@rm -rf build

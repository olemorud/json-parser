all:

BUILD ?= debug
COMPILER ?= gcc

# ==== set compiler flags ====
# credits Maxim Egorushkin:
# https://stackoverflow.com/questions/48791883/best-practice-for-building-a-make-file/48793058#48793058

CC := gcc

# -fsanitize={address,undefined}
CFLAGS.gcc.debug := -Og -ggdb -pg -DBACKTRACE -rdynamic -fsanitize=address -fno-omit-frame-pointer
CFLAGS.gcc.release := -O3 -g -march=native -DNDEBUG
CFLAGS.gcc := -fanalyzer

CFLAGS.clang.debug := -O0 -g3 -DBACKTRACE -rdynamic
CFLAGS.clang.release := -O3 -g -march=native -DNDEBUG
CFLAGS.clang := ${CFLAGS.clang.${BUILD}}

CFLAGS := ${CFLAGS.${COMPILER}} ${CFLAGS.${COMPILER}.${BUILD}} \
		  -Iinclude -Iarena-allocator/include \
		  -std=c2x \
		  -Wall -Wextra -Wpedantic -Werror

LD_PRELOAD:=

# ==== end set compiler flags ====

BUILD_DIR := bin/${BUILD}

OBJ_DIR := .obj/${BUILD}
_OBJS := main.o parse.o json_value.o util.o libarena.a
OBJS := $(patsubst %,$(OBJ_DIR)/%,$(_OBJS))

all : $(BUILD_DIR)/parse

$(BUILD_DIR)/parse : $(OBJS) $(OBJ_DIR)/libarena.a | $(BUILD_DIR) Makefile
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o : src/%.c $(wildcard include/%.h) | $(OBJ_DIR) Makefile arena-allocator
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

$(OBJ_DIR)/libarena.a : arena-allocator .gitmodules $(wildcard .git/modules/arena-allocator/HEAD) | $(OBJ_DIR) Makefile
	(cd arena-allocator && make static)
	cp --force arena-allocator/lib/libarena.a $(OBJ_DIR)/libarena.a

arena-allocator : .gitmodules
	git submodule update $@

$(BUILD_DIR) :
	mkdir -p $@

$(OBJ_DIR) :
	mkdir -p $@

clean :
	rm -rf $(OBJ_DIR) $(BUILD_DIR) arena-allocator

.PHONY : clean all

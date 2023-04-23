all:

BUILD ?= debug
COMPILER ?= gcc

# ==== set compiler flags ====
# credits Maxim Egorushkin:
# https://stackoverflow.com/questions/48791883/best-practice-for-building-a-make-file/48793058#48793058

CC := gcc

# -fsanitize={address,undefined}
CFLAGS.gcc.debug := -Og -ggdb -fanalyzer -DBACKTRACE -rdynamic -fsanitize=address
CFLAGS.gcc.release := -O3 -march=native -DNDEBUG
CFLAGS.gcc := ${CFLAGS.gcc.${BUILD}} -Iinclude -W{all,extra,error} -fstack-protector-all -std=gnu11

CFLAGS.clang.debug=-O0 -g3 -DBACKTRACE -rdynamic
CFLAGS.clang.release=-O3 -march=native -DNDEBUG
CFLAGS.clang=-Wextra -Wall -Wpedantic -fstack-protector-all ${CFLAGS.clang.${BUILD}}

CFLAGS := ${CFLAGS.${COMPILER}}

LD_PRELOAD:=

# ==== end set compiler flags ====

BUILD_DIR := bin/${BUILD}

OBJ_DIR := .obj/${BUILD}
_OBJS := main.o parse.o json_value.o util.o
OBJS := $(patsubst %,$(OBJ_DIR)/%,$(_OBJS))


all : $(BUILD_DIR)/parse

$(BUILD_DIR)/parse : $(OBJS) | $(BUILD_DIR) Makefile
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/main.o : src/main.c | $(OBJ_DIR) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

$(OBJ_DIR)/%.o : src/%.c include/%.h | $(OBJ_DIR) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

$(BUILD_DIR) :
	mkdir -p $@

$(OBJ_DIR) :
	mkdir -p $@

clean :
	rm -rfi $(OBJ_DIR) $(BUILD_DIR)

.PHONY : clean all

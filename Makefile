
CC=gcc
CFLAGS=-ggdb -O0
CFLAGS+=-Wextra -Wall -Wpedantic
CFLAGS+=-fsanitize=address -fsanitize=undefined
CFLAGS+=-fanalyzer
CFLAGS+=-rdynamic
CFLAGS+=-Iinclude

LDFLAGS=
LDLIBS=

_OBJS=main.o parse.o json_obj.o util.o
OBJS=$(patsubst %,.obj/%,$(_OBJS))

all: bin/parse

bin/parse: $(OBJS) | bin
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

.obj/main.o: src/main.c | .obj
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

.obj/%.o: src/%.c include/%.h | .obj
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

bin:
	mkdir -p $@

.obj:
	mkdir -p $@

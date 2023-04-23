
#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>
#include <stdio.h>

__attribute__((__noreturn__)) void err_ctx(int exit_code, FILE* fp, const char* format, ...);

void* malloc_or_die(size_t size);
void* realloc_or_die(void* ptr, size_t size);
void* calloc_or_die(size_t nmemb, size_t size);
void print_trace();

#endif

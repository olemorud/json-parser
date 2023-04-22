
#include "util.h"

#include <err.h>
#include <errno.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

void* malloc_or_die(size_t size)
{
    void* result = malloc(size);

    if (result == NULL)
        err(errno, "malloc_or_die failed");

    return result;
}

void* realloc_or_die(void* ptr, size_t size)
{
    ptr = realloc(ptr, size);

    if (ptr == NULL)
        err(errno, "realloc_or_die failed");

    return ptr;
}

void* calloc_or_die(size_t nmemb, size_t size)
{
    void* ptr = calloc(nmemb, size);

    if (ptr == NULL)
        err(errno, "calloc_or_die failed");

    return ptr;
}

// from the glibc man pages
// https://www.gnu.org/software/libc/manual/html_node/Backtraces.html
void print_trace()
{
#ifdef BACKTRACE
    void* array[500];
    char** strings;
    int size, i;

    size = backtrace(array, 500);
    strings = backtrace_symbols(array, size);

    if (strings != NULL) {
        printf("Obtained %d stack frames.\n", size);
        for (i = 0; i < size; i++)
            printf("%s\n", strings[i]);
    }

    free(strings);
#endif
}

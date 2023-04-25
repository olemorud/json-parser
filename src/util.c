/*
    This implements utility functions for the program, such as error
    handling, backtraces and malloc wrappers.

    Copyright (C) 2023 by Ole Morud

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "util.h"
#include "config.h"

#include <err.h> // err
#include <errno.h> // errno
#include <execinfo.h> // backtrace
#include <stdarg.h> // va_list
#include <stdio.h> // fprintf
#include <stdlib.h> // malloc, realloc, calloc

void* malloc_or_die(size_t size)
{
    void* result = malloc(size);

    if (result == NULL)
        err(errno, "malloc_or_die failed");

    return result;
}

void* realloc_or_die(void* ptr, size_t size)
{
    void* tmp = realloc(ptr, size);

    if (ptr == NULL) {
        if (errno != 0) {
            free(ptr);
            err(errno, "realloc_or_die failed");
        }

        fprintf(stderr, "\nrealloc_or_die returned NULL but errno is 0, ptr: %p size: %zu\n", ptr, size);
        exit(EXIT_FAILURE);
    }

    return tmp;
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
        printf("\n\n=== Obtained %d stack frames. === \n", size);
        for (i = 0; i < size; i++)
            printf("%s\n", strings[i]);
    }

    free(strings);
#endif
}

/*
    Prints parser errors with surrounding context and
        terminates the program.

        exit_code - code to exit with
        fp        - file causing parser errors
        format    - error message format string
        ...       - format string arguments
*/
__attribute__((__noreturn__)) void err_ctx(int exit_code, FILE* fp, const char* format, ...)
{
    va_list args;
    static char context[ERROR_CONTEXT_LEN];

    va_start(args, format);
    fputc('\n', stderr);
    vfprintf(stderr, format, args);
    fprintf(stderr, " (at index %zu)\n", ftell(fp));
    va_end(args);

    if (fseek(fp, -(ERROR_CONTEXT_LEN / 2), SEEK_CUR) == 0) {
        size_t n_read = fread(context, sizeof(char), ERROR_CONTEXT_LEN, fp);

        fprintf(stderr, "\ncontext:\n");

        int arrow_offset = 0;
        size_t i;
        for (i = 0; i < ERROR_CONTEXT_LEN / 2 && i < n_read; i++) {
            switch (context[i]) {
            case '\n':
                fprintf(stderr, "\\n");
                arrow_offset += 2;
                break;
            case '\r':
                fprintf(stderr, "\\r");
                arrow_offset += 2;
                break;
            case '\t':
                fprintf(stderr, "\\t");
                arrow_offset += 2;
                break;
            default:
                fputc(context[i], stderr);
                arrow_offset += 1;
                break;
            }
        }

        for (; i < ERROR_CONTEXT_LEN && i < n_read; i++) {
            switch (context[i]) {
            case '\n':
                fprintf(stderr, "\\n");
                break;
            case '\t':
                fprintf(stderr, "\\t");
                break;
            default:
                fputc(context[i], stderr);
                break;
            }
        }

        fputc('\n', stderr);
        for (int i = 0; i < arrow_offset - 2; i++)
            fputc(' ', stderr);

        fputc('^', stderr);
        fputc('\n', stderr);
    }

    exit(exit_code);
}

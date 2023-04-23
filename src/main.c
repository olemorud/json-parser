
#include "json_value.h"
#include "parse.h"
#include "util.h"

#include <err.h> // errx
#include <stdlib.h> // atexit

int main(int argc, char* argv[])
{
    if (argc != 2) {
        errx(EXIT_FAILURE, "Usage: %s <file>", argv[0]);
    }

    atexit(print_trace);

    FILE* fp = fopen(argv[1], "r");

    volatile struct json_value x = parse_json_value(fp);

    print_json(x, 1);

    return EXIT_SUCCESS;
}

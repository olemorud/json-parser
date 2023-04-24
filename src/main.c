
#include "json_obj.h"
#include "parse.h"
#include "util.h"

#include <stdlib.h> // atexit
#include <unistd.h> // _exit

int main()
{
    atexit(print_trace);

    FILE* fp = fopen("sample.json", "r");

    volatile struct json_value x = parse_json_value(fp);

    print_json(x, 1);

    return EXIT_SUCCESS;
}

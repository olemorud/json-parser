/*
    JSON parser

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

#include "json_value.h"
#include "parse.h"
#include "util.h"

#include <err.h> // errx
#include <stdlib.h> // atexit

int main(int argc, char* argv[])
{
    arena_t arena_default = arena_new();

    if (argc != 2) {
        errx(EXIT_FAILURE, "Usage: %s <file>", argv[0]);
    }

    atexit(print_trace);

    FILE* fp = fopen(argv[1], "r");

    struct json_value x = parse_json_value(fp, &arena_default);

    print_json(x, 1);

    fclose(fp);

    arena_delete(&arena_default);

    return EXIT_SUCCESS;
}

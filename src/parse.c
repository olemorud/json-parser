/*
    This implements the main JSON parsing part. C implementations of
    maps and json values can be found in json_value.c

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

#include "parse.h"

#include "arena.h"
#include "config.h"
#include "json_value.h"
#include "util.h"

#include <ctype.h> // isspace, isdigit
#include <err.h> // err, errx
#include <math.h> // NAN
#include <stdio.h>
#include <stdlib.h> // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h> // strcmp

bool read_boolean(FILE* fp);
char* read_string(FILE* fp, arena_t* arena);
double read_number(FILE* fp);
obj_t* read_object(FILE* fp, arena_t* arena);
struct json_value** read_array(FILE* fp, arena_t* arena);
void discard_whitespace(FILE* fp);
void read_null(FILE* fp);

/*
ghetto way to hunt down bugs
#ifdef __GNUC__
   #define _fgetc(fp) \
       ({int retval; retval = fgetc(fp); fputc(retval, stderr); retval;})
#else
   #define _fgetc(fp) \
       fgetc(fp)
#endif
*/
#define _fgetc(fp) fgetc(fp)

/*
    Consumes the next whitespace character in a file stream
    and discards them.
*/
void discard_whitespace(FILE* fp)
{
    int c;

    while (isspace(c = _fgetc(fp))) {
        if (c == EOF)
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);
    }

    ungetc(c, fp);
}

/*
    Consumes the next JSON value in a file stream and returns a
    corresponding json_value. See `json_value.h` for details.

    A JSON value can be a JSON string in double quotes, or a JSON number,
    or true or false or null, or a JOSN object or a JSON array.

    These structures can be nested.
*/
struct json_value parse_json_value(FILE* fp, arena_t* arena)
{
    discard_whitespace(fp);
    int c = _fgetc(fp);

    struct json_value result = { 0 };

    switch (c) {
    case EOF:
        err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

    case '{':
        result.type = object;
        result.object = read_object(fp, arena);
        break;

    case '"':
        result.type = string;
        result.string = read_string(fp, arena);
        break;

    case '[':
        result.type = array;
        result.array = read_array(fp, arena);
        break;

    case 't':
    case 'f':
        ungetc(c, fp);
        result.type = boolean;
        result.boolean = read_boolean(fp);
        break;

    case 'n':
        ungetc(c, fp);
        read_null(fp);
        result.type = null;
        result.number = 0L;
        break;

    default:
        if (isdigit(c)) {
            ungetc(c, fp);
            result.type = number;
            result.number = read_number(fp);
        } else {
            ungetc(c, fp);
            err_ctx(UNEXPECTED_CHAR, fp, "(%s) unexpected symbol %c", __func__, c);
        }
    }

    return result;
}

/*
    A JSON string is a sequence of zero or more Unicode characters, wrapped in
    double quotes, using backslash escapes.

    A character is represented as a single character JSON string. A JSON string
    is very much like a C or Java string.

    Consumes a JSON string from a file stream and returns a corresponding char*
*/
char* read_string(FILE* fp, arena_t* arena)
{
    int c;
    size_t i = 0, result_size = 16;
    // char* result = malloc_or_die(result_size);
    char* result = arena_alloc(arena, result_size);

    bool escaped = false;

    while (true) {
        if (i + 1 >= result_size) {
            result_size *= 2;
            result = arena_realloc_tail(arena, result_size);
            if (result == NULL)
                err_ctx(EXIT_FAILURE, fp, "could not allocate memory for string"
                                          "%s",
                    __func__);
        }

        c = _fgetc(fp);

        if (escaped) {
            escaped = false;
            result[i++] = c;
        } else {
            switch (c) {
            case '\\':
                escaped = true;
                result[i++] = c;
                break;

            case '"':
                result[i++] = '\0';
                return arena_realloc_tail(arena, i);

            case EOF:
                err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

            default:
                result[i++] = c;
                break;
            }
        }
    }
}

/*
   A JSON object is an unordered set of name/value pairs.

   An object begins with "{" and ends with "}".
   Each name is followed by ":" and the name/value pairs are separated by ",".

   Consumes a JSON object from a file stream and returns a corresponding obj_t
*/
obj_t* read_object(FILE* fp, arena_t* arena)
{
    // obj_t* result = calloc_or_die(1, sizeof(obj_t));
    obj_t* result = arena_calloc(arena, 1, sizeof *result);
    char* key;

    while (true) {
        /* read key */
        discard_whitespace(fp);

        switch (_fgetc(fp)) {
        case EOF:
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

        default:
            err_ctx(UNEXPECTED_CHAR, fp, "(%s) expected \"", __func__);

        case '"':
            key = read_string(fp, arena);
            break;

        case '}':
            return result;
        }

        /* check for ':' separator */
        discard_whitespace(fp);

        switch (_fgetc(fp)) {
        case ':':
            break;

        case EOF:
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

        default:
            err_ctx(UNEXPECTED_CHAR, fp, "(%s) expected ':'", __func__);
        }

        /* read value */
        discard_whitespace(fp);

        // struct json_value* val = calloc_or_die(1, sizeof(struct json_value));
        struct json_value* val = arena_calloc(arena, 1, sizeof *val);
        *val = parse_json_value(fp, arena);

        /* insert key-value pair to obj */
        if (!obj_insert(*result, key, val, arena)) {
            fprintf(stderr, "failed to insert pair (%s, %p)\n", key, (void*)val);
            exit(EXIT_FAILURE);
        }

        /* read separator or end of object */
        discard_whitespace(fp);

        switch (_fgetc(fp)) {
        case EOF:
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

        case ',':
            continue;

        case '}':
            return result;

        default:
            err_ctx(UNEXPECTED_CHAR, fp, "(%s) expected ',' or '}'", __func__);
        }
    }

    return NULL;
}

/*
   A JSON array is an ordered collection of values.
   It begins with "[" and ends with "]". Values are separated by ","

   Consumes a JSON array from a file stream and returns
   a NULL separated array of json_value pointers
*/
struct json_value** read_array(FILE* fp, arena_t* arena)
{
    int c;
    size_t i = 0, output_size = 16;
    struct json_value** output = malloc_or_die(output_size * sizeof *output);
    // struct json_value** output = arena_alloc(arena, output_size * sizeof *output);

    while (true) {
        if (i >= output_size) {
            output_size *= 2;
            // output = arena_realloc_tail(arena, output_size * sizeof *output);
            output = realloc_or_die(output, output_size * sizeof *output);
        }

        discard_whitespace(fp);
        c = _fgetc(fp);

        switch (c) {
        case EOF:
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);
            break;

        case ']':
            output[i++] = NULL;
            struct json_value** x = arena_alloc(arena, i * sizeof *output);
            memcpy(x, output, i * sizeof *output);
            free(output);
            return x;

        case ',':
            continue;

        default:
            ungetc(c, fp);
            // output[i] = malloc_or_die(sizeof(struct json_value));
            output[i] = arena_alloc(arena, sizeof *(output[i]));
            *output[i] = parse_json_value(fp, arena);
            i++;
            break;
        }
    }
}

/*
    Consumes and discards a literal "null" from a file stream

    If the next characters do not match "null"
    it terminates the program with a non-zero code
*/
void read_null(FILE* fp)
{
    static const char ok[] = { 'n', 'u', 'l', 'l' };
    static char buf[sizeof(ok)];

    size_t n_read = fread(buf, sizeof(char), sizeof(ok), fp);

    if (n_read != sizeof(ok))
        err_ctx(EXIT_FAILURE, fp, "(%s) read failure", __func__);

    if (strncmp(buf, ok, sizeof(ok)) != 0)
        err_ctx(UNEXPECTED_CHAR, fp, "(%s) unexpected symbol", __func__);
}

/*
    JSON boolean values are either "true" or "false".

    Consumes a JSON boolean from a file stream and returns true or false.

    Terminates with a non-zero code if the next characters do not match these.
*/
bool read_boolean(FILE* fp)
{
    static const char t[] = { 't', 'r', 'u', 'e' };
    static const char f[] = { 'f', 'a', 'l', 's', 'e' };

    static char buf[sizeof(f)] = { 0 };

    size_t n_read = fread(buf, sizeof(char), sizeof(f), fp);

    if (n_read != sizeof(f))
        exit(EXIT_FAILURE);

    if (strncmp(buf, t, sizeof(t)) == 0) {
        ungetc(buf[4], fp);
        return true;
    }

    if (strncmp(buf, f, sizeof(f)) == 0)
        return false;

    fseek(fp, -sizeof(f), SEEK_CUR);
    err_ctx(UNEXPECTED_CHAR, fp, "(%s) unexpected symbol", __func__);
}

/*
    A JSON number is very much like a C or Java number,
    except that the octal and hexadecimal formats are not used.

    Consumes a JSON number from a file stream and returns the number
*/
double read_number(FILE* fp)
{
    double n = NAN;

    int n_read = fscanf(fp, "%lf", &n);

    if (n_read == 0)
        err_ctx(UNEXPECTED_CHAR, fp, "(%s) number expected, found %lf", __func__, n);

    if (n_read == EOF)
        err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

    return n;
}

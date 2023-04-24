
#include "parse.h"

#include "config.h"
#include "json_value.h"
#include "util.h"

#include <ctype.h> // isspace, isdigit
#include <err.h> // err, errx
#include <stdio.h>
#include <stdlib.h> // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h> // strcmp

char* read_string(FILE* fp);
obj_t* read_object(FILE* fp);
void discard_whitespace(FILE* fp);
bool read_boolean(FILE* fp);
void read_null(FILE* fp);
double read_number(FILE* fp);
struct json_value** read_array(FILE* fp);
void discard_whitespace(FILE* fp);

/*
    Consumes the next whitespace character in a file stream
    and discards them.
*/
void discard_whitespace(FILE* fp)
{
    int c;

    while (isspace(c = fgetc(fp))) {
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
struct json_value parse_json_value(FILE* fp)
{
    discard_whitespace(fp);
    int c = fgetc(fp);

    struct json_value result = { 0 };

    switch (c) {
    case EOF:
        err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

    case '{':
        result.type = object;
        result.object = read_object(fp);
        break;

    case '"':
        result.type = string;
        result.string = read_string(fp);
        break;

    case '[':
        result.type = array;
        result.array = read_array(fp);
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
char* read_string(FILE* fp)
{
    int c;
    size_t i = 0, result_size = 16;
    char* result = malloc_or_die(result_size);

    bool escaped = false;

    while (true) {
        if (i + 1 >= result_size) {
            result_size *= 2;
            result = realloc_or_die(result, result_size);
        }

        c = fgetc(fp);

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
                return realloc_or_die(result, i);

            case EOF:
                free(result);
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
obj_t* read_object(FILE* fp)
{
    obj_t* result = calloc_or_die(1, sizeof(obj_t));
    char* key;

    while (true) {
        /* read key */
        discard_whitespace(fp);

        switch (fgetc(fp)) {
        case EOF:
            free(result);
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

        default:
            free(result);
            err_ctx(UNEXPECTED_CHAR, fp, "(%s) expected \"", __func__);

        case '"':
            key = read_string(fp);
            break;

        case '}':
            return result;
        }

        /* check for ':' separator */
        discard_whitespace(fp);

        switch (fgetc(fp)) {
        case ':':
            break;

        case EOF:
            free(result);
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

        default:
            free(result);
            err_ctx(UNEXPECTED_CHAR, fp, "(%s) expected ':'", __func__);
        }

        /* read value */
        discard_whitespace(fp);

        struct json_value* val = calloc_or_die(1, sizeof(struct json_value));
        *val = parse_json_value(fp);

        /* insert key-value pair to obj */
        if (!obj_insert(*result, key, val)) {
            free(result);
            free(val);
            errx(EXIT_FAILURE, "failed to insert pair (%s, %p)", key, (void*)val);
        }

        /* read separator or end of object */
        discard_whitespace(fp);

        switch (fgetc(fp)) {
        case EOF:
            free(val);
            free(result);
            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);

        case ',':
            continue;

        case '}':
            return result;

        default:
            free(val);
            free(result);
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
struct json_value** read_array(FILE* fp)
{
    int c;
    size_t i = 0, output_size = 16;
    struct json_value** output = malloc_or_die(output_size * sizeof(struct json_value*));

    while (true) {
        if (i + 1 >= output_size) {
            output_size *= 2;
            output = realloc_or_die(output, output_size * sizeof(struct json_value*));
        }

        discard_whitespace(fp);
        c = fgetc(fp);

        switch (c) {
        case EOF:
            free(output);

            for (size_t j = 0; j < i; j++)
                free(output[j]);

            err_ctx(EARLY_EOF, fp, "(%s) unexpected EOF", __func__);
            break;

        case ']':
            output[i++] = NULL;
            return realloc_or_die(output, i * sizeof(struct json_value*));

        case ',':
            continue;

        default:
            ungetc(c, fp);
            output[i] = malloc_or_die(sizeof(struct json_value));
            *output[i] = parse_json_value(fp);
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
    double n;

    fscanf(fp, "%lf", &n);

    return n;
}

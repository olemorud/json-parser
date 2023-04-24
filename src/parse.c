
#include "parse.h"

#include <ctype.h> // isalpha
#include <err.h> // err, warn
#include <errno.h>
#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <string.h> // strdup

#include "json_obj.h"
#include "util.h"

#define EARLY_EOF 202
#define MALLOC_DIE 201
#define UNEXPECTED_CHAR 200

char* read_string(FILE* fp);
obj_t* read_object(FILE* fp);
void discard_whitespace(FILE* fp);
bool read_boolean(FILE* fp);
void read_null(FILE* fp);
int64_t read_number(FILE* fp);
struct json_value** read_array(FILE* fp);

void print_object(obj_t obj, int cur_indent, int indent_amount);
void print_json_value(struct json_value val, int cur_indent, int indent_amount);
void print_array(struct json_value** arr, int cur_indent, int indent_amount);

char* read_string(FILE* fp)
{
    int c;
    size_t i = 0, result_size = 16 * sizeof(char);
    char* result = malloc_or_die(result_size);

    while (true) {
        if (i + 1 >= result_size) {
            result_size *= 2;
            result = realloc_or_die(result, result_size);
        }

        switch (c = fgetc(fp)) {
        default:
            result[i++] = c;
            break;

        case '"':
            result[i++] = '\0';
            return realloc_or_die(result, i);

        case '\\':
            break;

        case EOF:
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);
        }
    }
}

void discard_whitespace(FILE* fp)
{
    int c;

    while (isspace(c = fgetc(fp)))
        if (c == EOF)
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

    ungetc(c, fp);
}

obj_t* read_object(FILE* fp)
{
    obj_t* result = calloc_or_die(1, sizeof(obj_t));
    char* key;
    struct json_value* val = calloc_or_die(1, sizeof(struct json_value));
    int c;

    while (true) {
        discard_whitespace(fp);

        if ((c = fgetc(fp)) == EOF)
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        if (c == '"')
            key = read_string(fp);
        else if (c == '}')
            return result;

        discard_whitespace(fp);

        if ((c = fgetc(fp)) == EOF)
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        if (c != ':')
            errx(UNEXPECTED_CHAR, "(%s) expected separator (':') at index %zu",
                __func__, ftell(fp));

        discard_whitespace(fp);

        *val = parse_json_value(fp);

        bool ok = obj_insert(*result, key, val);

        if (!ok)
            err(EXIT_FAILURE, "failed to insert pair (%s, %p)", key, (void*)val);

        discard_whitespace(fp);

        if ((c = fgetc(fp)) == EOF)
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        if (c == ',')
            continue;
        else if (c == '}')
            return result;
        else
            errx(UNEXPECTED_CHAR, "(%s) expected ',' or '}' at index %zu", __func__,
                ftell(fp));
    }

    return NULL;
}

struct json_value** read_array(FILE* fp)
{
    int c;
    size_t i = 0, output_size = 16 * sizeof(struct json_value*);
    struct json_value** output = malloc_or_die(output_size);

    while (true) {
        c = fgetc(fp);

        if (c == EOF)
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        if (c == ']')
            break;

        if (c == ',')
            continue;

        ungetc(c, fp);

        if (i > output_size) {
            output_size *= 2;
            output = realloc_or_die(output, output_size);
        }

        output[i] = malloc_or_die(sizeof(struct json_value));
        *output[i] = parse_json_value(fp);
        i++;
    }

    output[i] = NULL;

    return realloc_or_die(output, i * sizeof(void*));
}

void read_null(FILE* fp)
{
    static const char ok[] = { 'n', 'u', 'l', 'l' };
    char buf[sizeof(ok)];

    size_t n_read = fread(buf, sizeof(char), sizeof(ok), fp);

    if (n_read != sizeof(ok))
        err(EXIT_FAILURE, "(%s) read failure at index %zu", __func__, ftell(fp));

    if (strncmp(buf, ok, sizeof(ok)) != 0)
        errx(UNEXPECTED_CHAR, "(%s) unexpected symbol at index %zu", __func__,
            ftell(fp));
}

bool read_boolean(FILE* fp)
{
    static const char t[] = { 't', 'r', 'u', 'e' };
    static const char f[] = { 'f', 'a', 'l', 's', 'e' };

    char buf[sizeof(f)] = { 0 };

    size_t n_read = fread(buf, sizeof(char), sizeof(f), fp);

    if (n_read != sizeof(f))
        exit(EXIT_FAILURE);

    if (strncmp(buf, t, sizeof(t)) == 0)
        return true;

    if (strncmp(buf, f, sizeof(f)) == 0)
        return false;

    errx(UNEXPECTED_CHAR, "(%s) unexpected symbol at index %zu", __func__,
        ftell(fp) - n_read);
}

// TODO: fix int overflow
int64_t read_number(FILE* fp)
{
    int c;

    int64_t sum = 0;

    do {
        c = fgetc(fp);

        if (c == EOF)
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        sum *= 10;
        sum += c - '0';
    } while (isdigit(c));

    ungetc(c, fp);

    return sum;
}

struct json_value parse_json_value(FILE* fp)
{
    discard_whitespace(fp);
    int c = fgetc(fp);
    struct json_value result = { 0 };

    switch (c) {
    case EOF:
        err(EARLY_EOF, "(%s) unexpected EOF", __func__);
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
            errx(UNEXPECTED_CHAR, "(%s) unexpected symbol %c at index %zu", __func__,
                c, ftell(fp) - 1);
        }
    }

    return result;
}

void add_indent(int n)
{
    for (int i = 0; i < n; i++) {
        putchar(' ');
    }
}

void print_object(obj_t obj, int cur_indent, int indent_amount)
{
    printf("{");

    for (size_t i = 0; i < OBJ_SIZE; i++) {
        struct obj_entry* e = obj[i];

        if (e == NULL)
            continue;

        while (e != NULL) {
            putchar('\n');
            add_indent(cur_indent);
            printf("\"%s\": ", e->key);
            print_json_value(*(e->val), cur_indent + indent_amount, indent_amount);
            putchar(',');

            e = e->next;
        }
    }
    printf("\b"); // undo last comma
    putchar('\n');

    add_indent(cur_indent - indent_amount * 2);
    printf("}");
}

void print_array(struct json_value** arr, int cur_indent, int indent_amount)
{
    putchar('[');

    for (size_t i = 0; arr[i] != NULL; i++) {
        print_json_value(*arr[i], cur_indent + indent_amount, indent_amount);
        putchar(',');
    }

    putchar(']');
}

void print_json_value(struct json_value val, int cur_indent,
    int indent_amount)
{
    switch (val.type) {
    case string:
        printf("\"%s\"", val.string);
        break;
    case number:
        printf("%zu", val.number);
        break;
    case boolean:
        printf("%s", val.boolean ? "true" : "false");
        break;
    case null:
        printf("null");
        break;
    case object:
        print_object(*val.object, cur_indent + indent_amount, indent_amount);
        break;
    case array:
        print_array(val.array, cur_indent + indent_amount, indent_amount);
        break;
    default:
        printf("<unknown>");
    }
}

void print_json(struct json_value val, int indent)
{
    print_json_value(val, 0, indent);
    putchar('\n');
}

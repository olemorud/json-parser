
#include "parse.h"

#include <ctype.h> // isspace, isdigit
#include <err.h> // err, errx
#include <stdlib.h> // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h> // strcmp

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
double read_number(FILE* fp);
struct json_value** read_array(FILE* fp);

void print_object(obj_t obj, int cur_indent, int indent_amount);
void print_json_value(struct json_value val, int cur_indent, int indent_amount);
void print_array(struct json_value** arr, int cur_indent, int indent_amount);

// define as a macro to make debugging smoother
#define discard_whitespace(fp)                                   \
    do {                                                         \
        int c;                                                   \
        while (isspace(c = fgetc(fp))) {                         \
            if (c == EOF)                                        \
                err(EARLY_EOF, "(%s) unexpected EOF", __func__); \
        }                                                        \
        ungetc(c, fp);                                           \
    } while (0);

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
    size_t i = 0, result_size = 16 * sizeof(char);
    char* result = malloc_or_die(result_size);
    bool escaped = false;

    while (true) {
        if (i + 1 >= result_size) {
            result_size *= 2;
            result = realloc_or_die(result, result_size);
        }

        if (escaped) {
            result[i++] = 'c';
            escaped = false;
            continue;
        }

        switch (c = fgetc(fp)) {
        default:
            result[i++] = c;
            break;

        case '\\':
            escaped = true;
            // intentional fallthrough
        case '"':
            result[i++] = '\0';
            return realloc_or_die(result, i);

        case EOF:
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);
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
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        default:
            errx(UNEXPECTED_CHAR, "(%s) expected \" at index %zu", __func__, ftell(fp));

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
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        default:
            errx(UNEXPECTED_CHAR, "(%s) expected ':' at index %zu", __func__, ftell(fp));
        }

        /* read value */
        discard_whitespace(fp);

        struct json_value* val = calloc_or_die(1, sizeof(struct json_value));
        *val = parse_json_value(fp);

        /* insert key-value pair to obj */
        if (!obj_insert(*result, key, val))
            err(EXIT_FAILURE, "failed to insert pair (%s, %p)", key, (void*)val);

        /* read separator or end of object */
        discard_whitespace(fp);

        switch (fgetc(fp)) {
        case EOF:
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        case ',':
            continue;

        case '}':
            return result;

        default:
            errx(UNEXPECTED_CHAR, "(%s) expected ',' or '}' at index %zu", __func__, ftell(fp));
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
    size_t i = 0, output_size = 16 * sizeof(struct json_value*);
    struct json_value** output = malloc_or_die(output_size);

    while (true) {
        if (i > output_size) {
            output_size *= 2;
            output = realloc_or_die(output, output_size);
        }

        discard_whitespace(fp);
        c = fgetc(fp);

        switch (c) {
        case EOF:
            err(EARLY_EOF, "(%s) unexpected EOF", __func__);

        case ']':
            output[i] = NULL;
            return realloc_or_die(output, i * sizeof(struct json_value*));

        case ',':
            continue;

        default:
            ungetc(c, fp);
            output[i] = malloc_or_die(sizeof(struct json_value));
            *output[i] = parse_json_value(fp);
            break;
        }

        i++;
    }

    output[i] = NULL;

    return realloc_or_die(output, i * sizeof(void*));
}

/*
    Consumes and discards a literal "null" from a file stream

    If the next characters do not match "null"
    it terminates the program with a non-zero code
*/
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

/*
    JSON boolean values are either "true" or "false".

    Consumes a JSON boolean from a file stream and returns true or false.

    Terminates with a non-zero code if the next characters do not match these.
*/
bool read_boolean(FILE* fp)
{
    static const char t[] = { 't', 'r', 'u', 'e' };
    static const char f[] = { 'f', 'a', 'l', 's', 'e' };

    char buf[sizeof(f)] = { 0 };

    size_t n_read = fread(buf, sizeof(char), sizeof(f), fp);

    if (n_read != sizeof(f))
        exit(EXIT_FAILURE);

    if (strncmp(buf, t, sizeof(t)) == 0) {
        ungetc(buf[4], fp);
        return true;
    }

    if (strncmp(buf, f, sizeof(f)) == 0)
        return false;

    errx(UNEXPECTED_CHAR, "(%s) unexpected symbol at index %zu", __func__, ftell(fp) - n_read);
}

// TODO: fix int overflow
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

/*
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
            errx(UNEXPECTED_CHAR, "(%s) unexpected symbol %c at index %zu", __func__, c, ftell(fp) - 1);
        }
    }

    return result;
}

void add_indent(int n)
{
    for (int i = 0; i < n; i++)
        putchar(' ');
}

void print_object(obj_t obj, int cur_indent, int indent_amount)
{
    putchar('{');

    bool first = true;

    for (size_t i = 0; i < OBJ_SIZE; i++) {
        struct obj_entry* e = obj[i];

        while (e != NULL) {
            if (!first)
                putchar(',');

            first = false;

            putchar('\n');
            add_indent(cur_indent);
            printf("\"%s\": ", e->key);
            print_json_value(*(e->val), cur_indent + indent_amount, indent_amount);

            e = e->next;
        }
    }

    putchar('\n');
    add_indent(cur_indent - indent_amount * 2);
    putchar('}');
}

void print_array(struct json_value** arr, int cur_indent, int indent_amount)
{
    putchar('[');

    size_t i;

    for (i = 0; arr[i + 1] != NULL; i++) {
        putchar('\n');
        add_indent(cur_indent);
        print_json_value(*arr[i], cur_indent + indent_amount, indent_amount);
        putchar(',');
    }

    putchar('\n');
    add_indent(cur_indent);
    print_json_value(*arr[i], cur_indent + indent_amount, indent_amount);

    putchar('\n');
    add_indent(cur_indent - indent_amount * 2);
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
}

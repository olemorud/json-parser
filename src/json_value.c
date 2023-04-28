/*
    This implements JSON values such as objects.

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

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_value.h"
#include "util.h"

bool obj_insert(obj_t m, char* const key, struct json_value* value);
size_t obj_hash(char const* str);
void obj_delete(obj_t* m);
void* obj_at(obj_t m, char* const key);

void print_array(struct json_value** arr, int cur_indent, int indent_amount);
void print_json_value(struct json_value val, int cur_indent, int indent_amount);
void print_object(obj_t obj, int cur_indent, int indent_amount);

/*
    djb2 string hash
    credits: Daniel J. Bernstein

    Returns a hash of the string `str`.

    It seems to work well because 33 is coprime to
    2^32 and 2^64 (any odd number except 1 is),
    which probably improves the distribution  (I'm
    not a mathematician so  I can't prove this).
    Multiplying a number n by 33 is the same as
    bitshifting by 5 and adding n, which is faster
    than multiplying by, say, 37.  Maybe any 2^x±1
    are equally good.

    5381 is a large-ish prime number which are used
    as multipliers in most hash algorithms.
*/
size_t obj_hash(char const* str)
{
    size_t hash = 5381;
    unsigned int c;

    while ((c = *str++) != '\0')
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % OBJ_SIZE;
}

/*
   Value at index `key`

   m   - obj to retrieve from
   key - index to read

   returns value of index if exists, or NULL
   if key is not in obj
*/
void* obj_at(obj_t m, char* const key)
{
    struct obj_entry* hit = m[obj_hash(key)];

    /* traverse linked list to end or until key is found */
    while (hit != NULL && strcmp(hit->key, key))
        hit = hit->next;

    return hit ? hit->val : NULL;
}

/*
   Insert `value` at index `key`

   m        - obj to insert to
   key      - key to insert at
   val      - value to insert
   val_size - size of value in bytes

   returns true if successful
   returns false if key already exists
*/
bool obj_insert(obj_t m, char* const key, struct json_value* value)
{
    size_t i = obj_hash(key);
    struct obj_entry* cur = m[i];

    if (value == NULL)
        err(EINVAL, "value cannot be NULL");

    if (key == NULL)
        err(EINVAL, "key cannot be NULL");

    /* traverse linked list to end or until key is found */
    while (cur != NULL) {
        if (strcmp(cur->key, key) == 0)
            break;

        cur = cur->next;
    }

    /* fail if key already exists */
    if (cur != NULL)
        return false;

    /* populate new entry */
    cur = malloc_or_die(sizeof(struct obj_entry));
    cur->key = key;
    cur->val = value;
    cur->next = m[i];

    /* insert newest entry as head */
    m[i] = cur;

    return true;
}

/*
    Free memory allocated for json_value val
*/
void json_value_delete(struct json_value val)
{
    switch (val.type) {
    case array:
        for (size_t i = 0; val.array[i] != NULL; i++) {
            json_value_delete(*(val.array[i]));
            free(val.array[i]);
        }

        free(val.array);
        break;

    case object:
        obj_delete(val.object);
        break;

    case string:
        free(val.string);
        break;

    default:
        break;
    }
}

/*
    Free memory allocated for obj

    Recursively deletes children objects
*/
void obj_delete(obj_t* m)
{
    for (size_t i = 0; i < OBJ_SIZE; i++) {
        struct obj_entry *e = (*m)[i], *tmp;

        while (e != NULL) {
            json_value_delete(*(e->val));
            free((char*)e->key);
            free(e->val);

            tmp = e;
            e = e->next;
            free(tmp);
        }
    }
    free(m);
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

    if (arr[0] == NULL) {
        putchar(']');
        return;
    }

    for (size_t i = 0; arr[i] != NULL; i++) {
        putchar('\n');
        add_indent(cur_indent);
        print_json_value(*arr[i], cur_indent + indent_amount, indent_amount);

        if (arr[i + 1] != NULL)
            putchar(',');
    }

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
        printf("%lf", val.number);
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

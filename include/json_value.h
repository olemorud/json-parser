
#ifndef _JSON_VALUE_H
#define _JSON_VALUE_H

#include "arena.h"
#include "config.h"
#include <stdbool.h> // bool

typedef struct obj_entry {
    char const* key;
    struct json_value* val;
    struct obj_entry* next;
}* __p_obj_entry;

typedef __p_obj_entry obj_t[OBJ_SIZE];

enum json_type { object,
    array,
    string,
    number,
    boolean,
    null };

struct json_value {
    enum json_type type;
    union {
        obj_t* object;
        struct json_value** array; // we need an array of pointers to allow null termination
        char* string;
        bool boolean;
        double number;
    };
};

void* obj_at(obj_t m, char* const key);
bool obj_insert(obj_t m, char* const key, struct json_value* value, arena_t* arena);
void obj_delete(obj_t* m, arena_t* arena);

void print_json(struct json_value val, int indent);

void json_value_delete(struct json_value val, arena_t* arena);

#endif

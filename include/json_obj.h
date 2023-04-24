
#ifndef _obj_H
#define _obj_H

#include <stdbool.h>
#include <stddef.h>

#define OBJ_SIZE 1024

typedef struct obj_entry {
    char const* key;
    struct json_value* val;
    struct obj_entry* next;
} * __p_obj_entry;

typedef __p_obj_entry obj_t[OBJ_SIZE];

void* obj_at(obj_t m, char* const key);
bool obj_insert(obj_t m, char* const key, struct json_value* value);
void obj_delete(obj_t m);

#endif

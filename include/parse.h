
#ifndef _PARSE_H
#define _PARSE_H

#include "json_obj.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
        struct json_value** array;
        char* string;
        bool boolean;
        double number;
    };
};

struct json_value parse_json_value(FILE* fp);

void print_json(struct json_value val, int indent);

#endif

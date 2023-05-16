
#ifndef _PARSE_H
#define _PARSE_H

#include "json_value.h"

#include "arena.h"
#include <stdio.h> // FILE*

struct json_value parse_json_value(FILE* fp, arena_t* arena);

#endif

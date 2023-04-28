
# C JSON Parser

Just a simple project to practice writing parsers.
Not guaranteed to be safe, reliable or efficient.

## Usage

###  Compile

build (release build):

```sh
make BUILD=release
```

To make a debug build just run `make`.

### Run

```sh
./bin/{release|debug}/json_parser sample-files/large-file.json
```

## Implementation

Parsed JSON values are stored as structs containing type information and data.

```c
enum json_type { object, array, string, number, boolean, null };

struct json_value {
    enum json_type type;

    union {
        obj_t*              object;
        struct json_value** array;
        char*               string;
        bool                boolean;
        double              number;
    };
};
```

### Types

#### JSON Objects

Object values have the type field set to `(enum json_type) object`

They are stored using a poorly written map implementation with the type `obj_t`
in `json_value.object`

See [obj_t](#obj_t) for more details.

#### JSON Arrays

Array values have the type field set to `(enum json_type) array`

JSON Arrays are stored as null-terminated `json_value*` arrays,
i.e. `struct json_value**`, in `json_value.array`


#### JSON Numbers

Number values have the type field set to `(enum json_type) number`

Numbers are stored as `double` values in `json_value.number`

(The JSON specification doesn't strictly specify a maximum value, 
but `double` is sufficient for parsing almost all JSON data.)


#### JSON Strings

Strings have the type field set to `(enum json_type) string`

Strings are stored as null-terminated char arrays, `char*`, in `json_value.string`


#### JSON Booleans

Boolean values have the type field set to `(enum json_type) boolean`

The values are stored as `bool` in `json_value.boolean`


#### JSON Null

Null values have type field set to `(enum json_type) null`.


### obj\_t

`obj_t` is defined in `json_value.h` as:
```c
typedef struct obj_entry {
    char const* key;
    struct json_value* val;
    struct obj_entry* next;
} * __p_obj_entry;

typedef __p_obj_entry obj_t[OBJ_SIZE]; // OBJ_SIZE=1024
```

#### obj\_t methods

`void* obj_at(obj_t m, char* key)`

Returns a pointer to a value for the given key.

`bool obj_insert(obj_t m, char* const key, struct json_value* value)`

Insert a key-value pair

`void obj_delete(obj_t m)`

Recursively free memory allocated for object and children objects.


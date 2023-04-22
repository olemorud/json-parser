
# C JSON Parser

Just for fun.

Parses json values into structs containing type information and data.
Arrays are null-terminated arrays of pointers to json values.
Objects are represented with a poorly written map implementation.
Remaining data types are represented with native C types.

# Usage

##  Compile

**option 1** build (debug build):
```sh
make
```

**option 2** build (release build):
```sh
make BUILD=release
```

## Run

```sh
./bin/{release|debug}/json_parser
```


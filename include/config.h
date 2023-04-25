
#ifndef _CONFIG_H
#define _CONFIG_H

/*
    Number of elements in a hashmap

      Hashmaps are implemented as an array
      of linked lists. This specifies the
      length of the initial array.
*/
#define OBJ_SIZE 32

/*
    Length of surrounding context to print
    on parser errors.

      Upon parser errors (e.g. syntax errors
      in the JSON file), the program outputs
      the surrounding context of the error.
      E.g:
         | Expected ':' at index 123
         | context:
         | \n\t\t{ "foo" "bar" },\n\t
         |              ^
      In the above example, ERROR_CONTEXT_LEN
      is set to 20.
*/
#define ERROR_CONTEXT_LEN 60

/*
    Return codes on different errors:

      EARLY_EOF - End of file unexpectedly reached
      UNEXPECTED_CHAR - Unexpected character encountered
*/
#define EARLY_EOF 200
#define UNEXPECTED_CHAR 201

#endif

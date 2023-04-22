
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_obj.h"
#include "util.h"

/*
    djb2 string hash
    credits: Daniel J. Bernstein

        Returns a hash of the string `str`.

        It seems to work well because 33 is coprime to
        2^32 and 2^64 (any odd number except 1 is),
        which probably improves the distribution
        (I'm not a mathematician so I can't prove it this).
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
        if (cur->key == NULL)
            err(EXIT_FAILURE, "entry without key");

        if (strncmp(cur->key, key, strlen(key)) == 0)
            break;

        cur = cur->next;
    }

    /* fail if key already exists */
    if (cur != NULL)
        return false;

    /* populate new entry */
    cur = malloc_or_die(sizeof(struct obj_entry));
    cur->key = strdup(key);
    cur->val = value;
    cur->next = m[i];

    /* insert newest entry as head */
    m[i] = cur;

    return true;
}

/*
        Free memory allocated for obj

TODO: recurively delete children objects
*/
void obj_delete(obj_t m)
{
    for (size_t i = 0; i < OBJ_SIZE; i++) {
        if (m[i] == NULL)
            continue;

        struct obj_entry *e = m[i], *tmp;

        while (e != NULL) {
            tmp = e;
            free((char*)e->key);
            free(e->val);
            e = e->next;
            free(tmp);
        }
    }
}

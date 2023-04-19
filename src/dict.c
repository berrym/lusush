#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dict.h"
#include "errors.h"
#include "strings.h"

#define FNV1A_PRIME (0x01000193) // 16777619 (32 bit)
#define FNV1A_SEED (0x811C9DC5)  // 2166136261 (32 bit)
#define INITIAL_SIZE (1024)      // Initial size of the dictionary table
#define GROWTH_FACTOR (2)        // When increasing table size multiple current size by this
#define MAX_LOAD_FACTOR (1)      // Used to determine when a table needs to grow
#define GET_HASH() (fnv1a_hash(key) % d->size)   // Formula to calculate a hash value
#define GET_TABLE_INDEX() (d->table[GET_HASH()]) // Formula for accessing a table index

typedef struct dict_entry_s {
    char *key;                   // Dictionary entry key
    char *val;                   // Dictionary entry value
    struct dict_entry_s *next;   // Pointer for chaining identical hashes in a linked list
} dict_entry_s;

struct dict_s {                  // This is typedefed to dict_s in dict.h for external scope
    size_t size;                 // Size of the pointer table
    size_t len;                  // Number of records stored
    dict_entry_s **table;        // Table for dictionary entries
};

/**
 * internal_dict_create:
 *      Dictionary initialization code used in both dict_create and internal_dict_grow.
 */
dict_s *internal_dict_create(size_t size)
{
    dict_s *d = NULL;

    d = calloc(1, sizeof(*d));
    if (!d) {
        error_return("internal_dict_create");
        return NULL;
    }

    d->size = size;
    d->len = 0;
    d->table = calloc(d->size, sizeof(dict_entry_s *));
    if (!d->table) {
        error_return("internal_dict_create");
        return NULL;
    }

    return d;
}

/**
 * dict_create:
 *      Create and return a pointer to a new dictionary.
 */
dict_s *dict_create(void)
{
    return internal_dict_create(INITIAL_SIZE);
}

/**
 * print_dict:
 *      Print all entries in a dictionary.
 */
void print_dict(dict_s *d)
{
    dict_entry_s *e = NULL, *next = NULL;

    for (size_t i = 0; i < d->size; i++) {
        for (e = d->table[i]; e; e = next) {
            next = e->next;
            printf("%s=\"%s\"\n", e->key, e->val);
        }
    }
}

/**
 * dict_destroy:
 *      Free all entries in a dictionary, then free the table and the dictionary itself.
 */
void dict_destroy(dict_s *d)
{
    dict_entry_s *e = NULL, *next = NULL;

    for (size_t i = 0; i < d->size; i++) {
        for (e = d->table[i]; e; e = next) {
            next = e->next;

            free_str(e->key);
            free_str(e->val);
            free(e);
            e = NULL;
        }
    }

    free(d->table);
    free(d);
    d = NULL;
}

/**
 * fnv1a_hash:
 *      Return a hash key using the 32 bit FNV1A algorithm.
 */
static uint32_t fnv1a_hash(const char* key) {
    uint32_t h = FNV1A_SEED;

    for (const char *p = key; *p; p++) {
        h ^= (uint32_t)(unsigned char)(*p);
        h *= FNV1A_PRIME;
    }

    return h;
}

/**
 * hash_function:
 *      Calculate then return a hash code based on the key given.
 */
// #define MULTIPLIER (97)          // Prime number multiplier for hash function
// static size_t hash_function(const char *s)
// {
//     unsigned const char *us = NULL;
//     size_t h = 0;

//     for (us = (unsigned const char *)s; *us; us++)
//         h = h * MULTIPLIER + *us;

//     return h;
// }

/**
 * internal_dict_grow:
 *      Grow a dictionary by a factor of two.
 */
static void internal_dict_grow(dict_s *d)
{
    dict_s *d2 = NULL;            // New dictionary we'll create
    dict_s swap;                  // Temporary structure for a value swap
    dict_entry_s *e = NULL;

    d2 = internal_dict_create(d->size * GROWTH_FACTOR);

    for (size_t i = 0; i < d->size; i++) {
        for (e = d->table[i]; e; e = e->next) {
            // note: this recopies everything
            // a more efficient implementation would
            // patch out the strdups inside dict_insert
            // to avoid this problem
            dict_insert(d2, e->key, e->val);
        }
    }

    // the tricky bit
    // we'll swap the contents of d and d2
    // then call dict_destoy on d2
    swap = *d;
    *d = *d2;
    *d2 = swap;

    dict_destroy(d2);
}

/**
 * dict_insert:
 *      Insert a new key-value pair into an existing dictionary.
 */
bool dict_insert(dict_s *d, const char *key, const char *val)
{
    dict_entry_s *e = NULL;
    size_t h = 0;

    if (!d)
        return false;

    if (!key || !val)
        return false;

    e = calloc(1, sizeof(*e));
    if (!e) {
        error_return("dict_insert");
        return false;
    }

    e->key = strdup(key);
    e->val = strdup(val);

    h = GET_HASH();

    e->next = d->table[h];
    d->table[h] = e;
    d->len++;

    // Grow table if there is not enough room
    if (d->len >= d->size * MAX_LOAD_FACTOR)
        internal_dict_grow(d);

    return true;
}

/**
 * dict_search:
 *      Return the most recently inserted value associated with a key
 *      or NULL if no matching key is present.
 */
char *dict_search(dict_s *d, const char *key)
{
    dict_entry_s *e = NULL;

    for (e = GET_TABLE_INDEX(); e; e = e->next)
        if (!strcmp(e->key, key))
            return e->val;

    return NULL;
}

/**
 * dict_delete:
 *      Delete the most recently inserted entry with the given key,
 *      if there is no such entry, it has no effect.
 */
void dict_delete(dict_s *d, const char *key)
{
    dict_entry_s **prev = NULL;                  // What to change when dictionary entry is deleted
    dict_entry_s *e = NULL;                      // What to delete

    for (prev = &GET_TABLE_INDEX(); *prev; prev = &((*prev)->next)) {
        if (!strcmp((*prev)->key, key)) {
            e = *prev;
            *prev = e->next;

            // Delete the entry
            free_str(e->key);
            free_str(e->val);
            free(e);
            e = NULL;

            return;
        }
    }
}

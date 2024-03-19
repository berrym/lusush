#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/ht.h"
#include "../include/errors.h"
#include "../include/strings.h"

#define INITIAL_CAPACITY (128)   // Initial capacity of a new hash table
#define GROWTH_FACTOR (2)        // When increasing table capacity multiply current capacity by this
#define MAX_LOAD_FACTOR (0.75)   // Max used capacity of a table, when reached a table needs to grow
#define GET_HASH() (__fnv1a_hash(key) % ht->capacity) // Formula to calculate a hash value
#define GET_TABLE_INDEX() (ht->table[GET_HASH()])     // Formula for accessing a table index

typedef struct ht_entry_s {
    char *key;                   // Hash table entry key
    char *val;                   // Hash table entry value
    struct ht_entry_s *next;     // Pointer for chaining identical hashes in a linked list
} ht_entry_s;

struct ht_s {                    // This is typedefed to ht_s in ht.h for external scope
    size_t capacity;             // Size of the hash table
    size_t length;               // Number of entrys stored
    ht_entry_s **table;          // Table for hash table entries
};

/**
 * __ht_create:
 *      Hash table initialization code used in both ht_create and __ht_grow.
 */
static ht_s *__ht_create(size_t capacity)
{
    ht_s *ht = NULL;

    ht = calloc(1, sizeof(*ht));
    if (!ht) {
        error_return("__ht_create");
        return NULL;
    }

    ht->capacity = capacity;
    ht->length = 0;
    ht->table = calloc(ht->capacity, sizeof(ht_entry_s *));
    if (!ht->table) {
        error_return("__ht_create");
        return NULL;
    }

    return ht;
}

/**
 * ht_create:
 *      Create and return a pointer to a new hash table.
 */
ht_s *ht_create(void)
{
    return __ht_create(INITIAL_CAPACITY);
}

/**
 * ht_print:
 *      Print all entries in a hash table.
 */
void ht_print(ht_s *ht)
{
    ht_entry_s *e = NULL, *next = NULL;

    for (size_t i = 0; i < ht->capacity; i++) {
        for (e = ht->table[i]; e; e = next) {
            next = e->next;
            printf("%s='%s'\n", e->key, e->val);
        }
    }
}

/**
 * ht_destroy:
 *      Free all entries in a hash table, then free the table and the hash table itself.
 */
void ht_destroy(ht_s *d)
{
    ht_entry_s *e = NULL, *next = NULL;

    for (size_t i = 0; i < d->capacity; i++) {
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

#if defined(X86_32_BIT_CPU)

#define FNV1A_OFFSET (0x811C9DC5) // 2166136261 (32 bit)
#define FNV1A_PRIME (0x01000193)  // 16777619 (32 bit)

/**
 * __fnv1a_hash:
 *      Return a hash key using the 32 bit FNV1A algorithm.
 */
static uint32_t __fnv1a_hash(const char* key) {
    uint32_t h = FNV1A_OFFSET;

    for (const char *p = key; *p; p++) {
        h ^= (uint32_t)(unsigned char)(*p);
        h *= FNV1A_PRIME;
    }

    return h;
}

#elif defined(X86_64_BIT_CPU)

#define FNV1A_OFFSET (0xCBF29CE484222325) // 14695981039346656037 (64 bit)
#define FNV1A_PRIME (0x00000100000001B3)  // 1099511628211 (64 bit)

/**
 * __fnv1a_hash:
 *      Return a hash key using the 64 bit FNV1A algorithm.
 */
static uint64_t __fnv1a_hash(const char* key) {
    uint64_t h = FNV1A_OFFSET;

    for (const char *p = key; *p; p++) {
        h ^= (uint64_t)(unsigned char)(*p);
        h *= FNV1A_PRIME;
    }

    return h;
}

#endif

/**
 * __ht_grow:
 *      Grow a hash table by the value of GROWTH_FACTOR.
 */
static void __ht_grow(ht_s *ht)
{
    ht_s *ht2 = NULL;           // New hash table we'll create
    ht_s swap;                  // Temporary structure for a value swap
    ht_entry_s *e = NULL;

    ht2 = __ht_create(ht->capacity * GROWTH_FACTOR);

    // Rehash everything into the new hash table
    for (size_t i = 0; i < ht->capacity; i++)
        for (e = ht->table[i]; e; e = e->next)
            ht_insert(ht2, e->key, e->val);

    // Swap the contents of ht and ht2 then call dict_destoy on ht2
    swap = *ht;
    *ht = *ht2;
    *ht2 = swap;

    ht_destroy(ht2);
}

/**
 * ht_insert:
 *      Insert a new key-value pair into an existing hash table.
 */
bool ht_insert(ht_s *ht, const char *key, const char *val)
{
    ht_entry_s *e = NULL;
    size_t h = 0;

    if (!ht || !key || !val)
        return false;

    e = calloc(1, sizeof(*e));
    if (!e) {
        error_return("ht_insert");
        return false;
    }

    e->key = strdup(key);
    e->val = strdup(val);

    h = GET_HASH();

    if (ht_search(ht, key)) {    // Replace existing entry if keys are the same
        free_str(ht->table[h]->key);
        free_str(ht->table[h]->val);
        free(ht->table[h]);
        ht->table[h] = e;
    } else if (ht->table[h]) {   // Chain out entry if hashes are the same
        error_message("ht_insert: hash collision occured, chaining out");
        e->next = ht->table[h];
        ht->table[h] = e;
        ht->length++;
    } else {                     // Insert a new entry
        ht->table[h] = e;
        ht->length++;
    }

    // Grow table if there is not enough room
    if (ht->length >= ht->capacity * MAX_LOAD_FACTOR)
        __ht_grow(ht);

    return true;
}

/**
 * ht_search:
 *      Return the most recently inserted value associated with a key
 *      or NULL if no matching key is present.
 */
char *ht_search(ht_s *ht, const char *key)
{
    ht_entry_s *e = NULL;

    for (e = GET_TABLE_INDEX(); e; e = e->next)
        if (!strcmp(e->key, key))
            return e->val;

    return NULL;
}

/**
 * ht_delete:
 *      Delete the most recently inserted entry with the given key,
 *      if there is no such entry, it has no effect.
 */
void ht_delete(ht_s *ht, const char *key)
{
    ht_entry_s **prev = NULL;                  // What to change when hash table entry is deleted
    ht_entry_s *e = NULL;                      // What to delete

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

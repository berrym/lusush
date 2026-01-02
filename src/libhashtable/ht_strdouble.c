/**
 * @file ht_strdouble.c
 * @brief String to Double Hash Table Implementation
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2023 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a pointer duplicating a double value
 * @param val Pointer to the double value to duplicate
 * @return Pointer to the duplicated double, or NULL on failure
 */
static double *__doubledup(const double *val) {
    double *d = calloc(1, sizeof(double));
    if (!d) {
        perror("__floatdup");
        return NULL;
    }

    return memcpy(d, val, sizeof(double));
}

/**
 * @brief Create a string to double hash table
 *
 * Wrapper around ht_create that creates a string->double hash table.
 *
 * @param flags Configuration flags (e.g., HT_STR_CASECMP, HT_SEED_RANDOM)
 * @return Pointer to the new hash table, or NULL on failure
 */
ht_strdouble_t *ht_strdouble_create(unsigned int flags) {
    ht_hash hash = fnv1a_hash_str;
    ht_keyeq keyeq = str_eq;
    const ht_callbacks_t callbacks = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))__doubledup, (void (*)(const void *))free};

    if (flags & HT_STR_CASECMP) {
        hash = fnv1a_hash_str_casecmp;
        keyeq = str_caseeq;
    }

    return (ht_strdouble_t *)ht_create(hash, keyeq, &callbacks, flags);
}

/**
 * @brief Destroy a string to double hash table
 * @param ht Pointer to the hash table to destroy
 */
void ht_strdouble_destroy(ht_strdouble_t *ht) { ht_destroy((ht_t *)ht); }

/**
 * @brief Insert a string to double key value pair into a hash table
 * @param ht Pointer to the hash table
 * @param key Pointer to the string key
 * @param val Pointer to the double value
 */
void ht_strdouble_insert(ht_strdouble_t *ht, const char *key,
                         const double *val) {
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
}

/**
 * @brief Remove a bucket from a string to double hash table
 * @param ht Pointer to the hash table
 * @param key Pointer to the string key to remove
 */
void ht_strdouble_remove(ht_strdouble_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * @brief Get a value from a string to double hash table
 * @param ht Pointer to the hash table
 * @param key Pointer to the string key
 * @return Pointer to the double value, or NULL if not found
 */
void *ht_strdouble_get(ht_strdouble_t *ht, const char *key) {
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * @brief Create an enumeration object for a string to double hash table
 * @param ht Pointer to the hash table
 * @return Pointer to the enumeration object, or NULL on failure
 */
ht_enum_t *ht_strdouble_enum_create(ht_strdouble_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/**
 * @brief Get the next bucket contents from a string to double hash table
 * @param he Pointer to the enumeration object
 * @param key Pointer to store the string key
 * @param val Pointer to store the double value
 * @return true if a next bucket was found, false if enumeration is complete
 */
bool ht_strdouble_enum_next(ht_enum_t *he, const char **key,
                            const double **val) {
    return ht_enum_next(he, (const void **)key, (const void **)val);
}

/**
 * @brief Destroy a string to double hash table enumeration object
 * @param he Pointer to the enumeration object to destroy
 */
void ht_strdouble_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

/**
 * @file ht_fnv1a.c
 * @brief FNV1A Hash Algorithm Implementation
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2023 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

int strcasecmp(const char *string1, const char *string2);

/**
 * @brief Return a hash key using the 64 bit FNV1A algorithm
 * @param key Pointer to the key to hash
 * @param seed Initial seed value for hashing
 * @param ignore_case If true, perform case-insensitive hashing
 * @return 64-bit hash value
 */
static uint64_t __fnv1a_hash(const void *key, uint64_t seed, bool ignore_case) {
    uint64_t h, c;

    h = seed;

    for (unsigned char *p = (unsigned char *)key; *p; p++) {
        c = (uint64_t)(*p);
        if (ignore_case) {
            c = tolower(c);
        }
        h ^= (uint64_t)c;
        h *= FNV1A_PRIME;
    }

    return h;
}

/**
 * @brief Hash a string using case sensitive FNV1A algorithm
 *
 * Wrapper around __fnv1a_hash that uses case sensitive keys.
 *
 * @param key Pointer to the string key to hash
 * @param seed Initial seed value for hashing
 * @return 64-bit hash value
 */
uint64_t fnv1a_hash_str(const void *key, uint64_t seed) {
    return __fnv1a_hash(key, seed, false);
}

/**
 * @brief Hash a string using case insensitive FNV1A algorithm
 *
 * Wrapper around __fnv1a_hash that uses case insensitive keys.
 *
 * @param key Pointer to the string key to hash
 * @param seed Initial seed value for hashing
 * @return 64-bit hash value
 */
uint64_t fnv1a_hash_str_casecmp(const void *key, uint64_t seed) {
    return __fnv1a_hash(key, seed, true);
}

/**
 * @brief Case sensitive string comparison function
 * @param a Pointer to the first string
 * @param b Pointer to the second string
 * @return true if strings are equal, false otherwise
 */
bool str_eq(const void *a, const void *b) {
    return (strcmp(a, b) == 0) ? true : false;
}

/**
 * @brief Case insensitive string comparison function
 * @param a Pointer to the first string
 * @param b Pointer to the second string
 * @return true if strings are equal (ignoring case), false otherwise
 */
bool str_caseeq(const void *a, const void *b) {
    return (strcasecmp(a, b) == 0) ? true : false;
}

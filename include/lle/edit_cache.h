/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 4 */
/* Edit Cache: LRU cache for frequently edited history entries */

#ifndef LLE_EDIT_CACHE_H
#define LLE_EDIT_CACHE_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_edit_cache lle_edit_cache_t;
typedef struct lle_edit_cache_entry lle_edit_cache_entry_t;

/**
 * Cache entry data
 */
typedef struct lle_edit_cache_entry {
    /* Entry identification */
    size_t history_index;
    uint64_t entry_id;
    
    /* Cached data */
    char *original_text;
    size_t original_length;
    
    char *reconstructed_text;
    size_t reconstructed_length;
    
    /* Cache metadata */
    struct timespec cached_at;
    struct timespec last_accessed;
    uint64_t access_count;
    
    /* LRU chain */
    struct lle_edit_cache_entry *prev;
    struct lle_edit_cache_entry *next;
    
    /* Reserved for future use */
    void *reserved[2];
} lle_edit_cache_entry_t;

/**
 * Cache configuration
 */
typedef struct lle_edit_cache_config {
    /* Maximum cache entries */
    size_t max_entries;
    
    /* Cache entry TTL (milliseconds, 0 = no expiry) */
    uint32_t entry_ttl_ms;
    
    /* Enable access tracking */
    bool track_access;
    
    /* Reserved for future use */
    void *reserved[4];
} lle_edit_cache_config_t;

/**
 * Cache statistics
 */
typedef struct lle_edit_cache_stats {
    /* Hit/miss statistics */
    uint64_t hits;
    uint64_t misses;
    
    /* Cache utilization */
    size_t current_entries;
    size_t max_entries;
    
    /* Eviction statistics */
    uint64_t evictions;
    uint64_t expirations;
    
    /* Reserved for future use */
    void *reserved[2];
} lle_edit_cache_stats_t;

/**
 * Create an edit cache
 *
 * @param cache Output parameter for created cache
 * @param memory_pool Memory pool for allocations
 * @param config Configuration (NULL for defaults)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_create(
    lle_edit_cache_t **cache,
    lle_memory_pool_t *memory_pool,
    const lle_edit_cache_config_t *config);

/**
 * Destroy an edit cache
 *
 * @param cache Cache to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_destroy(
    lle_edit_cache_t *cache);

/**
 * Look up an entry in the cache
 *
 * @param cache Edit cache
 * @param history_index History entry index to look up
 * @param entry Output parameter for cache entry (NULL if not found)
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_lookup(
    lle_edit_cache_t *cache,
    size_t history_index,
    lle_edit_cache_entry_t **entry);

/**
 * Insert an entry into the cache
 *
 * @param cache Edit cache
 * @param history_index History entry index
 * @param entry_id History entry ID
 * @param original_text Original command text
 * @param original_length Length of original text
 * @param reconstructed_text Reconstructed text (can be NULL)
 * @param reconstructed_length Length of reconstructed text
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_insert(
    lle_edit_cache_t *cache,
    size_t history_index,
    uint64_t entry_id,
    const char *original_text,
    size_t original_length,
    const char *reconstructed_text,
    size_t reconstructed_length);

/**
 * Invalidate a specific cache entry
 *
 * @param cache Edit cache
 * @param history_index History entry index to invalidate
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_invalidate(
    lle_edit_cache_t *cache,
    size_t history_index);

/**
 * Clear all cache entries
 *
 * @param cache Edit cache
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_clear(
    lle_edit_cache_t *cache);

/**
 * Get cache statistics
 *
 * @param cache Edit cache
 * @param stats Output parameter for statistics
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_get_stats(
    lle_edit_cache_t *cache,
    lle_edit_cache_stats_t *stats);

/**
 * Evict expired entries
 *
 * @param cache Edit cache
 * @param expired_count Output parameter for number of entries evicted
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_evict_expired(
    lle_edit_cache_t *cache,
    size_t *expired_count);

/**
 * Get default cache configuration
 *
 * @param config Configuration structure to fill with defaults
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_edit_cache_get_default_config(
    lle_edit_cache_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* LLE_EDIT_CACHE_H */

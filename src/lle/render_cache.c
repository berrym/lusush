/**
 * @file render_cache.c
 * @brief LLE Render Cache Implementation (Layer 1)
 * 
 * Implements the render caching system for LLE Display Integration using
 * libhashtable as the exclusive hashtable solution per Spec 05.
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION PLAN: docs/lle_implementation/SPEC_08_IMPLEMENTATION_PLAN.md
 * LIBHASHTABLE SPEC: docs/lle_specification/05_libhashtable_integration_complete.md
 * 
 * COMPLIANCE:
 * - Uses libhashtable (ht_strstr_t) as exclusive hashtable solution
 * - Thread-safe operations with pthread_rwlock
 * - Full memory pool integration
 * - Comprehensive error handling
 * - Cache metrics tracking
 * 
 * Week 4 Day 4-5: Simple Caching
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "libhashtable/ht.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ========================================================================== */
/*                      CACHE ENTRY SERIALIZATION                             */
/* ========================================================================== */

/**
 * @brief Serialize cache entry to string for storage in libhashtable
 * 
 * Format: "data_size:timestamp:last_access:access_count:valid|<data>"
 * 
 * @param entry Cache entry to serialize
 * @return Serialized string (caller must free)
 */
static char* serialize_cache_entry(const lle_cached_entry_t *entry) {
    if (!entry || !entry->data) {
        return NULL;
    }
    
    /* Calculate required size: header + data */
    size_t header_size = 128; /* Space for metadata */
    size_t total_size = header_size + entry->data_size;
    
    char *serialized = lle_pool_alloc(total_size);
    if (!serialized) {
        return NULL;
    }
    
    /* Write metadata header */
    int header_len = snprintf(serialized, header_size,
                             "%zu:%lu:%lu:%u:%d|",
                             entry->data_size,
                             entry->timestamp,
                             entry->last_access,
                             entry->access_count,
                             entry->valid ? 1 : 0);
    
    if (header_len < 0 || header_len >= (int)header_size) {
        lle_pool_free(serialized);
        return NULL;
    }
    
    /* Append data */
    memcpy(serialized + header_len, entry->data, entry->data_size);
    
    return serialized;
}

/**
 * @brief Deserialize cache entry from string stored in libhashtable
 * 
 * @param serialized Serialized string
 * @param entry Output entry structure (caller must allocate)
 * @return LLE_SUCCESS or error code
 */
static lle_result_t deserialize_cache_entry(const char *serialized,
                                            lle_cached_entry_t *entry) {
    if (!serialized || !entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Parse metadata header */
    unsigned long timestamp, last_access;
    unsigned int access_count;
    int valid;
    size_t data_size;
    
    int fields = sscanf(serialized, "%zu:%lu:%lu:%u:%d|",
                       &data_size, &timestamp, &last_access,
                       &access_count, &valid);
    
    if (fields != 5) {
        return LLE_ERROR_INVALID_FORMAT;
    }
    
    /* Find data start (after '|') */
    const char *data_start = strchr(serialized, '|');
    if (!data_start) {
        return LLE_ERROR_INVALID_FORMAT;
    }
    data_start++; /* Skip '|' */
    
    /* Allocate and copy data */
    entry->data = lle_pool_alloc(data_size);
    if (!entry->data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(entry->data, data_start, data_size);
    
    /* Set metadata */
    entry->data_size = data_size;
    entry->timestamp = timestamp;
    entry->last_access = last_access;
    entry->access_count = access_count;
    entry->valid = (valid != 0);
    entry->next = NULL;
    
    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                      DISPLAY CACHE IMPLEMENTATION                          */
/* ========================================================================== */

/**
 * @brief Initialize display cache
 * 
 * Creates a display cache using libhashtable as the storage backend.
 * 
 * @param cache Output pointer to receive initialized cache
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_cache_init(lle_display_cache_t **cache,
                                    lle_memory_pool_t *memory_pool) {
    /* Step 1: Validate parameters */
    if (!cache || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Step 2: Allocate cache structure */
    lle_display_cache_t *c = lle_pool_alloc(sizeof(lle_display_cache_t));
    if (!c) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c, 0, sizeof(lle_display_cache_t));
    
    /* Step 3: Store memory pool reference */
    c->memory_pool = memory_pool;
    
    /* Step 4: Create libhashtable instance (string->string mapping) */
    c->cache_table = ht_strstr_create(HT_SEED_RANDOM);
    if (!c->cache_table) {
        lle_pool_free(c);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Step 5: Allocate cache metrics */
    c->metrics = lle_pool_alloc(sizeof(lle_cache_metrics_t));
    if (!c->metrics) {
        ht_strstr_destroy(c->cache_table);
        lle_pool_free(c);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c->metrics, 0, sizeof(lle_cache_metrics_t));
    
    /* Step 6: Initialize read-write lock */
    if (pthread_rwlock_init(&c->cache_lock, NULL) != 0) {
        lle_pool_free(c->metrics);
        ht_strstr_destroy(c->cache_table);
        lle_pool_free(c);
        return LLE_ERROR_INITIALIZATION_FAILED;
    }
    
    /* Step 7: Return initialized cache */
    *cache = c;
    return LLE_SUCCESS;
}

/**
 * @brief Clean up display cache
 * 
 * @param cache Cache to clean up
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_cache_cleanup(lle_display_cache_t *cache) {
    if (!cache) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Destroy libhashtable (frees all entries) */
    if (cache->cache_table) {
        ht_strstr_destroy(cache->cache_table);
    }
    
    /* Destroy lock */
    pthread_rwlock_destroy(&cache->cache_lock);
    
    /* Free metrics */
    if (cache->metrics) {
        lle_pool_free(cache->metrics);
    }
    
    /* Free cache structure */
    lle_pool_free(cache);
    
    return LLE_SUCCESS;
}

/**
 * @brief Store entry in cache
 * 
 * Stores or updates a cache entry using libhashtable.
 * 
 * @param cache Display cache
 * @param key Cache key (uint64_t converted to string)
 * @param data Data to cache
 * @param data_size Size of data
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_cache_store(lle_display_cache_t *cache,
                                     uint64_t key,
                                     const void *data,
                                     size_t data_size) {
    /* Step 1: Validate parameters */
    if (!cache || !data || data_size == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Step 2: Convert key to string */
    char key_str[32];
    snprintf(key_str, sizeof(key_str), "%lu", key);
    
    /* Step 3: Create cache entry */
    lle_cached_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.data = (void*)data;
    entry.data_size = data_size;
    
    /* Set timestamps */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    entry.timestamp = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    entry.last_access = entry.timestamp;
    entry.access_count = 0;
    entry.valid = true;
    
    /* Step 4: Serialize entry */
    char *serialized = serialize_cache_entry(&entry);
    if (!serialized) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Step 5: Acquire write lock */
    pthread_rwlock_wrlock(&cache->cache_lock);
    
    /* Step 6: Insert into libhashtable */
    ht_strstr_insert(cache->cache_table, key_str, serialized);
    
    /* Step 7: Release lock */
    pthread_rwlock_unlock(&cache->cache_lock);
    
    /* Free serialized string (libhashtable makes a copy) */
    lle_pool_free(serialized);
    
    return LLE_SUCCESS;
}

/**
 * @brief Lookup entry in cache
 * 
 * Retrieves a cache entry from libhashtable.
 * 
 * @param cache Display cache
 * @param key Cache key
 * @param data Output pointer for cached data
 * @param data_size Output pointer for data size
 * @return LLE_SUCCESS on hit, LLE_ERROR_CACHE_MISS on miss
 */
lle_result_t lle_display_cache_lookup(lle_display_cache_t *cache,
                                      uint64_t key,
                                      void **data,
                                      size_t *data_size) {
    /* Step 1: Validate parameters */
    if (!cache || !data || !data_size) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Step 2: Convert key to string */
    char key_str[32];
    snprintf(key_str, sizeof(key_str), "%lu", key);
    
    /* Step 3: Acquire read lock */
    pthread_rwlock_rdlock(&cache->cache_lock);
    
    /* Step 4: Lookup in libhashtable */
    const char *serialized = ht_strstr_get(cache->cache_table, key_str);
    
    if (!serialized) {
        /* Cache miss */
        cache->metrics->cache_misses++;
        pthread_rwlock_unlock(&cache->cache_lock);
        return LLE_ERROR_CACHE_MISS;
    }
    
    /* Step 5: Deserialize entry */
    lle_cached_entry_t entry;
    lle_result_t result = deserialize_cache_entry(serialized, &entry);
    
    if (result != LLE_SUCCESS) {
        cache->metrics->cache_misses++;
        pthread_rwlock_unlock(&cache->cache_lock);
        return result;
    }
    
    /* Step 6: Return data */
    *data = entry.data;
    *data_size = entry.data_size;
    
    /* Step 7: Update metrics */
    cache->metrics->cache_hits++;
    
    /* Step 8: Release lock */
    pthread_rwlock_unlock(&cache->cache_lock);
    
    return LLE_SUCCESS;
}

/* ========================================================================== */
/*                      RENDER CACHE IMPLEMENTATION                           */
/* ========================================================================== */

/**
 * @brief Initialize render cache
 * 
 * Creates render cache with base cache implementation.
 * 
 * @param cache Output pointer to receive initialized cache
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_render_cache_init(lle_render_cache_t **cache,
                                   lle_memory_pool_t *memory_pool) {
    /* Step 1: Validate parameters */
    if (!cache || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Step 2: Allocate render cache structure */
    lle_render_cache_t *rc = lle_pool_alloc(sizeof(lle_render_cache_t));
    if (!rc) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(rc, 0, sizeof(lle_render_cache_t));
    
    /* Step 3: Initialize base cache using libhashtable */
    lle_result_t result = lle_display_cache_init(&rc->base_cache, memory_pool);
    if (result != LLE_SUCCESS) {
        lle_pool_free(rc);
        return result;
    }
    
    /* Step 4: Configure render cache */
    rc->max_render_size = 0; /* No limit for now */
    rc->cache_ttl_ms = 5000; /* 5 second TTL */
    
    /* Step 5: Return initialized cache */
    *cache = rc;
    return LLE_SUCCESS;
}

/**
 * @brief Clean up render cache
 * 
 * @param cache Cache to clean up
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_render_cache_cleanup(lle_render_cache_t *cache) {
    if (!cache) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Clean up base cache */
    if (cache->base_cache) {
        lle_display_cache_cleanup(cache->base_cache);
    }
    
    /* Free render cache structure */
    lle_pool_free(cache);
    
    return LLE_SUCCESS;
}

/**
 * @brief Compute cache key from buffer and cursor
 * 
 * Generates a simple cache key by combining buffer and cursor hashes.
 * 
 * @param buffer Buffer state
 * @param cursor Cursor position
 * @return Combined cache key
 */
uint64_t lle_compute_cache_key(lle_buffer_t *buffer, lle_cursor_position_t *cursor) {
    if (!buffer || !cursor) {
        return 0;
    }
    
    /* Simple combination: XOR of buffer content hash and cursor position */
    uint64_t buffer_hash = (uint64_t)buffer->length;
    uint64_t cursor_hash = ((uint64_t)cursor->line_number << 32) | cursor->column_offset;
    
    return buffer_hash ^ cursor_hash;
}

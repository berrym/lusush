/**
 * @file render_cache.c
 * @brief LLE Render Cache Implementation
 * 
 * Implements caching system for rendered output with hash-based lookups
 * and simple cache management.
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION PLAN: docs/lle_implementation/SPEC_08_IMPLEMENTATION_PLAN.md
 * 
 * ZERO-TOLERANCE COMPLIANCE:
 * - Complete implementations (no stubs)
 * - Full error handling
 * - 100% spec-compliant
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/buffer_management.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

/* ========================================================================== */
/*                       HASH FUNCTION IMPLEMENTATION                         */
/* ========================================================================== */

/**
 * @brief Compute hash for cache key
 * 
 * Simple FNV-1a hash function for generating cache keys.
 * 
 * @param data Data to hash
 * @param length Length of data
 * @return Hash value
 */
static uint64_t lle_cache_hash(const void *data, size_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t hash = 14695981039346656037ULL; /* FNV offset basis */
    
    for (size_t i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL; /* FNV prime */
    }
    
    return hash;
}

/**
 * @brief Compute hash for buffer content
 * 
 * Generates a hash value for buffer content to use as cache key.
 * 
 * @param buffer Buffer to hash
 * @return Hash value
 */
static uint64_t lle_buffer_compute_hash(lle_buffer_t *buffer) {
    if (!buffer || !buffer->data || buffer->length == 0) {
        return 0;
    }
    
    return lle_cache_hash(buffer->data, buffer->length);
}

/**
 * @brief Compute hash for cursor position
 * 
 * Generates a hash value for cursor position to use in cache key.
 * 
 * @param cursor Cursor position to hash
 * @return Hash value
 */
static uint64_t lle_cursor_compute_hash(lle_cursor_position_t *cursor) {
    if (!cursor) {
        return 0;
    }
    
    /* Hash relevant cursor fields */
    uint64_t hash = cursor->line_number;
    hash = (hash << 8) ^ cursor->visual_column;
    hash = (hash << 8) ^ cursor->byte_offset;
    
    return hash;
}

/* ========================================================================== */
/*                       CACHE INITIALIZATION                                 */
/* ========================================================================== */

/**
 * @brief Initialize display cache
 * 
 * Creates and initializes the base cache structure with hash table.
 * 
 * @param cache Output pointer to receive initialized cache
 * @param table_size Size of hash table
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_cache_init(lle_display_cache_t **cache,
                                    size_t table_size,
                                    lle_memory_pool_t *memory_pool) {
    /* Step 1: Validate parameters */
    if (!cache || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    if (table_size == 0) {
        table_size = 128; /* Default size */
    }
    
    /* Step 2: Allocate cache structure */
    lle_display_cache_t *c = lle_pool_alloc(sizeof(lle_display_cache_t));
    if (!c) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c, 0, sizeof(lle_display_cache_t));
    
    /* Step 3: Store configuration */
    c->table_size = table_size;
    c->entry_count = 0;
    c->memory_pool = memory_pool;
    
    /* Step 4: Allocate hash table */
    c->entries = lle_pool_alloc(sizeof(lle_cached_entry_t *) * table_size);
    if (!c->entries) {
        lle_pool_free(c);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c->entries, 0, sizeof(lle_cached_entry_t *) * table_size);
    
    /* Step 5: Allocate metrics structure */
    c->metrics = lle_pool_alloc(sizeof(lle_cache_metrics_t));
    if (!c->metrics) {
        lle_pool_free(c->entries);
        lle_pool_free(c);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(c->metrics, 0, sizeof(lle_cache_metrics_t));
    
    /* Step 6: Initialize read-write lock */
    if (pthread_rwlock_init(&c->cache_lock, NULL) != 0) {
        lle_pool_free(c->metrics);
        lle_pool_free(c->entries);
        lle_pool_free(c);
        return LLE_ERROR_INITIALIZATION_FAILED;
    }
    
    /* Step 7: Return initialized cache */
    *cache = c;
    return LLE_SUCCESS;
}

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
    
    /* Step 3: Initialize base cache */
    lle_result_t result = lle_display_cache_init(&rc->base_cache, 128, memory_pool);
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

/* ========================================================================== */
/*                       CACHE OPERATIONS                                     */
/* ========================================================================== */

/**
 * @brief Store entry in cache
 * 
 * Stores rendered output in cache with hash-based key.
 * 
 * @param cache Display cache
 * @param key Cache key (hash value)
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
    
    /* Step 2: Acquire write lock */
    pthread_rwlock_wrlock(&cache->cache_lock);
    
    /* Step 3: Compute hash table index */
    size_t index = key % cache->table_size;
    
    /* Step 4: Check if entry already exists */
    lle_cached_entry_t *entry = cache->entries[index];
    while (entry) {
        if (entry->cache_key && strcmp(entry->cache_key, (const char *)&key) == 0) {
            /* Entry exists, update it */
            if (entry->data) {
                lle_pool_free(entry->data);
            }
            entry->data = lle_pool_alloc(data_size);
            if (!entry->data) {
                pthread_rwlock_unlock(&cache->cache_lock);
                return LLE_ERROR_OUT_OF_MEMORY;
            }
            memcpy(entry->data, data, data_size);
            entry->data_size = data_size;
            
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            entry->timestamp = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
            entry->last_access = entry->timestamp;
            entry->valid = true;
            
            pthread_rwlock_unlock(&cache->cache_lock);
            return LLE_SUCCESS;
        }
        entry = entry->next;
    }
    
    /* Step 5: Create new entry */
    lle_cached_entry_t *new_entry = lle_pool_alloc(sizeof(lle_cached_entry_t));
    if (!new_entry) {
        pthread_rwlock_unlock(&cache->cache_lock);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memset(new_entry, 0, sizeof(lle_cached_entry_t));
    
    /* Step 6: Allocate and copy key */
    new_entry->cache_key = lle_pool_alloc(sizeof(uint64_t) + 1);
    if (!new_entry->cache_key) {
        lle_pool_free(new_entry);
        pthread_rwlock_unlock(&cache->cache_lock);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(new_entry->cache_key, &key, sizeof(uint64_t));
    new_entry->cache_key[sizeof(uint64_t)] = '\0';
    
    /* Step 7: Allocate and copy data */
    new_entry->data = lle_pool_alloc(data_size);
    if (!new_entry->data) {
        lle_pool_free(new_entry->cache_key);
        lle_pool_free(new_entry);
        pthread_rwlock_unlock(&cache->cache_lock);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(new_entry->data, data, data_size);
    new_entry->data_size = data_size;
    
    /* Step 8: Set timestamps */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    new_entry->timestamp = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    new_entry->last_access = new_entry->timestamp;
    new_entry->access_count = 0;
    new_entry->valid = true;
    
    /* Step 9: Insert into hash table (prepend to chain) */
    new_entry->next = cache->entries[index];
    cache->entries[index] = new_entry;
    cache->entry_count++;
    
    /* Step 10: Unlock and return */
    pthread_rwlock_unlock(&cache->cache_lock);
    return LLE_SUCCESS;
}

/**
 * @brief Lookup entry in cache
 * 
 * Searches for cached entry by key.
 * 
 * @param cache Display cache
 * @param key Cache key (hash value)
 * @param data Output pointer to receive cached data
 * @param data_size Output pointer to receive data size
 * @return LLE_SUCCESS on cache hit, LLE_ERROR_NOT_FOUND on miss
 */
lle_result_t lle_display_cache_lookup(lle_display_cache_t *cache,
                                      uint64_t key,
                                      void **data,
                                      size_t *data_size) {
    /* Step 1: Validate parameters */
    if (!cache || !data || !data_size) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Step 2: Acquire read lock */
    pthread_rwlock_rdlock(&cache->cache_lock);
    
    /* Step 3: Compute hash table index */
    size_t index = key % cache->table_size;
    
    /* Step 4: Search chain for entry */
    lle_cached_entry_t *entry = cache->entries[index];
    while (entry) {
        if (entry->cache_key && strcmp(entry->cache_key, (const char *)&key) == 0) {
            /* Found entry */
            if (entry->valid) {
                *data = entry->data;
                *data_size = entry->data_size;
                
                /* Update access metrics */
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                entry->last_access = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
                entry->access_count++;
                
                cache->metrics->cache_hits++;
                
                pthread_rwlock_unlock(&cache->cache_lock);
                return LLE_SUCCESS;
            }
        }
        entry = entry->next;
    }
    
    /* Step 5: Cache miss */
    cache->metrics->cache_misses++;
    pthread_rwlock_unlock(&cache->cache_lock);
    return LLE_ERROR_CACHE_MISS;
}

/**
 * @brief Compute combined cache key
 * 
 * Combines buffer and cursor hashes into a single cache key.
 * 
 * @param buffer Buffer to hash
 * @param cursor Cursor position to hash
 * @return Combined hash value
 */
uint64_t lle_compute_cache_key(lle_buffer_t *buffer, lle_cursor_position_t *cursor) {
    uint64_t buffer_hash = lle_buffer_compute_hash(buffer);
    uint64_t cursor_hash = lle_cursor_compute_hash(cursor);
    
    /* Combine hashes using XOR */
    return buffer_hash ^ cursor_hash;
}

/**
 * @brief Clean up display cache
 * 
 * Releases all resources allocated for the cache.
 * 
 * @param cache Cache to clean up
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_display_cache_cleanup(lle_display_cache_t *cache) {
    if (!cache) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Free all entries */
    for (size_t i = 0; i < cache->table_size; i++) {
        lle_cached_entry_t *entry = cache->entries[i];
        while (entry) {
            lle_cached_entry_t *next = entry->next;
            if (entry->cache_key) {
                lle_pool_free(entry->cache_key);
            }
            if (entry->data) {
                lle_pool_free(entry->data);
            }
            lle_pool_free(entry);
            entry = next;
        }
    }
    
    /* Destroy lock */
    pthread_rwlock_destroy(&cache->cache_lock);
    
    /* Free structures */
    if (cache->metrics) {
        lle_pool_free(cache->metrics);
    }
    if (cache->entries) {
        lle_pool_free(cache->entries);
    }
    lle_pool_free(cache);
    
    return LLE_SUCCESS;
}

/**
 * @brief Clean up render cache
 * 
 * Releases resources for render cache.
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

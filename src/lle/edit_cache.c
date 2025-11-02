/* SPDX-License-Identifier: MIT */
/* LLE Specification 22: History-Buffer Integration - Phase 4 */
/* Edit Cache Implementation */

#include "lle/edit_cache.h"
#include "lle/memory_management.h"
#include <string.h>
#include <time.h>

/* Default configuration values */
#define DEFAULT_MAX_ENTRIES 100
#define DEFAULT_ENTRY_TTL_MS 300000  /* 5 minutes */

/* Edit cache implementation */
struct lle_edit_cache {
    lle_memory_pool_t *memory_pool;
    lle_edit_cache_config_t config;
    
    /* LRU list (most recent at head) */
    lle_edit_cache_entry_t *head;
    lle_edit_cache_entry_t *tail;
    
    /* Statistics */
    lle_edit_cache_stats_t stats;
    
    bool active;
};

/* Forward declarations for internal functions */
static void move_to_head(lle_edit_cache_t *cache, lle_edit_cache_entry_t *entry);
static void remove_from_list(lle_edit_cache_t *cache, lle_edit_cache_entry_t *entry);
static void evict_lru(lle_edit_cache_t *cache);
static bool is_expired(lle_edit_cache_entry_t *entry, uint32_t ttl_ms);
static int64_t timespec_diff_ms(const struct timespec *start, const struct timespec *end);

lle_result_t lle_edit_cache_get_default_config(
    lle_edit_cache_config_t *config)
{
    if (!config) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    config->max_entries = DEFAULT_MAX_ENTRIES;
    config->entry_ttl_ms = DEFAULT_ENTRY_TTL_MS;
    config->track_access = true;
    memset(config->reserved, 0, sizeof(config->reserved));
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_create(
    lle_edit_cache_t **cache,
    lle_memory_pool_t *memory_pool,
    const lle_edit_cache_config_t *config)
{
    if (!cache || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_edit_cache_t *new_cache = lle_pool_alloc(sizeof(lle_edit_cache_t));
    if (!new_cache) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_cache, 0, sizeof(lle_edit_cache_t));
    
    new_cache->memory_pool = memory_pool;
    
    if (config) {
        memcpy(&new_cache->config, config, sizeof(lle_edit_cache_config_t));
    } else {
        lle_edit_cache_get_default_config(&new_cache->config);
    }
    
    new_cache->head = NULL;
    new_cache->tail = NULL;
    new_cache->stats.max_entries = new_cache->config.max_entries;
    new_cache->active = true;
    
    *cache = new_cache;
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_destroy(
    lle_edit_cache_t *cache)
{
    if (!cache) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    cache->active = false;
    /* Memory pool owns all allocations, no explicit frees needed */
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_lookup(
    lle_edit_cache_t *cache,
    size_t history_index,
    lle_edit_cache_entry_t **entry)
{
    if (!cache || !cache->active || !entry) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    *entry = NULL;
    
    /* Search for entry */
    lle_edit_cache_entry_t *current = cache->head;
    while (current) {
        if (current->history_index == history_index) {
            /* Check if expired */
            if (cache->config.entry_ttl_ms > 0 &&
                is_expired(current, cache->config.entry_ttl_ms)) {
                /* Remove expired entry */
                remove_from_list(cache, current);
                cache->stats.current_entries--;
                cache->stats.expirations++;
                cache->stats.misses++;
                return LLE_SUCCESS;
            }
            
            /* Cache hit */
            *entry = current;
            cache->stats.hits++;
            
            /* Update access metadata */
            if (cache->config.track_access) {
                clock_gettime(CLOCK_MONOTONIC, &current->last_accessed);
                current->access_count++;
            }
            
            /* Move to head (most recently used) */
            move_to_head(cache, current);
            
            return LLE_SUCCESS;
        }
        current = current->next;
    }
    
    /* Cache miss */
    cache->stats.misses++;
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_insert(
    lle_edit_cache_t *cache,
    size_t history_index,
    uint64_t entry_id,
    const char *original_text,
    size_t original_length,
    const char *reconstructed_text,
    size_t reconstructed_length)
{
    if (!cache || !cache->active || !original_text) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if entry already exists */
    lle_edit_cache_entry_t *existing = NULL;
    lle_edit_cache_lookup(cache, history_index, &existing);
    
    if (existing) {
        /* Update existing entry */
        /* For simplicity, we'll invalidate and re-insert */
        lle_edit_cache_invalidate(cache, history_index);
    }
    
    /* Check if we need to evict */
    if (cache->stats.current_entries >= cache->config.max_entries) {
        evict_lru(cache);
    }
    
    /* Allocate new entry */
    lle_edit_cache_entry_t *new_entry = lle_pool_alloc(sizeof(lle_edit_cache_entry_t));
    if (!new_entry) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(new_entry, 0, sizeof(lle_edit_cache_entry_t));
    
    new_entry->history_index = history_index;
    new_entry->entry_id = entry_id;
    
    /* Copy original text */
    new_entry->original_text = lle_pool_alloc(original_length + 1);
    if (!new_entry->original_text) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    memcpy(new_entry->original_text, original_text, original_length);
    new_entry->original_text[original_length] = '\0';
    new_entry->original_length = original_length;
    
    /* Copy reconstructed text if provided */
    if (reconstructed_text && reconstructed_length > 0) {
        new_entry->reconstructed_text = lle_pool_alloc(reconstructed_length + 1);
        if (!new_entry->reconstructed_text) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(new_entry->reconstructed_text, reconstructed_text, reconstructed_length);
        new_entry->reconstructed_text[reconstructed_length] = '\0';
        new_entry->reconstructed_length = reconstructed_length;
    }
    
    /* Set timestamps */
    clock_gettime(CLOCK_MONOTONIC, &new_entry->cached_at);
    new_entry->last_accessed = new_entry->cached_at;
    new_entry->access_count = 0;
    
    /* Add to head of list */
    new_entry->next = cache->head;
    new_entry->prev = NULL;
    
    if (cache->head) {
        cache->head->prev = new_entry;
    }
    cache->head = new_entry;
    
    if (!cache->tail) {
        cache->tail = new_entry;
    }
    
    cache->stats.current_entries++;
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_invalidate(
    lle_edit_cache_t *cache,
    size_t history_index)
{
    if (!cache || !cache->active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Find and remove entry */
    lle_edit_cache_entry_t *current = cache->head;
    while (current) {
        if (current->history_index == history_index) {
            remove_from_list(cache, current);
            cache->stats.current_entries--;
            return LLE_SUCCESS;
        }
        current = current->next;
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_clear(
    lle_edit_cache_t *cache)
{
    if (!cache || !cache->active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    cache->head = NULL;
    cache->tail = NULL;
    cache->stats.current_entries = 0;
    
    /* Memory pool owns allocations */
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_get_stats(
    lle_edit_cache_t *cache,
    lle_edit_cache_stats_t *stats)
{
    if (!cache || !cache->active || !stats) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    memcpy(stats, &cache->stats, sizeof(lle_edit_cache_stats_t));
    
    return LLE_SUCCESS;
}

lle_result_t lle_edit_cache_evict_expired(
    lle_edit_cache_t *cache,
    size_t *expired_count)
{
    if (!cache || !cache->active) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (cache->config.entry_ttl_ms == 0) {
        if (expired_count) *expired_count = 0;
        return LLE_SUCCESS;  /* No TTL configured */
    }
    
    size_t count = 0;
    lle_edit_cache_entry_t *current = cache->head;
    
    while (current) {
        lle_edit_cache_entry_t *next = current->next;
        
        if (is_expired(current, cache->config.entry_ttl_ms)) {
            remove_from_list(cache, current);
            cache->stats.current_entries--;
            cache->stats.expirations++;
            count++;
        }
        
        current = next;
    }
    
    if (expired_count) {
        *expired_count = count;
    }
    
    return LLE_SUCCESS;
}

/* Internal helper functions */

static void move_to_head(lle_edit_cache_t *cache, lle_edit_cache_entry_t *entry)
{
    if (cache->head == entry) {
        return;  /* Already at head */
    }
    
    /* Remove from current position */
    if (entry->prev) {
        entry->prev->next = entry->next;
    }
    if (entry->next) {
        entry->next->prev = entry->prev;
    }
    if (cache->tail == entry) {
        cache->tail = entry->prev;
    }
    
    /* Add to head */
    entry->next = cache->head;
    entry->prev = NULL;
    
    if (cache->head) {
        cache->head->prev = entry;
    }
    cache->head = entry;
    
    if (!cache->tail) {
        cache->tail = entry;
    }
}

static void remove_from_list(lle_edit_cache_t *cache, lle_edit_cache_entry_t *entry)
{
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        cache->head = entry->next;
    }
    
    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        cache->tail = entry->prev;
    }
}

static void evict_lru(lle_edit_cache_t *cache)
{
    if (!cache->tail) {
        return;  /* Cache is empty */
    }
    
    lle_edit_cache_entry_t *lru = cache->tail;
    remove_from_list(cache, lru);
    cache->stats.current_entries--;
    cache->stats.evictions++;
}

static bool is_expired(lle_edit_cache_entry_t *entry, uint32_t ttl_ms)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    int64_t age_ms = timespec_diff_ms(&entry->cached_at, &now);
    
    return (age_ms >= (int64_t)ttl_ms);
}

static int64_t timespec_diff_ms(const struct timespec *start, const struct timespec *end)
{
    int64_t sec_diff = end->tv_sec - start->tv_sec;
    int64_t nsec_diff = end->tv_nsec - start->tv_nsec;
    
    return (sec_diff * 1000) + (nsec_diff / 1000000);
}

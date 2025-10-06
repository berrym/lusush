/*
 * Lusush Shell - Memory Pool System Implementation
 * Enterprise-Grade Memory Management for Display Operations
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lusush_memory_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

// Global memory pool system instance
lusush_memory_pool_system_t *global_memory_pool = NULL;

// Thread safety mutex for pool operations
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// Debug and error tracking
static bool debug_mode = false;
static lusush_pool_error_t last_error = LUSUSH_POOL_SUCCESS;

// Fallback size tracking for analysis
static size_t fallback_sizes[100];
static int fallback_count = 0;

// Pool size definitions (optimized for display operations)
static const size_t POOL_SIZES[LUSUSH_POOL_COUNT] = {
    128,    // SMALL: state hashes, cache keys
    512,    // MEDIUM: prompts, short outputs
    4096,   // LARGE: display compositions, multiline inputs
    16384   // XLARGE: tab completions, complex outputs
};

// Default pool block counts (optimized for typical usage)
static const size_t DEFAULT_BLOCK_COUNTS[LUSUSH_POOL_COUNT] = {
    512,    // SMALL: High frequency allocations (4x increase - analysis shows 100% fallbacks here)
    64,     // MEDIUM: Moderate frequency (doubled from 32)
    32,     // LARGE: Lower frequency, larger impact (doubled from 16)
    16      // XLARGE: Infrequent but critical (doubled from 8)
};

// Performance monitoring macros
#define POOL_DEBUG(fmt, ...) do { \
    if (debug_mode) { \
        fprintf(stderr, "[POOL] " fmt "\n", ##__VA_ARGS__); \
    } \
} while(0)

#define POOL_ERROR(fmt, ...) do { \
    fprintf(stderr, "[POOL ERROR] " fmt "\n", ##__VA_ARGS__); \
} while(0)

// Utility function to get current timestamp in microseconds
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

// Utility function to get current timestamp in nanoseconds
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
}

// Set last error with debug information
static void set_last_error(lusush_pool_error_t error) {
    last_error = error;
    if (error != LUSUSH_POOL_SUCCESS && debug_mode) {
        POOL_ERROR("Operation failed: %s", lusush_pool_error_string(error));
    }
}

// Initialize a single memory pool
static lusush_pool_error_t init_single_pool(lusush_pool_t *pool, size_t block_size, size_t initial_blocks) {
    if (!pool || block_size == 0 || initial_blocks == 0) {
        return LUSUSH_POOL_ERROR_INVALID_SIZE;
    }

    // Initialize pool structure
    memset(pool, 0, sizeof(lusush_pool_t));
    pool->block_size = block_size;
    pool->initial_blocks = initial_blocks;
    pool->max_blocks = initial_blocks * 4; // Allow growth up to 4x initial
    
    // Allocate block management array
    pool->all_blocks = calloc(pool->max_blocks, sizeof(lusush_pool_block_t));
    if (!pool->all_blocks) {
        POOL_ERROR("Failed to allocate block management array for pool (size=%zu)", block_size);
        return LUSUSH_POOL_ERROR_MALLOC_FAILED;
    }

    // Pre-allocate initial blocks
    for (size_t i = 0; i < initial_blocks; i++) {
        lusush_pool_block_t *block = &pool->all_blocks[i];
        
        // Allocate memory for this block
        block->memory = malloc(block_size);
        if (!block->memory) {
            POOL_ERROR("Failed to allocate memory block %zu (size=%zu)", i, block_size);
            // Cleanup previously allocated blocks
            for (size_t j = 0; j < i; j++) {
                free(pool->all_blocks[j].memory);
            }
            free(pool->all_blocks);
            return LUSUSH_POOL_ERROR_MALLOC_FAILED;
        }

        // Initialize block metadata
        block->size = block_size;
        block->in_use = false;
        block->allocation_time_us = 0;
        block->allocation_id = 0;

        // Add to free list
        block->next = pool->free_list;
        block->prev = NULL;
        if (pool->free_list) {
            pool->free_list->prev = block;
        }
        pool->free_list = block;
    }

    pool->current_blocks = initial_blocks;
    pool->free_blocks = initial_blocks;

    POOL_DEBUG("Initialized pool: block_size=%zu, initial_blocks=%zu", block_size, initial_blocks);
    return LUSUSH_POOL_SUCCESS;
}

// Cleanup a single memory pool
static void cleanup_single_pool(lusush_pool_t *pool) {
    if (!pool || !pool->all_blocks) {
        return;
    }

    // Free all allocated memory blocks
    for (size_t i = 0; i < pool->current_blocks; i++) {
        if (pool->all_blocks[i].memory) {
            free(pool->all_blocks[i].memory);
        }
    }

    // Free block management array
    free(pool->all_blocks);
    
    // Clear pool structure
    memset(pool, 0, sizeof(lusush_pool_t));
    
    POOL_DEBUG("Cleaned up pool");
}

// Find the appropriate pool for a given size
static lusush_pool_size_t find_pool_for_size(size_t size) {
    for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
        if (size <= POOL_SIZES[i]) {
            return (lusush_pool_size_t)i;
        }
    }
    // Size is larger than any pool - will use malloc fallback
    return LUSUSH_POOL_COUNT; // Invalid pool index indicates malloc fallback
}

// Allocate a block from specific pool
static void *allocate_from_pool(lusush_pool_t *pool) {
    if (!pool || !pool->free_list) {
        return NULL;
    }

    // Take first block from free list
    lusush_pool_block_t *block = pool->free_list;
    
    // Remove from free list
    pool->free_list = block->next;
    if (pool->free_list) {
        pool->free_list->prev = NULL;
    }

    // Mark as in use
    block->in_use = true;
    block->allocation_time_us = get_timestamp_us();
    block->allocation_id = ++global_memory_pool->next_allocation_id;

    // Update pool statistics
    pool->free_blocks--;
    pool->pool_allocations++;

    POOL_DEBUG("Allocated block: size=%zu, id=%u, ptr=%p", 
               block->size, block->allocation_id, block->memory);

    return block->memory;
}

// Return a block to specific pool
static bool return_to_pool(lusush_pool_t *pool, void *ptr) {
    if (!pool || !ptr) {
        return false;
    }

    // Find the block that contains this pointer
    lusush_pool_block_t *block = NULL;
    for (size_t i = 0; i < pool->current_blocks; i++) {
        if (pool->all_blocks[i].memory == ptr) {
            block = &pool->all_blocks[i];
            break;
        }
    }

    if (!block || !block->in_use) {
        return false; // Not found or double free
    }

    // Mark as free
    block->in_use = false;
    block->allocation_time_us = 0;
    block->allocation_id = 0;

    // Add back to free list
    block->next = pool->free_list;
    block->prev = NULL;
    if (pool->free_list) {
        pool->free_list->prev = block;
    }
    pool->free_list = block;

    // Update statistics
    pool->free_blocks++;
    pool->pool_deallocations++;

    POOL_DEBUG("Returned block to pool: size=%zu, ptr=%p", block->size, ptr);
    return true;
}

// Update global statistics
static void update_stats(bool pool_hit, size_t size, uint64_t allocation_time_ns) {
    if (!global_memory_pool->enable_statistics) {
        return;
    }

    lusush_pool_stats_t *stats = &global_memory_pool->stats;
    
    stats->total_allocations++;
    stats->total_bytes_allocated += size;
    stats->active_allocations++;

    if (pool_hit) {
        stats->pool_hits++;
        stats->current_pool_usage += size;
        if (stats->current_pool_usage > stats->peak_pool_usage) {
            stats->peak_pool_usage = stats->current_pool_usage;
        }
    } else {
        stats->pool_misses++;
        stats->malloc_fallbacks++;
    }

    // Update hit rate
    stats->pool_hit_rate = (double)stats->pool_hits / stats->total_allocations * 100.0;

    // Update average allocation time
    if (stats->total_allocations == 1) {
        stats->avg_allocation_time_ns = allocation_time_ns;
    } else {
        stats->avg_allocation_time_ns = 
            (stats->avg_allocation_time_ns * (stats->total_allocations - 1) + allocation_time_ns) / 
            stats->total_allocations;
    }
}

/**
 * Public API Implementation
 */

lusush_pool_error_t lusush_pool_init(const lusush_pool_config_t *config) {
    pthread_mutex_lock(&pool_mutex);

    // Check if already initialized
    if (global_memory_pool) {
        pthread_mutex_unlock(&pool_mutex);
        set_last_error(LUSUSH_POOL_SUCCESS);
        return LUSUSH_POOL_SUCCESS; // Allow multiple inits
    }

    // Allocate global pool system
    global_memory_pool = calloc(1, sizeof(lusush_memory_pool_system_t));
    if (!global_memory_pool) {
        pthread_mutex_unlock(&pool_mutex);
        set_last_error(LUSUSH_POOL_ERROR_MALLOC_FAILED);
        return LUSUSH_POOL_ERROR_MALLOC_FAILED;
    }

    // Use provided config or defaults
    lusush_pool_config_t default_config = lusush_pool_get_default_config();
    if (!config) {
        config = &default_config;
    }

    // Set system configuration
    global_memory_pool->enable_statistics = config->enable_statistics;
    global_memory_pool->enable_malloc_fallback = config->enable_malloc_fallback;
    debug_mode = config->enable_debugging;
    
    // Initialize timestamp
    clock_gettime(CLOCK_MONOTONIC, &global_memory_pool->init_time);

    // Initialize individual pools
    size_t block_counts[LUSUSH_POOL_COUNT] = {
        config->small_pool_blocks,
        config->medium_pool_blocks,
        config->large_pool_blocks,
        config->xlarge_pool_blocks
    };

    for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
        lusush_pool_error_t result = init_single_pool(
            &global_memory_pool->pools[i], 
            POOL_SIZES[i], 
            block_counts[i]
        );
        
        if (result != LUSUSH_POOL_SUCCESS) {
            POOL_ERROR("Failed to initialize pool %d", i);
            // Cleanup already initialized pools
            for (int j = 0; j < i; j++) {
                cleanup_single_pool(&global_memory_pool->pools[j]);
            }
            free(global_memory_pool);
            global_memory_pool = NULL;
            pthread_mutex_unlock(&pool_mutex);
            set_last_error(result);
            return result;
        }
    }

    global_memory_pool->initialized = true;
    
    pthread_mutex_unlock(&pool_mutex);

    POOL_DEBUG("Memory pool system initialized successfully");
    POOL_DEBUG("Pools: Small=%zu, Medium=%zu, Large=%zu, XLarge=%zu", 
               block_counts[0], block_counts[1], block_counts[2], block_counts[3]);
    
    set_last_error(LUSUSH_POOL_SUCCESS);
    return LUSUSH_POOL_SUCCESS;
}

void lusush_pool_shutdown(void) {
    pthread_mutex_lock(&pool_mutex);

    if (!global_memory_pool) {
        pthread_mutex_unlock(&pool_mutex);
        return;
    }

    POOL_DEBUG("Shutting down memory pool system");

    // Print final statistics if enabled
    if (global_memory_pool->enable_statistics && debug_mode) {
        lusush_pool_print_status_report();
    }

    // Cleanup all pools
    for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
        cleanup_single_pool(&global_memory_pool->pools[i]);
    }

    // Free global pool system
    free(global_memory_pool);
    global_memory_pool = NULL;

    pthread_mutex_unlock(&pool_mutex);
    
    POOL_DEBUG("Memory pool system shutdown complete");
}

void *lusush_pool_alloc(size_t size) {
    if (size == 0) {
        set_last_error(LUSUSH_POOL_ERROR_INVALID_SIZE);
        return NULL;
    }

    uint64_t start_time = get_timestamp_ns();
    void *result = NULL;
    bool pool_hit = false;

    pthread_mutex_lock(&pool_mutex);

    if (!global_memory_pool || !global_memory_pool->initialized) {
        // Fallback to malloc if pool not initialized
        pthread_mutex_unlock(&pool_mutex);
        result = malloc(size);
        if (result && global_memory_pool) {
            update_stats(false, size, get_timestamp_ns() - start_time);
        }
        set_last_error(result ? LUSUSH_POOL_SUCCESS : LUSUSH_POOL_ERROR_MALLOC_FAILED);
        return result;
    }

    // Find appropriate pool
    lusush_pool_size_t pool_type = find_pool_for_size(size);
    
    if (pool_type < LUSUSH_POOL_COUNT) {
        // Try to allocate from pool
        result = allocate_from_pool(&global_memory_pool->pools[pool_type]);
        if (result) {
            pool_hit = true;
            POOL_DEBUG("Pool allocation: size=%zu, pool=%d", size, pool_type);
        }
    }

    // Fallback to malloc if pool allocation failed or size too large
    if (!result && global_memory_pool->enable_malloc_fallback) {
        result = malloc(size);
        
        // Track fallback sizes for optimization analysis
        if (fallback_count < 100) {
            fallback_sizes[fallback_count++] = size;
        }
        
        POOL_DEBUG("Malloc fallback: size=%zu (total fallbacks: %d)", size, fallback_count);
    }

    // Update statistics
    if (result) {
        update_stats(pool_hit, size, get_timestamp_ns() - start_time);
    }

    pthread_mutex_unlock(&pool_mutex);

    set_last_error(result ? LUSUSH_POOL_SUCCESS : LUSUSH_POOL_ERROR_MALLOC_FAILED);
    return result;
}

void lusush_pool_free(void *ptr) {
    if (!ptr) {
        return;
    }

    pthread_mutex_lock(&pool_mutex);

    bool returned_to_pool = false;

    if (global_memory_pool && global_memory_pool->initialized) {
        // Try to return to appropriate pool
        for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
            if (return_to_pool(&global_memory_pool->pools[i], ptr)) {
                returned_to_pool = true;
                
                // Update statistics
                if (global_memory_pool->enable_statistics) {
                    global_memory_pool->stats.active_allocations--;
                    global_memory_pool->stats.current_pool_usage -= POOL_SIZES[i];
                }
                
                POOL_DEBUG("Freed to pool: size=%zu, pool=%d", POOL_SIZES[i], i);
                break;
            }
        }
    }

    pthread_mutex_unlock(&pool_mutex);

    // If not returned to pool, use standard free
    if (!returned_to_pool) {
        free(ptr);
        POOL_DEBUG("Standard free: ptr=%p", ptr);
        
        // Update statistics for malloc fallback free
        if (global_memory_pool && global_memory_pool->enable_statistics) {
            pthread_mutex_lock(&pool_mutex);
            global_memory_pool->stats.active_allocations--;
            pthread_mutex_unlock(&pool_mutex);
        }
    }

    set_last_error(LUSUSH_POOL_SUCCESS);
}

void *lusush_pool_realloc(void *ptr, size_t new_size) {
    if (new_size == 0) {
        lusush_pool_free(ptr);
        return NULL;
    }

    if (!ptr) {
        return lusush_pool_alloc(new_size);
    }

    // For simplicity, we use malloc/free for realloc operations
    // This could be optimized in the future to use pools when possible
    void *new_ptr = lusush_pool_alloc(new_size);
    if (new_ptr && ptr) {
        // Copy data (we don't know the original size, so copy conservatively)
        // This is a limitation - in practice, we might need to track allocation sizes
        memcpy(new_ptr, ptr, new_size); // Assumes new_size <= old_size or undefined behavior
        lusush_pool_free(ptr);
    }

    return new_ptr;
}

char *lusush_pool_strdup(const char *str) {
    if (!str) {
        set_last_error(LUSUSH_POOL_ERROR_INVALID_POINTER);
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *dup = (char *)lusush_pool_alloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    
    return dup;
}

void *lusush_pool_calloc(size_t count, size_t size) {
    size_t total_size = count * size;
    void *ptr = lusush_pool_alloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

lusush_pool_stats_t lusush_pool_get_stats(void) {
    lusush_pool_stats_t stats = {0};
    
    if (!global_memory_pool) {
        return stats;
    }

    pthread_mutex_lock(&pool_mutex);
    stats = global_memory_pool->stats;
    pthread_mutex_unlock(&pool_mutex);

    return stats;
}

void lusush_pool_reset_stats(void) {
    if (!global_memory_pool) {
        return;
    }

    pthread_mutex_lock(&pool_mutex);
    memset(&global_memory_pool->stats, 0, sizeof(lusush_pool_stats_t));
    pthread_mutex_unlock(&pool_mutex);

    POOL_DEBUG("Pool statistics reset");
}

bool lusush_pool_is_healthy(void) {
    if (!global_memory_pool || !global_memory_pool->initialized) {
        return false;
    }

    pthread_mutex_lock(&pool_mutex);
    
    // Check each pool has some free blocks available
    bool healthy = true;
    for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
        if (global_memory_pool->pools[i].free_blocks == 0 && 
            global_memory_pool->pools[i].current_blocks >= global_memory_pool->pools[i].max_blocks) {
            healthy = false;
            break;
        }
    }

    pthread_mutex_unlock(&pool_mutex);
    return healthy;
}

lusush_pool_size_t lusush_pool_get_recommended_size(size_t size) {
    return find_pool_for_size(size);
}

void lusush_pool_analyze_fallback_patterns(void) {
    if (fallback_count == 0) {
        printf("No malloc fallbacks recorded\n");
        return;
    }
    
    printf("=== Memory Pool Fallback Analysis ===\n");
    printf("Total fallbacks: %d\n", fallback_count);
    
    // Count fallbacks by size ranges
    int small_misses = 0, medium_misses = 0, large_misses = 0, xlarge_misses = 0, oversized = 0;
    
    for (int i = 0; i < fallback_count; i++) {
        size_t size = fallback_sizes[i];
        if (size <= 128) small_misses++;
        else if (size <= 512) medium_misses++;
        else if (size <= 4096) large_misses++;
        else if (size <= 16384) xlarge_misses++;
        else oversized++;
    }
    
    printf("Fallback breakdown:\n");
    printf("  ≤ 128B (SMALL pool):  %d fallbacks\n", small_misses);
    printf("  ≤ 512B (MEDIUM pool): %d fallbacks\n", medium_misses);
    printf("  ≤ 4KB (LARGE pool):   %d fallbacks\n", large_misses);
    printf("  ≤ 16KB (XLARGE pool): %d fallbacks\n", xlarge_misses);
    printf("  > 16KB (oversized):   %d fallbacks\n", oversized);
    
    // Show pool status
    if (global_memory_pool && global_memory_pool->initialized) {
        printf("Pool Status:\n");
        for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
            lusush_pool_t *pool = &global_memory_pool->pools[i];
            printf("  Pool %d (%zuB): %zu/%zu blocks free\n", 
                   i, POOL_SIZES[i], pool->free_blocks, pool->current_blocks);
        }
    }
    
    // Show actual sizes for first 20 fallbacks
    printf("First %d fallback sizes: ", (fallback_count < 20) ? fallback_count : 20);
    for (int i = 0; i < fallback_count && i < 20; i++) {
        printf("%zu ", fallback_sizes[i]);
    }
    printf("\n=====================================\n");
}

bool lusush_pool_is_pool_pointer(const void *ptr) {
    if (!ptr || !global_memory_pool) {
        return false;
    }

    pthread_mutex_lock(&pool_mutex);

    bool is_pool_ptr = false;
    for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
        lusush_pool_t *pool = &global_memory_pool->pools[i];
        for (size_t j = 0; j < pool->current_blocks; j++) {
            if (pool->all_blocks[j].memory == ptr) {
                is_pool_ptr = true;
                goto cleanup;
            }
        }
    }

cleanup:
    pthread_mutex_unlock(&pool_mutex);
    return is_pool_ptr;
}

void lusush_pool_print_status_report(void) {
    if (!global_memory_pool) {
        printf("Memory pool system not initialized\n");
        return;
    }

    pthread_mutex_lock(&pool_mutex);

    printf("\n=== Lusush Memory Pool Status Report ===\n");
    
    lusush_pool_stats_t *stats = &global_memory_pool->stats;
    printf("Overall Statistics:\n");
    printf("  Total allocations: %lu\n", stats->total_allocations);
    printf("  Pool hits: %lu (%.2f%%)\n", stats->pool_hits, stats->pool_hit_rate);
    printf("  Malloc fallbacks: %lu\n", stats->malloc_fallbacks);
    printf("  Active allocations: %u\n", stats->active_allocations);
    printf("  Pool memory usage: %lu bytes (peak: %lu bytes)\n", 
           stats->current_pool_usage, stats->peak_pool_usage);
    printf("  Average allocation time: %lu ns\n", stats->avg_allocation_time_ns);

    printf("\nIndividual Pool Status:\n");
    const char* pool_names[] = {"Small", "Medium", "Large", "XLarge"};
    for (int i = 0; i < LUSUSH_POOL_COUNT; i++) {
        lusush_pool_t *pool = &global_memory_pool->pools[i];
        printf("  %s Pool (%zu bytes): %zu/%zu blocks free, %lu allocs, %lu deallocs\n",
               pool_names[i], pool->block_size, pool->free_blocks, pool->current_blocks,
               pool->pool_allocations, pool->pool_deallocations);
    }

    printf("========================================\n\n");

    pthread_mutex_unlock(&pool_mutex);
}

lusush_pool_config_t lusush_pool_get_default_config(void) {
    lusush_pool_config_t config = {
        .small_pool_blocks = DEFAULT_BLOCK_COUNTS[LUSUSH_POOL_SMALL],
        .medium_pool_blocks = DEFAULT_BLOCK_COUNTS[LUSUSH_POOL_MEDIUM],
        .large_pool_blocks = DEFAULT_BLOCK_COUNTS[LUSUSH_POOL_LARGE],
        .xlarge_pool_blocks = DEFAULT_BLOCK_COUNTS[LUSUSH_POOL_XLARGE],
        .enable_statistics = true,
        .enable_malloc_fallback = true,
        .enable_debugging = false
    };
    return config;
}

lusush_pool_config_t lusush_pool_get_display_optimized_config(void) {
    lusush_pool_config_t config = lusush_pool_get_default_config();
    
    // Optimize for display operations based on fallback analysis
    config.small_pool_blocks = 512;  // Analysis shows 100% fallbacks are small allocations
    config.medium_pool_blocks = 64;  // More medium blocks for prompts
    config.large_pool_blocks = 32;   // More large blocks for compositions
    config.xlarge_pool_blocks = 16;  // More XL blocks for complex outputs
    
    return config;
}

const char *lusush_pool_error_string(lusush_pool_error_t error) {
    switch (error) {
        case LUSUSH_POOL_SUCCESS: return "Success";
        case LUSUSH_POOL_ERROR_NOT_INITIALIZED: return "Pool system not initialized";
        case LUSUSH_POOL_ERROR_INVALID_SIZE: return "Invalid allocation size";
        case LUSUSH_POOL_ERROR_POOL_EXHAUSTED: return "Pool exhausted";
        case LUSUSH_POOL_ERROR_INVALID_POINTER: return "Invalid pointer";
        case LUSUSH_POOL_ERROR_MALLOC_FAILED: return "Malloc allocation failed";
        case LUSUSH_POOL_ERROR_BLOCK_NOT_FOUND: return "Block not found";
        case LUSUSH_POOL_ERROR_DOUBLE_FREE: return "Double free detected";
        case LUSUSH_POOL_ERROR_INITIALIZATION_FAILED: return "Initialization failed";
        default: return "Unknown error";
    }
}

void lusush_pool_set_debug_mode(bool enabled) {
    debug_mode = enabled;
    POOL_DEBUG("Debug mode %s", enabled ? "enabled" : "disabled");
}

lusush_pool_error_t lusush_pool_get_last_error(void) {
    return last_error;
}

void lusush_pool_get_memory_usage(uint64_t *pool_bytes, uint64_t *malloc_bytes, double *pool_efficiency) {
    if (!global_memory_pool) {
        if (pool_bytes) *pool_bytes = 0;
        if (malloc_bytes) *malloc_bytes = 0;
        if (pool_efficiency) *pool_efficiency = 0.0;
        return;
    }

    pthread_mutex_lock(&pool_mutex);
    
    if (pool_bytes) {
        *pool_bytes = global_memory_pool->stats.current_pool_usage;
    }
    
    if (malloc_bytes) {
        *malloc_bytes = global_memory_pool->stats.total_bytes_allocated - 
                        global_memory_pool->stats.current_pool_usage;
    }
    
    if (pool_efficiency) {
        *pool_efficiency = global_memory_pool->stats.pool_hit_rate;
    }
    
    pthread_mutex_unlock(&pool_mutex);
}

bool lusush_pool_meets_performance_targets(void) {
    if (!global_memory_pool) {
        return false;
    }

    pthread_mutex_lock(&pool_mutex);
    
    // Performance targets:
    // - Pool hit rate > 80%
    // - Average allocation time < 1000 ns
    // - System healthy (pools not exhausted)
    
    bool meets_targets = (global_memory_pool->stats.pool_hit_rate > 80.0) &&
                        (global_memory_pool->stats.avg_allocation_time_ns < 1000) &&
                        lusush_pool_is_healthy();
    
    pthread_mutex_unlock(&pool_mutex);
    return meets_targets;
}
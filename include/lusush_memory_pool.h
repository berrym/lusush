/*
 * Lusush Shell - Memory Pool System
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

#ifndef LUSUSH_MEMORY_POOL_H
#define LUSUSH_MEMORY_POOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Memory Pool System - Enterprise-Grade Memory Management
 * 
 * Designed for high-frequency allocation patterns in display operations,
 * cache management, and composition engine. Provides automatic fallback
 * to malloc for robustness and comprehensive memory usage tracking.
 */

// Memory pool size categories
typedef enum {
    LUSUSH_POOL_SMALL = 0,    // 128B - state hashes, small strings, cache keys
    LUSUSH_POOL_MEDIUM = 1,   // 512B - prompts, short outputs, command strings
    LUSUSH_POOL_LARGE = 2,    // 4KB - display outputs, compositions, multiline inputs
    LUSUSH_POOL_XLARGE = 3,   // 16KB - tab completions, large buffers, complex outputs
    LUSUSH_POOL_COUNT = 4
} lusush_pool_size_t;

// Pool block structure for memory management
typedef struct lusush_pool_block {
    void *memory;                       // Allocated memory block
    size_t size;                        // Block size
    bool in_use;                        // Allocation status
    uint64_t allocation_time_us;        // Allocation timestamp for debugging
    uint32_t allocation_id;             // Unique allocation ID for tracking
    struct lusush_pool_block *next;     // Next block in free list
    struct lusush_pool_block *prev;     // Previous block for efficient removal
} lusush_pool_block_t;

// Memory pool statistics for performance monitoring
typedef struct {
    uint64_t total_allocations;         // Total allocation requests
    uint64_t pool_hits;                 // Successful pool allocations
    uint64_t pool_misses;               // Fallback to malloc count
    uint64_t current_pool_usage;        // Current bytes allocated from pools
    uint64_t peak_pool_usage;           // Maximum pool usage recorded
    uint64_t malloc_fallbacks;          // Count of malloc fallback calls
    uint64_t total_bytes_allocated;     // Total bytes allocated (pool + malloc)
    double pool_hit_rate;               // Pool allocation success rate
    uint64_t avg_allocation_time_ns;    // Average allocation time
    uint32_t active_allocations;        // Current active allocation count
} lusush_pool_stats_t;

// Individual pool configuration and state
typedef struct {
    size_t block_size;                  // Size of each block in this pool
    size_t initial_blocks;              // Initial number of blocks to pre-allocate
    size_t max_blocks;                  // Maximum blocks allowed (0 = unlimited)
    size_t current_blocks;              // Current number of allocated blocks
    size_t free_blocks;                 // Number of available blocks
    lusush_pool_block_t *free_list;     // Linked list of free blocks
    lusush_pool_block_t *all_blocks;    // Array of all blocks for cleanup
    uint64_t pool_allocations;          // Allocations from this specific pool
    uint64_t pool_deallocations;        // Deallocations to this specific pool
} lusush_pool_t;

// Main memory pool system
typedef struct {
    lusush_pool_t pools[LUSUSH_POOL_COUNT];    // Individual size pools
    lusush_pool_stats_t stats;                  // Performance statistics
    bool initialized;                           // Initialization status
    bool enable_statistics;                     // Statistics collection toggle
    bool enable_malloc_fallback;               // Automatic malloc fallback
    uint32_t next_allocation_id;               // Unique ID counter
    struct timespec init_time;                 // Pool system initialization time
} lusush_memory_pool_system_t;

// Error codes for memory pool operations
typedef enum {
    LUSUSH_POOL_SUCCESS = 0,
    LUSUSH_POOL_ERROR_NOT_INITIALIZED,
    LUSUSH_POOL_ERROR_INVALID_SIZE,
    LUSUSH_POOL_ERROR_POOL_EXHAUSTED,
    LUSUSH_POOL_ERROR_INVALID_POINTER,
    LUSUSH_POOL_ERROR_MALLOC_FAILED,
    LUSUSH_POOL_ERROR_BLOCK_NOT_FOUND,
    LUSUSH_POOL_ERROR_DOUBLE_FREE,
    LUSUSH_POOL_ERROR_INITIALIZATION_FAILED
} lusush_pool_error_t;

// Memory pool configuration structure
typedef struct {
    size_t small_pool_blocks;       // Number of 128B blocks (default: 64)
    size_t medium_pool_blocks;      // Number of 512B blocks (default: 32)
    size_t large_pool_blocks;       // Number of 4KB blocks (default: 16)
    size_t xlarge_pool_blocks;      // Number of 16KB blocks (default: 8)
    bool enable_statistics;         // Enable detailed statistics collection
    bool enable_malloc_fallback;    // Enable automatic malloc fallback
    bool enable_debugging;          // Enable debug tracking and validation
} lusush_pool_config_t;

// Global memory pool system instance
extern lusush_memory_pool_system_t *global_memory_pool;

/**
 * Core Memory Pool API
 */

// Initialize the memory pool system with configuration
lusush_pool_error_t lusush_pool_init(const lusush_pool_config_t *config);

// Shutdown and cleanup the memory pool system
void lusush_pool_shutdown(void);

// Allocate memory from appropriate pool or fallback to malloc
void *lusush_pool_alloc(size_t size);

// Free memory back to pool or call free() for malloc allocations
void lusush_pool_free(void *ptr);

// Reallocate memory (may use malloc for complex resizing)
void *lusush_pool_realloc(void *ptr, size_t new_size);

// Duplicate string using pool allocation
char *lusush_pool_strdup(const char *str);

// Allocate and zero-initialize memory
void *lusush_pool_calloc(size_t count, size_t size);

/**
 * Pool Management and Information API
 */

// Get current pool statistics
lusush_pool_stats_t lusush_pool_get_stats(void);

// Reset statistics counters
void lusush_pool_reset_stats(void);

// Get pool system status and health information
bool lusush_pool_is_healthy(void);

// Perform pool maintenance (defragmentation, cleanup)
void lusush_pool_maintenance(void);

// Get recommended pool size for given allocation size
lusush_pool_size_t lusush_pool_get_recommended_size(size_t size);

// Check if pointer was allocated from pool system
bool lusush_pool_is_pool_pointer(const void *ptr);

/**
 * Advanced Pool Operations
 */

// Pre-allocate blocks in specific pool for performance
lusush_pool_error_t lusush_pool_preallocate(lusush_pool_size_t pool_type, size_t count);

// Get detailed information about specific pool
void lusush_pool_get_pool_info(lusush_pool_size_t pool_type, 
                               size_t *block_size, size_t *free_blocks, 
                               size_t *total_blocks);

// Validate pool integrity (debug/testing)
bool lusush_pool_validate_integrity(void);

// Generate detailed pool status report for debugging
void lusush_pool_print_status_report(void);

/**
 * Integration with Performance Monitoring System
 */

// Get pool memory usage for display performance reporting
void lusush_pool_get_memory_usage(uint64_t *pool_bytes, uint64_t *malloc_bytes, 
                                  double *pool_efficiency);

// Check if pool system is meeting performance targets
bool lusush_pool_meets_performance_targets(void);

/**
 * Configuration Helpers
 */

// Get default pool configuration for typical usage
lusush_pool_config_t lusush_pool_get_default_config(void);

// Create optimized configuration for display-heavy workloads
lusush_pool_config_t lusush_pool_get_display_optimized_config(void);

// Create configuration for memory-constrained environments
lusush_pool_config_t lusush_pool_get_minimal_config(void);

/**
 * Error Handling and Debugging
 */

// Convert error code to human-readable string
const char *lusush_pool_error_string(lusush_pool_error_t error);

// Enable/disable debug mode with detailed logging
void lusush_pool_set_debug_mode(bool enabled);

// Get last error that occurred in pool operations
lusush_pool_error_t lusush_pool_get_last_error(void);

/**
 * Thread Safety Note:
 * 
 * The memory pool system is designed to be thread-safe for future expansion.
 * All public API functions use appropriate synchronization mechanisms to
 * ensure safe concurrent access from multiple threads.
 */

/**
 * Usage Examples:
 * 
 * // Initialize with default configuration
 * lusush_pool_config_t config = lusush_pool_get_default_config();
 * lusush_pool_init(&config);
 * 
 * // Allocate memory (automatically selects appropriate pool)
 * char *buffer = lusush_pool_alloc(256);
 * 
 * // Use the buffer...
 * 
 * // Free memory (automatically returns to correct pool)
 * lusush_pool_free(buffer);
 * 
 * // Get statistics
 * lusush_pool_stats_t stats = lusush_pool_get_stats();
 * printf("Pool hit rate: %.2f%%\n", stats.pool_hit_rate);
 * 
 * // Shutdown when done
 * lusush_pool_shutdown();
 */

#ifdef __cplusplus
}
#endif

#endif /* LUSUSH_MEMORY_POOL_H */
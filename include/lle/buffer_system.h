/**
 * @file buffer_system.h
 * @brief LLE Buffer Management System - Phase 1 Foundation
 * 
 * Provides core buffer management infrastructure for LLE's buffer-oriented
 * command editing. Phase 1 implements foundation: system lifecycle, buffer
 * pool, and basic buffer creation/destruction.
 * 
 * Phase 1 Scope:
 * - Buffer system initialization and lifecycle
 * - Buffer pool management
 * - Core buffer structures (simplified)
 * - Basic memory management
 * 
 * Deferred to Later Phases:
 * - Line structure tracking (Phase 2)
 * - UTF-8 indexing (Phase 3)
 * - Cursor management (Phase 4)
 * - Change tracking/undo (Phase 5)
 * - Multiline parsing (Phase 6)
 * - Validation system (Phase 7)
 * - Event integration (Phase 8)
 * 
 * Spec: 03_buffer_management_complete.md
 * Implementation: SPEC_03_PHASED_IMPLEMENTATION_PLAN.md
 */

#ifndef LLE_BUFFER_SYSTEM_H
#define LLE_BUFFER_SYSTEM_H

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/performance.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Constants and Configuration
 * ======================================================================== */

/** Default buffer capacity (4KB) */
#define LLE_BUFFER_DEFAULT_CAPACITY 4096

/** Maximum buffer name length */
#define LLE_BUFFER_NAME_MAX 64

/** Initial buffer pool capacity */
#define LLE_BUFFER_POOL_INITIAL_CAPACITY 16

/* ========================================================================
 * Phase 1: Core Data Structures (Simplified)
 * ======================================================================== */

/**
 * Core buffer structure (Phase 1 - minimal)
 * 
 * Phase 1 includes only essential fields for buffer creation and
 * basic memory management. Additional fields added in later phases.
 */
typedef struct lle_buffer {
    /* Buffer identification */
    uint32_t buffer_id;                     /**< Unique buffer identifier */
    char name[LLE_BUFFER_NAME_MAX];         /**< Buffer name/description */
    
    /* Buffer content storage */
    char *data;                             /**< Buffer data (null-terminated) */
    size_t capacity;                        /**< Allocated capacity */
    size_t length;                          /**< Current length (bytes) */
    
    /* Memory management */
    struct lle_buffer_pool *pool;           /**< Associated buffer pool */
    lle_memory_pool_t *memory_pool;         /**< LLE memory pool */
    
    /* Timestamps (Phase 1) */
    uint64_t creation_time;                 /**< Creation timestamp */
    uint64_t last_modified_time;            /**< Last modification timestamp */
    
} lle_buffer_t;

/**
 * Buffer pool structure (Phase 1)
 * 
 * Manages a collection of buffers with memory pooling.
 */
typedef struct lle_buffer_pool {
    lle_buffer_t **buffers;                 /**< Buffer array */
    size_t capacity;                        /**< Pool capacity */
    size_t count;                           /**< Current buffer count */
    lle_memory_pool_t *memory_pool;         /**< LLE memory pool */
    uint32_t next_buffer_id;                /**< Next buffer ID */
} lle_buffer_pool_t;

/**
 * Buffer system structure (Phase 1 - simplified)
 * 
 * Main system container. Phase 1 includes only buffer pool and
 * performance monitoring. Other subsystems added in later phases.
 */
typedef struct lle_buffer_system {
    lle_buffer_t *current_buffer;           /**< Active buffer */
    lle_buffer_pool_t *buffer_pool;         /**< Buffer pool */
    lle_performance_monitor_t *perf_monitor; /**< Performance tracking */
    lle_memory_pool_t *memory_pool;         /**< LLE memory pool */
} lle_buffer_system_t;

/* ========================================================================
 * Phase 1: System Lifecycle Functions
 * ======================================================================== */

/**
 * Initialize buffer management system
 * 
 * Creates the buffer system with performance monitoring and memory pool.
 * Phase 1 creates a simplified system - additional subsystems added in
 * later phases.
 * 
 * @param system    Output pointer to initialized system
 * @param pool      LLE memory pool to use
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * Example:
 *   lle_memory_pool_t *pool = lle_create_memory_pool(1024*1024, 8, LLE_POOL_TYPE_GENERAL);
 *   lle_buffer_system_t *system = NULL;
 *   lle_result_t result = lle_buffer_system_init(&system, pool);
 */
lle_result_t lle_buffer_system_init(lle_buffer_system_t **system,
                                    lle_memory_pool_t *pool);

/**
 * Destroy buffer management system
 * 
 * Cleans up all buffers, pools, and subsystems. Frees all associated
 * memory.
 * 
 * @param system    Buffer system to destroy
 */
void lle_buffer_system_destroy(lle_buffer_system_t *system);

/**
 * Validate buffer system state
 * 
 * Verifies system integrity. Phase 1 performs basic validation only.
 * More comprehensive validation added in Phase 7.
 * 
 * @param system    Buffer system to validate
 * @return LLE_SUCCESS if valid, error code if validation fails
 */
lle_result_t lle_buffer_system_validate(lle_buffer_system_t *system);

/* ========================================================================
 * Phase 1: Buffer Pool Functions
 * ======================================================================== */

/**
 * Initialize buffer pool
 * 
 * Creates a buffer pool with initial capacity. Pool grows dynamically
 * as needed.
 * 
 * @param pool          Output pointer to initialized pool
 * @param memory_pool   LLE memory pool to use
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_buffer_pool_init(lle_buffer_pool_t **pool,
                                  lle_memory_pool_t *memory_pool);

/**
 * Destroy buffer pool
 * 
 * Destroys all buffers in pool and frees pool memory.
 * 
 * @param pool    Buffer pool to destroy
 */
void lle_buffer_pool_destroy(lle_buffer_pool_t *pool);

/**
 * Allocate buffer from pool
 * 
 * Gets next available buffer ID and prepares pool for new buffer.
 * Grows pool capacity if needed.
 * 
 * @param pool          Buffer pool
 * @param buffer_id     Output buffer ID
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_buffer_pool_allocate(lle_buffer_pool_t *pool,
                                      uint32_t *buffer_id);

/* ========================================================================
 * Phase 1: Core Buffer Functions
 * ======================================================================== */

/**
 * Create new buffer
 * 
 * Creates an empty buffer with specified capacity. Buffer is added to
 * the pool and initialized with basic metadata.
 * 
 * @param buffer        Output pointer to created buffer
 * @param pool          Buffer pool
 * @param capacity      Initial capacity (bytes)
 * @return LLE_SUCCESS on success, error code on failure
 * 
 * Example:
 *   lle_buffer_t *buf = NULL;
 *   lle_buffer_create(&buf, system->buffer_pool, LLE_BUFFER_DEFAULT_CAPACITY);
 */
lle_result_t lle_buffer_create(lle_buffer_t **buffer,
                               lle_buffer_pool_t *pool,
                               size_t capacity);

/**
 * Destroy buffer
 * 
 * Frees buffer memory and removes from pool.
 * 
 * @param buffer    Buffer to destroy
 */
void lle_buffer_destroy(lle_buffer_t *buffer);

/* ========================================================================
 * Phase 1: Utility Functions
 * ======================================================================== */

/**
 * Get current time in microseconds
 * 
 * Used for buffer timestamps. Uses CLOCK_MONOTONIC.
 * 
 * @return Current time in microseconds
 */
uint64_t lle_buffer_get_time_us(void);

#ifdef __cplusplus
}
#endif

#endif /* LLE_BUFFER_SYSTEM_H */

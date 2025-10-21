/**
 * @file buffer_system.c
 * @brief LLE Buffer System Implementation - Phase 1
 * 
 * Implements buffer system lifecycle and pool management.
 */

#include "lle/buffer_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

uint64_t lle_buffer_get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/* ========================================================================
 * Buffer System Lifecycle
 * ======================================================================== */

lle_result_t lle_buffer_system_init(lle_buffer_system_t **system,
                                    lle_memory_pool_t *pool) {
    if (!system || !pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    // Allocate system structure from memory pool
    lle_buffer_system_t *sys = lle_pool_allocate_fast(pool, sizeof(lle_buffer_system_t));
    if (!sys) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(sys, 0, sizeof(lle_buffer_system_t));
    sys->memory_pool = pool;
    
    // Allocate and initialize performance monitoring
    sys->perf_monitor = lle_pool_allocate_fast(pool, sizeof(lle_performance_monitor_t));
    if (!sys->perf_monitor) {
        lle_pool_free_fast(pool, sys);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    lle_performance_config_t perf_config = {0};
    result = lle_perf_monitor_init(sys->perf_monitor, &perf_config);
    if (result != LLE_SUCCESS) {
        lle_pool_free_fast(pool, sys->perf_monitor);
        lle_pool_free_fast(pool, sys);
        return result;
    }
    
    // Initialize buffer pool
    result = lle_buffer_pool_init(&sys->buffer_pool, pool);
    if (result != LLE_SUCCESS) {
        lle_perf_monitor_destroy(sys->perf_monitor);
        lle_pool_free_fast(pool, sys);
        return result;
    }
    
    // Create initial buffer
    result = lle_buffer_create(&sys->current_buffer, sys->buffer_pool, 
                               LLE_BUFFER_DEFAULT_CAPACITY);
    if (result != LLE_SUCCESS) {
        lle_buffer_pool_destroy(sys->buffer_pool);
        lle_perf_monitor_destroy(sys->perf_monitor);
        lle_pool_free_fast(pool, sys);
        return result;
    }
    
    // Validate system
    result = lle_buffer_system_validate(sys);
    if (result != LLE_SUCCESS) {
        lle_buffer_destroy(sys->current_buffer);
        lle_buffer_pool_destroy(sys->buffer_pool);
        lle_perf_monitor_destroy(sys->perf_monitor);
        lle_pool_free_fast(pool, sys);
        return result;
    }
    
    *system = sys;
    return LLE_SUCCESS;
}

void lle_buffer_system_destroy(lle_buffer_system_t *system) {
    if (!system) {
        return;
    }
    
    // Destroy current buffer
    if (system->current_buffer) {
        lle_buffer_destroy(system->current_buffer);
    }
    
    // Destroy buffer pool (destroys all buffers in pool)
    if (system->buffer_pool) {
        lle_buffer_pool_destroy(system->buffer_pool);
    }
    
    // Destroy performance monitor
    if (system->perf_monitor) {
        lle_perf_monitor_destroy(system->perf_monitor);
        lle_pool_free_fast(system->memory_pool, system->perf_monitor);
    }
    
    // Free system structure
    if (system->memory_pool) {
        lle_pool_free_fast(system->memory_pool, system);
    }
}

lle_result_t lle_buffer_system_validate(lle_buffer_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Validate memory pool
    if (!system->memory_pool) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    // Validate buffer pool
    if (!system->buffer_pool) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    // Validate performance monitor
    if (!system->perf_monitor) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    // Phase 1: Basic validation only
    // More comprehensive validation in Phase 7
    
    return LLE_SUCCESS;
}

/* ========================================================================
 * Buffer Pool Management
 * ======================================================================== */

lle_result_t lle_buffer_pool_init(lle_buffer_pool_t **pool,
                                  lle_memory_pool_t *memory_pool) {
    if (!pool || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Allocate pool structure
    lle_buffer_pool_t *p = lle_pool_allocate_fast(memory_pool, sizeof(lle_buffer_pool_t));
    if (!p) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(p, 0, sizeof(lle_buffer_pool_t));
    
    // Allocate buffer array
    p->capacity = LLE_BUFFER_POOL_INITIAL_CAPACITY;
    p->buffers = lle_pool_allocate_fast(memory_pool, 
                                        p->capacity * sizeof(lle_buffer_t *));
    if (!p->buffers) {
        lle_pool_free_fast(memory_pool, p);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(p->buffers, 0, p->capacity * sizeof(lle_buffer_t *));
    p->count = 0;
    p->memory_pool = memory_pool;
    p->next_buffer_id = 1; // Start IDs at 1
    
    *pool = p;
    return LLE_SUCCESS;
}

void lle_buffer_pool_destroy(lle_buffer_pool_t *pool) {
    if (!pool) {
        return;
    }
    
    // Destroy all buffers in pool
    if (pool->buffers) {
        for (size_t i = 0; i < pool->count; i++) {
            if (pool->buffers[i]) {
                lle_buffer_destroy(pool->buffers[i]);
            }
        }
        
        // Free buffer array
        if (pool->memory_pool) {
            lle_pool_free_fast(pool->memory_pool, pool->buffers);
        }
    }
    
    // Free pool structure
    if (pool->memory_pool) {
        lle_pool_free_fast(pool->memory_pool, pool);
    }
}

lle_result_t lle_buffer_pool_allocate(lle_buffer_pool_t *pool,
                                      uint32_t *buffer_id) {
    if (!pool || !buffer_id) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    // Check if pool needs to grow
    if (pool->count >= pool->capacity) {
        size_t new_capacity = pool->capacity * 2;
        lle_buffer_t **new_buffers = lle_pool_allocate_fast(
            pool->memory_pool,
            new_capacity * sizeof(lle_buffer_t *)
        );
        
        if (!new_buffers) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy existing buffers
        memcpy(new_buffers, pool->buffers, pool->count * sizeof(lle_buffer_t *));
        memset(new_buffers + pool->count, 0, 
               (new_capacity - pool->count) * sizeof(lle_buffer_t *));
        
        // Free old array
        lle_pool_free_fast(pool->memory_pool, pool->buffers);
        
        pool->buffers = new_buffers;
        pool->capacity = new_capacity;
    }
    
    // Allocate next buffer ID
    *buffer_id = pool->next_buffer_id++;
    
    return LLE_SUCCESS;
}

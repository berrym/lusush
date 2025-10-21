/**
 * @file buffer_core.c
 * @brief LLE Core Buffer Implementation - Phase 1
 * 
 * Implements basic buffer creation and destruction.
 */

#include "lle/buffer_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================
 * Core Buffer Functions
 * ======================================================================== */

lle_result_t lle_buffer_create(lle_buffer_t **buffer,
                               lle_buffer_pool_t *pool,
                               size_t capacity) {
    if (!buffer || !pool || capacity == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t result = LLE_SUCCESS;
    
    // Allocate buffer ID from pool
    uint32_t buffer_id = 0;
    result = lle_buffer_pool_allocate(pool, &buffer_id);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Allocate buffer structure
    lle_buffer_t *buf = lle_pool_allocate_fast(pool->memory_pool, sizeof(lle_buffer_t));
    if (!buf) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(buf, 0, sizeof(lle_buffer_t));
    
    // Allocate buffer data
    buf->data = lle_pool_allocate_fast(pool->memory_pool, capacity + 1); // +1 for null terminator
    if (!buf->data) {
        lle_pool_free_fast(pool->memory_pool, buf);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize buffer
    buf->buffer_id = buffer_id;
    buf->capacity = capacity;
    buf->length = 0;
    buf->data[0] = '\0'; // Start with empty string
    buf->pool = pool;
    buf->memory_pool = pool->memory_pool;
    
    // Set timestamps
    uint64_t now = lle_buffer_get_time_us();
    buf->creation_time = now;
    buf->last_modified_time = now;
    
    // Generate default name
    snprintf(buf->name, LLE_BUFFER_NAME_MAX, "buffer_%u", buffer_id);
    
    // Add to pool
    pool->buffers[pool->count] = buf;
    pool->count++;
    
    *buffer = buf;
    return LLE_SUCCESS;
}

void lle_buffer_destroy(lle_buffer_t *buffer) {
    if (!buffer) {
        return;
    }
    
    // Free buffer data
    if (buffer->data && buffer->memory_pool) {
        lle_pool_free_fast(buffer->memory_pool, buffer->data);
    }
    
    // Remove from pool
    if (buffer->pool) {
        lle_buffer_pool_t *pool = buffer->pool;
        
        // Find buffer in pool array
        for (size_t i = 0; i < pool->count; i++) {
            if (pool->buffers[i] == buffer) {
                // Shift remaining buffers down
                for (size_t j = i; j < pool->count - 1; j++) {
                    pool->buffers[j] = pool->buffers[j + 1];
                }
                pool->buffers[pool->count - 1] = NULL;
                pool->count--;
                break;
            }
        }
    }
    
    // Free buffer structure
    if (buffer->memory_pool) {
        lle_pool_free_fast(buffer->memory_pool, buffer);
    }
}

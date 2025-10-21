/**
 * @file event_system.c
 * @brief LLE Event System Main Implementation
 * 
 * Core event system initialization, lifecycle, and coordination.
 * Phase 1: Simple event creation, queueing, and dispatching.
 * 
 * Spec: 04_event_system_complete.md - Phase 1
 */

#include "lle/event_system.h"
#include "lle/error_handling.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * INTERNAL FUNCTION DECLARATIONS
 * ============================================================================
 */

/* From event_queue.c */
extern lle_result_t event_queue_enqueue_internal(lle_event_queue_t *queue, lle_event_t *event);
extern lle_result_t event_queue_dequeue_internal(lle_event_queue_t *queue, lle_event_t **event);
extern size_t event_queue_count_internal(lle_event_queue_t *queue);

/* From event_handlers.c */
extern lle_result_t event_handlers_dispatch_internal(lle_event_system_t *system, lle_event_t *event);

/* ============================================================================
 * EVENT SYSTEM LIFECYCLE
 * ============================================================================
 */

lle_result_t lle_event_system_init(lle_event_system_t **system, lle_memory_pool_t *pool) {
    if (!system || !pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate system structure */
    lle_event_system_t *sys = calloc(1, sizeof(lle_event_system_t));
    if (!sys) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize system mutex */
    if (pthread_mutex_init(&sys->system_mutex, NULL) != 0) {
        free(sys);
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    /* Create event queue (default capacity: 1024) */
    lle_result_t result = lle_event_queue_init(&sys->queue, 1024);
    if (result != LLE_SUCCESS) {
        pthread_mutex_destroy(&sys->system_mutex);
        free(sys);
        return result;
    }
    
    /* Initialize handler registry (initial capacity: 64) */
    sys->handler_capacity = 64;
    sys->handlers = calloc(sys->handler_capacity, sizeof(lle_event_handler_t *));
    if (!sys->handlers) {
        lle_event_queue_destroy(sys->queue);
        pthread_mutex_destroy(&sys->system_mutex);
        free(sys);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    sys->handler_count = 0;
    
    /* Store event pool */
    sys->event_pool = pool;
    
    /* Initialize state */
    sys->sequence_counter = 1;
    sys->active = false; /* Start inactive */
    
    *system = sys;
    return LLE_SUCCESS;
}

void lle_event_system_destroy(lle_event_system_t *system) {
    if (!system) {
        return;
    }
    
    /* Stop system if active */
    if (system->active) {
        lle_event_system_stop(system);
    }
    
    /* Destroy all queued events */
    if (system->queue) {
        lle_event_t *event = NULL;
        while (event_queue_count_internal(system->queue) > 0) {
            if (event_queue_dequeue_internal(system->queue, &event) == LLE_SUCCESS && event) {
                lle_event_destroy(system, event);
            }
        }
        lle_event_queue_destroy(system->queue);
    }
    
    /* Free all registered handlers */
    if (system->handlers) {
        for (size_t i = 0; i < system->handler_count; i++) {
            free(system->handlers[i]);
        }
        free(system->handlers);
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&system->system_mutex);
    
    /* Free system structure */
    free(system);
}

lle_result_t lle_event_system_start(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    if (system->active) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_INVALID_STATE; /* Already active */
    }
    
    system->active = true;
    
    pthread_mutex_unlock(&system->system_mutex);
    return LLE_SUCCESS;
}

lle_result_t lle_event_system_stop(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    if (!system->active) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_INVALID_STATE; /* Already stopped */
    }
    
    system->active = false;
    
    pthread_mutex_unlock(&system->system_mutex);
    return LLE_SUCCESS;
}

/* ============================================================================
 * EVENT CREATION AND DESTRUCTION
 * ============================================================================
 */

lle_result_t lle_event_create(lle_event_system_t *system,
                               lle_system_event_type_t type,
                               const void *data,
                               size_t data_size,
                               lle_event_t **event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate event from pool */
    lle_event_t *e = lle_pool_allocate_fast(system->event_pool, sizeof(lle_event_t));
    if (!e) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize event */
    memset(e, 0, sizeof(lle_event_t));
    e->type = type;
    e->timestamp = lle_event_get_timestamp_us();
    
    /* Assign sequence number */
    pthread_mutex_lock(&system->system_mutex);
    e->sequence_number = system->sequence_counter++;
    pthread_mutex_unlock(&system->system_mutex);
    
    /* Copy event data if provided */
    if (data && data_size > 0) {
        void *data_copy = lle_pool_allocate_fast(system->event_pool, data_size);
        if (!data_copy) {
            lle_pool_free(system->event_pool, e);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        
        memcpy(data_copy, data, data_size);
        e->data = data_copy;
        e->data_size = data_size;
    } else {
        e->data = NULL;
        e->data_size = 0;
    }
    
    *event = e;
    return LLE_SUCCESS;
}

void lle_event_destroy(lle_event_system_t *system, lle_event_t *event) {
    if (!system || !event) {
        return;
    }
    
    /* Free event data if present */
    if (event->data) {
        lle_pool_free(system->event_pool, event->data);
    }
    
    /* Free event structure */
    lle_pool_free(system->event_pool, event);
}

lle_result_t lle_event_clone(lle_event_system_t *system,
                              const lle_event_t *source,
                              lle_event_t **dest) {
    if (!system || !source || !dest) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Create new event with same type and data */
    return lle_event_create(system, source->type, source->data, source->data_size, dest);
}

/* ============================================================================
 * QUEUE OPERATIONS (Wrappers)
 * ============================================================================
 */

lle_result_t lle_event_enqueue(lle_event_system_t *system, lle_event_t *event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    if (!system->active) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_INVALID_STATE;
    }
    
    lle_result_t result = event_queue_enqueue_internal(system->queue, event);
    
    pthread_mutex_unlock(&system->system_mutex);
    return result;
}

lle_result_t lle_event_dequeue(lle_event_system_t *system, lle_event_t **event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    lle_result_t result = event_queue_dequeue_internal(system->queue, event);
    
    pthread_mutex_unlock(&system->system_mutex);
    return result;
}

/* ============================================================================
 * EVENT PROCESSING
 * ============================================================================
 */

lle_result_t lle_event_dispatch(lle_event_system_t *system, lle_event_t *event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    if (!system->active) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Dispatch to all registered handlers */
    lle_result_t result = event_handlers_dispatch_internal(system, event);
    
    pthread_mutex_unlock(&system->system_mutex);
    return result;
}

lle_result_t lle_event_process_queue(lle_event_system_t *system, uint32_t max_events) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!system->active) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    uint32_t processed = 0;
    lle_result_t result = LLE_SUCCESS;
    
    while (processed < max_events) {
        lle_event_t *event = NULL;
        
        /* Dequeue event */
        result = lle_event_dequeue(system, &event);
        if (result != LLE_SUCCESS) {
            break;
        }
        
        /* If queue is empty, we're done */
        if (!event) {
            break;
        }
        
        /* Dispatch event */
        result = lle_event_dispatch(system, event);
        
        /* Destroy event after dispatch */
        lle_event_destroy(system, event);
        
        processed++;
        
        /* Continue even if dispatch failed */
        if (result != LLE_SUCCESS) {
            /* Log error but continue processing */
        }
    }
    
    return LLE_SUCCESS;
}

lle_result_t lle_event_process_all(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Process up to UINT32_MAX events (effectively all) */
    return lle_event_process_queue(system, UINT32_MAX);
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

uint64_t lle_event_get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

const char *lle_event_type_name(lle_system_event_type_t type) {
    switch (type) {
        case LLE_EVENT_KEY_PRESS:         return "KEY_PRESS";
        case LLE_EVENT_KEY_SEQUENCE:      return "KEY_SEQUENCE";
        case LLE_EVENT_TERMINAL_RESIZE:   return "TERMINAL_RESIZE";
        case LLE_EVENT_BUFFER_CHANGED:    return "BUFFER_CHANGED";
        case LLE_EVENT_CURSOR_MOVED:      return "CURSOR_MOVED";
        case LLE_EVENT_DISPLAY_UPDATE:    return "DISPLAY_UPDATE";
        case LLE_EVENT_DISPLAY_REFRESH:   return "DISPLAY_REFRESH";
        case LLE_EVENT_SYSTEM_ERROR:      return "SYSTEM_ERROR";
        case LLE_EVENT_SYSTEM_SHUTDOWN:   return "SYSTEM_SHUTDOWN";
        default:                           return "UNKNOWN";
    }
}

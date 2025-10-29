/*
 * event_system.c - Event System Implementation (Phase 1)
 * 
 * Core event system lifecycle and event creation/destruction.
 * 
 * Spec 04: Event System - Phase 1
 */

#include "../../include/lle/event_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LLE_EVENT_QUEUE_DEFAULT_CAPACITY 1024
#define LLE_EVENT_HANDLER_INITIAL_CAPACITY 32

/*
 * Get current timestamp in microseconds
 */
uint64_t lle_event_get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
}

/*
 * Get event type name (for debugging)
 */
const char *lle_event_type_name(lle_event_kind_t type) {
    switch (type) {
        case LLE_EVENT_KEY_PRESS: return "KEY_PRESS";
        case LLE_EVENT_KEY_SEQUENCE: return "KEY_SEQUENCE";
        case LLE_EVENT_MOUSE_EVENT: return "MOUSE_EVENT";
        case LLE_EVENT_TERMINAL_RESIZE: return "TERMINAL_RESIZE";
        case LLE_EVENT_BUFFER_CHANGED: return "BUFFER_CHANGED";
        case LLE_EVENT_CURSOR_MOVED: return "CURSOR_MOVED";
        case LLE_EVENT_DISPLAY_UPDATE: return "DISPLAY_UPDATE";
        case LLE_EVENT_DISPLAY_REFRESH: return "DISPLAY_REFRESH";
        case LLE_EVENT_SYSTEM_ERROR: return "SYSTEM_ERROR";
        case LLE_EVENT_SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        default: return "UNKNOWN";
    }
}

/*
 * Initialize event system
 */
lle_result_t lle_event_system_init(lle_event_system_t **system,
                                   lle_memory_pool_t *pool) {
    if (!system || !pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate system structure */
    lle_event_system_t *sys = lle_pool_alloc(sizeof(lle_event_system_t));
    if (!sys) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(sys, 0, sizeof(lle_event_system_t));
    
    /* Initialize queue */
    lle_result_t result = lle_event_queue_init(&sys->queue,
                                               LLE_EVENT_QUEUE_DEFAULT_CAPACITY);
    if (result != LLE_SUCCESS) {
        lle_pool_free(sys);
        return result;
    }
    
    /* Initialize handler array */
    sys->handler_capacity = LLE_EVENT_HANDLER_INITIAL_CAPACITY;
    sys->handlers = lle_pool_alloc(sizeof(lle_event_handler_t*) * sys->handler_capacity);
    if (!sys->handlers) {
        lle_event_queue_destroy(sys->queue);
        lle_pool_free(sys);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    sys->handler_count = 0;
    sys->event_pool = pool;
    sys->sequence_counter = 1;  /* Start at 1 so first event gets sequence 1 */
    sys->active = true;  /* System is active after successful initialization */
    sys->events_created = 0;
    sys->events_dispatched = 0;
    sys->events_dropped = 0;
    
    /* Initialize mutex */
    pthread_mutex_init(&sys->system_mutex, NULL);
    
    *system = sys;
    return LLE_SUCCESS;
}

/*
 * Destroy event system
 */
void lle_event_system_destroy(lle_event_system_t *system) {
    if (!system) {
        return;
    }
    
    /* Stop system if active */
    if (system->active) {
        lle_event_system_stop(system);
    }
    
    /* Destroy queue */
    if (system->queue) {
        lle_event_queue_destroy(system->queue);
    }
    
    /* Free handlers */
    if (system->handlers) {
        for (size_t i = 0; i < system->handler_count; i++) {
            if (system->handlers[i]) {
                lle_pool_free(system->handlers[i]);
            }
        }
        lle_pool_free(system->handlers);
    }
    
    /* Destroy mutex */
    pthread_mutex_destroy(&system->system_mutex);
    
    /* Free system structure */
    lle_pool_free(system);
}

/*
 * Start event system
 */
lle_result_t lle_event_system_start(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    if (system->active) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_ALREADY_INITIALIZED;
    }
    
    system->active = true;
    
    pthread_mutex_unlock(&system->system_mutex);
    
    return LLE_SUCCESS;
}

/*
 * Stop event system
 */
lle_result_t lle_event_system_stop(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    if (!system->active) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_SUCCESS;
    }
    
    system->active = false;
    
    pthread_mutex_unlock(&system->system_mutex);
    
    return LLE_SUCCESS;
}

/*
 * Create event
 */
lle_result_t lle_event_create(lle_event_system_t *system,
                              lle_event_kind_t type,
                              void *data,
                              size_t data_size,
                              lle_event_t **event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate event structure */
    lle_event_t *evt = lle_pool_alloc(sizeof(lle_event_t));
    if (!evt) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(evt, 0, sizeof(lle_event_t));
    
    /* Allocate and copy data if provided */
    if (data && data_size > 0) {
        evt->data = lle_pool_alloc(data_size);
        if (!evt->data) {
            lle_pool_free(evt);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(evt->data, data, data_size);
        evt->data_size = data_size;
    }
    
    /* Set event fields */
    evt->type = type;
    evt->sequence_number = __atomic_fetch_add(&system->sequence_counter, 1, __ATOMIC_SEQ_CST);
    evt->timestamp = lle_event_get_timestamp_us();
    evt->next = NULL;
    
    /* Update statistics */
    __atomic_fetch_add(&system->events_created, 1, __ATOMIC_SEQ_CST);
    
    *event = evt;
    return LLE_SUCCESS;
}

/*
 * Destroy event
 */
void lle_event_destroy(lle_event_system_t *system, lle_event_t *event) {
    if (!event) {
        return;
    }
    
    /* Free event data */
    if (event->data) {
        lle_pool_free(event->data);
    }
    
    /* Free event structure */
    lle_pool_free(event);
    
    (void)system;  /* Unused for now */
}

/*
 * Clone event
 */
lle_result_t lle_event_clone(lle_event_system_t *system,
                             lle_event_t *source,
                             lle_event_t **dest) {
    if (!system || !source || !dest) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Create new event with same type and data */
    return lle_event_create(system, source->type, source->data, 
                           source->data_size, dest);
}

/*
 * Get event system statistics
 */
lle_result_t lle_event_system_get_stats(lle_event_system_t *system,
                                        uint64_t *created,
                                        uint64_t *dispatched,
                                        uint64_t *dropped) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (created) {
        *created = __atomic_load_n(&system->events_created, __ATOMIC_SEQ_CST);
    }
    if (dispatched) {
        *dispatched = __atomic_load_n(&system->events_dispatched, __ATOMIC_SEQ_CST);
    }
    if (dropped) {
        *dropped = __atomic_load_n(&system->events_dropped, __ATOMIC_SEQ_CST);
    }
    
    return LLE_SUCCESS;
}

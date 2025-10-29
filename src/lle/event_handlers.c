/*
 * event_handlers.c - Event Handler Management and Dispatching (Phase 1)
 * 
 * Handler registration, unregistration, and event dispatching.
 * 
 * Spec 04: Event System - Phase 1
 */

#include "../../include/lle/event_system.h"
#include <stdlib.h>
#include <string.h>

/*
 * Register event handler
 */
lle_result_t lle_event_handler_register(lle_event_system_t *system,
                                        lle_event_kind_t type,
                                        lle_event_handler_fn handler,
                                        void *user_data,
                                        const char *name) {
    if (!system || !handler || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    /* Check if we need to grow handler array */
    if (system->handler_count >= system->handler_capacity) {
        size_t new_capacity = system->handler_capacity * 2;
        lle_event_handler_t **new_handlers = lle_pool_alloc(
            sizeof(lle_event_handler_t*) * new_capacity);
        
        if (!new_handlers) {
            pthread_mutex_unlock(&system->system_mutex);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        
        /* Copy existing handlers */
        memcpy(new_handlers, system->handlers,
               sizeof(lle_event_handler_t*) * system->handler_count);
        
        /* Free old array */
        lle_pool_free(system->handlers);
        
        system->handlers = new_handlers;
        system->handler_capacity = new_capacity;
    }
    
    /* Allocate new handler */
    lle_event_handler_t *h = lle_pool_alloc(sizeof(lle_event_handler_t));
    if (!h) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize handler */
    h->event_type = type;
    h->handler = handler;
    h->user_data = user_data;
    strncpy(h->name, name, sizeof(h->name) - 1);
    h->name[sizeof(h->name) - 1] = '\0';
    
    /* Add to array */
    system->handlers[system->handler_count++] = h;
    
    pthread_mutex_unlock(&system->system_mutex);
    
    return LLE_SUCCESS;
}

/*
 * Unregister specific handler
 */
lle_result_t lle_event_handler_unregister(lle_event_system_t *system,
                                          lle_event_kind_t type,
                                          const char *name) {
    if (!system || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    /* Find and remove handler */
    for (size_t i = 0; i < system->handler_count; i++) {
        lle_event_handler_t *h = system->handlers[i];
        
        if (h->event_type == type && strcmp(h->name, name) == 0) {
            /* Free handler */
            lle_pool_free(h);
            
            /* Shift remaining handlers down */
            for (size_t j = i; j < system->handler_count - 1; j++) {
                system->handlers[j] = system->handlers[j + 1];
            }
            
            system->handler_count--;
            
            pthread_mutex_unlock(&system->system_mutex);
            return LLE_SUCCESS;
        }
    }
    
    pthread_mutex_unlock(&system->system_mutex);
    return LLE_ERROR_NOT_FOUND;
}

/*
 * Unregister all handlers for event type
 */
lle_result_t lle_event_handler_unregister_all(lle_event_system_t *system,
                                              lle_event_kind_t type) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    size_t removed = 0;
    
    /* Remove all handlers for this type */
    for (size_t i = 0; i < system->handler_count;) {
        lle_event_handler_t *h = system->handlers[i];
        
        if (h->event_type == type) {
            /* Free handler */
            lle_pool_free(h);
            
            /* Shift remaining handlers down */
            for (size_t j = i; j < system->handler_count - 1; j++) {
                system->handlers[j] = system->handlers[j + 1];
            }
            
            system->handler_count--;
            removed++;
        } else {
            i++;
        }
    }
    
    pthread_mutex_unlock(&system->system_mutex);
    
    return (removed > 0) ? LLE_SUCCESS : LLE_ERROR_NOT_FOUND;
}

/*
 * Get handler count for event type
 */
size_t lle_event_handler_count(lle_event_system_t *system,
                               lle_event_kind_t type) {
    if (!system) {
        return 0;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    size_t count = 0;
    for (size_t i = 0; i < system->handler_count; i++) {
        if (system->handlers[i]->event_type == type) {
            count++;
        }
    }
    
    pthread_mutex_unlock(&system->system_mutex);
    
    return count;
}

/*
 * Dispatch event to all registered handlers
 */
lle_result_t lle_event_dispatch(lle_event_system_t *system, lle_event_t *event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    /* Find all handlers for this event type */
    lle_result_t last_result = LLE_SUCCESS;
    size_t dispatched = 0;
    
    for (size_t i = 0; i < system->handler_count; i++) {
        lle_event_handler_t *h = system->handlers[i];
        
        if (h->event_type == event->type) {
            /* Call handler */
            lle_result_t result = h->handler(event, h->user_data);
            
            /* Track last error (but continue with other handlers) */
            if (result != LLE_SUCCESS) {
                last_result = result;
            }
            
            dispatched++;
        }
    }
    
    pthread_mutex_unlock(&system->system_mutex);
    
    /* Update statistics */
    if (dispatched > 0) {
        __atomic_fetch_add(&system->events_dispatched, 1, __ATOMIC_SEQ_CST);
    }
    
    return last_result;
}

/*
 * Process events from queue (up to max_events)
 */
lle_result_t lle_event_process_queue(lle_event_system_t *system,
                                     uint32_t max_events) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    if (!system->active) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    uint32_t processed = 0;
    lle_result_t last_result = LLE_SUCCESS;
    
    while (processed < max_events) {
        /* Dequeue event */
        lle_event_t *event = NULL;
        lle_result_t result = lle_event_dequeue(system, &event);
        
        if (result == LLE_ERROR_QUEUE_EMPTY) {
            /* No more events */
            break;
        }
        
        if (result != LLE_SUCCESS) {
            last_result = result;
            break;
        }
        
        /* Dispatch event */
        result = lle_event_dispatch(system, event);
        if (result != LLE_SUCCESS) {
            last_result = result;
        }
        
        /* Destroy event */
        lle_event_destroy(system, event);
        
        processed++;
    }
    
    return last_result;
}

/*
 * Process all events in queue
 */
lle_result_t lle_event_process_all(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Process until queue is empty */
    while (!lle_event_queue_empty(system)) {
        lle_result_t result = lle_event_process_queue(system, 100);
        if (result != LLE_SUCCESS && result != LLE_ERROR_QUEUE_EMPTY) {
            return result;
        }
    }
    
    return LLE_SUCCESS;
}

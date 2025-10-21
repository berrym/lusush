/**
 * @file event_handlers.c
 * @brief LLE Event Handler Registry Implementation
 * 
 * Manages registration and lookup of event handlers.
 * Phase 1: Simple array-based registry.
 * 
 * Spec: 04_event_system_complete.md - Phase 1
 */

#include "lle/event_system.h"
#include "lle/error_handling.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Find handler index by type and name
 * 
 * @return Index if found, -1 if not found
 */
static int find_handler_index(lle_event_system_t *system, 
                              lle_system_event_type_t type,
                              const char *name) {
    if (!system || !name) {
        return -1;
    }
    
    for (size_t i = 0; i < system->handler_count; i++) {
        if (system->handlers[i]->event_type == type &&
            strcmp(system->handlers[i]->name, name) == 0) {
            return (int)i;
        }
    }
    
    return -1;
}

/**
 * @brief Grow handler array if needed
 */
static lle_result_t grow_handler_array(lle_event_system_t *system) {
    if (system->handler_count < system->handler_capacity) {
        return LLE_SUCCESS; /* No growth needed */
    }
    
    /* Double capacity */
    size_t new_capacity = system->handler_capacity * 2;
    lle_event_handler_t **new_handlers = realloc(
        system->handlers,
        new_capacity * sizeof(lle_event_handler_t *)
    );
    
    if (!new_handlers) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    system->handlers = new_handlers;
    system->handler_capacity = new_capacity;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================
 */

lle_result_t lle_event_handler_register(lle_event_system_t *system,
                                         lle_system_event_type_t type,
                                         lle_event_handler_fn handler,
                                         void *user_data,
                                         const char *name) {
    if (!system || !handler) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    /* Check if handler with same name already exists */
    if (name && find_handler_index(system, type, name) >= 0) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_INVALID_STATE; /* Handler already registered */
    }
    
    /* Grow handler array if needed */
    lle_result_t result = grow_handler_array(system);
    if (result != LLE_SUCCESS) {
        pthread_mutex_unlock(&system->system_mutex);
        return result;
    }
    
    /* Allocate new handler entry */
    lle_event_handler_t *h = calloc(1, sizeof(lle_event_handler_t));
    if (!h) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Initialize handler */
    h->event_type = type;
    h->handler = handler;
    h->user_data = user_data;
    
    if (name) {
        strncpy(h->name, name, sizeof(h->name) - 1);
        h->name[sizeof(h->name) - 1] = '\0';
    } else {
        snprintf(h->name, sizeof(h->name), "handler_%zu", system->handler_count);
    }
    
    /* Add to registry */
    system->handlers[system->handler_count] = h;
    system->handler_count++;
    
    pthread_mutex_unlock(&system->system_mutex);
    return LLE_SUCCESS;
}

lle_result_t lle_event_handler_unregister(lle_event_system_t *system,
                                           lle_system_event_type_t type,
                                           const char *name) {
    if (!system || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    /* Find handler */
    int index = find_handler_index(system, type, name);
    if (index < 0) {
        pthread_mutex_unlock(&system->system_mutex);
        return LLE_ERROR_INVALID_STATE; /* Handler not found */
    }
    
    /* Free handler */
    free(system->handlers[index]);
    
    /* Shift remaining handlers down */
    for (size_t i = (size_t)index; i < system->handler_count - 1; i++) {
        system->handlers[i] = system->handlers[i + 1];
    }
    
    system->handler_count--;
    system->handlers[system->handler_count] = NULL;
    
    pthread_mutex_unlock(&system->system_mutex);
    return LLE_SUCCESS;
}

lle_result_t lle_event_handler_unregister_all(lle_event_system_t *system,
                                               lle_system_event_type_t type) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&system->system_mutex);
    
    /* Find and remove all handlers for this type */
    size_t removed = 0;
    size_t write_pos = 0;
    
    for (size_t read_pos = 0; read_pos < system->handler_count; read_pos++) {
        if (system->handlers[read_pos]->event_type == type) {
            /* Free this handler */
            free(system->handlers[read_pos]);
            removed++;
        } else {
            /* Keep this handler */
            if (write_pos != read_pos) {
                system->handlers[write_pos] = system->handlers[read_pos];
            }
            write_pos++;
        }
    }
    
    /* Update count and clear freed slots */
    system->handler_count = write_pos;
    for (size_t i = write_pos; i < write_pos + removed; i++) {
        system->handlers[i] = NULL;
    }
    
    pthread_mutex_unlock(&system->system_mutex);
    return LLE_SUCCESS;
}

size_t lle_event_handler_count(lle_event_system_t *system, lle_system_event_type_t type) {
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

/* ============================================================================
 * INTERNAL DISPATCH FUNCTION (called from event_system.c)
 * ============================================================================
 */

/**
 * @brief Dispatch event to all registered handlers (internal)
 * 
 * Called from lle_event_dispatch() with system mutex held.
 */
lle_result_t event_handlers_dispatch_internal(lle_event_system_t *system,
                                               lle_event_t *event) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_result_t first_error = LLE_SUCCESS;
    size_t handlers_called = 0;
    
    /* Call all handlers registered for this event type */
    for (size_t i = 0; i < system->handler_count; i++) {
        if (system->handlers[i]->event_type == event->type) {
            lle_result_t result = system->handlers[i]->handler(
                event,
                system->handlers[i]->user_data
            );
            
            handlers_called++;
            
            /* Record first error but continue calling other handlers */
            if (result != LLE_SUCCESS && first_error == LLE_SUCCESS) {
                first_error = result;
            }
        }
    }
    
    /* Return first error encountered, or SUCCESS if all succeeded */
    return first_error;
}

/*
 * event_filter.c - Event Filter System (Phase 2C)
 *
 * Callback-based event filtering, hooks, and system state tracking.
 *
 * Spec 04: Event System - Phase 2C
 */

#include "lle/event_system.h"
#include <stdlib.h>
#include <string.h>

#define LLE_FILTER_INITIAL_CAPACITY 8

/* ============================================================================
 * Phase 2C: Event Filter System Implementation
 * ============================================================================
 */

/*
 * Initialize event filter system
 */
lle_result_t lle_event_filter_system_init(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Already initialized */
    if (system->filter_system) {
        return LLE_SUCCESS;
    }

    /* Allocate filter system */
    lle_event_filter_system_t *filter_sys =
        lle_pool_alloc(sizeof(lle_event_filter_system_t));
    if (!filter_sys) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    memset(filter_sys, 0, sizeof(lle_event_filter_system_t));

    /* Allocate filter array */
    filter_sys->filter_capacity = LLE_FILTER_INITIAL_CAPACITY;
    filter_sys->filters = lle_pool_alloc(sizeof(lle_event_filter_t *) *
                                         filter_sys->filter_capacity);
    if (!filter_sys->filters) {
        lle_pool_free(filter_sys);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    memset(filter_sys->filters, 0,
           sizeof(lle_event_filter_t *) * filter_sys->filter_capacity);
    filter_sys->filter_count = 0;

    /* Initialize mutex */
    pthread_mutex_init(&filter_sys->filter_mutex, NULL);

    system->filter_system = filter_sys;
    return LLE_SUCCESS;
}

/*
 * Destroy event filter system
 */
void lle_event_filter_system_destroy(lle_event_system_t *system) {
    if (!system || !system->filter_system) {
        return;
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    /* Destroy mutex */
    pthread_mutex_destroy(&filter_sys->filter_mutex);

    /* Free all filters */
    if (filter_sys->filters) {
        for (size_t i = 0; i < filter_sys->filter_count; i++) {
            if (filter_sys->filters[i]) {
                lle_pool_free(filter_sys->filters[i]);
            }
        }
        lle_pool_free(filter_sys->filters);
    }

    /* Free filter system */
    lle_pool_free(filter_sys);

    system->filter_system = NULL;
}

/*
 * Add event filter
 */
lle_result_t lle_event_filter_add(lle_event_system_t *system, const char *name,
                                  lle_event_filter_fn filter, void *user_data) {
    if (!system || !name || !filter) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Initialize filter system if needed */
    if (!system->filter_system) {
        lle_result_t result = lle_event_filter_system_init(system);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    pthread_mutex_lock(&filter_sys->filter_mutex);

    /* Check if filter with this name already exists */
    for (size_t i = 0; i < filter_sys->filter_count; i++) {
        if (strcmp(filter_sys->filters[i]->name, name) == 0) {
            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_ERROR_ALREADY_INITIALIZED; /* Filter with this name
                                                     already exists */
        }
    }

    /* Check capacity */
    if (filter_sys->filter_count >= filter_sys->filter_capacity) {
        /* Need to grow array */
        size_t new_capacity = filter_sys->filter_capacity * 2;
        lle_event_filter_t **new_array =
            lle_pool_alloc(sizeof(lle_event_filter_t *) * new_capacity);
        if (!new_array) {
            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_ERROR_OUT_OF_MEMORY;
        }

        /* Copy existing filters */
        memcpy(new_array, filter_sys->filters,
               sizeof(lle_event_filter_t *) * filter_sys->filter_count);
        memset(new_array + filter_sys->filter_count, 0,
               sizeof(lle_event_filter_t *) *
                   (new_capacity - filter_sys->filter_count));

        /* Free old array and update */
        lle_pool_free(filter_sys->filters);
        filter_sys->filters = new_array;
        filter_sys->filter_capacity = new_capacity;
    }

    /* Allocate new filter */
    lle_event_filter_t *new_filter = lle_pool_alloc(sizeof(lle_event_filter_t));
    if (!new_filter) {
        pthread_mutex_unlock(&filter_sys->filter_mutex);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize filter */
    memset(new_filter, 0, sizeof(lle_event_filter_t));
    new_filter->filter = filter;
    new_filter->user_data = user_data;
    new_filter->enabled = true;
    strncpy(new_filter->name, name, sizeof(new_filter->name) - 1);
    new_filter->name[sizeof(new_filter->name) - 1] = '\0';

    /* Add to array */
    filter_sys->filters[filter_sys->filter_count] = new_filter;
    filter_sys->filter_count++;

    pthread_mutex_unlock(&filter_sys->filter_mutex);

    return LLE_SUCCESS;
}

/*
 * Remove event filter by name
 */
lle_result_t lle_event_filter_remove(lle_event_system_t *system,
                                     const char *name) {
    if (!system || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!system->filter_system) {
        return LLE_ERROR_NOT_FOUND;
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    pthread_mutex_lock(&filter_sys->filter_mutex);

    /* Find filter */
    for (size_t i = 0; i < filter_sys->filter_count; i++) {
        if (strcmp(filter_sys->filters[i]->name, name) == 0) {
            /* Free filter */
            lle_pool_free(filter_sys->filters[i]);

            /* Shift remaining filters down */
            for (size_t j = i; j < filter_sys->filter_count - 1; j++) {
                filter_sys->filters[j] = filter_sys->filters[j + 1];
            }

            filter_sys->filters[filter_sys->filter_count - 1] = NULL;
            filter_sys->filter_count--;

            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_SUCCESS;
        }
    }

    pthread_mutex_unlock(&filter_sys->filter_mutex);
    return LLE_ERROR_NOT_FOUND;
}

/*
 * Enable event filter by name
 */
lle_result_t lle_event_filter_enable(lle_event_system_t *system,
                                     const char *name) {
    if (!system || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!system->filter_system) {
        return LLE_ERROR_NOT_FOUND;
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    pthread_mutex_lock(&filter_sys->filter_mutex);

    /* Find and enable filter */
    for (size_t i = 0; i < filter_sys->filter_count; i++) {
        if (strcmp(filter_sys->filters[i]->name, name) == 0) {
            filter_sys->filters[i]->enabled = true;
            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_SUCCESS;
        }
    }

    pthread_mutex_unlock(&filter_sys->filter_mutex);
    return LLE_ERROR_NOT_FOUND;
}

/*
 * Disable event filter by name
 */
lle_result_t lle_event_filter_disable(lle_event_system_t *system,
                                      const char *name) {
    if (!system || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!system->filter_system) {
        return LLE_ERROR_NOT_FOUND;
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    pthread_mutex_lock(&filter_sys->filter_mutex);

    /* Find and disable filter */
    for (size_t i = 0; i < filter_sys->filter_count; i++) {
        if (strcmp(filter_sys->filters[i]->name, name) == 0) {
            filter_sys->filters[i]->enabled = false;
            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_SUCCESS;
        }
    }

    pthread_mutex_unlock(&filter_sys->filter_mutex);
    return LLE_ERROR_NOT_FOUND;
}

/*
 * Apply filters to event (internal helper)
 */
lle_filter_result_t lle_event_filter_apply(lle_event_system_t *system,
                                           lle_event_t *event) {
    if (!system || !event) {
        return LLE_FILTER_ERROR;
    }

    if (!system->filter_system) {
        return LLE_FILTER_PASS; /* No filters, pass through */
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    pthread_mutex_lock(&filter_sys->filter_mutex);

    filter_sys->total_events_filtered++;

    /* Apply each enabled filter in sequence */
    for (size_t i = 0; i < filter_sys->filter_count; i++) {
        lle_event_filter_t *filter = filter_sys->filters[i];

        if (!filter->enabled) {
            continue;
        }

        /* Call filter function */
        lle_filter_result_t result = filter->filter(event, filter->user_data);

        /* Update filter statistics */
        filter->events_filtered++;

        switch (result) {
        case LLE_FILTER_PASS:
            filter->events_passed++;
            break;

        case LLE_FILTER_BLOCK:
            filter->events_blocked++;
            filter_sys->total_events_blocked++;
            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_FILTER_BLOCK; /* Stop processing, block event */

        case LLE_FILTER_TRANSFORM:
            filter->events_transformed++;
            /* Continue to next filter with transformed event */
            break;

        case LLE_FILTER_ERROR:
            filter->events_errored++;
            /* Continue to next filter on error (don't block) */
            break;
        }
    }

    pthread_mutex_unlock(&filter_sys->filter_mutex);

    return LLE_FILTER_PASS; /* All filters passed or transformed */
}

/*
 * Get filter statistics
 */
lle_result_t lle_event_filter_get_stats(lle_event_system_t *system,
                                        const char *name, uint64_t *filtered,
                                        uint64_t *passed, uint64_t *blocked,
                                        uint64_t *transformed,
                                        uint64_t *errored) {
    if (!system || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (!system->filter_system) {
        return LLE_ERROR_NOT_FOUND;
    }

    lle_event_filter_system_t *filter_sys = system->filter_system;

    pthread_mutex_lock(&filter_sys->filter_mutex);

    /* Find filter */
    for (size_t i = 0; i < filter_sys->filter_count; i++) {
        if (strcmp(filter_sys->filters[i]->name, name) == 0) {
            lle_event_filter_t *filter = filter_sys->filters[i];

            if (filtered)
                *filtered = filter->events_filtered;
            if (passed)
                *passed = filter->events_passed;
            if (blocked)
                *blocked = filter->events_blocked;
            if (transformed)
                *transformed = filter->events_transformed;
            if (errored)
                *errored = filter->events_errored;

            pthread_mutex_unlock(&filter_sys->filter_mutex);
            return LLE_SUCCESS;
        }
    }

    pthread_mutex_unlock(&filter_sys->filter_mutex);
    return LLE_ERROR_NOT_FOUND;
}

/* ============================================================================
 * Phase 2C: Dispatch Hooks Implementation
 * ============================================================================
 */

/*
 * Set pre-dispatch hook
 */
lle_result_t lle_event_set_pre_dispatch_hook(lle_event_system_t *system,
                                             lle_event_pre_dispatch_fn hook,
                                             void *user_data) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&system->system_mutex);
    system->pre_dispatch_hook = hook;
    system->pre_dispatch_data = user_data;
    pthread_mutex_unlock(&system->system_mutex);

    return LLE_SUCCESS;
}

/*
 * Set post-dispatch hook
 */
lle_result_t lle_event_set_post_dispatch_hook(lle_event_system_t *system,
                                              lle_event_post_dispatch_fn hook,
                                              void *user_data) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&system->system_mutex);
    system->post_dispatch_hook = hook;
    system->post_dispatch_data = user_data;
    pthread_mutex_unlock(&system->system_mutex);

    return LLE_SUCCESS;
}

/* ============================================================================
 * Phase 2C: System State Implementation
 * ============================================================================
 */

/*
 * Set system state
 */
lle_result_t lle_event_system_set_state(lle_event_system_t *system,
                                        lle_system_state_t state) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&system->system_mutex);

    system->previous_state = system->current_state;
    system->current_state = state;
    system->state_changed_time = lle_event_get_timestamp_us();

    pthread_mutex_unlock(&system->system_mutex);

    return LLE_SUCCESS;
}

/*
 * Get current system state
 */
lle_system_state_t lle_event_system_get_state(lle_event_system_t *system) {
    if (!system) {
        return LLE_STATE_ERROR;
    }

    pthread_mutex_lock(&system->system_mutex);
    lle_system_state_t state = system->current_state;
    pthread_mutex_unlock(&system->system_mutex);

    return state;
}

/*
 * Get previous system state
 */
lle_system_state_t
lle_event_system_get_previous_state(lle_event_system_t *system) {
    if (!system) {
        return LLE_STATE_ERROR;
    }

    pthread_mutex_lock(&system->system_mutex);
    lle_system_state_t state = system->previous_state;
    pthread_mutex_unlock(&system->system_mutex);

    return state;
}

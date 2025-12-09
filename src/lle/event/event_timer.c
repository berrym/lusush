/*
 * event_timer.c - Timer Event Implementation (Phase 2D)
 *
 * Implements timer event support for the LLE event system.
 *
 * Features:
 * - One-shot timers (fire once)
 * - Repeating timers (fire at intervals)
 * - Timer enable/disable (without destroying)
 * - Timer statistics
 * - Thread-safe operations
 *
 * Design:
 * - Simple array-based storage (optimal for small N)
 * - Sorted by trigger_time_us for efficient processing
 * - Timers own their events (deep copy on creation)
 * - Manual processing via lle_event_timer_process()
 */

#include "lle/event_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Initial capacity for timer array */
#define TIMER_INITIAL_CAPACITY 16

/* Helper function to find timer by ID (must be called with mutex held) */
static int find_timer_index(lle_timer_system_t *ts, uint64_t timer_id) {
    for (size_t i = 0; i < ts->timer_count; i++) {
        if (ts->timers[i]->timer_id == timer_id) {
            return (int)i;
        }
    }
    return -1;
}

/* Helper function to insert timer in sorted position (by trigger_time_us) */
static lle_result_t insert_timer_sorted(lle_timer_system_t *ts,
                                        lle_timer_event_t *timer) {
    /* Grow array if needed */
    if (ts->timer_count >= ts->timer_capacity) {
        size_t new_capacity = ts->timer_capacity * 2;
        lle_timer_event_t **new_array =
            realloc(ts->timers, new_capacity * sizeof(lle_timer_event_t *));
        if (!new_array) {
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        ts->timers = new_array;
        ts->timer_capacity = new_capacity;
    }

    /* Find insertion point (keep sorted by trigger_time_us) */
    size_t insert_pos = ts->timer_count;
    for (size_t i = 0; i < ts->timer_count; i++) {
        if (timer->trigger_time_us < ts->timers[i]->trigger_time_us) {
            insert_pos = i;
            break;
        }
    }

    /* Shift elements to make room */
    if (insert_pos < ts->timer_count) {
        memmove(&ts->timers[insert_pos + 1], &ts->timers[insert_pos],
                (ts->timer_count - insert_pos) * sizeof(lle_timer_event_t *));
    }

    /* Insert timer */
    ts->timers[insert_pos] = timer;
    ts->timer_count++;

    return LLE_SUCCESS;
}

/* Helper function to remove timer at index (must be called with mutex held) */
static void remove_timer_at_index(lle_timer_system_t *ts, size_t index) {
    if (index >= ts->timer_count) {
        return;
    }

    /* Free the timer and its event */
    if (ts->timers[index]->event) {
        /* Note: We need the event system to properly destroy the event,
         * but we don't have it here. For now, just free the event data. */
        if (ts->timers[index]->event->data) {
            free(ts->timers[index]->event->data);
        }
        free(ts->timers[index]->event);
    }
    free(ts->timers[index]);

    /* Shift remaining timers down */
    if (index < ts->timer_count - 1) {
        memmove(&ts->timers[index], &ts->timers[index + 1],
                (ts->timer_count - index - 1) * sizeof(lle_timer_event_t *));
    }

    ts->timer_count--;
}

/* Initialize timer system */
lle_result_t lle_event_timer_system_init(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Already initialized? */
    if (system->timer_system) {
        return LLE_ERROR_ALREADY_INITIALIZED;
    }

    /* Allocate timer system */
    lle_timer_system_t *ts = calloc(1, sizeof(lle_timer_system_t));
    if (!ts) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Allocate timer array */
    ts->timers = calloc(TIMER_INITIAL_CAPACITY, sizeof(lle_timer_event_t *));
    if (!ts->timers) {
        free(ts);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    ts->timer_count = 0;
    ts->timer_capacity = TIMER_INITIAL_CAPACITY;
    ts->next_timer_id = 1;
    ts->total_timers_created = 0;
    ts->total_timers_fired = 0;
    ts->total_timers_cancelled = 0;

    /* Initialize mutex */
    if (pthread_mutex_init(&ts->timer_mutex, NULL) != 0) {
        free(ts->timers);
        free(ts);
        return LLE_ERROR_SYSTEM_CALL;
    }

    system->timer_system = ts;
    return LLE_SUCCESS;
}

/* Destroy timer system */
void lle_event_timer_system_destroy(lle_event_system_t *system) {
    if (!system || !system->timer_system) {
        return;
    }

    lle_timer_system_t *ts = system->timer_system;

    /* Destroy all timers */
    pthread_mutex_lock(&ts->timer_mutex);
    for (size_t i = 0; i < ts->timer_count; i++) {
        if (ts->timers[i]->event) {
            if (ts->timers[i]->event->data) {
                free(ts->timers[i]->event->data);
            }
            free(ts->timers[i]->event);
        }
        free(ts->timers[i]);
    }
    free(ts->timers);
    pthread_mutex_unlock(&ts->timer_mutex);

    /* Destroy mutex */
    pthread_mutex_destroy(&ts->timer_mutex);

    /* Free timer system */
    free(ts);
    system->timer_system = NULL;
}

/* Create a one-shot timer */
lle_result_t lle_event_timer_add_oneshot(lle_event_system_t *system,
                                         lle_event_t *event, uint64_t delay_us,
                                         uint64_t *timer_id_out) {
    if (!system || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Initialize timer system if needed */
    if (!system->timer_system) {
        lle_result_t result = lle_event_timer_system_init(system);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }

    lle_timer_system_t *ts = system->timer_system;

    /* Allocate timer */
    lle_timer_event_t *timer = calloc(1, sizeof(lle_timer_event_t));
    if (!timer) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Clone the event (deep copy) */
    timer->event = calloc(1, sizeof(lle_event_t));
    if (!timer->event) {
        free(timer);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Copy event structure */
    memcpy(timer->event, event, sizeof(lle_event_t));

    /* Deep copy event data if present */
    if (event->data && event->data_size > 0) {
        timer->event->data = malloc(event->data_size);
        if (!timer->event->data) {
            free(timer->event);
            free(timer);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(timer->event->data, event->data, event->data_size);
    } else {
        timer->event->data = NULL;
    }

    /* Reset queue linkage (timer event is not in queue) */
    timer->event->next = NULL;
    timer->event->prev = NULL;

    /* Set up timer */
    pthread_mutex_lock(&ts->timer_mutex);

    timer->timer_id = ts->next_timer_id++;
    timer->trigger_time_us = lle_event_get_timestamp_us() + delay_us;
    timer->interval_us = 0;
    timer->repeating = false;
    timer->enabled = true;
    timer->fire_count = 0;

    /* Insert in sorted order */
    lle_result_t result = insert_timer_sorted(ts, timer);
    if (result != LLE_SUCCESS) {
        pthread_mutex_unlock(&ts->timer_mutex);
        if (timer->event->data) {
            free(timer->event->data);
        }
        free(timer->event);
        free(timer);
        return result;
    }

    ts->total_timers_created++;

    if (timer_id_out) {
        *timer_id_out = timer->timer_id;
    }

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Create a repeating timer */
lle_result_t lle_event_timer_add_repeating(lle_event_system_t *system,
                                           lle_event_t *event,
                                           uint64_t initial_delay_us,
                                           uint64_t interval_us,
                                           uint64_t *timer_id_out) {
    if (!system || !event || interval_us == 0) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Initialize timer system if needed */
    if (!system->timer_system) {
        lle_result_t result = lle_event_timer_system_init(system);
        if (result != LLE_SUCCESS) {
            return result;
        }
    }

    lle_timer_system_t *ts = system->timer_system;

    /* Allocate timer */
    lle_timer_event_t *timer = calloc(1, sizeof(lle_timer_event_t));
    if (!timer) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Clone the event (deep copy) */
    timer->event = calloc(1, sizeof(lle_event_t));
    if (!timer->event) {
        free(timer);
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Copy event structure */
    memcpy(timer->event, event, sizeof(lle_event_t));

    /* Deep copy event data if present */
    if (event->data && event->data_size > 0) {
        timer->event->data = malloc(event->data_size);
        if (!timer->event->data) {
            free(timer->event);
            free(timer);
            return LLE_ERROR_OUT_OF_MEMORY;
        }
        memcpy(timer->event->data, event->data, event->data_size);
    } else {
        timer->event->data = NULL;
    }

    /* Reset queue linkage */
    timer->event->next = NULL;
    timer->event->prev = NULL;

    /* Set up timer */
    pthread_mutex_lock(&ts->timer_mutex);

    timer->timer_id = ts->next_timer_id++;
    timer->trigger_time_us = lle_event_get_timestamp_us() + initial_delay_us;
    timer->interval_us = interval_us;
    timer->repeating = true;
    timer->enabled = true;
    timer->fire_count = 0;

    /* Insert in sorted order */
    lle_result_t result = insert_timer_sorted(ts, timer);
    if (result != LLE_SUCCESS) {
        pthread_mutex_unlock(&ts->timer_mutex);
        if (timer->event->data) {
            free(timer->event->data);
        }
        free(timer->event);
        free(timer);
        return result;
    }

    ts->total_timers_created++;

    if (timer_id_out) {
        *timer_id_out = timer->timer_id;
    }

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Cancel a timer */
lle_result_t lle_event_timer_cancel(lle_event_system_t *system,
                                    uint64_t timer_id) {
    if (!system || !system->timer_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_timer_system_t *ts = system->timer_system;

    pthread_mutex_lock(&ts->timer_mutex);

    int index = find_timer_index(ts, timer_id);
    if (index < 0) {
        pthread_mutex_unlock(&ts->timer_mutex);
        return LLE_ERROR_NOT_FOUND;
    }

    remove_timer_at_index(ts, (size_t)index);
    ts->total_timers_cancelled++;

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Enable a timer */
lle_result_t lle_event_timer_enable(lle_event_system_t *system,
                                    uint64_t timer_id) {
    if (!system || !system->timer_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_timer_system_t *ts = system->timer_system;

    pthread_mutex_lock(&ts->timer_mutex);

    int index = find_timer_index(ts, timer_id);
    if (index < 0) {
        pthread_mutex_unlock(&ts->timer_mutex);
        return LLE_ERROR_NOT_FOUND;
    }

    ts->timers[index]->enabled = true;

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Disable a timer */
lle_result_t lle_event_timer_disable(lle_event_system_t *system,
                                     uint64_t timer_id) {
    if (!system || !system->timer_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_timer_system_t *ts = system->timer_system;

    pthread_mutex_lock(&ts->timer_mutex);

    int index = find_timer_index(ts, timer_id);
    if (index < 0) {
        pthread_mutex_unlock(&ts->timer_mutex);
        return LLE_ERROR_NOT_FOUND;
    }

    ts->timers[index]->enabled = false;

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Get timer information */
lle_result_t lle_event_timer_get_info(lle_event_system_t *system,
                                      uint64_t timer_id,
                                      uint64_t *next_fire_time_us,
                                      uint64_t *fire_count,
                                      bool *is_repeating) {
    if (!system || !system->timer_system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_timer_system_t *ts = system->timer_system;

    pthread_mutex_lock(&ts->timer_mutex);

    int index = find_timer_index(ts, timer_id);
    if (index < 0) {
        pthread_mutex_unlock(&ts->timer_mutex);
        return LLE_ERROR_NOT_FOUND;
    }

    lle_timer_event_t *timer = ts->timers[index];

    if (next_fire_time_us) {
        *next_fire_time_us = timer->trigger_time_us;
    }
    if (fire_count) {
        *fire_count = timer->fire_count;
    }
    if (is_repeating) {
        *is_repeating = timer->repeating;
    }

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Process all timers that are ready to fire */
lle_result_t lle_event_timer_process(lle_event_system_t *system) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* No timer system? Nothing to do */
    if (!system->timer_system) {
        return LLE_SUCCESS;
    }

    lle_timer_system_t *ts = system->timer_system;
    uint64_t current_time = lle_event_get_timestamp_us();

    pthread_mutex_lock(&ts->timer_mutex);

    /* Process timers in order (array is sorted by trigger_time_us) */
    size_t i = 0;
    while (i < ts->timer_count) {
        lle_timer_event_t *timer = ts->timers[i];

        /* If this timer is not ready, none of the rest are either */
        if (timer->trigger_time_us > current_time) {
            break;
        }

        /* Skip disabled timers */
        if (!timer->enabled) {
            i++;
            continue;
        }

        /* Clone the event for dispatch */
        lle_event_t *event_to_dispatch = calloc(1, sizeof(lle_event_t));
        if (event_to_dispatch) {
            memcpy(event_to_dispatch, timer->event, sizeof(lle_event_t));

            /* Clone event data if present */
            if (timer->event->data && timer->event->data_size > 0) {
                event_to_dispatch->data = malloc(timer->event->data_size);
                if (event_to_dispatch->data) {
                    memcpy(event_to_dispatch->data, timer->event->data,
                           timer->event->data_size);
                } else {
                    /* Failed to allocate data, just use NULL */
                    event_to_dispatch->data = NULL;
                    event_to_dispatch->data_size = 0;
                }
            } else {
                event_to_dispatch->data = NULL;
            }

            /* Reset queue linkage */
            event_to_dispatch->next = NULL;
            event_to_dispatch->prev = NULL;

            /* Update timer */
            timer->fire_count++;
            ts->total_timers_fired++;

            /* Unlock mutex before dispatching (avoid deadlock) */
            pthread_mutex_unlock(&ts->timer_mutex);

            /* Dispatch the event */
            lle_event_dispatch(system, event_to_dispatch);

            /* Free the dispatched event (dispatch doesn't take ownership) */
            if (event_to_dispatch->data) {
                free(event_to_dispatch->data);
            }
            free(event_to_dispatch);

            /* Re-lock mutex */
            pthread_mutex_lock(&ts->timer_mutex);

            /* Timer array may have changed during dispatch - find timer again
             */
            int index = find_timer_index(ts, timer->timer_id);
            if (index < 0) {
                /* Timer was cancelled during dispatch */
                continue;
            }

            /* Update i to reflect current position */
            i = (size_t)index;
            timer = ts->timers[i];
        }

        /* Handle repeating vs one-shot */
        if (timer->repeating) {
            /* Update trigger time */
            timer->trigger_time_us += timer->interval_us;

            /* Remove from current position */
            lle_timer_event_t *timer_copy = timer;
            if (i < ts->timer_count - 1) {
                memmove(&ts->timers[i], &ts->timers[i + 1],
                        (ts->timer_count - i - 1) *
                            sizeof(lle_timer_event_t *));
            }
            ts->timer_count--;

            /* Re-insert in sorted position */
            insert_timer_sorted(ts, timer_copy);

            /* Don't increment i - we removed this element */
        } else {
            /* One-shot timer - remove it */
            remove_timer_at_index(ts, i);
            /* Don't increment i - we removed this element */
        }
    }

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

/* Get timer system statistics */
lle_result_t lle_event_timer_get_stats(lle_event_system_t *system,
                                       uint64_t *created, uint64_t *fired,
                                       uint64_t *cancelled) {
    if (!system) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* No timer system? Return zeros */
    if (!system->timer_system) {
        if (created)
            *created = 0;
        if (fired)
            *fired = 0;
        if (cancelled)
            *cancelled = 0;
        return LLE_SUCCESS;
    }

    lle_timer_system_t *ts = system->timer_system;

    pthread_mutex_lock(&ts->timer_mutex);

    if (created) {
        *created = ts->total_timers_created;
    }
    if (fired) {
        *fired = ts->total_timers_fired;
    }
    if (cancelled) {
        *cancelled = ts->total_timers_cancelled;
    }

    pthread_mutex_unlock(&ts->timer_mutex);
    return LLE_SUCCESS;
}

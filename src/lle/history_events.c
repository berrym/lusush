/**
 * @file history_events.c
 * @brief LLE History System - Event System Integration
 * 
 * Specification: Spec 09 - History System (Phase 2 Day 6)
 * Version: 1.0.0
 * Status: Phase 2 Day 6 - Event System Integration
 * 
 * Integrates the LLE history system with the Spec 04 event system,
 * providing real-time event notification for history operations:
 * - Entry added
 * - Entry accessed
 * - Entry deleted
 * - History loaded
 * - History saved
 * - History searched
 * 
 * This enables other LLE components to react to history changes
 * and provides hooks for performance monitoring and logging.
 */

#include "lle/history.h"
#include "lle/event_system.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/performance.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* ============================================================================
 * HISTORY EVENT DATA STRUCTURES
 * ============================================================================ */

/**
 * History entry event data
 * Emitted when an entry is added, accessed, or deleted
 */
typedef struct {
    uint64_t entry_id;                  /* Entry ID */
    const char *command;                /* Command text (read-only) */
    size_t command_length;              /* Command length */
    int exit_code;                      /* Exit code */
    uint64_t timestamp;                 /* Entry timestamp */
} lle_history_entry_event_data_t;

/**
 * History file operation event data
 * Emitted when history is loaded or saved
 */
typedef struct {
    const char *file_path;              /* File path */
    size_t entry_count;                 /* Number of entries affected */
    bool success;                       /* Operation success */
    uint64_t duration_us;               /* Operation duration */
} lle_history_file_event_data_t;

/**
 * History search event data
 * Emitted when a search is performed
 */
typedef struct {
    const char *search_query;           /* Search query */
    size_t result_count;                /* Number of results */
    uint64_t duration_us;               /* Search duration */
} lle_history_search_event_data_t;

/* ============================================================================
 * EVENT SYSTEM STATE
 * ============================================================================ */

/**
 * History event system state
 */
typedef struct {
    lle_event_system_t *event_system;   /* Event system reference */
    lle_history_core_t *history_core;   /* History core reference */
    
    /* Event emission statistics */
    uint64_t events_emitted;            /* Total events emitted */
    uint64_t entry_added_events;        /* Entry added events */
    uint64_t entry_accessed_events;     /* Entry accessed events */
    uint64_t entry_deleted_events;      /* Entry deleted events */
    uint64_t history_loaded_events;     /* History loaded events */
    uint64_t history_saved_events;      /* History saved events */
    uint64_t history_searched_events;   /* History searched events */
    
    /* Configuration */
    bool events_enabled;                /* Event emission enabled */
    bool emit_access_events;            /* Emit access events (can be noisy) */
    
    bool initialized;                   /* Initialization flag */
} lle_history_event_state_t;

/* Global event state */
static lle_history_event_state_t *g_event_state = NULL;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * Get current timestamp in microseconds
 */
static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/* ============================================================================
 * INITIALIZATION AND LIFECYCLE
 * ============================================================================ */

/**
 * Initialize history event integration
 */
lle_result_t lle_history_events_init(
    lle_event_system_t *event_system,
    lle_history_core_t *history_core
) {
    if (!event_system || !history_core) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check if already initialized */
    if (g_event_state && g_event_state->initialized) {
        return LLE_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Allocate event state */
    g_event_state = (lle_history_event_state_t *)lle_pool_alloc(sizeof(lle_history_event_state_t));
    if (!g_event_state) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    memset(g_event_state, 0, sizeof(lle_history_event_state_t));
    
    /* Initialize state */
    g_event_state->event_system = event_system;
    g_event_state->history_core = history_core;
    g_event_state->events_enabled = true;
    g_event_state->emit_access_events = false;  /* Disabled by default (can be noisy) */
    g_event_state->initialized = true;
    
    return LLE_SUCCESS;
}

/**
 * Shutdown history event integration
 */
lle_result_t lle_history_events_shutdown(void) {
    if (!g_event_state) {
        return LLE_SUCCESS;
    }
    
    /* Free event state */
    lle_pool_free(g_event_state);
    g_event_state = NULL;
    
    return LLE_SUCCESS;
}

/**
 * Check if event system is initialized
 */
bool lle_history_events_is_initialized(void) {
    return (g_event_state && g_event_state->initialized);
}

/* ============================================================================
 * EVENT EMISSION FUNCTIONS
 * ============================================================================ */

/**
 * Emit entry added event
 */
lle_result_t lle_history_emit_entry_added(
    uint64_t entry_id,
    const char *command,
    int exit_code
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_event_state->events_enabled) {
        return LLE_SUCCESS;  /* Events disabled, not an error */
    }
    
    if (!command) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Create event data */
    lle_history_entry_event_data_t *event_data = 
        (lle_history_entry_event_data_t *)lle_pool_alloc(sizeof(lle_history_entry_event_data_t));
    if (!event_data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    event_data->entry_id = entry_id;
    event_data->command = command;  /* Read-only reference */
    event_data->command_length = strlen(command);
    event_data->exit_code = exit_code;
    event_data->timestamp = get_timestamp_us();
    
    /* Create and emit event */
    lle_event_t *event = NULL;
    lle_result_t result = lle_event_create(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_CHANGED,
        event_data,
        sizeof(lle_history_entry_event_data_t),
        &event
    );
    
    if (result != LLE_SUCCESS) {
        lle_pool_free(event_data);
        return result;
    }
    
    /* Set event metadata */
    event->source = LLE_EVENT_SOURCE_HISTORY;
    event->priority = LLE_PRIORITY_LOW;
    
    /* Dispatch event */
    result = lle_event_dispatch(g_event_state->event_system, event);
    
    /* Update statistics */
    if (result == LLE_SUCCESS) {
        g_event_state->events_emitted++;
        g_event_state->entry_added_events++;
    }
    
    return result;
}

/**
 * Emit entry accessed event
 */
lle_result_t lle_history_emit_entry_accessed(
    uint64_t entry_id,
    const char *command
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_event_state->events_enabled || !g_event_state->emit_access_events) {
        return LLE_SUCCESS;  /* Access events disabled */
    }
    
    if (!command) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Create event data */
    lle_history_entry_event_data_t *event_data = 
        (lle_history_entry_event_data_t *)lle_pool_alloc(sizeof(lle_history_entry_event_data_t));
    if (!event_data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    event_data->entry_id = entry_id;
    event_data->command = command;
    event_data->command_length = strlen(command);
    event_data->exit_code = -1;  /* Not applicable for access */
    event_data->timestamp = get_timestamp_us();
    
    /* Create and emit event */
    lle_event_t *event = NULL;
    lle_result_t result = lle_event_create(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_NAVIGATE,
        event_data,
        sizeof(lle_history_entry_event_data_t),
        &event
    );
    
    if (result != LLE_SUCCESS) {
        lle_pool_free(event_data);
        return result;
    }
    
    event->source = LLE_EVENT_SOURCE_HISTORY;
    event->priority = LLE_PRIORITY_LOW;
    
    result = lle_event_dispatch(g_event_state->event_system, event);
    
    if (result == LLE_SUCCESS) {
        g_event_state->events_emitted++;
        g_event_state->entry_accessed_events++;
    }
    
    return result;
}

/**
 * Emit history loaded event
 */
lle_result_t lle_history_emit_history_loaded(
    const char *file_path,
    size_t entry_count,
    uint64_t duration_us,
    bool success
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_event_state->events_enabled) {
        return LLE_SUCCESS;
    }
    
    /* Create event data */
    lle_history_file_event_data_t *event_data = 
        (lle_history_file_event_data_t *)lle_pool_alloc(sizeof(lle_history_file_event_data_t));
    if (!event_data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    event_data->file_path = file_path;  /* Read-only reference */
    event_data->entry_count = entry_count;
    event_data->success = success;
    event_data->duration_us = duration_us;
    
    /* Create and emit event */
    lle_event_t *event = NULL;
    lle_result_t result = lle_event_create(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_CHANGED,
        event_data,
        sizeof(lle_history_file_event_data_t),
        &event
    );
    
    if (result != LLE_SUCCESS) {
        lle_pool_free(event_data);
        return result;
    }
    
    event->source = LLE_EVENT_SOURCE_HISTORY;
    event->priority = LLE_PRIORITY_LOW;
    
    result = lle_event_dispatch(g_event_state->event_system, event);
    
    if (result == LLE_SUCCESS) {
        g_event_state->events_emitted++;
        g_event_state->history_loaded_events++;
    }
    
    return result;
}

/**
 * Emit history saved event
 */
lle_result_t lle_history_emit_history_saved(
    const char *file_path,
    size_t entry_count,
    uint64_t duration_us,
    bool success
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_event_state->events_enabled) {
        return LLE_SUCCESS;
    }
    
    /* Create event data */
    lle_history_file_event_data_t *event_data = 
        (lle_history_file_event_data_t *)lle_pool_alloc(sizeof(lle_history_file_event_data_t));
    if (!event_data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    event_data->file_path = file_path;
    event_data->entry_count = entry_count;
    event_data->success = success;
    event_data->duration_us = duration_us;
    
    /* Create and emit event */
    lle_event_t *event = NULL;
    lle_result_t result = lle_event_create(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_CHANGED,
        event_data,
        sizeof(lle_history_file_event_data_t),
        &event
    );
    
    if (result != LLE_SUCCESS) {
        lle_pool_free(event_data);
        return result;
    }
    
    event->source = LLE_EVENT_SOURCE_HISTORY;
    event->priority = LLE_PRIORITY_LOW;
    
    result = lle_event_dispatch(g_event_state->event_system, event);
    
    if (result == LLE_SUCCESS) {
        g_event_state->events_emitted++;
        g_event_state->history_saved_events++;
    }
    
    return result;
}

/**
 * Emit history search event
 */
lle_result_t lle_history_emit_history_search(
    const char *search_query,
    size_t result_count,
    uint64_t duration_us
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!g_event_state->events_enabled) {
        return LLE_SUCCESS;
    }
    
    /* Create event data */
    lle_history_search_event_data_t *event_data = 
        (lle_history_search_event_data_t *)lle_pool_alloc(sizeof(lle_history_search_event_data_t));
    if (!event_data) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    event_data->search_query = search_query;
    event_data->result_count = result_count;
    event_data->duration_us = duration_us;
    
    /* Create and emit event */
    lle_event_t *event = NULL;
    lle_result_t result = lle_event_create(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_SEARCH,
        event_data,
        sizeof(lle_history_search_event_data_t),
        &event
    );
    
    if (result != LLE_SUCCESS) {
        lle_pool_free(event_data);
        return result;
    }
    
    event->source = LLE_EVENT_SOURCE_HISTORY;
    event->priority = LLE_PRIORITY_LOW;
    
    result = lle_event_dispatch(g_event_state->event_system, event);
    
    if (result == LLE_SUCCESS) {
        g_event_state->events_emitted++;
        g_event_state->history_searched_events++;
    }
    
    return result;
}

/* ============================================================================
 * EVENT HANDLER REGISTRATION
 * ============================================================================ */

/**
 * Register handler for history change events
 */
lle_result_t lle_history_register_change_handler(
    lle_event_handler_fn handler,
    void *user_data,
    const char *handler_name
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!handler) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    return lle_event_handler_register(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_CHANGED,
        handler,
        user_data,
        handler_name ? handler_name : "history_change_handler"
    );
}

/**
 * Register handler for history navigation events
 */
lle_result_t lle_history_register_navigate_handler(
    lle_event_handler_fn handler,
    void *user_data,
    const char *handler_name
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!handler) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    return lle_event_handler_register(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_NAVIGATE,
        handler,
        user_data,
        handler_name ? handler_name : "history_navigate_handler"
    );
}

/**
 * Register handler for history search events
 */
lle_result_t lle_history_register_search_handler(
    lle_event_handler_fn handler,
    void *user_data,
    const char *handler_name
) {
    if (!g_event_state || !g_event_state->initialized) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (!handler) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    return lle_event_handler_register(
        g_event_state->event_system,
        LLE_EVENT_HISTORY_SEARCH,
        handler,
        user_data,
        handler_name ? handler_name : "history_search_handler"
    );
}

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

/**
 * Enable or disable event emission
 */
lle_result_t lle_history_events_set_enabled(bool enabled) {
    if (!g_event_state) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    g_event_state->events_enabled = enabled;
    return LLE_SUCCESS;
}

/**
 * Enable or disable access event emission
 */
lle_result_t lle_history_events_set_emit_access(bool enabled) {
    if (!g_event_state) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    g_event_state->emit_access_events = enabled;
    return LLE_SUCCESS;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

/**
 * Get event emission statistics
 */
lle_result_t lle_history_events_get_stats(
    uint64_t *total_events,
    uint64_t *entry_added,
    uint64_t *entry_accessed,
    uint64_t *history_loaded,
    uint64_t *history_saved,
    uint64_t *history_searched
) {
    if (!g_event_state) {
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    if (total_events) *total_events = g_event_state->events_emitted;
    if (entry_added) *entry_added = g_event_state->entry_added_events;
    if (entry_accessed) *entry_accessed = g_event_state->entry_accessed_events;
    if (history_loaded) *history_loaded = g_event_state->history_loaded_events;
    if (history_saved) *history_saved = g_event_state->history_saved_events;
    if (history_searched) *history_searched = g_event_state->history_searched_events;
    
    return LLE_SUCCESS;
}

/**
 * Print event statistics
 */
lle_result_t lle_history_events_print_stats(void) {
    if (!g_event_state) {
        printf("History Events: Not initialized\n");
        return LLE_ERROR_NOT_INITIALIZED;
    }
    
    printf("=== History Event Statistics ===\n");
    printf("Events enabled: %s\n", g_event_state->events_enabled ? "Yes" : "No");
    printf("Access events: %s\n", g_event_state->emit_access_events ? "Yes" : "No");
    printf("\n");
    printf("Total events emitted: %llu\n", (unsigned long long)g_event_state->events_emitted);
    printf("  Entry added: %llu\n", (unsigned long long)g_event_state->entry_added_events);
    printf("  Entry accessed: %llu\n", (unsigned long long)g_event_state->entry_accessed_events);
    printf("  History loaded: %llu\n", (unsigned long long)g_event_state->history_loaded_events);
    printf("  History saved: %llu\n", (unsigned long long)g_event_state->history_saved_events);
    printf("  History searched: %llu\n", (unsigned long long)g_event_state->history_searched_events);
    
    return LLE_SUCCESS;
}

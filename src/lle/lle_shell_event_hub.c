/**
 * @file lle_shell_event_hub.c
 * @brief LLE Shell Event Hub - Implementation
 *
 * Implements the shell event hub for coordinating shell lifecycle events.
 * Routes events to registered handlers and propagates to LLE event system.
 *
 * Specification: docs/lle_specification/26_initialization_system_complete.md
 * Date: 2025-01-16
 */

#include "lle/lle_shell_event_hub.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief Check if event debug logging is enabled
 *
 * Set LUSUSH_EVENT_DEBUG=1 to enable verbose event system logging.
 * Logs event firing, handler dispatch, and silent failure conditions.
 */
static inline bool lle_event_debug_enabled(void) {
    static int cached = -1;
    if (cached == -1) {
        const char *env = getenv("LUSUSH_EVENT_DEBUG");
        cached = (env && env[0] == '1') ? 1 : 0;
    }
    return cached == 1;
}

/* Include shell integration header for g_lle_integration access.
 * The convenience fire functions need access to the global integration
 * to get the event hub. When linking unit tests without shell integration,
 * a weak fallback symbol is provided below. */
#include "lle/lle_shell_integration.h"

/* Weak fallback for g_lle_integration when shell integration is not linked.
 * This allows liblle.a to be used in unit tests without linking
 * lle_shell_integration.c (which has shell dependencies). */
__attribute__((weak)) lle_shell_integration_t *g_lle_integration = NULL;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

uint64_t lle_shell_event_get_timestamp_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
    }
    return 0;
}

const char *lle_shell_event_type_name(lle_shell_event_type_t event_type) {
    switch (event_type) {
    case LLE_SHELL_EVENT_DIRECTORY_CHANGED:
        return "DIRECTORY_CHANGED";
    case LLE_SHELL_EVENT_PRE_COMMAND:
        return "PRE_COMMAND";
    case LLE_SHELL_EVENT_POST_COMMAND:
        return "POST_COMMAND";
    default:
        return "UNKNOWN";
    }
}

/* ============================================================================
 * LIFECYCLE FUNCTIONS
 * ============================================================================
 */

lle_result_t lle_shell_event_hub_create(lle_shell_event_hub_t **hub_out) {
    if (!hub_out) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    lle_shell_event_hub_t *hub = calloc(1, sizeof(lle_shell_event_hub_t));
    if (!hub) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize handler counts */
    for (int i = 0; i < LLE_SHELL_EVENT_TYPE_COUNT; i++) {
        hub->handler_counts[i] = 0;
    }

    /* Capture current working directory */
    if (getcwd(hub->current_dir, sizeof(hub->current_dir)) == NULL) {
        hub->current_dir[0] = '\0';
    }

    hub->command_start_time_us = 0;
    hub->current_command[0] = '\0';
    hub->initialized = true;
    hub->events_fired = 0;
    hub->directory_changes = 0;
    hub->commands_executed = 0;

    *hub_out = hub;
    return LLE_SUCCESS;
}

void lle_shell_event_hub_destroy(lle_shell_event_hub_t *hub) {
    if (!hub) {
        return;
    }

    hub->initialized = false;
    free(hub);
}

/* ============================================================================
 * HANDLER REGISTRATION
 * ============================================================================
 */

lle_result_t lle_shell_event_hub_register(lle_shell_event_hub_t *hub,
                                          lle_shell_event_type_t event_type,
                                          lle_shell_event_handler_t handler,
                                          void *user_data, const char *name) {

    if (!hub || !handler) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (event_type >= LLE_SHELL_EVENT_TYPE_COUNT) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t count = hub->handler_counts[event_type];
    if (count >= LLE_SHELL_EVENT_MAX_HANDLERS) {
        return LLE_ERROR_RESOURCE_EXHAUSTED;
    }

    /* Add handler at next available slot */
    hub->handlers[event_type][count].handler = handler;
    hub->handlers[event_type][count].user_data = user_data;
    hub->handlers[event_type][count].name = name;
    hub->handler_counts[event_type]++;

    return LLE_SUCCESS;
}

lle_result_t lle_shell_event_hub_unregister(lle_shell_event_hub_t *hub,
                                            lle_shell_event_type_t event_type,
                                            const char *name) {

    if (!hub || !name) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    if (event_type >= LLE_SHELL_EVENT_TYPE_COUNT) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    size_t count = hub->handler_counts[event_type];
    for (size_t i = 0; i < count; i++) {
        if (hub->handlers[event_type][i].name &&
            strcmp(hub->handlers[event_type][i].name, name) == 0) {
            /* Found - shift remaining handlers down */
            for (size_t j = i; j < count - 1; j++) {
                hub->handlers[event_type][j] = hub->handlers[event_type][j + 1];
            }
            hub->handler_counts[event_type]--;
            return LLE_SUCCESS;
        }
    }

    return LLE_ERROR_NOT_FOUND;
}

/* ============================================================================
 * EVENT FIRING
 * ============================================================================
 */

void lle_shell_event_hub_fire(lle_shell_event_hub_t *hub,
                              lle_shell_event_type_t event_type,
                              void *event_data) {

    if (!hub || !hub->initialized) {
        if (lle_event_debug_enabled()) {
            fprintf(stderr,
                    "[LLE_EVENT] WARN: lle_shell_event_hub_fire: "
                    "hub=%p initialized=%d\n",
                    (void *)hub, hub ? hub->initialized : 0);
        }
        return;
    }

    if (event_type >= LLE_SHELL_EVENT_TYPE_COUNT) {
        if (lle_event_debug_enabled()) {
            fprintf(stderr,
                    "[LLE_EVENT] WARN: lle_shell_event_hub_fire: "
                    "invalid event_type=%d (max=%d)\n",
                    event_type, LLE_SHELL_EVENT_TYPE_COUNT);
        }
        return;
    }

    /* Update statistics */
    hub->events_fired++;

    /* Call all registered handlers */
    size_t count = hub->handler_counts[event_type];
    if (lle_event_debug_enabled()) {
        fprintf(stderr, "[LLE_EVENT] Firing %s to %zu handlers\n",
                lle_shell_event_type_name(event_type), count);
    }

    for (size_t i = 0; i < count; i++) {
        lle_shell_event_handler_t handler =
            hub->handlers[event_type][i].handler;
        void *user_data = hub->handlers[event_type][i].user_data;
        if (handler) {
            if (lle_event_debug_enabled()) {
                fprintf(stderr, "[LLE_EVENT]   -> handler[%zu]: %s\n", i,
                        hub->handlers[event_type][i].name
                            ? hub->handlers[event_type][i].name
                            : "(unnamed)");
            }
            handler(event_data, user_data);
        }
    }
}

/* ============================================================================
 * CONVENIENCE EVENT FIRING FUNCTIONS
 * ============================================================================
 */

void lle_fire_directory_changed(const char *old_dir, const char *new_dir) {
    /* Get global shell integration */
    if (!g_lle_integration || !g_lle_integration->event_hub) {
        if (lle_event_debug_enabled()) {
            fprintf(stderr,
                    "[LLE_EVENT] WARN: lle_fire_directory_changed: "
                    "g_lle_integration=%p event_hub=%p\n",
                    (void *)g_lle_integration,
                    g_lle_integration ? (void *)g_lle_integration->event_hub
                                      : NULL);
        }
        return;
    }

    lle_shell_event_hub_t *hub = g_lle_integration->event_hub;
    char new_dir_buf[PATH_MAX];

    /* Use provided old_dir or hub's tracked current_dir */
    const char *actual_old_dir = old_dir ? old_dir : hub->current_dir;

    /* Get new directory if not provided */
    const char *actual_new_dir = new_dir;
    if (!actual_new_dir) {
        if (getcwd(new_dir_buf, sizeof(new_dir_buf)) != NULL) {
            actual_new_dir = new_dir_buf;
        } else {
            if (lle_event_debug_enabled()) {
                fprintf(stderr,
                        "[LLE_EVENT] WARN: lle_fire_directory_changed: "
                        "getcwd() failed: %s\n",
                        strerror(errno));
            }
            return; /* Can't determine new directory */
        }
    }

    /* Create event data */
    lle_directory_changed_event_t event = {.old_dir = actual_old_dir,
                                           .new_dir = actual_new_dir};

    /* Update hub's current directory tracking */
    strncpy(hub->current_dir, actual_new_dir, sizeof(hub->current_dir) - 1);
    hub->current_dir[sizeof(hub->current_dir) - 1] = '\0';

    /* Update statistics */
    hub->directory_changes++;

    /* Fire to registered handlers */
    lle_shell_event_hub_fire(hub, LLE_SHELL_EVENT_DIRECTORY_CHANGED, &event);
}

void lle_fire_pre_command(const char *command, bool is_background) {
    /* Get global shell integration */
    if (!g_lle_integration || !g_lle_integration->event_hub) {
        if (lle_event_debug_enabled()) {
            fprintf(stderr,
                    "[LLE_EVENT] WARN: lle_fire_pre_command: "
                    "g_lle_integration=%p event_hub=%p\n",
                    (void *)g_lle_integration,
                    g_lle_integration ? (void *)g_lle_integration->event_hub
                                      : NULL);
        }
        return;
    }

    if (lle_event_debug_enabled()) {
        fprintf(stderr,
                "[LLE_EVENT] lle_fire_pre_command: cmd='%.40s%s' bg=%d\n",
                command ? command : "(null)",
                (command && strlen(command) > 40) ? "..." : "", is_background);
    }

    lle_shell_event_hub_t *hub = g_lle_integration->event_hub;

    /* Record command start time */
    hub->command_start_time_us = lle_shell_event_get_timestamp_us();

    /* Store command for post-command event */
    if (command) {
        strncpy(hub->current_command, command,
                sizeof(hub->current_command) - 1);
        hub->current_command[sizeof(hub->current_command) - 1] = '\0';
    } else {
        hub->current_command[0] = '\0';
    }

    /* Create event data */
    lle_pre_command_event_t event = {.command = command,
                                     .is_background = is_background};

    /* Fire to registered handlers */
    lle_shell_event_hub_fire(hub, LLE_SHELL_EVENT_PRE_COMMAND, &event);
}

void lle_fire_post_command(const char *command, int exit_code,
                           uint64_t duration_us) {
    /* Get global shell integration */
    if (!g_lle_integration || !g_lle_integration->event_hub) {
        if (lle_event_debug_enabled()) {
            fprintf(stderr,
                    "[LLE_EVENT] WARN: lle_fire_post_command: "
                    "g_lle_integration=%p event_hub=%p exit=%d\n",
                    (void *)g_lle_integration,
                    g_lle_integration ? (void *)g_lle_integration->event_hub
                                      : NULL,
                    exit_code);
        }
        return;
    }

    if (lle_event_debug_enabled()) {
        fprintf(stderr,
                "[LLE_EVENT] lle_fire_post_command: exit=%d duration=%lluus\n",
                exit_code, (unsigned long long)duration_us);
    }

    lle_shell_event_hub_t *hub = g_lle_integration->event_hub;

    /* Calculate duration if not provided */
    uint64_t actual_duration = duration_us;
    if (actual_duration == 0 && hub->command_start_time_us > 0) {
        uint64_t end_time = lle_shell_event_get_timestamp_us();
        actual_duration = end_time - hub->command_start_time_us;
    }

    /* Use stored command if not provided */
    const char *actual_command = command ? command : hub->current_command;

    /* Create event data */
    lle_post_command_event_t event = {.command = actual_command,
                                      .exit_code = exit_code,
                                      .duration_us = actual_duration};

    /* Update statistics */
    hub->commands_executed++;

    /* Reset command tracking */
    hub->command_start_time_us = 0;
    hub->current_command[0] = '\0';

    /* Fire to registered handlers */
    lle_shell_event_hub_fire(hub, LLE_SHELL_EVENT_POST_COMMAND, &event);
}

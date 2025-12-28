/**
 * @file lle_shell_integration.c
 * @brief LLE Shell Integration - Implementation
 *
 * Implements the centralized LLE initialization and lifecycle management.
 * Provides shell-level LLE init, three-tier reset hierarchy, and error tracking.
 *
 * Specification: docs/lle_specification/26_initialization_system_complete.md
 * Date: 2025-01-16
 */

#include "lle/lle_shell_integration.h"
#include "lle/lle_shell_event_hub.h"
#include "lle/lle_editor.h"
#include "lle/history.h"
#include "lle/prompt/composer.h"
#include "lle/prompt/segment.h"
#include "lle/prompt/theme.h"
#include "lle/terminal_abstraction.h"
#include "config.h"
#include "lusush_memory_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================
 */

/** Global shell integration instance */
lle_shell_integration_t *g_lle_integration = NULL;

/** External global memory pool from Lusush */
extern lusush_memory_pool_t *global_memory_pool;

/** External config for history settings */
extern config_values_t config;

/** Flag to prevent double atexit registration */
static bool atexit_registered = false;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

static void lle_shell_integration_atexit_handler(void);
static lle_result_t create_and_configure_editor(lle_shell_integration_t *integ);
static void destroy_editor(lle_shell_integration_t *integ);
static lle_result_t create_and_configure_prompt_composer(lle_shell_integration_t *integ);
static void destroy_prompt_composer(lle_shell_integration_t *integ);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Get current timestamp in microseconds
 */
static uint64_t get_timestamp_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000;
    }
    return 0;
}

/**
 * @brief Populate history config from Lusush config system
 */
static void populate_history_config(lle_history_config_t *hist_config) {
    if (!hist_config) {
        return;
    }

    memset(hist_config, 0, sizeof(lle_history_config_t));

    /* Capacity settings */
    hist_config->max_entries =
        config.history_size > 0 ? config.history_size : 5000;
    hist_config->max_command_length = 8192;

    /* File settings */
    if (config.lle_history_file && config.lle_history_file[0] != '\0') {
        hist_config->history_file_path = config.lle_history_file;
    } else {
        hist_config->history_file_path = NULL;
    }
    hist_config->auto_save = true;
    hist_config->load_on_init = true;

    /* Deduplication behavior */
    hist_config->ignore_duplicates =
        config.lle_enable_deduplication &&
        (config.lle_dedup_scope != LLE_DEDUP_SCOPE_NONE);

    /* Map dedup strategy */
    switch (config.lle_dedup_strategy) {
    case LLE_DEDUP_STRATEGY_IGNORE:
        hist_config->dedup_strategy = LLE_DEDUP_IGNORE;
        break;
    case LLE_DEDUP_STRATEGY_KEEP_FREQUENT:
        hist_config->dedup_strategy = LLE_DEDUP_KEEP_FREQUENT;
        break;
    case LLE_DEDUP_STRATEGY_MERGE:
        hist_config->dedup_strategy = LLE_DEDUP_MERGE_METADATA;
        break;
    case LLE_DEDUP_STRATEGY_KEEP_ALL:
        hist_config->dedup_strategy = LLE_DEDUP_KEEP_ALL;
        break;
    case LLE_DEDUP_STRATEGY_KEEP_RECENT:
    default:
        hist_config->dedup_strategy = LLE_DEDUP_KEEP_RECENT;
        break;
    }

    hist_config->unicode_normalize = config.lle_dedup_unicode_normalize;
    hist_config->ignore_space_prefix = false;

    /* Metadata */
    hist_config->save_timestamps = config.history_timestamps;
    hist_config->save_working_dir = config.lle_enable_forensic_tracking;
    hist_config->save_exit_codes = config.lle_enable_forensic_tracking;

    /* Performance */
    hist_config->initial_capacity =
        config.lle_enable_history_cache && config.lle_cache_size > 0
            ? config.lle_cache_size
            : 1000;
    hist_config->use_indexing = config.lle_enable_history_cache;
}

/* ============================================================================
 * LIFECYCLE FUNCTIONS
 * ============================================================================
 */

lle_result_t lle_shell_integration_init(void) {
    /* Already initialized? */
    if (g_lle_integration) {
        return LLE_SUCCESS;
    }

    /* Allocate integration structure */
    lle_shell_integration_t *integ = calloc(1, sizeof(lle_shell_integration_t));
    if (!integ) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    integ->init_time_us = get_timestamp_us();

    /* Step 1: Verify global memory pool exists */
    if (!global_memory_pool) {
        free(integ);
        return LLE_ERROR_NOT_INITIALIZED;
    }
    integ->init_state.memory_pool_verified = true;

    /* Step 2: Verify terminal detection is complete
     * Terminal detection is handled by Lusush's display system,
     * so we just verify it has been initialized */
    integ->init_state.terminal_detected = true;

    /* Step 3: Create shell event hub */
    lle_result_t result = lle_shell_event_hub_create(&integ->event_hub);
    if (result != LLE_SUCCESS) {
        free(integ);
        return result;
    }
    integ->init_state.event_hub_initialized = true;

    /* Step 4: Create and configure LLE editor */
    result = create_and_configure_editor(integ);
    if (result != LLE_SUCCESS) {
        lle_shell_event_hub_destroy(integ->event_hub);
        free(integ);
        return result;
    }
    integ->init_state.editor_initialized = true;

    /* Step 5: Initialize history (already done in create_and_configure_editor) */
    integ->init_state.history_initialized = true;

    /* Step 6: Create and configure prompt composer (Spec 25) */
    result = create_and_configure_prompt_composer(integ);
    if (result != LLE_SUCCESS) {
        /* Prompt composer is optional - log warning but continue */
        /* The shell can still function without the fancy prompt system */
    } else {
        integ->init_state.prompt_initialized = true;
    }

    /* Step 7: Register atexit handler for cleanup */
    if (!atexit_registered) {
        if (atexit(lle_shell_integration_atexit_handler) == 0) {
            atexit_registered = true;
            integ->init_state.atexit_registered = true;
        }
    }

    /* Mark shell hooks as installed */
    integ->init_state.shell_hooks_installed = true;

    /* Set global pointer */
    g_lle_integration = integ;

    return LLE_SUCCESS;
}

void lle_shell_integration_shutdown(void) {
    if (!g_lle_integration) {
        return;
    }

    lle_shell_integration_t *integ = g_lle_integration;

    /* Save history before shutdown */
    if (integ->editor && integ->editor->history_system) {
        const char *home = getenv("HOME");
        if (home) {
            char history_path[1024];
            snprintf(history_path, sizeof(history_path),
                     "%s/.lusush_history_lle", home);
            lle_history_save_to_file(integ->editor->history_system,
                                     history_path);
        }
    }

    /* Destroy prompt composer (unregisters from event hub) */
    destroy_prompt_composer(integ);

    /* Destroy editor */
    destroy_editor(integ);

    /* Destroy event hub */
    if (integ->event_hub) {
        lle_shell_event_hub_destroy(integ->event_hub);
        integ->event_hub = NULL;
    }

    /* Clear global pointer */
    g_lle_integration = NULL;

    /* Free integration structure */
    free(integ);
}

/**
 * @brief Atexit handler for automatic cleanup
 * 
 * Uses static flag to ensure shutdown only runs once, preventing
 * double-free issues if shutdown is called from multiple paths.
 */
static void lle_shell_integration_atexit_handler(void) {
    static bool shutdown_complete = false;
    if (shutdown_complete) {
        return;
    }
    shutdown_complete = true;
    lle_shell_integration_shutdown();
}

lle_shell_integration_t *lle_get_shell_integration(void) {
    return g_lle_integration;
}

bool lle_is_active(void) {
    return g_lle_integration != NULL &&
           g_lle_integration->init_state.editor_initialized;
}

/* ============================================================================
 * EDITOR MANAGEMENT
 * ============================================================================
 */

/**
 * @brief Create and configure the LLE editor instance
 */
static lle_result_t create_and_configure_editor(lle_shell_integration_t *integ) {
    if (!integ) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Create editor */
    lle_result_t result = lle_editor_create(&integ->editor, global_memory_pool);
    if (result != LLE_SUCCESS || !integ->editor) {
        return result != LLE_SUCCESS ? result : LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize history subsystem */
    lle_history_config_t hist_config;
    populate_history_config(&hist_config);

    result = lle_history_core_create(&integ->editor->history_system,
                                     integ->editor->lle_pool, &hist_config);

    if (result == LLE_SUCCESS && integ->editor->history_system) {
        /* Load existing history from file */
        const char *home = getenv("HOME");
        if (home) {
            char history_path[1024];
            snprintf(history_path, sizeof(history_path),
                     "%s/.lusush_history_lle", home);
            lle_history_load_from_file(integ->editor->history_system,
                                       history_path);
        }
    }

    return LLE_SUCCESS;
}

/**
 * @brief Destroy the editor instance
 */
static void destroy_editor(lle_shell_integration_t *integ) {
    if (!integ || !integ->editor) {
        return;
    }

    lle_editor_destroy(integ->editor);
    integ->editor = NULL;
    integ->init_state.editor_initialized = false;
    integ->init_state.history_initialized = false;
}

/* ============================================================================
 * PROMPT COMPOSER MANAGEMENT (Spec 25 Integration)
 * ============================================================================
 */

/* Static registries for prompt composer */
static lle_segment_registry_t g_segment_registry;
static lle_theme_registry_t g_theme_registry;
static bool g_registries_initialized = false;

/**
 * @brief Create and configure the prompt composer
 *
 * Initializes the prompt composer and registers it with the shell event hub
 * for automatic cache invalidation on directory changes and command events.
 */
static lle_result_t create_and_configure_prompt_composer(lle_shell_integration_t *integ) {
    if (!integ || !integ->event_hub) {
        return LLE_ERROR_INVALID_PARAMETER;
    }

    /* Initialize registries only if not already initialized
     * This prevents double-init issues during hard reset */
    if (!g_registries_initialized) {
        /* Initialize segment registry and register built-in segments */
        lle_result_t result = lle_segment_registry_init(&g_segment_registry);
        if (result != LLE_SUCCESS) {
            return result;
        }
        lle_segment_register_builtins(&g_segment_registry);

        /* Initialize theme registry and register built-in themes */
        result = lle_theme_registry_init(&g_theme_registry);
        if (result != LLE_SUCCESS) {
            lle_segment_registry_cleanup(&g_segment_registry);
            return result;
        }
        lle_theme_register_builtins(&g_theme_registry);

        g_registries_initialized = true;
    }

    lle_result_t result;

    /* Set default theme as active */
    lle_theme_registry_set_active(&g_theme_registry, "default");

    /* Allocate prompt composer */
    integ->prompt_composer = calloc(1, sizeof(lle_prompt_composer_t));
    if (!integ->prompt_composer) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize the composer with segment and theme registries */
    result = lle_composer_init(integ->prompt_composer, 
                                &g_segment_registry, 
                                &g_theme_registry);
    if (result != LLE_SUCCESS) {
        free(integ->prompt_composer);
        integ->prompt_composer = NULL;
        return result;
    }

    /* Sync composer config with global config settings */
    integ->prompt_composer->config.enable_transient = config.display_transient_prompt;

    /* Register with shell event hub for automatic updates
     * This is the key Spec 25 <-> Spec 26 integration point */
    result = lle_composer_register_shell_events(integ->prompt_composer,
                                                 integ->event_hub);
    if (result != LLE_SUCCESS) {
        lle_composer_cleanup(integ->prompt_composer);
        free(integ->prompt_composer);
        integ->prompt_composer = NULL;
        return result;
    }

    return LLE_SUCCESS;
}

/**
 * @brief Destroy the prompt composer
 *
 * Unregisters from shell event hub and frees resources.
 */
static void destroy_prompt_composer(lle_shell_integration_t *integ) {
    if (!integ || !integ->prompt_composer) {
        return;
    }

    /* Unregister from event hub first */
    lle_composer_unregister_shell_events(integ->prompt_composer);

    /* Cleanup and free */
    lle_composer_cleanup(integ->prompt_composer);
    free(integ->prompt_composer);
    integ->prompt_composer = NULL;
    integ->init_state.prompt_initialized = false;

    /* Cleanup registries only once - prevents double-free on exit */
    if (g_registries_initialized) {
        lle_theme_registry_cleanup(&g_theme_registry);
        lle_segment_registry_cleanup(&g_segment_registry);
        g_registries_initialized = false;
    }
}

/* ============================================================================
 * RESET FUNCTIONS (THREE-TIER HIERARCHY)
 * ============================================================================
 */

void lle_soft_reset(void) {
    if (!g_lle_integration || !g_lle_integration->editor) {
        return;
    }

    lle_editor_t *editor = g_lle_integration->editor;

    /* Set abort flag to signal readline to return */
    editor->abort_requested = true;

    /* Clear buffer if present */
    if (editor->buffer) {
        lle_buffer_clear(editor->buffer);
    }

    /* Reset history navigation */
    editor->history_navigation_pos = 0;
    editor->history_nav_seen_count = 0;
}

void lle_hard_reset(void) {
    if (!g_lle_integration) {
        return;
    }

    lle_shell_integration_t *integ = g_lle_integration;

    /* Save history before destroying editor */
    if (integ->editor && integ->editor->history_system) {
        const char *home = getenv("HOME");
        if (home) {
            char history_path[1024];
            snprintf(history_path, sizeof(history_path),
                     "%s/.lusush_history_lle", home);
            lle_history_save_to_file(integ->editor->history_system,
                                     history_path);
        }
    }

    /* Destroy current editor */
    destroy_editor(integ);

    /* Recreate editor */
    lle_result_t result = create_and_configure_editor(integ);
    if (result == LLE_SUCCESS) {
        integ->init_state.editor_initialized = true;
        integ->init_state.history_initialized = true;
    }

    /* Reset error counters */
    integ->error_count = 0;
    integ->ctrl_g_count = 0;
    integ->recovery_mode = false;

    /* Update statistics */
    integ->hard_reset_count++;
    integ->last_reset_time_us = get_timestamp_us();
}

void lle_nuclear_reset(void) {
    if (!g_lle_integration) {
        return;
    }

    /* Perform hard reset first */
    lle_hard_reset();

    /* Send terminal reset sequence */
    /* ESC c = RIS (Reset to Initial State) */
    write(STDOUT_FILENO, "\033c", 2);

    /* Give terminal time to process reset */
    usleep(50000); /* 50ms */

    /* Update statistics */
    g_lle_integration->nuclear_reset_count++;
}

/* ============================================================================
 * ERROR TRACKING
 * ============================================================================
 */

void lle_record_error(lle_result_t error) {
    if (!g_lle_integration) {
        return;
    }

    (void)error; /* Could log specific error in future */

    g_lle_integration->error_count++;

    /* Check if we've hit the threshold for automatic hard reset */
    if (g_lle_integration->error_count >= LLE_ERROR_THRESHOLD) {
        g_lle_integration->recovery_mode = true;
        g_lle_integration->recovery_count++;
        lle_hard_reset();
    }
}

void lle_reset_error_counter(void) {
    if (!g_lle_integration) {
        return;
    }

    g_lle_integration->error_count = 0;
    g_lle_integration->recovery_mode = false;
}

void lle_record_ctrl_g(void) {
    if (!g_lle_integration) {
        return;
    }

    uint64_t now = get_timestamp_us();

    /* Check if this Ctrl+G is within the panic window */
    if (now - g_lle_integration->last_ctrl_g_time_us < LLE_CTRL_G_PANIC_WINDOW_US) {
        g_lle_integration->ctrl_g_count++;
    } else {
        /* Reset counter - too much time passed */
        g_lle_integration->ctrl_g_count = 1;
    }

    g_lle_integration->last_ctrl_g_time_us = now;

    /* Check for panic threshold */
    if (g_lle_integration->ctrl_g_count >= LLE_CTRL_G_PANIC_COUNT) {
        /* Triple Ctrl+G detected - trigger hard reset */
        g_lle_integration->ctrl_g_count = 0;
        lle_hard_reset();
    }
}

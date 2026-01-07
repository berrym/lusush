/**
 * @file lle_shell_hooks.c
 * @brief LLE Shell Hooks - User-facing hook function bridge implementation
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 *
 * Bridges the LLE Shell Event Hub with user-defined shell functions.
 * When shell events fire (pre_command, post_command, directory_changed),
 * this module calls the corresponding user-defined hook functions
 * (preexec, precmd, chpwd) if they are defined.
 */

#include "lle/lle_shell_hooks.h"
#include "lle/lle_shell_event_hub.h"
#include "lle/lle_shell_integration.h"
#include "executor.h"
#include "lusush.h"
#include "shell_mode.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ============================================================================
 * STATIC STATE
 * ============================================================================ */

/** Whether hooks are initialized */
static bool g_hooks_initialized = false;

/** Current hook being executed (for recursion detection) */
static lle_hook_type_t g_current_hook = LLE_HOOK_COUNT;

/** Hook call statistics */
static size_t g_hook_call_counts[LLE_HOOK_COUNT] = {0};

/** Hook function names */
static const char *g_hook_names[LLE_HOOK_COUNT] = {
    [LLE_HOOK_PRECMD] = "precmd",
    [LLE_HOOK_PREEXEC] = "preexec",
    [LLE_HOOK_CHPWD] = "chpwd",
};

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * @brief Call a hook function via the executor
 *
 * @param hook_type Type of hook to call
 * @param arg Optional argument (command string for preexec)
 * @return Exit status of hook, or 0 if not defined
 */
static int call_hook_function(lle_hook_type_t hook_type, const char *arg) {
    if (hook_type >= LLE_HOOK_COUNT) {
        return 0;
    }

    // Get the global executor
    executor_t *executor = get_global_executor();
    if (!executor) {
        return 0;
    }

    const char *hook_name = g_hook_names[hook_type];
    if (!hook_name) {
        return 0;
    }

    // Use executor's hook calling mechanism (handles recursion guard internally)
    int result = executor_call_hook(executor, hook_name, arg);

    // Update statistics
    g_hook_call_counts[hook_type]++;

    return result;
}

/* ============================================================================
 * EVENT HANDLERS
 * ============================================================================ */

/**
 * @brief Handler for POST_COMMAND events -> precmd hook
 *
 * Called after a command completes, before the next prompt is displayed.
 */
static void hook_precmd_handler(void *event_data, void *user_data) {
    (void)event_data;
    (void)user_data;

    // Check feature flag
    if (!shell_mode_allows(FEATURE_HOOK_FUNCTIONS)) {
        return;
    }

    // Prevent recursive calls
    if (executor_in_hook()) {
        return;
    }

    g_current_hook = LLE_HOOK_PRECMD;
    call_hook_function(LLE_HOOK_PRECMD, NULL);
    g_current_hook = LLE_HOOK_COUNT;
}

/**
 * @brief Handler for PRE_COMMAND events -> preexec hook
 *
 * Called before a command is executed. The command string is passed as $1.
 */
static void hook_preexec_handler(void *event_data, void *user_data) {
    (void)user_data;

    // Check feature flag
    if (!shell_mode_allows(FEATURE_HOOK_FUNCTIONS)) {
        return;
    }

    // Prevent recursive calls
    if (executor_in_hook()) {
        return;
    }

    lle_pre_command_event_t *event = (lle_pre_command_event_t *)event_data;
    const char *command = event ? event->command : NULL;

    g_current_hook = LLE_HOOK_PREEXEC;
    call_hook_function(LLE_HOOK_PREEXEC, command);
    g_current_hook = LLE_HOOK_COUNT;
}

/**
 * @brief Handler for DIRECTORY_CHANGED events -> chpwd hook
 *
 * Called after the working directory changes (e.g., via cd).
 * PWD and OLDPWD are already set by the cd builtin.
 */
static void hook_chpwd_handler(void *event_data, void *user_data) {
    (void)event_data;
    (void)user_data;

    // Check feature flag
    if (!shell_mode_allows(FEATURE_HOOK_FUNCTIONS)) {
        return;
    }

    // Prevent recursive calls
    if (executor_in_hook()) {
        return;
    }

    g_current_hook = LLE_HOOK_CHPWD;
    call_hook_function(LLE_HOOK_CHPWD, NULL);
    g_current_hook = LLE_HOOK_COUNT;
}

/* ============================================================================
 * LIFECYCLE FUNCTIONS
 * ============================================================================ */

/**
 * @brief Initialize the shell hooks bridge
 */
void lle_shell_hooks_init(void) {
    if (g_hooks_initialized) {
        return;
    }

    // Check if feature is enabled
    if (!shell_mode_allows(FEATURE_HOOK_FUNCTIONS)) {
        return;
    }

    // Get the shell event hub
    if (!g_lle_integration || !g_lle_integration->event_hub) {
        return;
    }

    lle_shell_event_hub_t *hub = g_lle_integration->event_hub;

    // Register event handlers
    // Note: precmd is triggered by POST_COMMAND (after command completes)
    lle_shell_event_hub_register(hub, LLE_SHELL_EVENT_POST_COMMAND,
                                  hook_precmd_handler, NULL, "precmd-hook");

    // preexec is triggered by PRE_COMMAND (before command runs)
    lle_shell_event_hub_register(hub, LLE_SHELL_EVENT_PRE_COMMAND,
                                  hook_preexec_handler, NULL, "preexec-hook");

    // chpwd is triggered by DIRECTORY_CHANGED
    lle_shell_event_hub_register(hub, LLE_SHELL_EVENT_DIRECTORY_CHANGED,
                                  hook_chpwd_handler, NULL, "chpwd-hook");

    g_hooks_initialized = true;
}

/**
 * @brief Clean up the shell hooks bridge
 */
void lle_shell_hooks_cleanup(void) {
    if (!g_hooks_initialized) {
        return;
    }

    // Unregister handlers if event hub is still available
    if (g_lle_integration && g_lle_integration->event_hub) {
        lle_shell_event_hub_t *hub = g_lle_integration->event_hub;

        lle_shell_event_hub_unregister(hub, LLE_SHELL_EVENT_POST_COMMAND,
                                        "precmd-hook");
        lle_shell_event_hub_unregister(hub, LLE_SHELL_EVENT_PRE_COMMAND,
                                        "preexec-hook");
        lle_shell_event_hub_unregister(hub, LLE_SHELL_EVENT_DIRECTORY_CHANGED,
                                        "chpwd-hook");
    }

    g_hooks_initialized = false;
}

/**
 * @brief Check if shell hooks are initialized
 */
bool lle_shell_hooks_initialized(void) {
    return g_hooks_initialized;
}

/* ============================================================================
 * HOOK QUERY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Check if a hook function is defined
 */
bool lle_shell_hook_defined(lle_hook_type_t hook_type) {
    if (hook_type >= LLE_HOOK_COUNT) {
        return false;
    }

    executor_t *executor = get_global_executor();
    if (!executor) {
        return false;
    }

    const char *hook_name = g_hook_names[hook_type];
    if (!hook_name) {
        return false;
    }

    // Use executor_call_hook with a NULL check - if hook returns 0 and
    // no function exists, that's the same as if it ran successfully.
    // We need a way to check if the function exists without calling it.
    // For now, we'll rely on the fact that executor_call_hook checks internally.
    // TODO: Add executor_function_exists() for clean checking
    
    // Workaround: Check if the function name is in the executor's function table
    // by attempting to call with special marker - but this isn't ideal.
    // For now, just return true if the feature is enabled - the hook will
    // silently do nothing if the function isn't defined.
    return shell_mode_allows(FEATURE_HOOK_FUNCTIONS);
}

/**
 * @brief Get the function name for a hook type
 */
const char *lle_shell_hook_name(lle_hook_type_t hook_type) {
    if (hook_type >= LLE_HOOK_COUNT) {
        return NULL;
    }
    return g_hook_names[hook_type];
}

/* ============================================================================
 * MANUAL HOOK INVOCATION
 * ============================================================================ */

/**
 * @brief Manually call a hook function
 */
int lle_shell_hook_call(lle_hook_type_t hook_type, int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (hook_type >= LLE_HOOK_COUNT) {
        return -1;
    }

    // Check feature flag
    if (!shell_mode_allows(FEATURE_HOOK_FUNCTIONS)) {
        return 0;
    }

    // For preexec, extract command from argv[1] if present
    const char *arg = NULL;
    if (hook_type == LLE_HOOK_PREEXEC && argc > 1 && argv && argv[1]) {
        arg = argv[1];
    }

    return call_hook_function(hook_type, arg);
}

/**
 * @brief Call a hook function by name
 */
int lle_shell_hook_call_by_name(const char *hook_name, int argc, char **argv) {
    if (!hook_name) {
        return -1;
    }

    // Find the hook type
    for (int i = 0; i < LLE_HOOK_COUNT; i++) {
        if (g_hook_names[i] && strcmp(g_hook_names[i], hook_name) == 0) {
            return lle_shell_hook_call((lle_hook_type_t)i, argc, argv);
        }
    }

    return -1;  // Unknown hook name
}

/* ============================================================================
 * HOOK STATISTICS
 * ============================================================================ */

/**
 * @brief Get the number of times a hook has been called
 */
size_t lle_shell_hook_call_count(lle_hook_type_t hook_type) {
    if (hook_type >= LLE_HOOK_COUNT) {
        return 0;
    }
    return g_hook_call_counts[hook_type];
}

/**
 * @brief Reset hook call statistics
 */
void lle_shell_hook_reset_stats(void) {
    for (int i = 0; i < LLE_HOOK_COUNT; i++) {
        g_hook_call_counts[i] = 0;
    }
}

/* ============================================================================
 * RECURSION PROTECTION
 * ============================================================================ */

/**
 * @brief Check if we're currently inside a hook
 */
bool lle_shell_hook_in_progress(void) {
    return g_current_hook != LLE_HOOK_COUNT || executor_in_hook();
}

/**
 * @brief Get the currently executing hook type
 */
lle_hook_type_t lle_shell_hook_current(void) {
    return g_current_hook;
}

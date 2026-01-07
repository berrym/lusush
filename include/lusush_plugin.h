/**
 * @file lusush_plugin.h
 * @brief Lusush Plugin System Foundation
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 *
 * Plugin system for extending lusush shell functionality. Plugins can:
 * - Register new builtins
 * - Add hook functions
 * - Provide custom completions
 * - Extend the editor (via LLE widgets)
 * - Subscribe to shell events
 *
 * Design Principles:
 * - Simple, focused API following lusush patterns
 * - Dynamic loading via dlopen/dlsym
 * - Sandboxed execution with permission system
 * - Memory-safe with proper lifecycle management
 * - Version compatibility checking
 *
 * Example Plugin:
 *
 *     // my_plugin.c
 *     #include <lusush_plugin.h>
 *
 *     static int my_builtin(int argc, char **argv) {
 *         printf("Hello from plugin!\n");
 *         return 0;
 *     }
 *
 *     LUSUSH_PLUGIN_DEFINE(
 *         .name = "my-plugin",
 *         .version = "1.0.0",
 *         .description = "Example plugin",
 *         .init = my_plugin_init,
 *         .cleanup = my_plugin_cleanup,
 *     );
 *
 *     int my_plugin_init(lusush_plugin_context_t *ctx) {
 *         lusush_plugin_register_builtin(ctx, "my-cmd", my_builtin);
 *         return 0;
 *     }
 */

#ifndef LUSUSH_PLUGIN_H
#define LUSUSH_PLUGIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * VERSION AND COMPATIBILITY
 * ============================================================================ */

/** Plugin API version - increment on breaking changes */
#define LUSUSH_PLUGIN_API_VERSION 1

/** Minimum API version for backwards compatibility */
#define LUSUSH_PLUGIN_API_VERSION_MIN 1

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct lusush_plugin lusush_plugin_t;
typedef struct lusush_plugin_manager lusush_plugin_manager_t;
typedef struct lusush_plugin_context lusush_plugin_context_t;

/* Forward declarations from other lusush components */
struct executor;
struct symtable;

/* ============================================================================
 * PLUGIN PERMISSIONS
 * ============================================================================ */

/**
 * @brief Plugin permission flags
 *
 * Controls what a plugin is allowed to do. Plugins declare required
 * permissions, and users can grant or deny them.
 */
typedef enum {
    LUSUSH_PLUGIN_PERM_NONE = 0,

    /* Builtin registration */
    LUSUSH_PLUGIN_PERM_REGISTER_BUILTIN = 1 << 0,

    /* Hook registration */
    LUSUSH_PLUGIN_PERM_REGISTER_HOOK = 1 << 1,

    /* Variable access */
    LUSUSH_PLUGIN_PERM_READ_VARS = 1 << 2,
    LUSUSH_PLUGIN_PERM_WRITE_VARS = 1 << 3,

    /* File system access */
    LUSUSH_PLUGIN_PERM_FILE_READ = 1 << 4,
    LUSUSH_PLUGIN_PERM_FILE_WRITE = 1 << 5,

    /* Network access */
    LUSUSH_PLUGIN_PERM_NETWORK = 1 << 6,

    /* Event subscription */
    LUSUSH_PLUGIN_PERM_EVENTS = 1 << 7,

    /* Completion provider */
    LUSUSH_PLUGIN_PERM_COMPLETIONS = 1 << 8,

    /* Widget registration (LLE) */
    LUSUSH_PLUGIN_PERM_WIDGETS = 1 << 9,

    /* Execute external commands */
    LUSUSH_PLUGIN_PERM_EXEC = 1 << 10,

    /* All permissions (for trusted plugins) */
    LUSUSH_PLUGIN_PERM_ALL = 0x7FF,
} lusush_plugin_permission_t;

/* ============================================================================
 * PLUGIN STATE
 * ============================================================================ */

/**
 * @brief Plugin lifecycle states
 */
typedef enum {
    LUSUSH_PLUGIN_STATE_UNLOADED,    /**< Not loaded */
    LUSUSH_PLUGIN_STATE_LOADING,     /**< Being loaded */
    LUSUSH_PLUGIN_STATE_LOADED,      /**< Loaded but not initialized */
    LUSUSH_PLUGIN_STATE_INITIALIZING,/**< Being initialized */
    LUSUSH_PLUGIN_STATE_ACTIVE,      /**< Fully active */
    LUSUSH_PLUGIN_STATE_SUSPENDED,   /**< Temporarily suspended */
    LUSUSH_PLUGIN_STATE_ERROR,       /**< Error state */
    LUSUSH_PLUGIN_STATE_UNLOADING,   /**< Being unloaded */
} lusush_plugin_state_t;

/* ============================================================================
 * PLUGIN RESULT CODES
 * ============================================================================ */

/**
 * @brief Plugin operation result codes
 */
typedef enum {
    LUSUSH_PLUGIN_OK = 0,             /**< Success */
    LUSUSH_PLUGIN_ERROR = -1,         /**< Generic error */
    LUSUSH_PLUGIN_ERROR_NOT_FOUND = -2,     /**< Plugin not found */
    LUSUSH_PLUGIN_ERROR_LOAD_FAILED = -3,   /**< dlopen failed */
    LUSUSH_PLUGIN_ERROR_SYMBOL_NOT_FOUND = -4, /**< Missing required symbol */
    LUSUSH_PLUGIN_ERROR_VERSION_MISMATCH = -5, /**< API version incompatible */
    LUSUSH_PLUGIN_ERROR_INIT_FAILED = -6,   /**< Plugin init() failed */
    LUSUSH_PLUGIN_ERROR_PERMISSION_DENIED = -7, /**< Permission not granted */
    LUSUSH_PLUGIN_ERROR_ALREADY_LOADED = -8,/**< Plugin already loaded */
    LUSUSH_PLUGIN_ERROR_INVALID_PLUGIN = -9,/**< Invalid plugin definition */
    LUSUSH_PLUGIN_ERROR_OUT_OF_MEMORY = -10,/**< Memory allocation failed */
} lusush_plugin_result_t;

/* ============================================================================
 * CALLBACK SIGNATURES
 * ============================================================================ */

/**
 * @brief Builtin command callback
 *
 * Same signature as internal builtins for consistency.
 */
typedef int (*lusush_plugin_builtin_fn)(int argc, char **argv);

/**
 * @brief Hook function callback
 *
 * Called for shell lifecycle events (precmd, preexec, chpwd).
 */
typedef void (*lusush_plugin_hook_fn)(lusush_plugin_context_t *ctx,
                                      const char *event_data);

/**
 * @brief Event handler callback
 *
 * Called when subscribed events occur.
 */
typedef void (*lusush_plugin_event_fn)(lusush_plugin_context_t *ctx,
                                       int event_type,
                                       void *event_data);

/**
 * @brief Completion provider callback
 *
 * Returns completions for a given input prefix.
 *
 * @param ctx Plugin context
 * @param line Current input line
 * @param cursor Cursor position
 * @param completions Output array (plugin allocates, shell frees)
 * @param count Output count of completions
 * @return 0 on success
 */
typedef int (*lusush_plugin_completion_fn)(lusush_plugin_context_t *ctx,
                                           const char *line,
                                           size_t cursor,
                                           char ***completions,
                                           size_t *count);

/* ============================================================================
 * PLUGIN DEFINITION
 * ============================================================================ */

/**
 * @brief Plugin definition structure
 *
 * Every plugin must export a `lusush_plugin_definition` symbol of this type.
 * Use the LUSUSH_PLUGIN_DEFINE macro for convenience.
 */
typedef struct lusush_plugin_def {
    /** API version - must match LUSUSH_PLUGIN_API_VERSION */
    uint32_t api_version;

    /** Plugin name (unique identifier) */
    const char *name;

    /** Human-readable version string (semver recommended) */
    const char *version;

    /** Short description of the plugin */
    const char *description;

    /** Author/maintainer */
    const char *author;

    /** License (e.g., "MIT", "GPL-3.0") */
    const char *license;

    /** Required permissions (bitfield) */
    lusush_plugin_permission_t required_permissions;

    /** Optional dependencies (NULL-terminated array of plugin names) */
    const char **dependencies;

    /**
     * @brief Initialize the plugin
     *
     * Called after loading. Plugin should register builtins, hooks, etc.
     *
     * @param ctx Plugin context for registration APIs
     * @return 0 on success, non-zero on failure
     */
    int (*init)(lusush_plugin_context_t *ctx);

    /**
     * @brief Clean up the plugin
     *
     * Called before unloading. Plugin should free resources.
     *
     * @param ctx Plugin context
     */
    void (*cleanup)(lusush_plugin_context_t *ctx);

    /**
     * @brief Suspend the plugin (optional)
     *
     * Called when plugin is temporarily suspended.
     *
     * @param ctx Plugin context
     */
    void (*suspend)(lusush_plugin_context_t *ctx);

    /**
     * @brief Resume the plugin (optional)
     *
     * Called when plugin is resumed after suspension.
     *
     * @param ctx Plugin context
     */
    void (*resume)(lusush_plugin_context_t *ctx);

} lusush_plugin_def_t;

/**
 * @brief Convenience macro for defining a plugin
 *
 * Usage:
 *     LUSUSH_PLUGIN_DEFINE(
 *         .name = "my-plugin",
 *         .version = "1.0.0",
 *         .init = my_init,
 *         .cleanup = my_cleanup,
 *     );
 */
#define LUSUSH_PLUGIN_DEFINE(...)                                              \
    const lusush_plugin_def_t lusush_plugin_definition = {                     \
        .api_version = LUSUSH_PLUGIN_API_VERSION,                              \
        __VA_ARGS__                                                            \
    }

/** Symbol name for plugin definition (for dlsym) */
#define LUSUSH_PLUGIN_SYMBOL "lusush_plugin_definition"

/* ============================================================================
 * PLUGIN CONTEXT
 * ============================================================================ */

/**
 * @brief Plugin execution context
 *
 * Passed to plugin callbacks, provides access to shell internals
 * (subject to permissions) and registration APIs.
 */
struct lusush_plugin_context {
    /** The plugin this context belongs to */
    lusush_plugin_t *plugin;

    /** Plugin manager for inter-plugin communication */
    lusush_plugin_manager_t *manager;

    /** Executor access (if permitted) */
    struct executor *executor;

    /** Symbol table access (if permitted) */
    struct symtable *symtable;

    /** Plugin-private data (set by plugin, freed by plugin) */
    void *user_data;

    /** Granted permissions (may be less than requested) */
    lusush_plugin_permission_t granted_permissions;
};

/* ============================================================================
 * PLUGIN INSTANCE
 * ============================================================================ */

/**
 * @brief Plugin instance structure
 *
 * Represents a loaded plugin. Managed by the plugin manager.
 */
struct lusush_plugin {
    /** Plugin definition (from shared object) */
    const lusush_plugin_def_t *def;

    /** Current state */
    lusush_plugin_state_t state;

    /** Path to the shared object */
    char *path;

    /** dlopen handle */
    void *handle;

    /** Plugin context */
    lusush_plugin_context_t *ctx;

    /** Registered builtins (for cleanup) */
    char **registered_builtins;
    size_t registered_builtin_count;

    /** Error message (if state == ERROR) */
    char *error_message;

    /** Load timestamp */
    uint64_t load_time;

    /** Next plugin in manager's list */
    struct lusush_plugin *next;
};

/* ============================================================================
 * PLUGIN MANAGER
 * ============================================================================ */

/**
 * @brief Plugin manager configuration
 */
typedef struct {
    /** Plugin search paths (NULL-terminated array) */
    const char **search_paths;

    /** Auto-load plugins from paths on init */
    bool auto_load;

    /** Default permissions for new plugins */
    lusush_plugin_permission_t default_permissions;

    /** Enable plugin sandboxing */
    bool enable_sandbox;

    /** Maximum loaded plugins (0 = unlimited) */
    size_t max_plugins;
} lusush_plugin_manager_config_t;

/**
 * @brief Plugin manager structure
 *
 * Central registry for all plugins. Handles loading, unloading,
 * and lifecycle management.
 */
struct lusush_plugin_manager {
    /** Loaded plugins (linked list) */
    lusush_plugin_t *plugins;

    /** Plugin count */
    size_t plugin_count;

    /** Configuration */
    lusush_plugin_manager_config_t config;

    /** Executor reference */
    struct executor *executor;

    /** Symbol table reference */
    struct symtable *symtable;

    /** Manager active flag */
    bool active;
};

/* ============================================================================
 * PLUGIN MANAGER API
 * ============================================================================ */

/**
 * @brief Create plugin manager
 *
 * @param manager Output pointer for created manager
 * @param config Configuration (NULL for defaults)
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_manager_create(lusush_plugin_manager_t **manager,
                             const lusush_plugin_manager_config_t *config);

/**
 * @brief Destroy plugin manager
 *
 * Unloads all plugins and frees resources.
 *
 * @param manager Manager to destroy
 */
void lusush_plugin_manager_destroy(lusush_plugin_manager_t *manager);

/**
 * @brief Set executor reference
 *
 * @param manager Plugin manager
 * @param executor Executor to use
 */
void lusush_plugin_manager_set_executor(lusush_plugin_manager_t *manager,
                                        struct executor *executor);

/**
 * @brief Set symbol table reference
 *
 * @param manager Plugin manager
 * @param symtable Symbol table to use
 */
void lusush_plugin_manager_set_symtable(lusush_plugin_manager_t *manager,
                                        struct symtable *symtable);

/**
 * @brief Load a plugin from a path
 *
 * @param manager Plugin manager
 * @param path Path to shared object
 * @param plugin Output pointer for loaded plugin (optional)
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_manager_load(lusush_plugin_manager_t *manager,
                           const char *path,
                           lusush_plugin_t **plugin);

/**
 * @brief Load a plugin by name
 *
 * Searches configured paths for the plugin.
 *
 * @param manager Plugin manager
 * @param name Plugin name
 * @param plugin Output pointer for loaded plugin (optional)
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_manager_load_by_name(lusush_plugin_manager_t *manager,
                                   const char *name,
                                   lusush_plugin_t **plugin);

/**
 * @brief Unload a plugin
 *
 * @param manager Plugin manager
 * @param name Plugin name
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_manager_unload(lusush_plugin_manager_t *manager,
                             const char *name);

/**
 * @brief Find a loaded plugin by name
 *
 * @param manager Plugin manager
 * @param name Plugin name
 * @return Plugin if found, NULL otherwise
 */
lusush_plugin_t *
lusush_plugin_manager_find(lusush_plugin_manager_t *manager,
                           const char *name);

/**
 * @brief Get all loaded plugins
 *
 * @param manager Plugin manager
 * @param plugins Output array (caller allocates)
 * @param count Input: array size, Output: actual count
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_manager_list(lusush_plugin_manager_t *manager,
                           lusush_plugin_t **plugins,
                           size_t *count);

/**
 * @brief Reload a plugin
 *
 * Unloads and reloads the plugin (useful for development).
 *
 * @param manager Plugin manager
 * @param name Plugin name
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_manager_reload(lusush_plugin_manager_t *manager,
                             const char *name);

/* ============================================================================
 * PLUGIN REGISTRATION API (for use by plugins)
 * ============================================================================ */

/**
 * @brief Register a builtin command
 *
 * @param ctx Plugin context
 * @param name Command name
 * @param fn Command function
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_register_builtin(lusush_plugin_context_t *ctx,
                               const char *name,
                               lusush_plugin_builtin_fn fn);

/**
 * @brief Unregister a builtin command
 *
 * @param ctx Plugin context
 * @param name Command name
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_unregister_builtin(lusush_plugin_context_t *ctx,
                                 const char *name);

/**
 * @brief Register a hook function
 *
 * @param ctx Plugin context
 * @param hook_name Hook name ("precmd", "preexec", "chpwd")
 * @param fn Hook function
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_register_hook(lusush_plugin_context_t *ctx,
                            const char *hook_name,
                            lusush_plugin_hook_fn fn);

/**
 * @brief Register a completion provider
 *
 * @param ctx Plugin context
 * @param name Provider name
 * @param fn Completion function
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_register_completion(lusush_plugin_context_t *ctx,
                                  const char *name,
                                  lusush_plugin_completion_fn fn);

/**
 * @brief Subscribe to shell events
 *
 * @param ctx Plugin context
 * @param event_type Event type to subscribe to
 * @param fn Event handler function
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_subscribe_event(lusush_plugin_context_t *ctx,
                              int event_type,
                              lusush_plugin_event_fn fn);

/* ============================================================================
 * PLUGIN VARIABLE ACCESS API
 * ============================================================================ */

/**
 * @brief Get a shell variable value
 *
 * @param ctx Plugin context
 * @param name Variable name
 * @return Variable value (do not free), NULL if not found
 */
const char *lusush_plugin_get_var(lusush_plugin_context_t *ctx,
                                  const char *name);

/**
 * @brief Set a shell variable value
 *
 * @param ctx Plugin context
 * @param name Variable name
 * @param value Variable value
 * @return LUSUSH_PLUGIN_OK on success
 */
lusush_plugin_result_t
lusush_plugin_set_var(lusush_plugin_context_t *ctx,
                      const char *name,
                      const char *value);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * @brief Get string description of result code
 *
 * @param result Result code
 * @return Static string describing the result
 */
const char *lusush_plugin_result_string(lusush_plugin_result_t result);

/**
 * @brief Get string description of plugin state
 *
 * @param state Plugin state
 * @return Static string describing the state
 */
const char *lusush_plugin_state_string(lusush_plugin_state_t state);

/**
 * @brief Check if a permission is granted
 *
 * @param ctx Plugin context
 * @param perm Permission to check
 * @return true if permission is granted
 */
bool lusush_plugin_has_permission(lusush_plugin_context_t *ctx,
                                  lusush_plugin_permission_t perm);

/**
 * @brief Log a message from plugin
 *
 * @param ctx Plugin context
 * @param level Log level (0=debug, 1=info, 2=warn, 3=error)
 * @param fmt Printf-style format string
 */
void lusush_plugin_log(lusush_plugin_context_t *ctx,
                       int level,
                       const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* LUSUSH_PLUGIN_H */

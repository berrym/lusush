#ifndef LLE_CONFIG_H
#define LLE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * @file config.h
 * @brief Lusush Line Editor Configuration Integration
 *
 * This module provides integration between the Lusush Line Editor (LLE) and
 * the Lusush configuration system. It allows LLE to load configuration values
 * from Lusush's configuration files and apply them dynamically to the line
 * editor instance.
 *
 * The configuration system supports all major LLE features including:
 * - Multiline editing mode
 * - Syntax highlighting
 * - Auto-completion
 * - History management
 * - Undo/redo functionality
 * - Theme integration
 * - Performance tuning
 *
 * Configuration values are loaded from the standard Lusush configuration
 * system and can be updated dynamically during runtime.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

// Forward declaration to avoid circular dependency
struct lle_line_editor;

/**
 * @brief LLE-specific configuration structure
 *
 * This structure contains all configuration options that can be applied
 * to a line editor instance. It maps Lusush configuration values to
 * LLE-specific settings and provides sensible defaults for all options.
 *
 * All boolean options default to true (enabled) and numeric options
 * use standard defaults that provide good performance and usability.
 */
typedef struct {
    /* Core editing features */
    bool multiline_mode;                /**< Enable multiline editing */
    bool syntax_highlighting;          /**< Enable syntax highlighting */
    bool show_completions;              /**< Show tab completions */
    bool history_enabled;               /**< Enable command history */
    bool undo_enabled;                  /**< Enable undo/redo operations */
    
    /* Resource limits */
    size_t history_size;                /**< Maximum history entries */
    size_t undo_levels;                 /**< Maximum undo operations */
    size_t max_completion_items;        /**< Maximum completion items */
    
    /* Theme and display settings */
    bool colors_enabled;                /**< Enable color output */
    bool theme_auto_detect;             /**< Auto-detect terminal capabilities */
    char *theme_name;                   /**< Active theme name */
    
    /* Advanced features */
    bool fuzzy_completion;              /**< Enable fuzzy matching */
    bool completion_case_sensitive;     /**< Case-sensitive completion */
    bool hints_enabled;                 /**< Enable input hints */
    bool history_no_dups;               /**< Remove duplicate history entries */
    bool history_timestamps;            /**< Add timestamps to history */
    
    /* Performance settings */
    size_t buffer_initial_size;         /**< Initial text buffer size */
    size_t display_cache_size;          /**< Display cache size */
    int refresh_rate_ms;                /**< Display refresh rate limit */
} lle_lusush_config_t;

/**
 * @brief Configuration loading result codes
 */
typedef enum {
    LLE_CONFIG_SUCCESS = 0,             /**< Configuration loaded successfully */
    LLE_CONFIG_ERROR_INVALID_PARAM,     /**< Invalid parameter provided */
    LLE_CONFIG_ERROR_MEMORY,            /**< Memory allocation failed */
    LLE_CONFIG_ERROR_FILE_ACCESS,       /**< Configuration file access error */
    LLE_CONFIG_ERROR_PARSE,             /**< Configuration parsing error */
    LLE_CONFIG_ERROR_LUSUSH_CONFIG      /**< Lusush configuration system error */
} lle_config_result_t;

/**
 * @brief Configuration change callback function type
 *
 * This callback is invoked when configuration values change during runtime.
 * It allows the line editor to respond to configuration updates immediately.
 *
 * @param config Pointer to the updated configuration
 * @param user_data User-provided data pointer
 */
typedef void (*lle_config_change_callback_t)(const lle_lusush_config_t *config, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Core Configuration Functions
// ============================================================================

/**
 * @brief Initialize configuration with default values
 *
 * Initializes a configuration structure with sensible default values.
 * All features are enabled and resource limits are set to standard values.
 * This function should be called before loading values from Lusush config.
 *
 * @param config Configuration structure to initialize (must not be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note The configuration structure is zeroed and then filled with defaults
 * @note String pointers are set to NULL and should be allocated when needed
 * @note This function never fails unless passed invalid parameters
 */
lle_config_result_t lle_config_init_defaults(lle_lusush_config_t *config);

/**
 * @brief Load configuration from Lusush configuration system
 *
 * Loads LLE configuration values from the active Lusush configuration.
 * This function reads all relevant configuration options and maps them
 * to LLE-specific settings. Values not found in the configuration use
 * the defaults previously set by lle_config_init_defaults().
 *
 * @param config Configuration structure to populate (must not be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note This function requires the Lusush configuration system to be initialized
 * @note Missing configuration values will retain their default values
 * @note Memory for string values is allocated and must be freed with lle_config_cleanup()
 * @note Configuration is validated during loading to ensure sensible values
 */
lle_config_result_t lle_config_load(lle_lusush_config_t *config);

/**
 * @brief Apply configuration to a line editor instance
 *
 * Applies the specified configuration to a line editor instance, updating
 * all relevant settings and features. This function can be called multiple
 * times to update the configuration dynamically.
 *
 * @param editor Line editor instance to configure (must not be NULL)
 * @param config Configuration to apply (must not be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note The editor must be properly initialized before calling this function
 * @note Some configuration changes may require component reinitialization
 * @note Resource limits are applied immediately and may affect existing data
 * @note Configuration callbacks are triggered after successful application
 */
lle_config_result_t lle_config_apply(struct lle_line_editor *editor, const lle_lusush_config_t *config);

/**
 * @brief Clean up configuration resources
 *
 * Frees all memory allocated for configuration values, including string
 * pointers and other dynamically allocated resources. The configuration
 * structure itself is not freed.
 *
 * @param config Configuration structure to clean up (can be NULL)
 *
 * @note This function is safe to call multiple times
 * @note The configuration structure is zeroed after cleanup
 * @note String pointers are set to NULL after freeing
 */
void lle_config_cleanup(lle_lusush_config_t *config);

// ============================================================================
// Dynamic Configuration Management
// ============================================================================

/**
 * @brief Reload configuration from Lusush system
 *
 * Reloads configuration values from the Lusush configuration system and
 * applies them to the specified line editor instance. This is useful for
 * picking up configuration changes made at runtime.
 *
 * @param editor Line editor instance to update (must not be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note The existing configuration is preserved if loading fails
 * @note Configuration callbacks are triggered after successful reload
 * @note This function may be called from signal handlers for config reload
 */
lle_config_result_t lle_config_reload(struct lle_line_editor *editor);

/**
 * @brief Get current configuration from line editor
 *
 * Retrieves the current configuration settings from a line editor instance.
 * This provides a snapshot of the active configuration that can be used
 * for introspection or saving.
 *
 * @param editor Line editor instance to query (must not be NULL)
 * @param config Configuration structure to populate (must not be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note The returned configuration is a copy and can be modified safely
 * @note String values are allocated and must be freed with lle_config_cleanup()
 * @note The configuration reflects the current runtime state
 */
lle_config_result_t lle_config_get_current(struct lle_line_editor *editor, lle_lusush_config_t *config);

// ============================================================================
// Configuration Validation and Utilities
// ============================================================================

/**
 * @brief Validate configuration values
 *
 * Validates all configuration values to ensure they are within acceptable
 * ranges and combinations. This function checks for logical inconsistencies
 * and applies corrections where necessary.
 *
 * @param config Configuration structure to validate (must not be NULL)
 * @return LLE_CONFIG_SUCCESS if valid, error code if invalid
 *
 * @note Invalid values are corrected to nearest valid values when possible
 * @note Critical errors prevent the configuration from being used
 * @note Warnings are logged for values that were automatically corrected
 */
lle_config_result_t lle_config_validate(lle_lusush_config_t *config);

/**
 * @brief Copy configuration structure
 *
 * Creates a deep copy of a configuration structure, including all string
 * values and dynamically allocated resources. The destination structure
 * should be uninitialized or cleaned up before calling this function.
 *
 * @param dest Destination configuration structure (must not be NULL)
 * @param src Source configuration structure (must not be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note Memory is allocated for string values in the destination
 * @note The destination should be cleaned up with lle_config_cleanup()
 * @note Existing values in the destination are overwritten
 */
lle_config_result_t lle_config_copy(lle_lusush_config_t *dest, const lle_lusush_config_t *src);

/**
 * @brief Compare two configuration structures
 *
 * Compares two configuration structures for equality. All fields including
 * string values are compared. This is useful for detecting configuration
 * changes.
 *
 * @param config1 First configuration structure (must not be NULL)
 * @param config2 Second configuration structure (must not be NULL)
 * @return true if configurations are identical, false otherwise
 *
 * @note String comparisons are case-sensitive
 * @note NULL string pointers are treated as equal to other NULL pointers
 * @note All numeric and boolean fields must match exactly
 */
bool lle_config_equals(const lle_lusush_config_t *config1, const lle_lusush_config_t *config2);

// ============================================================================
// Configuration Callback System
// ============================================================================

/**
 * @brief Register configuration change callback
 *
 * Registers a callback function that will be invoked whenever configuration
 * values change. Multiple callbacks can be registered and will be called
 * in registration order.
 *
 * @param callback Callback function to register (must not be NULL)
 * @param user_data User data to pass to callback (can be NULL)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note The same callback can be registered multiple times with different user data
 * @note Callbacks are invoked after configuration changes are applied
 * @note Callback execution time should be minimal to avoid blocking
 */
lle_config_result_t lle_config_register_callback(lle_config_change_callback_t callback, void *user_data);

/**
 * @brief Unregister configuration change callback
 *
 * Removes a previously registered configuration change callback. If the
 * callback was registered multiple times, only one instance is removed.
 *
 * @param callback Callback function to unregister (must not be NULL)
 * @param user_data User data associated with the callback (must match registration)
 * @return LLE_CONFIG_SUCCESS on success, error code if not found
 *
 * @note Both callback function and user data must match registration
 * @note It is safe to unregister callbacks from within callback functions
 */
lle_config_result_t lle_config_unregister_callback(lle_config_change_callback_t callback, void *user_data);

// ============================================================================
// Individual Setting Management
// ============================================================================

/**
 * @brief Update a single boolean configuration setting
 *
 * Updates a single boolean configuration setting in the line editor and
 * triggers configuration callbacks. This is more efficient than reloading
 * the entire configuration for single value changes.
 *
 * @param editor Line editor instance to update (must not be NULL)
 * @param setting_name Name of the boolean setting to update
 * @param value New value for the setting
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note Valid setting names include: multiline_mode, syntax_highlighting, show_completions, etc.
 * @note Unknown setting names return LLE_CONFIG_ERROR_INVALID_PARAM
 * @note Configuration callbacks are triggered for successful updates
 */
lle_config_result_t lle_config_set_bool(struct lle_line_editor *editor, const char *setting_name, bool value);

/**
 * @brief Update a single numeric configuration setting
 *
 * Updates a single numeric configuration setting in the line editor and
 * triggers configuration callbacks. Values are validated before application.
 *
 * @param editor Line editor instance to update (must not be NULL)
 * @param setting_name Name of the numeric setting to update
 * @param value New value for the setting
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note Valid setting names include: history_size, undo_levels, refresh_rate_ms, etc.
 * @note Values are validated and corrected to acceptable ranges
 * @note Configuration callbacks are triggered for successful updates
 */
lle_config_result_t lle_config_set_size(struct lle_line_editor *editor, const char *setting_name, size_t value);

/**
 * @brief Update a single string configuration setting
 *
 * Updates a single string configuration setting in the line editor and
 * triggers configuration callbacks. Memory is managed automatically.
 *
 * @param editor Line editor instance to update (must not be NULL)
 * @param setting_name Name of the string setting to update
 * @param value New value for the setting (NULL to clear)
 * @return LLE_CONFIG_SUCCESS on success, error code on failure
 *
 * @note Valid setting names include: theme_name
 * @note Memory for the string value is allocated automatically
 * @note Previous string values are freed before setting new values
 * @note Configuration callbacks are triggered for successful updates
 */
lle_config_result_t lle_config_set_string(struct lle_line_editor *editor, const char *setting_name, const char *value);

// ============================================================================
// Configuration Information and Debugging
// ============================================================================

/**
 * @brief Get configuration error message
 *
 * Returns a human-readable error message for the last configuration operation
 * that failed. This is useful for debugging configuration issues.
 *
 * @param result Error code from configuration operation
 * @return Static string describing the error (never NULL)
 *
 * @note The returned string is static and should not be freed
 * @note Error messages are in English and suitable for logging
 */
const char *lle_config_get_error_message(lle_config_result_t result);

/**
 * @brief Print configuration summary
 *
 * Prints a human-readable summary of the configuration to the specified
 * file handle. This is useful for debugging and configuration introspection.
 *
 * @param config Configuration structure to print (must not be NULL)
 * @param output File handle to write to (e.g., stdout, stderr)
 *
 * @note Output format is suitable for human reading and debugging
 * @note All configuration values are included in the output
 * @note String values are quoted and NULL pointers are indicated
 */
void lle_config_print_summary(const lle_lusush_config_t *config, FILE *output);

#ifdef __cplusplus
}
#endif

#endif /* LLE_CONFIG_H */
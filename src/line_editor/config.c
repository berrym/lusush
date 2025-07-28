/*
 * Lusush Line Editor - Configuration Integration Implementation
 * 
 * This file provides the implementation for integrating the Lusush Line Editor (LLE)
 * with the Lusush configuration system. It handles loading configuration values,
 * applying them to line editor instances, and managing dynamic configuration updates.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "line_editor.h"
#include "../../include/config.h"  // Lusush configuration system
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Default configuration values
 */
#define LLE_DEFAULT_HISTORY_SIZE 1000
#define LLE_DEFAULT_UNDO_LEVELS 100
#define LLE_DEFAULT_MAX_COMPLETIONS 50
#define LLE_DEFAULT_BUFFER_SIZE 256
#define LLE_DEFAULT_DISPLAY_CACHE 1024
#define LLE_DEFAULT_REFRESH_RATE 16  // ~60 FPS

/**
 * @brief Maximum number of configuration callbacks
 */
#define LLE_MAX_CONFIG_CALLBACKS 10

/**
 * @brief Configuration callback entry
 */
typedef struct {
    lle_config_change_callback_t callback;
    void *user_data;
    bool active;
} lle_config_callback_entry_t;

/**
 * @brief Global configuration callback registry
 */
static lle_config_callback_entry_t config_callbacks[LLE_MAX_CONFIG_CALLBACKS];
static bool callbacks_initialized = false;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Initialize callback system if needed
 */
static void lle_config_init_callbacks(void) {
    if (!callbacks_initialized) {
        memset(config_callbacks, 0, sizeof(config_callbacks));
        callbacks_initialized = true;
    }
}

/**
 * @brief Safely duplicate a string
 *
 * @param str String to duplicate (can be NULL)
 * @return Duplicated string or NULL
 */
static char *lle_config_strdup(const char *str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    
    memcpy(copy, str, len + 1);
    return copy;
}

/**
 * @brief Trigger configuration change callbacks
 *
 * @param config Updated configuration
 */
static void lle_config_trigger_callbacks(const lle_lusush_config_t *config) {
    if (!config || !callbacks_initialized) return;
    
    for (int i = 0; i < LLE_MAX_CONFIG_CALLBACKS; i++) {
        if (config_callbacks[i].active && config_callbacks[i].callback) {
            config_callbacks[i].callback(config, config_callbacks[i].user_data);
        }
    }
}

/**
 * @brief Validate and correct numeric configuration values
 *
 * @param config Configuration to validate
 */
static void lle_config_validate_numeric(lle_lusush_config_t *config) {
    if (!config) return;
    
    // History size: 10 to 50,000 entries
    if (config->history_size < 10) config->history_size = 10;
    if (config->history_size > 50000) config->history_size = 50000;
    
    // Undo levels: 5 to 1,000 operations
    if (config->undo_levels < 5) config->undo_levels = 5;
    if (config->undo_levels > 1000) config->undo_levels = 1000;
    
    // Completion items: 5 to 200 items
    if (config->max_completion_items < 5) config->max_completion_items = 5;
    if (config->max_completion_items > 200) config->max_completion_items = 200;
    
    // Buffer size: 64 to 64KB
    if (config->buffer_initial_size < 64) config->buffer_initial_size = 64;
    if (config->buffer_initial_size > 65536) config->buffer_initial_size = 65536;
    
    // Display cache: 256 to 1MB
    if (config->display_cache_size < 256) config->display_cache_size = 256;
    if (config->display_cache_size > 1048576) config->display_cache_size = 1048576;
    
    // Refresh rate: 1 to 100ms (10-1000 FPS)
    if (config->refresh_rate_ms < 1) config->refresh_rate_ms = 1;
    if (config->refresh_rate_ms > 100) config->refresh_rate_ms = 100;
}

// ============================================================================
// Core Configuration Functions
// ============================================================================

lle_config_result_t lle_config_init_defaults(lle_lusush_config_t *config) {
    if (!config) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Zero the structure
    memset(config, 0, sizeof(lle_lusush_config_t));
    
    // Set default boolean values (all features enabled)
    config->multiline_mode = true;
    config->syntax_highlighting = true;
    config->show_completions = true;
    config->history_enabled = true;
    config->undo_enabled = true;
    config->colors_enabled = true;
    config->theme_auto_detect = true;
    config->fuzzy_completion = true;
    config->completion_case_sensitive = false;
    config->hints_enabled = true;
    config->history_no_dups = false;
    config->history_timestamps = false;
    
    // Set default numeric values
    config->history_size = LLE_DEFAULT_HISTORY_SIZE;
    config->undo_levels = LLE_DEFAULT_UNDO_LEVELS;
    config->max_completion_items = LLE_DEFAULT_MAX_COMPLETIONS;
    config->buffer_initial_size = LLE_DEFAULT_BUFFER_SIZE;
    config->display_cache_size = LLE_DEFAULT_DISPLAY_CACHE;
    config->refresh_rate_ms = LLE_DEFAULT_REFRESH_RATE;
    
    // String values start as NULL (will be set during load)
    config->theme_name = NULL;
    
    return LLE_CONFIG_SUCCESS;
}

lle_config_result_t lle_config_load(lle_lusush_config_t *lle_config) {
    if (!lle_config) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Note: In a complete implementation, this would access the global Lusush configuration
    // For now, we use sensible defaults since the global config may not be available
    // during standalone LLE compilation/testing
    
    // Load boolean settings with defaults (would normally come from global config)
    lle_config->multiline_mode = true;      // Default: enabled
    lle_config->syntax_highlighting = true; // Always enabled in LLE
    lle_config->show_completions = true;    // Default: enabled
    lle_config->history_enabled = true;     // Default: enabled
    lle_config->undo_enabled = true;        // Always enabled in LLE
    lle_config->colors_enabled = true;      // Default: enabled
    lle_config->theme_auto_detect = true;   // Default: enabled
    lle_config->fuzzy_completion = true;    // Default: enabled
    lle_config->completion_case_sensitive = false; // Default: case insensitive
    lle_config->hints_enabled = true;       // Default: enabled
    lle_config->history_no_dups = false;    // Default: allow duplicates
    lle_config->history_timestamps = false; // Default: no timestamps
    
    // Use default history size if not already set
    if (lle_config->history_size == 0) {
        lle_config->history_size = LLE_DEFAULT_HISTORY_SIZE;
    }
    
    // Set LLE-specific defaults for settings not in Lusush config
    if (lle_config->undo_levels == 0) {
        lle_config->undo_levels = LLE_DEFAULT_UNDO_LEVELS;
    }
    if (lle_config->max_completion_items == 0) {
        lle_config->max_completion_items = LLE_DEFAULT_MAX_COMPLETIONS;
    }
    if (lle_config->buffer_initial_size == 0) {
        lle_config->buffer_initial_size = LLE_DEFAULT_BUFFER_SIZE;
    }
    if (lle_config->display_cache_size == 0) {
        lle_config->display_cache_size = LLE_DEFAULT_DISPLAY_CACHE;
    }
    if (lle_config->refresh_rate_ms == 0) {
        lle_config->refresh_rate_ms = LLE_DEFAULT_REFRESH_RATE;
    }
    
    // String settings use defaults (would normally come from global config)
    // Theme name remains NULL for default theme
    
    // Validate all loaded values
    lle_config_validate_numeric(lle_config);
    
    return LLE_CONFIG_SUCCESS;
}

lle_config_result_t lle_config_apply(struct lle_line_editor *editor, const lle_lusush_config_t *config) {
    if (!editor || !config) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Cast to typedef for API compatibility
    lle_line_editor_t *typed_editor = (lle_line_editor_t *)editor;
    
    // Verify editor is initialized
    if (!lle_is_initialized(typed_editor)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    // Apply boolean settings
    if (!lle_set_multiline_mode(typed_editor, config->multiline_mode)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    if (!lle_set_syntax_highlighting(typed_editor, config->syntax_highlighting)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    if (!lle_set_auto_completion(typed_editor, config->show_completions)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    if (!lle_set_history_enabled(typed_editor, config->history_enabled)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    if (!lle_set_undo_enabled(typed_editor, config->undo_enabled)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    // Apply resource limits
    // Note: These would require extended API functions in a real implementation
    // For now, we document that they are applied conceptually
    
    // Trigger configuration change callbacks
    lle_config_trigger_callbacks(config);
    
    return LLE_CONFIG_SUCCESS;
}

void lle_config_cleanup(lle_lusush_config_t *config) {
    if (!config) return;
    
    // Free string values
    if (config->theme_name) {
        free(config->theme_name);
        config->theme_name = NULL;
    }
    
    // Zero the structure
    memset(config, 0, sizeof(lle_lusush_config_t));
}

// ============================================================================
// Dynamic Configuration Management
// ============================================================================

lle_config_result_t lle_config_reload(struct lle_line_editor *editor) {
    if (!editor) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    lle_lusush_config_t new_config;
    lle_config_result_t result;
    
    // Initialize with defaults
    result = lle_config_init_defaults(&new_config);
    if (result != LLE_CONFIG_SUCCESS) {
        return result;
    }
    
    // Load from Lusush configuration
    result = lle_config_load(&new_config);
    if (result != LLE_CONFIG_SUCCESS) {
        lle_config_cleanup(&new_config);
        return result;
    }
    
    // Apply to editor
    result = lle_config_apply(editor, &new_config);
    
    // Clean up temporary configuration
    lle_config_cleanup(&new_config);
    
    return result;
}

lle_config_result_t lle_config_get_current(struct lle_line_editor *editor, lle_lusush_config_t *config) {
    if (!editor || !config) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Cast to typedef for API compatibility
    lle_line_editor_t *typed_editor = (lle_line_editor_t *)editor;
    
    if (!lle_is_initialized(typed_editor)) {
        return LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    // Initialize defaults and get current state from editor structure
    lle_config_init_defaults(config);
    
    // Map editor configuration to Lusush config structure
    // Note: These would need proper getter functions in a complete implementation
    config->multiline_mode = typed_editor->multiline_mode;
    config->syntax_highlighting = typed_editor->syntax_highlighting;
    config->show_completions = typed_editor->auto_completion;
    config->history_enabled = typed_editor->history_enabled;
    config->undo_enabled = typed_editor->undo_enabled;
    config->history_size = typed_editor->max_history_size;
    config->undo_levels = typed_editor->max_undo_actions;
    
    return LLE_CONFIG_SUCCESS;
}

// ============================================================================
// Configuration Validation and Utilities
// ============================================================================

lle_config_result_t lle_config_validate(lle_lusush_config_t *config) {
    if (!config) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Validate and correct numeric values
    lle_config_validate_numeric(config);
    
    // Validate string values
    if (config->theme_name && strlen(config->theme_name) == 0) {
        free(config->theme_name);
        config->theme_name = NULL;
    }
    
    return LLE_CONFIG_SUCCESS;
}

lle_config_result_t lle_config_copy(lle_lusush_config_t *dest, const lle_lusush_config_t *src) {
    if (!dest || !src) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Copy all fields
    memcpy(dest, src, sizeof(lle_lusush_config_t));
    
    // Duplicate string values
    dest->theme_name = lle_config_strdup(src->theme_name);
    if (src->theme_name && !dest->theme_name) {
        return LLE_CONFIG_ERROR_MEMORY;
    }
    
    return LLE_CONFIG_SUCCESS;
}

bool lle_config_equals(const lle_lusush_config_t *config1, const lle_lusush_config_t *config2) {
    if (!config1 || !config2) return false;
    
    // Compare boolean fields
    if (config1->multiline_mode != config2->multiline_mode) return false;
    if (config1->syntax_highlighting != config2->syntax_highlighting) return false;
    if (config1->show_completions != config2->show_completions) return false;
    if (config1->history_enabled != config2->history_enabled) return false;
    if (config1->undo_enabled != config2->undo_enabled) return false;
    if (config1->colors_enabled != config2->colors_enabled) return false;
    if (config1->theme_auto_detect != config2->theme_auto_detect) return false;
    if (config1->fuzzy_completion != config2->fuzzy_completion) return false;
    if (config1->completion_case_sensitive != config2->completion_case_sensitive) return false;
    if (config1->hints_enabled != config2->hints_enabled) return false;
    if (config1->history_no_dups != config2->history_no_dups) return false;
    if (config1->history_timestamps != config2->history_timestamps) return false;
    
    // Compare numeric fields
    if (config1->history_size != config2->history_size) return false;
    if (config1->undo_levels != config2->undo_levels) return false;
    if (config1->max_completion_items != config2->max_completion_items) return false;
    if (config1->buffer_initial_size != config2->buffer_initial_size) return false;
    if (config1->display_cache_size != config2->display_cache_size) return false;
    if (config1->refresh_rate_ms != config2->refresh_rate_ms) return false;
    
    // Compare string fields
    if (config1->theme_name && config2->theme_name) {
        if (strcmp(config1->theme_name, config2->theme_name) != 0) return false;
    } else if (config1->theme_name != config2->theme_name) {
        return false;  // One is NULL, the other is not
    }
    
    return true;
}

// ============================================================================
// Configuration Callback System
// ============================================================================

lle_config_result_t lle_config_register_callback(lle_config_change_callback_t callback, void *user_data) {
    if (!callback) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    lle_config_init_callbacks();
    
    // Find an empty slot
    for (int i = 0; i < LLE_MAX_CONFIG_CALLBACKS; i++) {
        if (!config_callbacks[i].active) {
            config_callbacks[i].callback = callback;
            config_callbacks[i].user_data = user_data;
            config_callbacks[i].active = true;
            return LLE_CONFIG_SUCCESS;
        }
    }
    
    return LLE_CONFIG_ERROR_MEMORY;  // No slots available
}

lle_config_result_t lle_config_unregister_callback(lle_config_change_callback_t callback, void *user_data) {
    if (!callback || !callbacks_initialized) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Find and remove the callback
    for (int i = 0; i < LLE_MAX_CONFIG_CALLBACKS; i++) {
        if (config_callbacks[i].active && 
            config_callbacks[i].callback == callback &&
            config_callbacks[i].user_data == user_data) {
            config_callbacks[i].active = false;
            config_callbacks[i].callback = NULL;
            config_callbacks[i].user_data = NULL;
            return LLE_CONFIG_SUCCESS;
        }
    }
    
    return LLE_CONFIG_ERROR_INVALID_PARAM;  // Not found
}

// ============================================================================
// Individual Setting Management
// ============================================================================

lle_config_result_t lle_config_set_bool(struct lle_line_editor *editor, const char *setting_name, bool value) {
    if (!editor || !setting_name) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // Cast to typedef for API compatibility
    lle_line_editor_t *typed_editor = (lle_line_editor_t *)editor;
    
    // Map setting names to editor functions
    if (strcmp(setting_name, "multiline_mode") == 0) {
        return lle_set_multiline_mode(typed_editor, value) ? LLE_CONFIG_SUCCESS : LLE_CONFIG_ERROR_INVALID_PARAM;
    } else if (strcmp(setting_name, "syntax_highlighting") == 0) {
        return lle_set_syntax_highlighting(typed_editor, value) ? LLE_CONFIG_SUCCESS : LLE_CONFIG_ERROR_INVALID_PARAM;
    } else if (strcmp(setting_name, "show_completions") == 0) {
        return lle_set_auto_completion(typed_editor, value) ? LLE_CONFIG_SUCCESS : LLE_CONFIG_ERROR_INVALID_PARAM;
    } else if (strcmp(setting_name, "history_enabled") == 0) {
        return lle_set_history_enabled(typed_editor, value) ? LLE_CONFIG_SUCCESS : LLE_CONFIG_ERROR_INVALID_PARAM;
    } else if (strcmp(setting_name, "undo_enabled") == 0) {
        return lle_set_undo_enabled(typed_editor, value) ? LLE_CONFIG_SUCCESS : LLE_CONFIG_ERROR_INVALID_PARAM;
    }
    
    return LLE_CONFIG_ERROR_INVALID_PARAM;
}

lle_config_result_t lle_config_set_size(struct lle_line_editor *editor, const char *setting_name, size_t value) {
    if (!editor || !setting_name) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // For now, size_t settings are not directly supported by the editor API
    // In a real implementation, we would extend the API to support these
    // settings or store them in an internal configuration structure
    
    return LLE_CONFIG_ERROR_INVALID_PARAM;
}

lle_config_result_t lle_config_set_string(struct lle_line_editor *editor, const char *setting_name, const char *value) {
    if (!editor || !setting_name) return LLE_CONFIG_ERROR_INVALID_PARAM;
    
    // For now, string settings are not directly supported by the editor API
    // In a real implementation, we would extend the API to support theme
    // name changes and other string settings
    
    return LLE_CONFIG_ERROR_INVALID_PARAM;
}

// ============================================================================
// Configuration Information and Debugging
// ============================================================================

const char *lle_config_get_error_message(lle_config_result_t result) {
    switch (result) {
        case LLE_CONFIG_SUCCESS:
            return "Success";
        case LLE_CONFIG_ERROR_INVALID_PARAM:
            return "Invalid parameter provided";
        case LLE_CONFIG_ERROR_MEMORY:
            return "Memory allocation failed";
        case LLE_CONFIG_ERROR_FILE_ACCESS:
            return "Configuration file access error";
        case LLE_CONFIG_ERROR_PARSE:
            return "Configuration parsing error";
        case LLE_CONFIG_ERROR_LUSUSH_CONFIG:
            return "Lusush configuration system error";
        default:
            return "Unknown error";
    }
}

void lle_config_print_summary(const lle_lusush_config_t *config, FILE *output) {
    if (!config || !output) return;
    
    fprintf(output, "LLE Configuration Summary:\n");
    fprintf(output, "=========================\n");
    
    // Boolean settings
    fprintf(output, "Core Features:\n");
    fprintf(output, "  Multiline mode: %s\n", config->multiline_mode ? "enabled" : "disabled");
    fprintf(output, "  Syntax highlighting: %s\n", config->syntax_highlighting ? "enabled" : "disabled");
    fprintf(output, "  Show completions: %s\n", config->show_completions ? "enabled" : "disabled");
    fprintf(output, "  History: %s\n", config->history_enabled ? "enabled" : "disabled");
    fprintf(output, "  Undo/redo: %s\n", config->undo_enabled ? "enabled" : "disabled");
    
    fprintf(output, "\nDisplay Settings:\n");
    fprintf(output, "  Colors: %s\n", config->colors_enabled ? "enabled" : "disabled");
    fprintf(output, "  Auto-detect theme: %s\n", config->theme_auto_detect ? "enabled" : "disabled");
    fprintf(output, "  Theme name: %s\n", config->theme_name ? config->theme_name : "(default)");
    
    fprintf(output, "\nCompletion Settings:\n");
    fprintf(output, "  Fuzzy matching: %s\n", config->fuzzy_completion ? "enabled" : "disabled");
    fprintf(output, "  Case sensitive: %s\n", config->completion_case_sensitive ? "enabled" : "disabled");
    fprintf(output, "  Hints: %s\n", config->hints_enabled ? "enabled" : "disabled");
    
    fprintf(output, "\nHistory Settings:\n");
    fprintf(output, "  No duplicates: %s\n", config->history_no_dups ? "enabled" : "disabled");
    fprintf(output, "  Timestamps: %s\n", config->history_timestamps ? "enabled" : "disabled");
    
    // Numeric settings
    fprintf(output, "\nResource Limits:\n");
    fprintf(output, "  History size: %zu entries\n", config->history_size);
    fprintf(output, "  Undo levels: %zu operations\n", config->undo_levels);
    fprintf(output, "  Max completions: %zu items\n", config->max_completion_items);
    
    fprintf(output, "\nPerformance Settings:\n");
    fprintf(output, "  Buffer size: %zu bytes\n", config->buffer_initial_size);
    fprintf(output, "  Display cache: %zu bytes\n", config->display_cache_size);
    fprintf(output, "  Refresh rate: %d ms\n", config->refresh_rate_ms);
    
    fprintf(output, "=========================\n");
}
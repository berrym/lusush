/*
 * Lusush Line Editor - Main Public API Implementation
 * 
 * This file provides the main public API implementation for the Lusush Line Editor (LLE).
 * It serves as the primary interface that integrates all LLE components into a cohesive
 * line editing system that can replace linenoise and other line editors.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "line_editor.h"
#include "text_buffer.h"
#include "terminal_manager.h"
#include "display.h"
#include "command_history.h"
#include "theme_integration.h"
#include "completion.h"
#include "undo.h"
#include "input_handler.h"
#include "edit_commands.h"
#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Default configuration values
 */
#define LLE_DEFAULT_MAX_HISTORY 1000
#define LLE_DEFAULT_MAX_UNDO_ACTIONS 100

/**
 * @brief Internal line editor state structure
 * 
 * This extends the public structure with additional internal state
 * needed for proper operation but not exposed to users.
 */
typedef struct lle_line_editor_internal {
    lle_line_editor_t public;           /**< Public interface */
    lle_error_t last_error;             /**< Last error that occurred */
    bool cleanup_needed;                /**< Whether cleanup is needed */
} lle_line_editor_internal_t;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Set the last error for an editor instance
 *
 * @param editor Line editor instance
 * @param error Error code to set
 */
static void lle_set_last_error(lle_line_editor_t *editor, lle_error_t error) {
    if (!editor) return;
    
    lle_line_editor_internal_t *internal = (lle_line_editor_internal_t *)editor;
    internal->last_error = error;
}

/**
 * @brief Initialize default configuration
 *
 * @param config Configuration structure to initialize
 */
static void lle_init_default_config(lle_config_t *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(lle_config_t));
    config->max_history_size = LLE_DEFAULT_MAX_HISTORY;
    config->max_undo_actions = LLE_DEFAULT_MAX_UNDO_ACTIONS;
    config->enable_multiline = true;
    config->enable_syntax_highlighting = true;
    config->enable_auto_completion = true;
    config->enable_history = true;
    config->enable_undo = true;
}

/**
 * @brief Initialize all editor components
 *
 * @param editor Line editor instance to initialize
 * @param config Configuration to use
 * @return true on success, false on failure
 */
static bool lle_initialize_components(lle_line_editor_t *editor, const lle_config_t *config) {
    if (!editor || !config) return false;
    
    // Initialize text buffer
    editor->buffer = lle_text_buffer_create(256);
    if (!editor->buffer) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    // Initialize terminal manager (stack-allocated, then initialized)
    editor->terminal = malloc(sizeof(lle_terminal_manager_t));
    if (!editor->terminal) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    lle_terminal_init_result_t term_result = lle_terminal_init(editor->terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS) {
        free(editor->terminal);
        editor->terminal = NULL;
        lle_set_last_error(editor, LLE_ERROR_TERMINAL_INIT);
        return false;
    }
    
    // Initialize display system (needs a prompt, we'll create a minimal one)
    lle_prompt_t *temp_prompt = lle_prompt_create(64);
    if (!temp_prompt) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    // Parse an empty prompt to initialize it
    if (!lle_prompt_parse(temp_prompt, "")) {
        lle_prompt_destroy(temp_prompt);
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    editor->display = lle_display_create(temp_prompt, editor->buffer, editor->terminal);
    lle_prompt_destroy(temp_prompt);  // Clean up temporary prompt
    
    if (!editor->display) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    // Initialize history if enabled
    if (config->enable_history) {
        editor->history = lle_history_create(config->max_history_size, false);
        if (!editor->history) {
            lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
            return false;
        }
    }
    
    // Initialize theme integration (stack-allocated, then initialized)
    editor->theme = malloc(sizeof(lle_theme_integration_t));
    if (!editor->theme) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    if (!lle_theme_init(editor->theme)) {
        free(editor->theme);
        editor->theme = NULL;
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return false;
    }
    
    // Initialize completion system if enabled
    if (config->enable_auto_completion) {
        editor->completions = lle_completion_list_create(16);  // Initial capacity
        if (!editor->completions) {
            lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
            return false;
        }
    }
    
    // Initialize undo system if enabled
    if (config->enable_undo) {
        editor->undo_stack = lle_undo_stack_create();
        if (!editor->undo_stack) {
            lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
            return false;
        }
    }
    
    // Set configuration flags
    editor->multiline_mode = config->enable_multiline;
    editor->syntax_highlighting = config->enable_syntax_highlighting;
    editor->auto_completion = config->enable_auto_completion;
    editor->history_enabled = config->enable_history;
    editor->undo_enabled = config->enable_undo;
    editor->max_history_size = config->max_history_size;
    editor->max_undo_actions = config->max_undo_actions;
    
    editor->initialized = true;
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

/**
 * @brief Clean up all editor components
 *
 * @param editor Line editor instance to clean up
 */
static void lle_cleanup_components(lle_line_editor_t *editor) {
    if (!editor) return;
    
    // Clean up in reverse order of initialization
    if (editor->undo_stack) {
        lle_undo_stack_destroy(editor->undo_stack);
        editor->undo_stack = NULL;
    }
    
    if (editor->completions) {
        lle_completion_list_destroy(editor->completions);
        editor->completions = NULL;
    }
    
    if (editor->theme) {
        lle_theme_cleanup(editor->theme);
        free(editor->theme);
        editor->theme = NULL;
    }
    
    if (editor->history) {
        lle_history_destroy(editor->history);
        editor->history = NULL;
    }
    
    if (editor->display) {
        lle_display_destroy(editor->display);
        editor->display = NULL;
    }
    
    if (editor->terminal) {
        lle_terminal_cleanup(editor->terminal);
        free(editor->terminal);
        editor->terminal = NULL;
    }
    
    if (editor->buffer) {
        lle_text_buffer_destroy(editor->buffer);
        editor->buffer = NULL;
    }
    
    if (editor->current_prompt) {
        free(editor->current_prompt);
        editor->current_prompt = NULL;
    }
    
    editor->initialized = false;
}

// ============================================================================
// Core API Functions
// ============================================================================

lle_line_editor_t *lle_create(void) {
    lle_config_t config;
    lle_init_default_config(&config);
    return lle_create_with_config(&config);
}

lle_line_editor_t *lle_create_with_config(const lle_config_t *config) {
    // Use default config if none provided
    lle_config_t default_config;
    if (!config) {
        lle_init_default_config(&default_config);
        config = &default_config;
    }
    
    // Allocate internal structure
    lle_line_editor_internal_t *internal = malloc(sizeof(lle_line_editor_internal_t));
    if (!internal) {
        return NULL;
    }
    
    // Initialize structure
    memset(internal, 0, sizeof(lle_line_editor_internal_t));
    internal->last_error = LLE_SUCCESS;
    
    lle_line_editor_t *editor = &internal->public;
    
    // Initialize components
    if (!lle_initialize_components(editor, config)) {
        // Cleanup on failure
        lle_cleanup_components(editor);
        free(internal);
        return NULL;
    }
    
    internal->cleanup_needed = true;
    return editor;
}

void lle_destroy(lle_line_editor_t *editor) {
    if (!editor) return;
    
    lle_line_editor_internal_t *internal = (lle_line_editor_internal_t *)editor;
    
    if (internal->cleanup_needed) {
        lle_cleanup_components(editor);
        internal->cleanup_needed = false;
    }
    
    free(internal);
}

char *lle_readline(lle_line_editor_t *editor, const char *prompt) {
    if (!editor || !prompt) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return NULL;
    }
    
    // TODO: This is a minimal implementation for LLE-038 (API definition)
    // Full implementation will be provided in LLE-039 (Line Editor Implementation)
    
    // For now, return NULL to indicate not yet implemented
    // This allows the API to compile and link for testing purposes
    lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
    return NULL;
}

bool lle_add_history(lle_line_editor_t *editor, const char *line) {
    if (!editor || !line) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_INVALID_PARAMETER);
        return false;
    }
    
    if (!editor->initialized || !editor->history_enabled || !editor->history) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    // Skip empty lines
    if (strlen(line) == 0 || strspn(line, " \t\n\r") == strlen(line)) {
        return false;
    }
    
    // Add to history
    bool result = lle_history_add(editor->history, line, false);
    if (!result) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
    } else {
        lle_set_last_error(editor, LLE_SUCCESS);
    }
    
    return result;
}

// ============================================================================
// Configuration and Status Functions
// ============================================================================

lle_error_t lle_get_last_error(lle_line_editor_t *editor) {
    if (!editor) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    lle_line_editor_internal_t *internal = (lle_line_editor_internal_t *)editor;
    return internal->last_error;
}

bool lle_is_initialized(lle_line_editor_t *editor) {
    if (!editor) return false;
    return editor->initialized;
}

bool lle_get_config(lle_line_editor_t *editor, lle_config_t *config) {
    if (!editor || !config) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_INVALID_PARAMETER);
        return false;
    }
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    // Fill configuration from current state
    config->max_history_size = editor->max_history_size;
    config->max_undo_actions = editor->max_undo_actions;
    config->enable_multiline = editor->multiline_mode;
    config->enable_syntax_highlighting = editor->syntax_highlighting;
    config->enable_auto_completion = editor->auto_completion;
    config->enable_history = editor->history_enabled;
    config->enable_undo = editor->undo_enabled;
    
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

// ============================================================================
// Feature Control Functions
// ============================================================================

bool lle_set_multiline_mode(lle_line_editor_t *editor, bool enable) {
    if (!editor) return false;
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    editor->multiline_mode = enable;
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

bool lle_set_syntax_highlighting(lle_line_editor_t *editor, bool enable) {
    if (!editor) return false;
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    editor->syntax_highlighting = enable;
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

bool lle_set_auto_completion(lle_line_editor_t *editor, bool enable) {
    if (!editor) return false;
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    editor->auto_completion = enable;
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

bool lle_set_history_enabled(lle_line_editor_t *editor, bool enable) {
    if (!editor) return false;
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    editor->history_enabled = enable;
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

bool lle_set_undo_enabled(lle_line_editor_t *editor, bool enable) {
    if (!editor) return false;
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    editor->undo_enabled = enable;
    lle_set_last_error(editor, LLE_SUCCESS);
    return true;
}

// ============================================================================
// History Management Functions
// ============================================================================

bool lle_clear_history(lle_line_editor_t *editor) {
    if (!editor) return false;
    
    if (!editor->initialized || !editor->history_enabled || !editor->history) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    bool result = lle_history_clear(editor->history);
    if (!result) {
        lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
    } else {
        lle_set_last_error(editor, LLE_SUCCESS);
    }
    
    return result;
}

size_t lle_get_history_count(lle_line_editor_t *editor) {
    if (!editor || !editor->initialized || !editor->history_enabled || !editor->history) {
        return 0;
    }
    
    lle_history_stats_t stats;
    if (lle_history_get_stats(editor->history, &stats)) {
        return stats.total_entries;
    }
    return 0;
}

bool lle_load_history(lle_line_editor_t *editor, const char *filename) {
    if (!editor || !filename) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_INVALID_PARAMETER);
        return false;
    }
    
    if (!editor->initialized || !editor->history_enabled || !editor->history) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    bool result = lle_history_load(editor->history, filename, false);
    if (!result) {
        lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
    } else {
        lle_set_last_error(editor, LLE_SUCCESS);
    }
    
    return result;
}

bool lle_save_history(lle_line_editor_t *editor, const char *filename) {
    if (!editor || !filename) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_INVALID_PARAMETER);
        return false;
    }
    
    if (!editor->initialized || !editor->history_enabled || !editor->history) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        return false;
    }
    
    bool result = lle_history_save(editor->history, filename);
    if (!result) {
        lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
    } else {
        lle_set_last_error(editor, LLE_SUCCESS);
    }
    
    return result;
}
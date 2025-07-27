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
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
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
    
    // Clear text buffer for new input
    lle_text_buffer_clear(editor->buffer);
    
    // Store current prompt
    if (editor->current_prompt) {
        free(editor->current_prompt);
    }
    editor->current_prompt = strdup(prompt);
    if (!editor->current_prompt) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    // Create and parse prompt
    lle_prompt_t *prompt_obj = lle_prompt_create(strlen(prompt) + 64);
    if (!prompt_obj) {
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    if (!lle_prompt_parse(prompt_obj, prompt)) {
        lle_prompt_destroy(prompt_obj);
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    // Store old prompt to restore on error
    lle_prompt_t *old_prompt = editor->display->prompt;
    editor->display->prompt = prompt_obj;
    
    // Initial display render
    if (!lle_display_render(editor->display)) {
        // Restore old prompt and clean up new one
        editor->display->prompt = old_prompt;
        lle_prompt_destroy(prompt_obj);
        lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
        return NULL;
    }
    
    // Main input loop
    lle_key_event_t event;
    bool line_complete = false;
    bool line_cancelled = false;
    char *result = NULL;
    
    while (!line_complete && !line_cancelled) {
        // Read key event
        if (!lle_input_read_key(editor->terminal, &event)) {
            // Error reading input
            lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
            break;
        }
        
        // Process key event
        lle_command_result_t cmd_result = LLE_CMD_SUCCESS;
        bool needs_display_update = true;
        
        switch (event.type) {
            case LLE_KEY_ENTER:
            case LLE_KEY_CTRL_M:
            case LLE_KEY_CTRL_J:
                // Accept line
                result = malloc(editor->buffer->length + 1);
                if (result) {
                    memcpy(result, editor->buffer->buffer, editor->buffer->length);
                    result[editor->buffer->length] = '\0';
                    line_complete = true;
                } else {
                    lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
                }
                break;
                
            case LLE_KEY_CTRL_C:
                // Cancel line
                line_cancelled = true;
                break;
                
            case LLE_KEY_CTRL_D:
                // EOF or delete character
                if (editor->buffer->length == 0) {
                    line_cancelled = true;
                } else {
                    cmd_result = lle_cmd_delete_char(editor->display);
                }
                break;
                
            case LLE_KEY_BACKSPACE:
            case LLE_KEY_CTRL_H:
                cmd_result = lle_cmd_backspace(editor->display);
                break;
                
            case LLE_KEY_DELETE:
                cmd_result = lle_cmd_delete_char(editor->display);
                break;
                
            case LLE_KEY_ARROW_LEFT:
            case LLE_KEY_CTRL_B:
                cmd_result = lle_cmd_move_cursor(editor->display, LLE_CMD_CURSOR_LEFT, 1);
                break;
                
            case LLE_KEY_ARROW_RIGHT:
            case LLE_KEY_CTRL_F:
                cmd_result = lle_cmd_move_cursor(editor->display, LLE_CMD_CURSOR_RIGHT, 1);
                break;
                
            case LLE_KEY_HOME:
            case LLE_KEY_CTRL_A:
                cmd_result = lle_cmd_move_home(editor->display);
                break;
                
            case LLE_KEY_END:
            case LLE_KEY_CTRL_E:
                cmd_result = lle_cmd_move_end(editor->display);
                break;
                
            case LLE_KEY_ARROW_UP:
            case LLE_KEY_CTRL_P:
                // History previous
                if (editor->history_enabled && editor->history) {
                    const lle_history_entry_t *entry = lle_history_navigate(editor->history, LLE_HISTORY_PREV);
                    if (entry && entry->command) {
                        lle_text_buffer_clear(editor->buffer);
                        for (size_t i = 0; i < entry->length; i++) {
                            lle_text_insert_char(editor->buffer, entry->command[i]);
                        }
                        lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                    }
                }
                break;
                
            case LLE_KEY_ARROW_DOWN:
            case LLE_KEY_CTRL_N:
                // History next
                if (editor->history_enabled && editor->history) {
                    const lle_history_entry_t *entry = lle_history_navigate(editor->history, LLE_HISTORY_NEXT);
                    if (entry && entry->command) {
                        lle_text_buffer_clear(editor->buffer);
                        for (size_t i = 0; i < entry->length; i++) {
                            lle_text_insert_char(editor->buffer, entry->command[i]);
                        }
                        lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                    } else {
                        // No next line, clear buffer
                        lle_text_buffer_clear(editor->buffer);
                    }
                }
                break;
                
            case LLE_KEY_CTRL_K:
                cmd_result = lle_cmd_kill_line(editor->display);
                break;
                
            case LLE_KEY_CTRL_U:
                cmd_result = lle_cmd_kill_beginning(editor->display);
                break;
                
            case LLE_KEY_CTRL_W:
                cmd_result = lle_cmd_backspace_word(editor->display);
                break;
                
            case LLE_KEY_ALT_D:
                cmd_result = lle_cmd_delete_word(editor->display);
                break;
                
            case LLE_KEY_ALT_B:
                cmd_result = lle_cmd_word_left(editor->display);
                break;
                
            case LLE_KEY_ALT_F:
                cmd_result = lle_cmd_word_right(editor->display);
                break;
                
            case LLE_KEY_CTRL_L:
                // Clear screen and redraw
                lle_terminal_clear_screen(editor->terminal);
                cmd_result = LLE_CMD_SUCCESS;
                break;
                
            case LLE_KEY_CTRL_Z:
                // Undo (if enabled)
                if (editor->undo_enabled && editor->undo_stack) {
                    if (lle_undo_can_undo(editor->undo_stack)) {
                        lle_undo_execute(editor->undo_stack, editor->buffer);
                    }
                }
                break;
                
            case LLE_KEY_CTRL_Y:
                // Redo (if enabled)
                if (editor->undo_enabled && editor->undo_stack) {
                    if (lle_redo_can_redo(editor->undo_stack)) {
                        lle_redo_execute(editor->undo_stack, editor->buffer);
                    }
                }
                break;
                
            case LLE_KEY_TAB:
                // Tab completion (if enabled)
                if (editor->auto_completion && editor->completions) {
                    // TODO: Implement tab completion in future task
                    needs_display_update = false;
                }
                break;
                
            case LLE_KEY_CHAR:
                // Insert regular character
                if (event.character >= 32 && event.character <= 126) {
                    cmd_result = lle_cmd_insert_char(editor->display, event.character);
                }
                break;
                
            default:
                // Unknown or unhandled key
                needs_display_update = false;
                break;
        }
        
        // Update display if needed
        if (needs_display_update && cmd_result != LLE_CMD_ERROR_DISPLAY_UPDATE) {
            lle_display_render(editor->display);
        }
    }
    
    // Clean up: restore original prompt and destroy the one we created
    if (old_prompt) {
        editor->display->prompt = old_prompt;
        lle_prompt_destroy(prompt_obj);
    } else {
        // If there was no old prompt, clear the display prompt
        editor->display->prompt = NULL;
        lle_prompt_destroy(prompt_obj);
    }
    
    // Add to history if we have a result and history is enabled
    if (result && editor->history_enabled && editor->history) {
        // Only add non-empty, non-whitespace lines
        bool add_to_history = false;
        for (size_t i = 0; result[i]; i++) {
            if (result[i] != ' ' && result[i] != '\t' && result[i] != '\n' && result[i] != '\r') {
                add_to_history = true;
                break;
            }
        }
        if (add_to_history) {
            lle_history_add(editor->history, result, false);
        }
    }
    
    // Set appropriate error status
    if (line_cancelled) {
        lle_set_last_error(editor, LLE_ERROR_INTERRUPTED);
        if (result) {
            free(result);
            result = NULL;
        }
    } else if (result) {
        lle_set_last_error(editor, LLE_SUCCESS);
    }
    
    return result;
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
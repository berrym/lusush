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
#include "syntax.h"
#include "buffer_trace.h"
#include <unistd.h>
#include "edit_commands.h"
#include "enhanced_tab_completion.h"
#include "display_stabilization.h"
#include "platform_detection.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

/**
 * @brief ASCII codes for control characters
 */
#define LLE_ASCII_CTRL_UNDERSCORE 0x1F  // Ctrl+_ (undo)
#define LLE_ASCII_CTRL_BACKSLASH 0x1C   // Ctrl+\ (SIGQUIT)
#define LLE_ASCII_CTRL_S 0x13           // Ctrl+S (XOFF/stop)
#define LLE_ASCII_CTRL_Q 0x11           // Ctrl+Q (XON/start)
#define LLE_ASCII_CTRL_G 0x07           // Ctrl+G (abort/cancel line)

/**
 * @brief Default configuration values
 */
#define LLE_DEFAULT_MAX_HISTORY 1000
#define LLE_DEFAULT_MAX_UNDO_ACTIONS 100

/**
 * @brief Reverse search state management
 */
static int reverse_search_mode = 0;
static char reverse_search_query[256] = {0};
static int reverse_search_index = -1;
static char *reverse_search_original_line = NULL;
static int reverse_search_start_index = -1;

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
    
    // Initialize buffer tracing system for debugging
    if (!lle_trace_init()) {
        // Continue even if tracing fails - it's optional
        // Tracing is controlled by LLE_TRACE_ENABLED environment variable
    }
    
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
        
        // Initialize enhanced tab completion system
        if (!lle_enhanced_tab_completion_init()) {
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
    
    // Initialize syntax highlighter if syntax highlighting is enabled
    lle_syntax_highlighter_t *syntax_highlighter = NULL;
    if (config->enable_syntax_highlighting) {
        syntax_highlighter = lle_syntax_create();
        if (syntax_highlighter) {
            lle_syntax_init(syntax_highlighter, true, 10000); // Enable shell syntax, max 10k chars
            lle_syntax_configure_shell(syntax_highlighter, true);
            lle_syntax_configure_strings(syntax_highlighter, true);
            lle_syntax_configure_variables(syntax_highlighter, true);
            lle_syntax_configure_comments(syntax_highlighter, true);
        }
    }
    
    // Connect syntax highlighter and theme to display system
    if (editor->display) {
        if (syntax_highlighter) {
            lle_display_set_syntax_highlighter(editor->display, syntax_highlighter);
        }
        if (editor->theme) {
            lle_display_set_theme_integration(editor->display, editor->theme);
        }
        lle_display_enable_syntax_highlighting(editor->display, config->enable_syntax_highlighting);
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
    
    // Clean up syntax highlighter if attached to display
    if (editor->display) {
        lle_syntax_highlighter_t *syntax_highlighter = editor->display->syntax_highlighter;
        if (syntax_highlighter) {
            lle_syntax_destroy(syntax_highlighter);
            lle_display_set_syntax_highlighter(editor->display, NULL);
        }
    }
    
    if (editor->undo_stack) {
        lle_undo_stack_destroy(editor->undo_stack);
        editor->undo_stack = NULL;
    }
    
    // Shutdown tracing system
    lle_trace_shutdown();
    
    if (editor->completions) {
        // Cleanup enhanced tab completion system
        lle_enhanced_tab_completion_cleanup();
        
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
// Input Event Loop Implementation
// ============================================================================

/**
 * @brief Main input processing loop
 *
 * This function implements the core input event loop that reads key events,
 * processes them, and manages the editing state until a line is complete
 * or cancelled.
 *
 * @param editor Line editor instance (must be initialized)
 * @return Completed input string on success, NULL on cancellation or error
 *
 * @note The returned string must be freed by the caller
 * @note Sets appropriate error codes via lle_set_last_error()
 */
static char *lle_input_loop(lle_line_editor_t *editor) {
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INPUT_LOOP] Starting input loop with editor=%p\n", (void*)editor);
    }
    
    if (!editor || !editor->initialized) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_LOOP] Editor invalid or not initialized\n");
        }
        return NULL;
    }
    
    lle_key_event_t event;
    bool line_complete = false;
    bool line_cancelled = false;
    char *result = NULL;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INPUT_LOOP] Starting main input processing loop\n");
    }
    
    // Main input processing loop
    while (!line_complete && !line_cancelled) {
        // Read key event from terminal
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_LOOP] About to read key event\n");
        }
        
        if (!lle_input_read_key(editor->terminal, &event)) {
            // Error reading input - could be EOF or terminal error
            if (debug_mode) {
                fprintf(stderr, "[LLE_INPUT_LOOP] lle_input_read_key failed - setting IO_ERROR\n");
            }
            lle_set_last_error(editor, LLE_ERROR_IO_ERROR);
            break;
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_LOOP] Read key event type: %d\n", event.type);
        }
        
        // Process key event and determine response
        lle_command_result_t cmd_result = LLE_CMD_SUCCESS;
        bool needs_display_update = true;
        size_t prompt_width; // Declare once for reuse in keybinding handlers
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_LOOP] Processing key event type: %d, character: 0x%02x\n", 
                    event.type, event.character);
            fprintf(stderr, "[LLE_INPUT_LOOP] About to enter switch statement with type %d\n", event.type);
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_LOOP] ENTERING SWITCH STATEMENT with event.type=%d\n", event.type);
        }
        
        switch (event.type) {
            case LLE_KEY_ENTER:
            case LLE_KEY_CTRL_M:
            case LLE_KEY_CTRL_J:
                if (reverse_search_mode) {
                    // Accept current match and exit reverse search
                    reverse_search_mode = 0;
                    if (reverse_search_original_line) {
                        free(reverse_search_original_line);
                        reverse_search_original_line = NULL;
                    }
                    reverse_search_query[0] = '\0';
                    reverse_search_index = -1;
                    
                    // Clear search line and restore prompt
                    lle_terminal_write(editor->terminal, "\r", 1);
                    lle_terminal_clear_to_eol(editor->terminal);
                    lle_terminal_move_cursor_up(editor->terminal, 1);
                    
                    // Redraw the accepted command
                    prompt_width = lle_prompt_get_last_line_width(editor->display->prompt);
                    lle_terminal_move_cursor_to_column(editor->terminal, prompt_width);
                    lle_terminal_clear_to_eol(editor->terminal);
                    if (editor->buffer->length > 0) {
                        lle_terminal_write(editor->terminal, editor->buffer->buffer, editor->buffer->length);
                    }
                    needs_display_update = false;
                    break;
                }
                
                // Accept current line and complete editing
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Enter key pressed - completing line with %zu characters\n", 
                            editor->buffer->length);
                }
                
                // Write newline and move cursor to beginning of line for command output
                if (!lle_terminal_write(editor->terminal, "\n", 1)) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Failed to write newline after Enter\n");
                    }
                }
                
                // Move cursor to column 0 to ensure command output starts at beginning of line
                if (!lle_terminal_move_cursor_to_column(editor->terminal, 0)) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Failed to move cursor to column 0 after Enter\n");
                    }
                }
                
                result = malloc(editor->buffer->length + 1);
                if (result) {
                    memcpy(result, editor->buffer->buffer, editor->buffer->length);
                    result[editor->buffer->length] = '\0';
                    line_complete = true;
                    lle_set_last_error(editor, LLE_SUCCESS);
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Line completed successfully: '%s'\n", result);
                    }
                } else {
                    lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
                    line_cancelled = true;
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Memory allocation failed for result\n");
                    }
                }
                break;
                
            case LLE_KEY_CTRL_C:
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] CTRL+C case executed\n");
                }
                // SIGNAL: Ctrl+C should generate SIGINT - let shell handle it
                // Do not intercept this - let it pass through for signal handling
                needs_display_update = false;
                break;
                
            case LLE_KEY_CTRL_D:
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] CTRL+D case executed\n");
                }
                // EOF: Standard behavior - EOF if buffer empty, delete char otherwise
                if (editor->buffer->length == 0) {
                    line_cancelled = true;  // EOF - exit
                    lle_set_last_error(editor, LLE_ERROR_INTERRUPTED);
                } else {
                    cmd_result = lle_cmd_delete_char(editor->display);
                }
                break;
                
            case LLE_KEY_BACKSPACE:
            case LLE_KEY_CTRL_H:
                if (reverse_search_mode) {
                    // Remove character from search query
                    size_t query_len = strlen(reverse_search_query);
                    if (query_len > 0) {
                        reverse_search_query[query_len - 1] = '\0';
                        
                        // Re-search with shortened term
                        if (query_len > 1 && editor->history_enabled && editor->history) {
                            for (int i = editor->history->count - 1; i >= 0; i--) {
                                const lle_history_entry_t *entry = lle_history_get(editor->history, i);
                                if (entry && entry->command && strstr(entry->command, reverse_search_query)) {
                                    lle_text_buffer_clear(editor->buffer);
                                    for (size_t j = 0; j < entry->length; j++) {
                                        lle_text_insert_char(editor->buffer, entry->command[j]);
                                    }
                                    lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                    reverse_search_index = i;
                                    break;
                                }
                            }
                        } else {
                            // No more query or no match - clear buffer
                            lle_text_buffer_clear(editor->buffer);
                            reverse_search_index = -1;
                        }
                        
                        // Update search prompt
                        lle_terminal_write(editor->terminal, "\r", 1);
                        lle_terminal_move_cursor_to_column(editor->terminal, 0);
                        lle_terminal_clear_to_eol(editor->terminal);
                        lle_terminal_write(editor->terminal, "(reverse-i-search)`", 19);
                        if (strlen(reverse_search_query) > 0) {
                            lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                        }
                        lle_terminal_write(editor->terminal, "': ", 3);
                        if (editor->buffer->length > 0) {
                            lle_terminal_write(editor->terminal, editor->buffer->buffer, editor->buffer->length);
                        }
                    }
                    needs_display_update = false;
                } else {
                    // Standard: Ctrl+H is backspace in most terminals
                    LLE_TRACE_CRITICAL("INPUT_LOOP_BACKSPACE_START", editor->buffer);
                    cmd_result = lle_cmd_backspace(editor->display);
                    LLE_TRACE_CRITICAL("INPUT_LOOP_BACKSPACE_END", editor->buffer);
                    needs_display_update = false; // Phase 2B.5: Command handles its own display update
                }
                break;
                
            case LLE_KEY_DELETE:
                cmd_result = lle_cmd_delete_char(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_ARROW_LEFT:
            case LLE_KEY_CTRL_B:
                cmd_result = lle_cmd_move_cursor(editor->display, LLE_CMD_CURSOR_LEFT, 1);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_ARROW_RIGHT:
            case LLE_KEY_CTRL_F:
                cmd_result = lle_cmd_move_cursor(editor->display, LLE_CMD_CURSOR_RIGHT, 1);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_HOME:
                cmd_result = lle_cmd_move_home(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Home key pressed\n");
                }
                break;
                
            case LLE_KEY_CTRL_A:
                cmd_result = lle_cmd_move_home(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+A (Home) pressed\n");
                }
                break;
                
            case LLE_KEY_END:
                cmd_result = lle_cmd_move_end(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] End key pressed\n");
                }
                break;
                
            case LLE_KEY_CTRL_E:
                cmd_result = lle_cmd_move_end(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+E (End) pressed\n");
                }
                break;
                
            case LLE_KEY_ARROW_UP:
            case LLE_KEY_CTRL_P:
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Arrow UP pressed - entering history navigation\n");
                }
                if (reverse_search_mode) {
                    // In search mode: navigate to previous match in history (older)
                    if (strlen(reverse_search_query) > 0 && editor->history_enabled && editor->history) {
                        int start_index = (reverse_search_index > 0) ? reverse_search_index - 1 : editor->history->count - 1;
                        
                        for (int i = start_index; i >= 0; i--) {
                            const lle_history_entry_t *entry = lle_history_get(editor->history, i);
                            if (entry && entry->command && strstr(entry->command, reverse_search_query)) {
                                reverse_search_index = i;
                                
                                // Update buffer with match
                                lle_text_buffer_clear(editor->buffer);
                                for (size_t j = 0; j < entry->length; j++) {
                                    lle_text_insert_char(editor->buffer, entry->command[j]);
                                }
                                lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                
                                // Update search prompt
                                lle_terminal_write(editor->terminal, "\r", 1);
                                lle_terminal_move_cursor_to_column(editor->terminal, 0);
                                lle_terminal_clear_to_eol(editor->terminal);
                                lle_terminal_write(editor->terminal, "(reverse-i-search)`", 19);
                                lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                                lle_terminal_write(editor->terminal, "': ", 3);
                                lle_terminal_write(editor->terminal, entry->command, entry->length);
                                break;
                            }
                        }
                    }
                } else {
                    // Normal history navigation
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Normal history navigation UP - history_enabled=%s, history=%p\n", 
                                editor->history_enabled ? "true" : "false", (void*)editor->history);
                    }
                    if (editor->history_enabled && editor->history) {
                        const lle_history_entry_t *entry = lle_history_navigate(editor->history, LLE_HISTORY_PREV);
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_INPUT_LOOP] lle_history_navigate returned entry=%p\n", (void*)entry);
                        }
                        if (entry && entry->command) {
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] History UP: Platform-aware exact backspace replication\n");
                                fprintf(stderr, "[LLE_INPUT_LOOP] Current buffer length: %zu, new content: %.20s...\n", 
                                        editor->buffer->length, entry->command);
                            }
                            
                            // 🎯 LINUX-FIXED HISTORY NAVIGATION: Simple cursor positioning approach
                            // Initialize platform detection for optimal sequences
                            if (!lle_platform_init()) {
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Platform detection failed, using fallback\n");
                                }
                            }
                            
                            if (debug_mode) {
                                const char *platform_desc = "Unknown";
                                if (lle_platform_is_macos()) platform_desc = "macOS";
                                else if (lle_platform_is_linux()) platform_desc = "Linux";
                                
                                fprintf(stderr, "[LLE_INPUT_LOOP] Platform: %s, clearing current line for history navigation\n", platform_desc);
                                fprintf(stderr, "[LLE_INPUT_LOOP] Current buffer length: %zu, new content length: %zu\n", 
                                       editor->buffer->length, entry->length);
                            }
                            
                            // LINUX FIX: Proper multi-line aware clearing for Linux terminals
                            if (lle_platform_is_linux()) {
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Using multi-line aware history navigation\n");
                                }
                                
                                // Step 1: Calculate current content dimensions
                                size_t current_length = editor->buffer->length;
                                size_t prompt_width = editor->display->prompt ? lle_prompt_get_last_line_width(editor->display->prompt) : 0;
                                size_t terminal_width = editor->display->geometry.width;
                                
                                // Step 2: Calculate how many lines current content occupies
                                size_t total_chars = prompt_width + current_length;
                                size_t lines_used = (total_chars / terminal_width) + 1;
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Current content uses %zu lines (prompt=%zu, content=%zu, total=%zu, width=%zu)\n", 
                                           lines_used, prompt_width, current_length, total_chars, terminal_width);
                                }
                                
                                // Step 3: Move to beginning of current line and clear all used lines
                                lle_terminal_write(editor->display->terminal, "\r", 1);  // Move to start of current line
                                
                                // Clear current line and any additional lines used by wrapped content
                                for (size_t i = 0; i < lines_used; i++) {
                                    lle_terminal_write(editor->display->terminal, "\x1b[K", 3);  // Clear to end of line
                                    if (i < lines_used - 1) {
                                        lle_terminal_write(editor->display->terminal, "\x1b[B", 3);  // Move down one line
                                    }
                                }
                                
                                // Step 4: Move back to start position
                                if (lines_used > 1) {
                                    char move_up[16];
                                    snprintf(move_up, sizeof(move_up), "\x1b[%zuA", lines_used - 1);
                                    lle_terminal_write(editor->display->terminal, move_up, strlen(move_up));
                                }
                                
                                // Step 5: Rewrite prompt
                                if (editor->display->prompt && editor->display->prompt->lines && editor->display->prompt->lines[0]) {
                                    lle_terminal_write(editor->display->terminal, editor->display->prompt->lines[0], strlen(editor->display->prompt->lines[0]));
                                }
                                
                                // Step 6: Write new history content
                                lle_terminal_write(editor->display->terminal, entry->command, entry->length);
                                
                                // Step 7: Update buffer state to match terminal
                                lle_text_buffer_clear(editor->buffer);
                                for (size_t i = 0; i < entry->length; i++) {
                                    lle_text_insert_char(editor->buffer, entry->command[i]);
                                }
                                lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Multi-line history navigation complete\n");
                                }
                                
                                cmd_result = LLE_CMD_SUCCESS;
                                needs_display_update = false;  // Skip display system updates
                            } else {
                                // macOS: Use proven working approach
                                lle_cmd_move_end(editor->display);
                                
                                size_t text_length = editor->buffer->length;
                                size_t backspace_count = text_length > 0 ? text_length - 1 : 0;
                                
                                const char *backspace_seq = lle_platform_get_backspace_sequence();
                                size_t backspace_seq_len = lle_platform_get_backspace_length();
                                
                                for (size_t i = 0; i < backspace_count; i++) {
                                    lle_terminal_write(editor->display->terminal, backspace_seq, backspace_seq_len);
                                }
                                
                                lle_terminal_clear_to_eol(editor->display->terminal);
                                
                                // Update buffer state to match cleared content
                                editor->buffer->length = 0;
                                editor->buffer->cursor_pos = 0;
                                
                                // Phase 3: Insert new content character by character (macOS only)
                                for (size_t i = 0; i < entry->length; i++) {
                                    lle_cmd_insert_char(editor->display, entry->command[i]);
                                }
                                
                                cmd_result = LLE_CMD_SUCCESS;
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] macOS: Used proven exact backspace approach\n");
                                }
                            }
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] History UP: exact backspace replication complete\n");
                            }
                        } else {
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] No history entry found, clearing content\n");
                            }
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] No history entry found - clearing current line\n");
                            }
                            
                            // LINUX FIX: Multi-line aware clearing when no history entry
                            if (lle_platform_is_linux()) {
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Multi-line clearing for no history entry\n");
                                }
                                
                                // Calculate current content dimensions
                                size_t current_length = editor->buffer->length;
                                size_t prompt_width = editor->display->prompt ? lle_prompt_get_last_line_width(editor->display->prompt) : 0;
                                size_t terminal_width = editor->display->geometry.width;
                                
                                // Calculate how many lines current content occupies
                                size_t total_chars = prompt_width + current_length;
                                size_t lines_used = (total_chars / terminal_width) + 1;
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Clearing %zu lines (prompt=%zu, content=%zu, total=%zu)\n", 
                                           lines_used, prompt_width, current_length, total_chars);
                                }
                                
                                // Move to beginning of current line and clear all used lines
                                lle_terminal_write(editor->display->terminal, "\r", 1);
                                
                                // Clear current line and any additional lines used by wrapped content
                                for (size_t i = 0; i < lines_used; i++) {
                                    lle_terminal_write(editor->display->terminal, "\x1b[K", 3);  // Clear to end of line
                                    if (i < lines_used - 1) {
                                        lle_terminal_write(editor->display->terminal, "\x1b[B", 3);  // Move down one line
                                    }
                                }
                                
                                // Move back to start position and rewrite prompt
                                if (lines_used > 1) {
                                    char move_up[16];
                                    snprintf(move_up, sizeof(move_up), "\x1b[%zuA", lines_used - 1);
                                    lle_terminal_write(editor->display->terminal, move_up, strlen(move_up));
                                }
                                
                                // Rewrite prompt
                                if (editor->display->prompt && editor->display->prompt->lines && editor->display->prompt->lines[0]) {
                                    lle_terminal_write(editor->display->terminal, editor->display->prompt->lines[0], strlen(editor->display->prompt->lines[0]));
                                }
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Multi-line clearing complete\n");
                                }
                            } else {
                                // macOS: Use proven working approach
                                lle_cmd_move_end(editor->display);
                                
                                size_t text_length = editor->buffer->length;
                                size_t backspace_count = text_length > 0 ? text_length - 1 : 0;
                                
                                for (size_t i = 0; i < backspace_count; i++) {
                                    lle_terminal_write(editor->display->terminal, "\b \b", 3);
                                }
                                lle_terminal_clear_to_eol(editor->display->terminal);
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] macOS: Used proven backspace clearing\n");
                                }
                            }
                            
                            // Update buffer state to match cleared content
                            editor->buffer->length = 0;
                            editor->buffer->cursor_pos = 0;
                            cmd_result = LLE_CMD_SUCCESS;
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] Line clearing complete\n");
                            }
                        }
                    } else {
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_INPUT_LOOP] History not enabled or not available\n");
                        }
                    }
                }
                needs_display_update = false; // Commands handle their own display updates (same as other key inputs)
                break;
                
            case LLE_KEY_ARROW_DOWN:
            case LLE_KEY_CTRL_N:
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Arrow DOWN pressed - entering history navigation\n");
                }
                if (reverse_search_mode) {
                    // In search mode: navigate to next match in history (newer)
                    if (strlen(reverse_search_query) > 0 && editor->history_enabled && editor->history && 
                        reverse_search_index < (int)editor->history->count - 1) {
                        
                        for (int i = reverse_search_index + 1; i < (int)editor->history->count; i++) {
                            const lle_history_entry_t *entry = lle_history_get(editor->history, i);
                            if (entry && entry->command && strstr(entry->command, reverse_search_query)) {
                                reverse_search_index = i;
                                
                                // Update buffer with match
                                lle_text_buffer_clear(editor->buffer);
                                for (size_t j = 0; j < entry->length; j++) {
                                    lle_text_insert_char(editor->buffer, entry->command[j]);
                                }
                                lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                
                                // Update search prompt
                                lle_terminal_write(editor->terminal, "\r", 1);
                                lle_terminal_move_cursor_to_column(editor->terminal, 0);
                                lle_terminal_clear_to_eol(editor->terminal);
                                lle_terminal_write(editor->terminal, "(reverse-i-search)`", 19);
                                lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                                lle_terminal_write(editor->terminal, "': ", 3);
                                lle_terminal_write(editor->terminal, entry->command, entry->length);
                                break;
                            }
                        }
                    }
                } else {
                    // Normal history navigation
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Normal history navigation DOWN - history_enabled=%s, history=%p\n", 
                                editor->history_enabled ? "true" : "false", (void*)editor->history);
                    }
                    if (editor->history_enabled && editor->history) {
                        const lle_history_entry_t *entry = lle_history_navigate(editor->history, LLE_HISTORY_NEXT);
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_INPUT_LOOP] lle_history_navigate returned entry=%p\n", (void*)entry);
                        }
                        
                        if (entry && entry->command) {
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] History DOWN: Linux-fixed history navigation\n");
                                fprintf(stderr, "[LLE_INPUT_LOOP] Current buffer length: %zu, new content: %.20s...\n", 
                                        editor->buffer->length, entry->command);
                            }
                            
                            // 🎯 LINUX-FIXED HISTORY NAVIGATION: Multi-line aware approach
                            // Initialize platform detection for optimal sequences
                            if (!lle_platform_init()) {
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Warning: Platform detection failed\n");
                                }
                            }
                            
                            if (debug_mode) {
                                const char *platform_desc = "Unknown";
                                if (lle_platform_is_macos()) platform_desc = "macOS";
                                else if (lle_platform_is_linux()) platform_desc = "Linux";
                                
                                fprintf(stderr, "[LLE_INPUT_LOOP] Platform: %s, clearing current line for history navigation\n", platform_desc);
                                fprintf(stderr, "[LLE_INPUT_LOOP] Current buffer length: %zu, new content length: %zu\n", 
                                       editor->buffer->length, entry->length);
                            }
                            
                            // LINUX FIX: Proper multi-line aware clearing for Linux terminals
                            if (lle_platform_is_linux()) {
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Using multi-line aware history navigation\n");
                                }
                                
                                // Step 1: Calculate current content dimensions
                                size_t current_length = editor->buffer->length;
                                size_t prompt_width = editor->display->prompt ? lle_prompt_get_last_line_width(editor->display->prompt) : 0;
                                size_t terminal_width = editor->display->geometry.width;
                                
                                // Step 2: Calculate how many lines current content occupies
                                size_t total_chars = prompt_width + current_length;
                                size_t lines_used = (total_chars / terminal_width) + 1;
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Current content uses %zu lines (prompt=%zu, content=%zu, total=%zu, width=%zu)\n", 
                                           lines_used, prompt_width, current_length, total_chars, terminal_width);
                                }
                                
                                // Step 3: Move to beginning of current line and clear all used lines
                                lle_terminal_write(editor->display->terminal, "\r", 1);  // Move to start of current line
                                
                                // Clear current line and any additional lines used by wrapped content
                                for (size_t i = 0; i < lines_used; i++) {
                                    lle_terminal_write(editor->display->terminal, "\x1b[K", 3);  // Clear to end of line
                                    if (i < lines_used - 1) {
                                        lle_terminal_write(editor->display->terminal, "\x1b[B", 3);  // Move down one line
                                    }
                                }
                                
                                // Step 4: Move back to start position
                                if (lines_used > 1) {
                                    char move_up[16];
                                    snprintf(move_up, sizeof(move_up), "\x1b[%zuA", lines_used - 1);
                                    lle_terminal_write(editor->display->terminal, move_up, strlen(move_up));
                                }
                                
                                // Step 5: Rewrite prompt
                                if (editor->display->prompt && editor->display->prompt->lines && editor->display->prompt->lines[0]) {
                                    lle_terminal_write(editor->display->terminal, editor->display->prompt->lines[0], strlen(editor->display->prompt->lines[0]));
                                }
                                
                                // Step 6: Write new history content
                                lle_terminal_write(editor->display->terminal, entry->command, entry->length);
                                
                                // Step 7: Update buffer state to match terminal
                                lle_text_buffer_clear(editor->buffer);
                                for (size_t i = 0; i < entry->length; i++) {
                                    lle_text_insert_char(editor->buffer, entry->command[i]);
                                }
                                lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Multi-line history navigation complete\n");
                                }
                                
                                cmd_result = LLE_CMD_SUCCESS;
                                needs_display_update = false;  // Skip display system updates
                            } else {
                                // macOS: Use proven working approach
                                lle_cmd_move_end(editor->display);
                                
                                size_t text_length = editor->buffer->length;
                                size_t backspace_count = text_length > 0 ? text_length - 1 : 0;
                                
                                const char *backspace_seq = lle_platform_get_backspace_sequence();
                                size_t backspace_seq_len = lle_platform_get_backspace_length();
                                
                                for (size_t i = 0; i < backspace_count; i++) {
                                    lle_terminal_write(editor->display->terminal, backspace_seq, backspace_seq_len);
                                }
                                
                                lle_terminal_clear_to_eol(editor->display->terminal);
                                
                                // Update buffer state to match cleared content
                                editor->buffer->length = 0;
                                editor->buffer->cursor_pos = 0;
                                
                                // Phase 3: Insert new content character by character (macOS only)
                                for (size_t i = 0; i < entry->length; i++) {
                                    lle_cmd_insert_char(editor->display, entry->command[i]);
                                }
                                
                                cmd_result = LLE_CMD_SUCCESS;
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] macOS: Used proven exact backspace approach\n");
                                }
                            }
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] History DOWN: exact backspace replication complete\n");
                            }
                        } else {
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] No history entry found, clearing content\n");
                            }
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] No history entry found - clearing current line\n");
                            }
                            
                            // LINUX FIX: Simple cursor positioning for clearing when no history entry
                            if (lle_platform_is_linux()) {
                                // Move cursor to beginning of input area and clear
                                size_t prompt_width = editor->display->prompt ? lle_prompt_get_last_line_width(editor->display->prompt) : 0;
                                
                                lle_terminal_write(editor->display->terminal, "\r", 1);
                                if (prompt_width > 0) {
                                    char move_cmd[32];
                                    snprintf(move_cmd, sizeof(move_cmd), "\x1b[%zuC", prompt_width);
                                    lle_terminal_write(editor->display->terminal, move_cmd, strlen(move_cmd));
                                }
                                lle_terminal_write(editor->display->terminal, "\x1b[K", 3);
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Cleared line, cursor positioned after prompt\n");
                                }
                            } else {
                                // macOS: Use proven working approach
                                lle_cmd_move_end(editor->display);
                                
                                size_t text_length = editor->buffer->length;
                                size_t backspace_count = text_length > 0 ? text_length - 1 : 0;
                                
                                for (size_t i = 0; i < backspace_count; i++) {
                                    lle_terminal_write(editor->display->terminal, "\b \b", 3);
                                }
                                lle_terminal_clear_to_eol(editor->display->terminal);
                                
                                if (debug_mode) {
                                    fprintf(stderr, "[LLE_INPUT_LOOP] macOS: Used proven backspace clearing\n");
                                }
                            }
                            
                            // Update buffer state to match cleared content
                            editor->buffer->length = 0;
                            editor->buffer->cursor_pos = 0;
                            cmd_result = LLE_CMD_SUCCESS;
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] Line clearing complete\n");
                            }
                        }
                    } else {
                        if (debug_mode) {
                            fprintf(stderr, "[LLE_INPUT_LOOP] History not enabled or not available\n");
                        }
                    }
                }
                needs_display_update = false; // Commands handle their own display updates (same as other key inputs)
                break;
                
            case LLE_KEY_CTRL_K:
                // Standard readline: kill from cursor to end of line
                cmd_result = lle_cmd_kill_line(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_CTRL_U:
                cmd_result = lle_cmd_clear_line(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+U (Clear line) pressed\n");
                }
                break;
                
            case LLE_KEY_CTRL_W:
                // Standard readline: kill word backward
                cmd_result = lle_cmd_backspace_word(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_ALT_D:
                cmd_result = lle_cmd_delete_word(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_ALT_B:
                cmd_result = lle_cmd_word_left(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_ALT_F:
                cmd_result = lle_cmd_word_right(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                break;
                
            case LLE_KEY_CTRL_L:
                if (reverse_search_mode) {
                    // Clear screen and redraw search interface
                    lle_terminal_clear_screen(editor->terminal);
                    
                    // Redraw search prompt
                    lle_terminal_write(editor->terminal, "(reverse-i-search)`", 19);
                    if (strlen(reverse_search_query) > 0) {
                        lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                    }
                    lle_terminal_write(editor->terminal, "': ", 3);
                    if (editor->buffer->length > 0) {
                        lle_terminal_write(editor->terminal, editor->buffer->buffer, editor->buffer->length);
                    }
                } else {
                    // Clear screen and redraw
                    lle_terminal_clear_screen(editor->terminal);
                    cmd_result = LLE_CMD_SUCCESS;
                }
                needs_display_update = false;
                break;
                
            case LLE_KEY_CTRL_S:
                // Forward search (standard readline behavior)
                if (reverse_search_mode && strlen(reverse_search_query) > 0 && 
                    editor->history_enabled && editor->history && 
                    reverse_search_index < (int)editor->history->count - 1) {
                    
                    // Search forward (towards newer entries)
                    for (int i = reverse_search_index + 1; i < (int)editor->history->count; i++) {
                        const lle_history_entry_t *entry = lle_history_get(editor->history, i);
                        if (entry && entry->command && strstr(entry->command, reverse_search_query)) {
                            reverse_search_index = i;
                            
                            // Update buffer with match
                            lle_text_buffer_clear(editor->buffer);
                            for (size_t j = 0; j < entry->length; j++) {
                                lle_text_insert_char(editor->buffer, entry->command[j]);
                            }
                            lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                            
                            // Update search prompt (show forward search)
                            lle_terminal_write(editor->terminal, "\r", 1);
                            lle_terminal_move_cursor_to_column(editor->terminal, 0);
                            lle_terminal_clear_to_eol(editor->terminal);
                            lle_terminal_write(editor->terminal, "(i-search)`", 11);
                            lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                            lle_terminal_write(editor->terminal, "': ", 3);
                            lle_terminal_write(editor->terminal, entry->command, entry->length);
                            break;
                        }
                    }
                }
                needs_display_update = false;
                break;
                
            case LLE_KEY_CTRL_R:
                // Working Ctrl+R implementation with complete navigation
                if (!reverse_search_mode) {
                    // Enter reverse search mode
                    reverse_search_mode = 1;
                    reverse_search_query[0] = '\0';
                    reverse_search_index = -1;
                    reverse_search_start_index = editor->history_enabled && editor->history ? editor->history->count - 1 : -1;
                    
                    // Save original line
                    if (reverse_search_original_line) {
                        free(reverse_search_original_line);
                    }
                    reverse_search_original_line = malloc(editor->buffer->length + 1);
                    if (reverse_search_original_line) {
                        memcpy(reverse_search_original_line, editor->buffer->buffer, editor->buffer->length);
                        reverse_search_original_line[editor->buffer->length] = '\0';
                    }
                    
                    // Clear buffer and show search prompt
                    lle_text_buffer_clear(editor->buffer);
                    
                    // Move to next line and show initial search prompt
                    lle_terminal_write(editor->terminal, "\n", 1);
                    lle_terminal_move_cursor_to_column(editor->terminal, 0);
                    lle_terminal_write(editor->terminal, "(reverse-i-search)`': ", 22);
                } else {
                    // Already in reverse search - find next match (search backwards)
                    if (strlen(reverse_search_query) > 0 && editor->history_enabled && editor->history) {
                        int start_index = (reverse_search_index > 0) ? reverse_search_index - 1 : editor->history->count - 1;
                        bool found = false;
                        
                        for (int i = start_index; i >= 0; i--) {
                            const lle_history_entry_t *entry = lle_history_get(editor->history, i);
                            if (entry && entry->command && strstr(entry->command, reverse_search_query)) {
                                reverse_search_index = i;
                                found = true;
                                
                                // Update buffer with match
                                lle_text_buffer_clear(editor->buffer);
                                for (size_t j = 0; j < entry->length; j++) {
                                    lle_text_insert_char(editor->buffer, entry->command[j]);
                                }
                                lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                
                                // Update search prompt
                                lle_terminal_write(editor->terminal, "\r", 1);
                                lle_terminal_move_cursor_to_column(editor->terminal, 0);
                                lle_terminal_clear_to_eol(editor->terminal);
                                lle_terminal_write(editor->terminal, "(reverse-i-search)`", 19);
                                lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                                lle_terminal_write(editor->terminal, "': ", 3);
                                lle_terminal_write(editor->terminal, entry->command, entry->length);
                                break;
                            }
                        }
                        
                        if (!found) {
                            // No more matches - beep or show "failing" indicator
                            lle_terminal_write(editor->terminal, "\r", 1);
                            lle_terminal_move_cursor_to_column(editor->terminal, 0);
                            lle_terminal_clear_to_eol(editor->terminal);
                            lle_terminal_write(editor->terminal, "(failed reverse-i-search)`", 26);
                            lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                            lle_terminal_write(editor->terminal, "': ", 3);
                        }
                    }
                }
                needs_display_update = false;
                break;
                
            case LLE_KEY_CHAR:
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Character key: 0x%02x ('%c')\n", 
                            event.character, (event.character >= 32 && event.character <= 126) ? event.character : '?');
                }
                
                // Handle reverse search character input
                if (reverse_search_mode && event.character >= 32 && event.character <= 126) {
                    // Add character to search query
                    size_t query_len = strlen(reverse_search_query);
                    if (query_len < sizeof(reverse_search_query) - 1) {
                        reverse_search_query[query_len] = event.character;
                        reverse_search_query[query_len + 1] = '\0';
                        
                        // Search for match
                        if (editor->history_enabled && editor->history) {
                            for (int i = editor->history->count - 1; i >= 0; i--) {
                                const lle_history_entry_t *entry = lle_history_get(editor->history, i);
                                if (entry && entry->command && strstr(entry->command, reverse_search_query)) {
                                    // Found match - update buffer and display
                                    lle_text_buffer_clear(editor->buffer);
                                    for (size_t j = 0; j < entry->length; j++) {
                                        lle_text_insert_char(editor->buffer, entry->command[j]);
                                    }
                                    lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                                    reverse_search_index = i;
                                    
                                    // Update search prompt
                                    lle_terminal_write(editor->terminal, "\r", 1);
                                    lle_terminal_move_cursor_to_column(editor->terminal, 0);
                                    lle_terminal_clear_to_eol(editor->terminal);
                                    lle_terminal_write(editor->terminal, "(reverse-i-search)`", 19);
                                    lle_terminal_write(editor->terminal, reverse_search_query, strlen(reverse_search_query));
                                    lle_terminal_write(editor->terminal, "': ", 3);
                                    lle_terminal_write(editor->terminal, entry->command, entry->length);
                                    
                                    needs_display_update = false;
                                    break;
                                }
                            }
                        }
                    }
                    needs_display_update = false;
                    break;
                }
                
                // Handle control characters that may be detected as CHAR instead of specific key types
                if (event.character == 0x01) { // Ctrl+A - move to beginning
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+A as CHAR - moving to beginning of line\n");
                    }
                    cmd_result = lle_cmd_move_home(editor->display);
                    needs_display_update = false; // Phase 2B.5: Command handles its own display update
                }
                else if (event.character == 0x05) { // Ctrl+E - move to end
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+E as CHAR - moving to end of line\n");
                    }
                    cmd_result = lle_cmd_move_end(editor->display);
                    needs_display_update = false; // Phase 2B.5: Command handles its own display update
                }
                else if (event.character == 0x12) { // Ctrl+R - reverse search
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+R as CHAR - trigger search\n");
                    }
                    // Same logic as LLE_KEY_CTRL_R case above
                    if (!reverse_search_mode) {
                        reverse_search_mode = 1;
                        reverse_search_query[0] = '\0';
                        reverse_search_index = -1;
                        reverse_search_start_index = -1;
                        
                        if (reverse_search_original_line) {
                            free(reverse_search_original_line);
                        }
                        reverse_search_original_line = malloc(editor->buffer->length + 1);
                        if (reverse_search_original_line) {
                            memcpy(reverse_search_original_line, editor->buffer->buffer, editor->buffer->length);
                            reverse_search_original_line[editor->buffer->length] = '\0';
                        }
                        
                        lle_text_buffer_clear(editor->buffer);
                        lle_terminal_write(editor->terminal, "\n", 1);
                        lle_terminal_move_cursor_to_column(editor->terminal, 0);
                        lle_terminal_write(editor->terminal, "(reverse-i-search)`': ", 22);
                    }
                    needs_display_update = false;
                }
                else if (event.character == LLE_ASCII_CTRL_UNDERSCORE) { // Ctrl+_ (undo)
                    if (editor->undo_enabled && editor->undo_stack) {
                        if (lle_undo_can_undo(editor->undo_stack)) {
                            lle_undo_execute(editor->undo_stack, editor->buffer);
                        }
                    }
                }
                else if (event.character == LLE_ASCII_CTRL_G) { // Ctrl+G (abort) - standard readline cancel
                    if (reverse_search_mode) {
                        // Exit reverse search mode and restore original line
                        reverse_search_mode = 0;
                        if (reverse_search_original_line) {
                            lle_text_buffer_clear(editor->buffer);
                            for (size_t i = 0; i < strlen(reverse_search_original_line); i++) {
                                lle_text_insert_char(editor->buffer, reverse_search_original_line[i]);
                            }
                            lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                            free(reverse_search_original_line);
                            reverse_search_original_line = NULL;
                            reverse_search_start_index = -1;
                        }
                        reverse_search_query[0] = '\0';
                        reverse_search_index = -1;
                        reverse_search_start_index = -1;
                        
                        // Clear search line and restore prompt
                        lle_terminal_write(editor->terminal, "\r", 1);
                        lle_terminal_clear_to_eol(editor->terminal);
                        lle_terminal_move_cursor_up(editor->terminal, 1);
                        needs_display_update = true;
                    } else {
                        line_cancelled = true;
                        lle_set_last_error(editor, LLE_ERROR_INTERRUPTED);
                    }
                }
                // Check for signal-generating control characters that should be ignored
                else if (event.character == LLE_ASCII_CTRL_BACKSLASH) { // Ctrl+\ (SIGQUIT) - let shell handle
                    needs_display_update = false;
                }
                else if (event.character == LLE_ASCII_CTRL_S) { // Ctrl+S (XOFF) - let terminal handle
                    needs_display_update = false;
                }
                else if (event.character == LLE_ASCII_CTRL_Q) { // Ctrl+Q (XON) - let terminal handle
                    needs_display_update = false;
                }
                // Insert regular printable characters (only if not in reverse search mode)
                else if (event.character >= 32 && event.character <= 126 && !reverse_search_mode) {
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Inserting printable character: '%c'\n", event.character);
                    }
                    
                    // LINUX FIX: Simple character insertion without display system
                    if (lle_platform_is_linux()) {
                        // Direct character insertion for Linux
                        if (editor->buffer->length < editor->buffer->capacity - 1) {
                            // Simple append to buffer
                            editor->buffer->buffer[editor->buffer->length] = event.character;
                            editor->buffer->length++;
                            editor->buffer->cursor_pos = editor->buffer->length;
                            editor->buffer->buffer[editor->buffer->length] = '\0';
                            
                            // Direct terminal write (no display system)
                            lle_terminal_write(editor->display->terminal, &event.character, 1);
                            
                            if (debug_mode) {
                                fprintf(stderr, "[LLE_INPUT_LOOP] Linux: Direct character insertion\n");
                            }
                        }
                        cmd_result = LLE_CMD_SUCCESS;
                        needs_display_update = false; // CRITICAL: Block display system
                    } else {
                        // macOS: Use sophisticated display system
                        cmd_result = lle_cmd_insert_char(editor->display, event.character);
                        needs_display_update = false; // Phase 2B.5: Command handles its own display update
                    }
                }
                else {
                    // Other control characters - ignore in line editor
                    needs_display_update = false;
                }
                break;
                
            case LLE_KEY_CTRL_Y:
                // TODO: Implement yank (paste from kill ring) - standard readline behavior
                // This is the correct use of Ctrl+Y (not redo)
                needs_display_update = false;
                break;
                
            case LLE_KEY_TAB:
                // Enhanced tab completion (if enabled)
                if (editor->auto_completion && editor->completions) {
                    if (lle_enhanced_tab_completion_handle(editor->buffer, editor->completions)) {
                        needs_display_update = true;
                        
                        // Phase 2B.2: Show completion menu for multiple options
                        const lle_enhanced_completion_info_t *info = 
                            lle_enhanced_tab_completion_get_info();
                        
                        if (info && info->total_count > 1) {
                            // Create completion display for menu
                            lle_completion_display_t *completion_display = 
                                lle_completion_display_create(editor->completions, 
                                                            info->total_count > 10 ? 10 : info->total_count);
                            
                            if (completion_display) {
                                // Show completion menu using Phase 2A absolute positioning
                                if (!lle_completion_display_show(editor->display, completion_display)) {
                                    if (debug_mode) {
                                        fprintf(stderr, "[PHASE_2B_TAB_COMPLETION] Failed to show completion menu\n");
                                    }
                                }
                                lle_completion_display_destroy(completion_display);
                            }
                        }
                        
                        // Debug info for completion state
                        if (debug_mode) {
                            fprintf(stderr, "[ENHANCED_TAB_COMPLETION] %d/%d: %s\n",
                                   info->current_index + 1, info->total_count,
                                   info->current_completion);
                        }
                    } else {
                        needs_display_update = false;
                    }
                } else {
                    needs_display_update = false;
                }
                break;
                
            case LLE_KEY_ESCAPE:
                if (reverse_search_mode) {
                    // Exit reverse search mode and restore original line
                    reverse_search_mode = 0;
                    if (reverse_search_original_line) {
                        lle_text_buffer_clear(editor->buffer);
                        for (size_t i = 0; i < strlen(reverse_search_original_line); i++) {
                            lle_text_insert_char(editor->buffer, reverse_search_original_line[i]);
                        }
                        lle_text_move_cursor(editor->buffer, LLE_MOVE_END);
                        free(reverse_search_original_line);
                        reverse_search_original_line = NULL;
                    }
                    reverse_search_query[0] = '\0';
                    reverse_search_index = -1;
                    reverse_search_start_index = -1;
                    
                    // Clear search line and restore prompt
                    lle_terminal_write(editor->terminal, "\r", 1);
                    lle_terminal_clear_to_eol(editor->terminal);
                    lle_terminal_move_cursor_up(editor->terminal, 1);
                    needs_display_update = true;
                } else {
                    // Normal escape key behavior
                    needs_display_update = false;
                }
                break;
                
            case LLE_KEY_CTRL_G:
                cmd_result = lle_cmd_cancel_line(editor->display);
                needs_display_update = false; // Phase 2B.5: Command handles its own display update
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] Ctrl+G (Cancel line) pressed\n");
                }
                break;
            
            default:
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] DEFAULT case executed for event.type=%d\n", event.type);
                }
                // Unknown or unhandled key - ignore
                if (debug_mode) {
                    fprintf(stderr, "[LLE_INPUT_LOOP] DEFAULT case reached with key type: %d\n", event.type);
                }
                needs_display_update = false;
                break;
        }
        
        // Update display if needed and command succeeded
        if (needs_display_update && cmd_result != LLE_CMD_ERROR_DISPLAY_UPDATE) {
            // LLE-R003: Use stabilized display update with error recovery
            if (!lle_display_update_incremental(editor->display)) {
                // Apply stabilization error recovery for reliable display updates
                if (!lle_display_error_recovery((struct lle_display_state *)editor->display, -1)) {
                    // Final fallback: clear and render if recovery fails
                    if (debug_mode) {
                        fprintf(stderr, "[LLE_INPUT_LOOP] Display error recovery failed, falling back to full render\n");
                    }
                    lle_display_render(editor->display);
                }
            }
        }
    }
    
    return result;
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
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Called with editor=%p, prompt='%s'\n", 
                (void*)editor, prompt ? prompt : "(null)");
    }
    
    if (!editor || !prompt) {
        if (editor) lle_set_last_error(editor, LLE_ERROR_INVALID_PARAMETER);
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Invalid parameters: editor=%p, prompt=%p\n", 
                    (void*)editor, (void*)prompt);
        }
        return NULL;
    }
    
    if (!editor->initialized) {
        lle_set_last_error(editor, LLE_ERROR_NOT_INITIALIZED);
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Editor not initialized\n");
        }
        return NULL;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Editor initialized, proceeding with input\n");
    }
    
    // Clear text buffer for new input
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Clearing text buffer\n");
    }
    lle_text_buffer_clear(editor->buffer);
    
    // Store current prompt
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Setting current prompt\n");
    }
    if (editor->current_prompt) {
        free(editor->current_prompt);
    }
    editor->current_prompt = strdup(prompt);
    if (!editor->current_prompt) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Failed to allocate memory for prompt\n");
        }
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    // Create and parse prompt
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Creating prompt object\n");
    }
    lle_prompt_t *prompt_obj = lle_prompt_create(strlen(prompt) + 64);
    if (!prompt_obj) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Failed to create prompt object\n");
        }
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Parsing prompt\n");
    }
    if (!lle_prompt_parse(prompt_obj, prompt)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Failed to parse prompt\n");
        }
        lle_prompt_destroy(prompt_obj);
        lle_set_last_error(editor, LLE_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    // Store old prompt to restore on error
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Setting up display with new prompt\n");
    }
    lle_prompt_t *old_prompt = editor->display->prompt;
    editor->display->prompt = prompt_obj;
    
    // Initial display render
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Rendering initial display\n");
    }
    if (!lle_display_render(editor->display)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Display render failed - using fallback prompt\n");
        }
        // Fallback: just write the prompt directly to terminal without fancy rendering
        const char *simple_prompt = prompt;
        if (simple_prompt && editor->terminal) {
            lle_terminal_write(editor->terminal, simple_prompt, strlen(simple_prompt));
        }
        // Continue with input loop despite render failure
    }
    
    // Enter raw mode for TTY input if needed
    bool entered_raw_mode = false;
    if (editor->terminal && isatty(editor->terminal->stdin_fd)) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Entering raw mode for TTY input\n");
        }
        
        // Flush output before entering raw mode
        fflush(stdout);
        fflush(stderr);
        
        if (lle_terminal_enter_raw_mode(editor->terminal)) {
            entered_raw_mode = true;
            if (debug_mode) {
                fprintf(stderr, "[LLE_READLINE] Successfully entered raw mode\n");
            }
            
            // Give terminal time to process mode change
            usleep(1000); // 1ms delay
            
        } else {
            if (debug_mode) {
                fprintf(stderr, "[LLE_READLINE] Failed to enter raw mode\n");
            }
        }
    }

    // Execute main input processing loop
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Starting input loop\n");
    }
    char *result = lle_input_loop(editor);

    // Exit raw mode if we entered it
    if (entered_raw_mode && editor->terminal) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Exiting raw mode\n");
        }
        if (lle_terminal_exit_raw_mode(editor->terminal)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_READLINE] Successfully exited raw mode\n");
            }
        } else {
            if (debug_mode) {
                fprintf(stderr, "[LLE_READLINE] Failed to exit raw mode\n");
            }
        }
    }
    
    if (debug_mode) {
        if (result) {
            fprintf(stderr, "[LLE_READLINE] Input loop returned result: '%s'\n", result);
        } else {
            fprintf(stderr, "[LLE_READLINE] Input loop returned NULL\n");
        }
    }
    
    // Clean up: restore original prompt and destroy the one we created
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Cleaning up prompt objects\n");
    }
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
        if (debug_mode) {
            fprintf(stderr, "[LLE_READLINE] Adding result to history\n");
        }
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
    
    // Handle result and error status (already set by lle_input_loop)
    if (debug_mode) {
        fprintf(stderr, "[LLE_READLINE] Returning result: %p\n", (void*)result);
    }
    
    if (!result) {
        // Input was cancelled or error occurred
        // Error code already set by lle_input_loop
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
/**
 * @file lle_readline.c
 * @brief LLE Readline Function - Core Orchestration
 * 
 * This file implements lle_readline(), the main readline loop that orchestrates
 * all LLE subsystems to provide interactive line editing.
 * 
 * CRITICAL: This implementation uses ONLY proper LLE subsystem APIs.
 * NO direct terminal I/O, NO escape sequences, NO architectural violations.
 * 
 * Implementation: Step 7 - Signal Handling
 * - Creates local terminal abstraction instance
 * - Uses terminal abstraction for raw mode
 * - Uses input processor for reading
 * - Uses lle_buffer_t for proper buffer management
 * - Uses event system to dispatch events to handlers
 * - Uses display generator and display client for rendering
 * - Event handlers modify buffer (decoupled architecture)
 * - Display refreshed after buffer modifications
 * - Returns on Enter key
 * - Adds support for arrow keys (Left/Right cursor movement)
 * - Adds support for Home/End keys
 * - Adds support for Delete key
 * - Adds support for Ctrl-K (kill to end of line)
 * - Adds support for Ctrl-U (kill entire line)
 * - Adds multiline detection for unclosed quotes
 * - Inserts newline and continues reading if input incomplete
 * - Supports multi-line input within quoted strings
 * 
 * Step 7 - Signal Handling:
 * - SIGWINCH: Window resize events trigger display refresh
 * - SIGTSTP: Ctrl-Z handled by terminal_unix_interface (exits raw mode before suspend)
 * - SIGCONT: Resume handled by terminal_unix_interface (re-enters raw mode)
 * - SIGINT: Ctrl-C handled by terminal_unix_interface (restores terminal, exits)
 * - All signal handlers installed by lle_unix_interface_install_signal_handlers()
 * 
 * Step 5 Enhancement - Complete Emacs Keybindings:
 * - Ctrl-A: Beginning of line
 * - Ctrl-B: Back one character
 * - Ctrl-E: End of line
 * - Ctrl-F: Forward one character
 * - Ctrl-G: Abort/cancel current line
 * - Ctrl-L: Clear screen and redraw
 * - Ctrl-W: Kill word backwards
 * - Ctrl-Y: Yank (paste) from kill buffer
 * - Kill buffer stores text from Ctrl-K/U/W for Ctrl-Y
 * 
 * NOTE: Creates own terminal, buffer, and event system for now.
 * Future steps will integrate with full LLE system initialization.
 */

#include "lle/terminal_abstraction.h"
#include "lle/buffer_management.h"
#include "lle/memory_management.h"
#include "lle/event_system.h"
#include "lle/error_handling.h"
#include "lle/display_integration.h"  /* Spec 08: Complete display integration */
#include "lle/utf8_support.h"         /* UTF-8 support for proper character deletion */
#include "lle/history.h"              /* History system for UP/DOWN navigation */
#include "lle/lle_editor.h"           /* Proper LLE editor architecture */
#include "lle/keybinding_actions.h"   /* Smart arrow navigation functions */
#include "lle/keybinding.h"           /* Keybinding manager for Group 1+ migration */
#include "input_continuation.h"
#include "display_integration.h"      /* Lusush display integration */
#include "display/display_controller.h"
#include "display/prompt_layer.h"
#include "config.h"                   /* For config_values_t and history config options */

/* Forward declarations for history action functions */
lle_result_t lle_history_previous(lle_editor_t *editor);
lle_result_t lle_history_next(lle_editor_t *editor);

/* Forward declaration for config */
extern config_values_t config;

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

/* External global memory pool */
extern lusush_memory_pool_t *global_memory_pool;

/* Global LLE editor instance (proper architecture) */
static lle_editor_t *global_lle_editor = NULL;

/**
 * @brief Get the global LLE editor instance
 * Allows other modules to access the editor for adding history, etc.
 */
lle_editor_t *lle_get_global_editor(void) {
    return global_lle_editor;
}

/**
 * @brief Populate LLE history config from Lusush config system
 * Maps LLE history config options from config.h to lle_history_config_t
 * 
 * This bridges Lusush's config system with LLE's history core, ensuring
 * all user preferences are properly applied to the history subsystem.
 */
static void populate_history_config_from_lusush_config(lle_history_config_t *hist_config) {
    if (!hist_config) return;
    
    /* Initialize to zero */
    memset(hist_config, 0, sizeof(lle_history_config_t));
    
    /* Capacity settings */
    hist_config->max_entries = config.history_size > 0 ? config.history_size : 5000;
    hist_config->max_command_length = 8192; /* Support long multiline commands */
    
    /* File settings */
    if (config.lle_history_file && config.lle_history_file[0] != '\0') {
        hist_config->history_file_path = config.lle_history_file;
    } else {
        hist_config->history_file_path = NULL; /* Use default ~/.lusush_history_lle */
    }
    hist_config->auto_save = true;         /* Always auto-save for safety */
    hist_config->load_on_init = true;      /* Load existing history on startup */
    
    /* Deduplication behavior */
    hist_config->ignore_duplicates = config.lle_enable_deduplication &&
                                    (config.lle_dedup_scope != LLE_DEDUP_SCOPE_NONE);
    hist_config->ignore_space_prefix = false; /* Standard bash behavior: space = don't save */
    
    /* Metadata to save */
    hist_config->save_timestamps = config.history_timestamps; /* Use global setting */
    hist_config->save_working_dir = config.lle_enable_forensic_tracking;
    hist_config->save_exit_codes = config.lle_enable_forensic_tracking;
    
    /* Performance settings */
    hist_config->initial_capacity = config.lle_enable_history_cache && config.lle_cache_size > 0
                                   ? config.lle_cache_size
                                   : 1000;
    hist_config->use_indexing = config.lle_enable_history_cache; /* Enable fast lookups if cache enabled */
}

/* Event handler context for Step 6 */
typedef struct readline_context {
    lle_buffer_t *buffer;
    bool *done;
    char **final_line;
    lle_terminal_abstraction_t *term;
    const char *prompt;
    continuation_state_t *continuation_state;  /* Step 6: Shared multiline parser state */
    char *kill_buffer;  /* Step 5 enhancement: Simple kill buffer for yank */
    size_t kill_buffer_size;  /* Allocated size of kill buffer */
    
    /* LLE Editor - proper architecture */
    lle_editor_t *editor;                       /* Full LLE editor context */
    
    /* Keybinding manager - incremental migration (Group 1+) */
    lle_keybinding_manager_t *keybinding_manager; /* Replaces hardcoded keybindings */
} readline_context_t;

/**
 * @brief Multiline detection using shared continuation parser
 * Step 6: Uses input_continuation.c for proper shell construct detection
 * 
 * Analyzes buffer content using the shared multiline parser which handles:
 * - Quote tracking (single, double, backtick)
 * - Bracket/brace/parenthesis counting
 * - Control structures (if/then/fi, case, loops)
 * - Here documents
 * - Function definitions
 * 
 * @param buffer_data The buffer content to check
 * @param state Continuation state to analyze with
 * @return true if input appears incomplete, false otherwise
 */
static bool is_input_incomplete(const char *buffer_data, continuation_state_t *state)
{
    if (buffer_data == NULL || state == NULL) {
        return false;
    }
    
    /* Reset state for fresh analysis */
    continuation_state_cleanup(state);
    continuation_state_init(state);
    
    /* Analyze the entire buffer content */
    continuation_analyze_line(buffer_data, state);
    
    /* Check if continuation is needed */
    return continuation_needs_continuation(state);
}

/**
 * @brief Refresh display after buffer modification using Spec 08 render system
 *
 * PROPER IMPLEMENTATION: Uses Spec 08 display integration components:
 * - render_controller: Coordinates rendering operations
 * - dirty_tracker: Tracks what changed for incremental updates
 * - render_cache: Caches rendered output for performance
 * - display_bridge: Communicates with Lusush display system
 *
 * This is the CORRECT way to render - using the Spec 08 rendering pipeline
 * instead of calling display_controller directly.
 */
static void refresh_display(readline_context_t *ctx)
{
    if (!ctx || !ctx->buffer) {
        return;
    }

    /* Get the global Spec 08 display integration instance */
    lle_display_integration_t *display_integration = lle_display_integration_get_global();
    if (!display_integration) {
        /* Fallback: If Spec 08 not initialized, display nothing rather than crash */
        return;
    }

    /* Get render controller from display integration */
    lle_render_controller_t *render_controller = display_integration->render_controller;
    if (!render_controller) {
        return;
    }

    /* Mark dirty regions in dirty tracker based on what actually changed
     * Use change tracking to mark only affected regions for efficient updates
     */
    if (render_controller->dirty_tracker) {
        /* Check if we have change tracking information */
        if (ctx->buffer->change_tracking_enabled &&
            ctx->buffer->current_sequence &&
            ctx->buffer->current_sequence->last_op) {

            /* Get the last operation that was performed */
            lle_change_operation_t *last_op = ctx->buffer->current_sequence->last_op;

            /* Mark only the affected region as dirty */
            lle_dirty_tracker_mark_range(
                render_controller->dirty_tracker,
                last_op->start_position,
                last_op->affected_length
            );
        } else {
            /* No change tracking info - mark entire buffer dirty (first render) */
            lle_dirty_tracker_mark_full(render_controller->dirty_tracker);
        }
    }

    /* Render buffer content through Spec 08 render system */
    lle_render_output_t *render_output = NULL;
    lle_result_t result = lle_render_buffer_content(
        render_controller,
        ctx->buffer,
        &ctx->buffer->cursor,
        &render_output
    );

    if (result != LLE_SUCCESS || !render_output) {
        return;
    }

    /* SPEC 08 COMPLIANT: Send rendered output through display_bridge to command_layer
     *
     * This is THE CORRECT ARCHITECTURE per Spec 08:
     * 1. render_controller produces rendered output
     * 2. display_bridge sends output to command_layer
     * 3. command_layer sends to display_controller
     * 4. display_controller handles terminal I/O
     *
     * NO DIRECT TERMINAL WRITES - this is a critical architectural principle!
     *
     * IMPORTANT: Always send to display_bridge even if buffer is empty, because we
     * still need to display the prompt. The prompt is rendered separately by the
     * prompt_layer and combined in the display_controller.
     */
    lle_display_bridge_t *display_bridge = display_integration->display_bridge;
    if (display_bridge) {
        result = lle_display_bridge_send_output(
            display_bridge,
            render_output,
            &ctx->buffer->cursor
        );

        if (result != LLE_SUCCESS) {
            /* Log error but continue - display bridge will track consecutive errors */
        }
    }

    /* Free the render output */
    lle_render_output_free(render_output);

    /* Clear dirty tracker after successful render */
    if (render_controller->dirty_tracker) {
        lle_dirty_tracker_clear(render_controller->dirty_tracker);
    }
}

/**
 * @brief Event handler for character input
 * Step 4: Handler modifies buffer and refreshes display
 */
static lle_result_t handle_character_input(lle_event_t *event, void *user_data)
{
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Get UTF-8 character from event */
    const char *utf8_char = event->event_data.key.utf8_char;
    size_t char_len = strlen(utf8_char);
    
    /* CRITICAL: Reset history navigation when user types a character */
    /* This follows bash/readline behavior: typing exits history mode */
    if (ctx->editor && ctx->editor->history_navigation_pos > 0) {
        ctx->editor->history_navigation_pos = 0;
    }
    
    /* Insert character into buffer at cursor position */
    lle_result_t result = lle_buffer_insert_text(
        ctx->buffer,
        ctx->buffer->cursor.byte_offset,
        utf8_char,
        char_len
    );
    
    /* Synchronize cursor fields after insert (PHASE 2 STEP 0 FIX) */
    if (result == LLE_SUCCESS && ctx->editor && ctx->editor->cursor_manager) {
        lle_cursor_manager_move_to_byte_offset(
            ctx->editor->cursor_manager,
            ctx->buffer->cursor.byte_offset
        );
    }
    
    /* Refresh display after buffer modification */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}

/**
 * @brief Event handler for backspace
 * Step 4: Handler modifies buffer and refreshes display
 * PHASE 2 FIX: Delete entire grapheme cluster, not just one codepoint
 */
static lle_result_t handle_backspace(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (ctx->buffer->cursor.byte_offset > 0 && ctx->editor && ctx->editor->cursor_manager) {
        /* Sync cursor manager position with buffer cursor before moving */
        lle_cursor_manager_move_to_byte_offset(ctx->editor->cursor_manager, ctx->buffer->cursor.byte_offset);
        
        /* Now check if we can move back (after sync) */
        if (ctx->buffer->cursor.grapheme_index == 0) {
            return LLE_SUCCESS;  /* Already at beginning */
        }
        
        /* Move cursor back by one grapheme to find the start of the grapheme to delete */
        size_t current_byte = ctx->buffer->cursor.byte_offset;
        
        lle_result_t result = lle_cursor_manager_move_by_graphemes(ctx->editor->cursor_manager, -1);
        if (result == LLE_SUCCESS) {
            /* CRITICAL: Sync buffer cursor back from cursor manager after movement */
            lle_cursor_manager_get_position(ctx->editor->cursor_manager, &ctx->buffer->cursor);
            
            size_t grapheme_start = ctx->buffer->cursor.byte_offset;
            size_t grapheme_len = current_byte - grapheme_start;
            
            /* Delete the entire grapheme cluster */
            result = lle_buffer_delete_text(ctx->buffer, grapheme_start, grapheme_len);
            
            /* Refresh display after buffer modification */
            if (result == LLE_SUCCESS) {
                refresh_display(ctx);
            }
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Enter key
 * Step 6: Check for multiline continuation before completing
 */
static lle_result_t handle_enter(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Check for incomplete input using shared continuation parser */
    bool incomplete = is_input_incomplete(ctx->buffer->data, ctx->continuation_state);
    
    if (incomplete) {
        /* Input incomplete - insert newline and continue */
        lle_result_t result = lle_buffer_insert_text(
            ctx->buffer,
            ctx->buffer->cursor.byte_offset,
            "\n",
            1
        );
        
        /* Synchronize cursor fields after insert (PHASE 2 STEP 0 FIX) */
        if (result == LLE_SUCCESS && ctx->editor && ctx->editor->cursor_manager) {
            lle_cursor_manager_move_to_byte_offset(
                ctx->editor->cursor_manager,
                ctx->buffer->cursor.byte_offset
            );
        }
        
        if (result == LLE_SUCCESS) {
            /* NOTE: Continuation prompts not yet supported in LLE
             * 
             * ARCHITECTURAL LIMITATION: LLE uses single-buffer model where entire
             * multiline command is edited in one buffer with embedded newlines.
             * Continuation prompts (loop>, if>, etc.) cannot be injected into
             * buffer (would break shell parsing) or easily displayed per-line
             * without major display system enhancements.
             * 
             * GNU Readline calls readline() multiple times with different prompts,
             * so each line naturally gets its own prompt. LLE would need display
             * system to parse buffer at newlines and inject prompts in rendered
             * output (not in buffer) - requires significant composition engine work.
             * 
             * FUTURE WORK: Enhance composition_engine to support per-line prompt
             * injection in display output without modifying command buffer content.
             * See: docs/lle_specification/future/continuation_prompts.md
             */
            
            refresh_display(ctx);
        }
        
        return result;
    }
    
    /* Line complete - accept entire buffer regardless of cursor position */
    
    /* Add to LLE history before completing */
    if (ctx->editor && ctx->editor->history_system && ctx->buffer->data && ctx->buffer->data[0] != '\0') {
        lle_history_add_entry(ctx->editor->history_system, ctx->buffer->data, 0, NULL);
        
        /* Save to history file (auto-save enabled in config) */
        const char *home = getenv("HOME");
        if (home) {
            char history_path[1024];
            snprintf(history_path, sizeof(history_path), "%s/.lusush_history_lle", home);
            lle_history_save_to_file(ctx->editor->history_system, history_path);
        }
    }
    
    *ctx->done = true;
    *ctx->final_line = ctx->buffer->data ? strdup(ctx->buffer->data) : strdup("");
    
    return LLE_SUCCESS;
}

/**
 * @brief Context-aware ENTER key action (Dual-Action Architecture)
 * 
 * Full readline-aware ENTER key handler with direct access to readline context.
 * This is a context-aware action (type LLE_ACTION_TYPE_CONTEXT) that has full
 * access to readline state, eliminating the need for flags or complex state management.
 * 
 * Architecture:
 * - Checks for incomplete input using continuation state parser
 * - If incomplete: inserts newline, syncs cursor, refreshes display, continues editing
 * - If complete: adds to history, saves history file, sets done flag, returns final line
 * 
 * This replaces the flag-based approach (line_accepted flag) which caused system-wide
 * regressions due to complex state management and flag checking after every action.
 * 
 * See: docs/lle_implementation/DUAL_ACTION_ARCHITECTURE.md
 * 
 * @param ctx Readline context with full access to buffer, history, completion state
 * @return LLE_SUCCESS on successful handling
 */
lle_result_t lle_accept_line_context(readline_context_t *ctx)
{
    if (!ctx || !ctx->buffer) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Check for incomplete input using shared continuation parser */
    bool incomplete = is_input_incomplete(ctx->buffer->data, ctx->continuation_state);
    
    if (incomplete) {
        /* Input incomplete - insert newline and continue editing */
        lle_result_t result = lle_buffer_insert_text(
            ctx->buffer,
            ctx->buffer->cursor.byte_offset,
            "\n",
            1
        );
        
        /* Synchronize cursor fields after insert */
        if (result == LLE_SUCCESS && ctx->editor && ctx->editor->cursor_manager) {
            lle_cursor_manager_move_to_byte_offset(
                ctx->editor->cursor_manager,
                ctx->buffer->cursor.byte_offset
            );
        }
        
        if (result == LLE_SUCCESS) {
            /* NOTE: Continuation prompts not yet supported in LLE
             * 
             * ARCHITECTURAL LIMITATION: LLE uses single-buffer model where entire
             * multiline command is edited in one buffer with embedded newlines.
             * Continuation prompts (loop>, if>, etc.) cannot be injected into
             * buffer (would break shell parsing) or easily displayed per-line
             * without major display system enhancements.
             * 
             * GNU Readline calls readline() multiple times with different prompts,
             * so each line naturally gets its own prompt. LLE would need display
             * system to parse buffer at newlines and inject prompts in rendered
             * output (not in buffer) - requires significant composition engine work.
             * 
             * FUTURE WORK: Enhance composition_engine to support per-line prompt
             * injection in display output without modifying command buffer content.
             * See: docs/lle_specification/future/continuation_prompts.md
             */
            
            refresh_display(ctx);
        }
        
        return result;
    }
    
    /* Line complete - accept entire buffer regardless of cursor position */
    
    /* Add to LLE history before completing */
    if (ctx->editor && ctx->editor->history_system && 
        ctx->buffer->data && ctx->buffer->data[0] != '\0') {
        lle_history_add_entry(ctx->editor->history_system, ctx->buffer->data, 0, NULL);
        
        /* Save to history file (auto-save enabled in config) */
        const char *home = getenv("HOME");
        if (home) {
            char history_path[1024];
            snprintf(history_path, sizeof(history_path), "%s/.lusush_history_lle", home);
            lle_history_save_to_file(ctx->editor->history_system, history_path);
        }
    }
    
    /* Signal completion to readline loop */
    *ctx->done = true;
    *ctx->final_line = ctx->buffer->data ? strdup(ctx->buffer->data) : strdup("");
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-D (EOF/delete-char)
 * Step 3: Handler signals EOF on empty line
 * Step 5 enhancement: Delete character at cursor when line is non-empty
 * 
 * Ctrl-D has dual behavior in readline:
 * - Empty line: Signals EOF (exits shell)
 * - Non-empty line: Deletes character at cursor (same as Delete key)
 */
static lle_result_t handle_eof(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (ctx->buffer->length == 0) {
        /* EOF on empty line - exit shell */
        *ctx->done = true;
        *ctx->final_line = NULL;
    } else {
        /* Non-empty line - delete grapheme cluster at cursor (same as Delete key) */
        if (ctx->buffer->cursor.grapheme_index < ctx->buffer->grapheme_count && 
            ctx->editor && ctx->editor->cursor_manager) {
            /* Sync cursor manager position with buffer cursor before moving */
            lle_cursor_manager_move_to_byte_offset(ctx->editor->cursor_manager, 
                                                    ctx->buffer->cursor.byte_offset);
            
            /* Move cursor forward by one grapheme to find the end of the grapheme to delete */
            size_t grapheme_start = ctx->buffer->cursor.byte_offset;
            
            lle_result_t result = lle_cursor_manager_move_by_graphemes(ctx->editor->cursor_manager, 1);
            if (result == LLE_SUCCESS) {
                /* CRITICAL: Sync buffer cursor back from cursor manager after movement */
                lle_cursor_manager_get_position(ctx->editor->cursor_manager, &ctx->buffer->cursor);
                
                size_t grapheme_end = ctx->buffer->cursor.byte_offset;
                size_t grapheme_len = grapheme_end - grapheme_start;
                
                /* Delete the entire grapheme cluster */
                result = lle_buffer_delete_text(ctx->buffer, grapheme_start, grapheme_len);
                
                /* Refresh display after buffer modification */
                if (result == LLE_SUCCESS) {
                    refresh_display(ctx);
                }
            }
            
            return result;
        }
    }
    
    return LLE_SUCCESS;
}

/*
 * NOTE: handle_interrupt() removed - Ctrl+C now handled via SIGINT signal.
 * With ISIG enabled in raw mode, Ctrl+C generates SIGINT caught by lusush's
 * sigint_handler() in src/signals.c, which properly manages child processes.
 */

/**
 * @brief Event handler for Ctrl-G (abort/cancel line) - LEGACY FALLBACK
 * Step 5 enhancement: Abort readline and return empty line to shell
 * 
 * This is the Emacs-style abort - it cancels the current input
 * and returns an empty line, causing the shell to display a fresh prompt.
 * 
 * NOTE: This is the legacy fallback handler. The context-aware action
 * lle_abort_line_context() is now used when keybinding manager is available.
 */
static lle_result_t handle_abort(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Signal done with empty result - this aborts the readline */
    *ctx->done = true;
    *ctx->final_line = strdup("");  /* Return empty string, not NULL (NULL signals EOF) */
    
    return LLE_SUCCESS;
}

/**
 * @brief Context-aware Ctrl-G action (abort-line) - Dual-Action Architecture
 * 
 * Emacs-style abort that cancels current input and returns empty line to shell.
 * This is a context-aware action that directly manages readline completion state.
 * 
 * Unlike the flag-based simple action approach, this directly sets done/final_line,
 * eliminating the need for abort_requested flag and preventing state persistence bugs.
 * 
 * @param ctx Readline context with full access to done/final_line
 * @return LLE_SUCCESS
 */
lle_result_t lle_abort_line_context(readline_context_t *ctx)
{
    if (!ctx) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Signal done with empty result - this aborts the readline */
    *ctx->done = true;
    *ctx->final_line = strdup("");  /* Return empty string, not NULL (NULL signals EOF) */
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-L (clear screen and redraw)
 * Step 5 enhancement: Trigger full screen refresh
 */
static lle_result_t handle_clear_screen(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Get the global display integration instance */
    lle_display_integration_t *display_integration = lle_display_integration_get_global();
    if (!display_integration || !display_integration->lusush_display) {
        return LLE_ERROR_INVALID_STATE;
    }
    
    /* Clear screen through display controller */
    display_controller_error_t result = display_controller_clear_screen(display_integration->lusush_display);
    if (result != DISPLAY_CONTROLLER_SUCCESS) {
        return LLE_ERROR_DISPLAY_INTEGRATION;
    }
    
    /* Refresh display to redraw the prompt and current input */
    refresh_display(ctx);
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-W (kill word backwards)
 * Step 5 enhancement: Delete word before cursor and save to kill buffer
 */
static lle_result_t handle_kill_word(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (ctx->buffer->cursor.byte_offset == 0) {
        return LLE_SUCCESS;  /* At beginning, nothing to kill */
    }
    
    const char *data = ctx->buffer->data;
    size_t pos = ctx->buffer->cursor.byte_offset;
    
    /* Scan backwards past whitespace */
    while (pos > 0 && (data[pos - 1] == ' ' || data[pos - 1] == '\t')) {
        pos--;
    }
    
    /* Scan backwards past non-whitespace (the word) */
    size_t word_start = pos;
    while (word_start > 0 && data[word_start - 1] != ' ' && data[word_start - 1] != '\t') {
        word_start--;
    }
    
    if (word_start < pos) {
        size_t kill_len = pos - word_start;
        
        /* Save killed text to kill buffer */
        if (ctx->kill_buffer_size < kill_len + 1) {
            char *new_buf = realloc(ctx->kill_buffer, kill_len + 1);
            if (new_buf) {
                ctx->kill_buffer = new_buf;
                ctx->kill_buffer_size = kill_len + 1;
            }
        }
        
        if (ctx->kill_buffer && ctx->kill_buffer_size >= kill_len + 1) {
            memcpy(ctx->kill_buffer, data + word_start, kill_len);
            ctx->kill_buffer[kill_len] = '\0';
        }
        
        /* Delete the word */
        lle_result_t result = lle_buffer_delete_text(ctx->buffer, word_start, kill_len);
        
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-Y (yank from kill buffer)
 * Step 5 enhancement: Insert killed text at cursor
 */
static lle_result_t handle_yank(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Insert kill buffer contents at cursor if we have something */
    if (ctx->kill_buffer && ctx->kill_buffer[0] != '\0') {
        size_t kill_len = strlen(ctx->kill_buffer);
        lle_result_t result = lle_buffer_insert_text(
            ctx->buffer,
            ctx->buffer->cursor.byte_offset,
            ctx->kill_buffer,
            kill_len
        );
        
        /* Synchronize cursor fields after insert (PHASE 2 STEP 0 FIX) */
        if (result == LLE_SUCCESS && ctx->editor && ctx->editor->cursor_manager) {
            lle_cursor_manager_move_to_byte_offset(
                ctx->editor->cursor_manager,
                ctx->buffer->cursor.byte_offset
            );
        }
        
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Left arrow key
 * Step 5: Move cursor left one grapheme cluster
 * PHASE 2 FIX: Use grapheme-based movement instead of codepoint-based
 */
static lle_result_t handle_arrow_left(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Move cursor left by one grapheme cluster if not at beginning */
    if (ctx->buffer->cursor.grapheme_index > 0 && ctx->editor && ctx->editor->cursor_manager) {
        /* Sync cursor manager position with buffer cursor before moving */
        lle_cursor_manager_move_to_byte_offset(ctx->editor->cursor_manager, ctx->buffer->cursor.byte_offset);
        
        /* Move cursor by graphemes */
        lle_cursor_manager_move_by_graphemes(ctx->editor->cursor_manager, -1);
        
        /* CRITICAL: Sync buffer cursor back from cursor manager after movement */
        lle_cursor_manager_get_position(ctx->editor->cursor_manager, &ctx->buffer->cursor);
        
        refresh_display(ctx);
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Right arrow key
 * Step 5: Move cursor right one grapheme cluster
 * PHASE 2 FIX: Use grapheme-based movement instead of codepoint-based
 */
static lle_result_t handle_arrow_right(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Move cursor right by one grapheme cluster if not at end */
    if (ctx->buffer->cursor.grapheme_index < ctx->buffer->grapheme_count && 
        ctx->editor && ctx->editor->cursor_manager) {
        /* Sync cursor manager position with buffer cursor before moving */
        lle_cursor_manager_move_to_byte_offset(ctx->editor->cursor_manager, ctx->buffer->cursor.byte_offset);
        
        /* Move cursor by graphemes */
        lle_cursor_manager_move_by_graphemes(ctx->editor->cursor_manager, 1);
        
        /* CRITICAL: Sync buffer cursor back from cursor manager after movement */
        lle_cursor_manager_get_position(ctx->editor->cursor_manager, &ctx->buffer->cursor);
        
        refresh_display(ctx);
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Home key
 * Step 5: Move cursor to beginning of line
 */
static lle_result_t handle_home(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Move to beginning */
    ctx->buffer->cursor.byte_offset = 0;
    refresh_display(ctx);
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for End key
 * Step 5: Move cursor to end of line
 */
static lle_result_t handle_end(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Move to end */
    ctx->buffer->cursor.byte_offset = ctx->buffer->length;
    refresh_display(ctx);
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Delete key
 * Step 5: Delete character at cursor position
 * PHASE 2 FIX: Delete entire grapheme cluster, not just one codepoint
 */
static lle_result_t handle_delete(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Delete grapheme cluster at cursor if not at end */
    if (ctx->buffer->cursor.grapheme_index < ctx->buffer->grapheme_count && 
        ctx->editor && ctx->editor->cursor_manager) {
        /* Sync cursor manager position with buffer cursor before moving */
        lle_cursor_manager_move_to_byte_offset(ctx->editor->cursor_manager, ctx->buffer->cursor.byte_offset);
        
        /* Move cursor forward by one grapheme to find the end of the grapheme to delete */
        size_t grapheme_start = ctx->buffer->cursor.byte_offset;
        
        lle_result_t result = lle_cursor_manager_move_by_graphemes(ctx->editor->cursor_manager, 1);
        if (result == LLE_SUCCESS) {
            /* CRITICAL: Sync buffer cursor back from cursor manager after movement */
            lle_cursor_manager_get_position(ctx->editor->cursor_manager, &ctx->buffer->cursor);
            
            size_t grapheme_end = ctx->buffer->cursor.byte_offset;
            size_t grapheme_len = grapheme_end - grapheme_start;
            
            /* Delete the entire grapheme cluster */
            result = lle_buffer_delete_text(ctx->buffer, grapheme_start, grapheme_len);
            
            /* Refresh display after buffer modification */
            if (result == LLE_SUCCESS) {
                refresh_display(ctx);
            }
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-K
 * Step 5: Kill (delete) text from cursor to end of line
 * Step 5 enhancement: Save killed text to kill buffer
 */
static lle_result_t handle_kill_to_end(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Delete from cursor to end of buffer */
    if (ctx->buffer->cursor.byte_offset < ctx->buffer->length) {
        size_t delete_length = ctx->buffer->length - ctx->buffer->cursor.byte_offset;
        
        /* Save killed text to kill buffer */
        if (ctx->kill_buffer_size < delete_length + 1) {
            char *new_buf = realloc(ctx->kill_buffer, delete_length + 1);
            if (new_buf) {
                ctx->kill_buffer = new_buf;
                ctx->kill_buffer_size = delete_length + 1;
            }
        }
        
        if (ctx->kill_buffer && ctx->kill_buffer_size >= delete_length + 1) {
            memcpy(ctx->kill_buffer, 
                   ctx->buffer->data + ctx->buffer->cursor.byte_offset,
                   delete_length);
            ctx->kill_buffer[delete_length] = '\0';
        }
        
        lle_result_t result = lle_buffer_delete_text(
            ctx->buffer,
            ctx->buffer->cursor.byte_offset,
            delete_length
        );
        
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-U
 * Step 5: Kill (delete) entire line
 * Step 5 enhancement: Save killed text to kill buffer
 */
static lle_result_t handle_kill_line(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Ctrl-U: Kill from beginning of line to cursor (backward-kill-line) */
    size_t kill_length = ctx->buffer->cursor.byte_offset;
    
    if (kill_length > 0) {
        /* Save killed text to kill buffer */
        if (ctx->kill_buffer_size < kill_length + 1) {
            char *new_buf = realloc(ctx->kill_buffer, kill_length + 1);
            if (new_buf) {
                ctx->kill_buffer = new_buf;
                ctx->kill_buffer_size = kill_length + 1;
            }
        }
        
        if (ctx->kill_buffer && ctx->kill_buffer_size >= kill_length + 1) {
            memcpy(ctx->kill_buffer, ctx->buffer->data, kill_length);
            ctx->kill_buffer[kill_length] = '\0';
        }
        
        lle_result_t result = lle_buffer_delete_text(
            ctx->buffer,
            0,
            kill_length
        );
        
        if (result == LLE_SUCCESS) {
            /* Cursor automatically moves to position 0 after deleting from start */
            refresh_display(ctx);
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Handle UP arrow - smart context-aware navigation
 * Single-line mode: Navigate history
 * Multi-line mode: Navigate to previous line in buffer
 * Proper architecture: delegates to lle_smart_up_arrow() action function
 */
static lle_result_t handle_arrow_up(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Check if editor is available */
    if (!ctx->editor) {
        return LLE_SUCCESS;  /* No editor available */
    }
    
    /* Call the smart arrow function (context-aware) */
    lle_result_t result = lle_smart_up_arrow(ctx->editor);
    
    /* Refresh display after navigation */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}

/**
 * @brief Handle DOWN arrow - smart context-aware navigation
 * Single-line mode: Navigate history
 * Multi-line mode: Navigate to next line in buffer
 * Proper architecture: delegates to lle_smart_down_arrow() action function
 */
static lle_result_t handle_arrow_down(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Check if editor is available */
    if (!ctx->editor) {
        return LLE_SUCCESS;  /* No editor available */
    }
    
    /* Call the smart arrow function (context-aware) */
    lle_result_t result = lle_smart_down_arrow(ctx->editor);
    
    /* Refresh display after navigation */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}

/**
 * @brief Execute a keybinding action via the keybinding manager
 * Group 1+ migration: Routes key sequences through keybinding manager
 * Falls back to provided handler function if keybinding manager lookup fails
 * 
 * @param ctx Readline context
 * @param key_sequence Key sequence string (e.g., "LEFT", "RIGHT", "HOME", "END")
 * @param fallback_handler Fallback handler function if lookup fails (can be NULL)
 * @return LLE_SUCCESS or error code
 */
static lle_result_t execute_keybinding_action(
    readline_context_t *ctx,
    const char *key_sequence,
    lle_result_t (*fallback_handler)(lle_event_t *, void *)
)
{
    /* Try keybinding manager first (Group 1+ migration) */
    if (ctx->keybinding_manager && ctx->editor) {
        lle_keybinding_action_t *action = NULL;
        lle_result_t result = lle_keybinding_manager_lookup(
            ctx->keybinding_manager,
            key_sequence,
            &action
        );
        
        if (result == LLE_SUCCESS && action != NULL) {
            lle_result_t exec_result;
            
            /* Dispatch based on action type */
            if (action->type == LLE_ACTION_TYPE_SIMPLE) {
                /* Simple action: operate on editor, then handle special flags */
                exec_result = action->func.simple(ctx->editor);
                
                /* Check for EOF request (legacy flag - used by Ctrl-D) */
                if (ctx->editor->eof_requested) {
                    *ctx->done = true;
                    *ctx->final_line = NULL;
                    return exec_result;
                }
                
                /* Check for abort request (legacy flag - used by Ctrl-G) */
                if (ctx->editor->abort_requested) {
                    *ctx->done = true;
                    *ctx->final_line = strdup("");  /* Return empty string, not NULL (NULL signals EOF) */
                    return exec_result;
                }
                
                /* Refresh display after simple action */
                if (exec_result == LLE_SUCCESS) {
                    refresh_display(ctx);
                }
                
                return exec_result;
            }
            else if (action->type == LLE_ACTION_TYPE_CONTEXT) {
                /* Context-aware action: has full access, handles everything including display refresh */
                exec_result = action->func.context(ctx);
                return exec_result;
            }
            
            /* Unknown action type - should never happen */
            return LLE_ERROR_FATAL_INTERNAL;
        }
    }
    
    /* Fallback to hardcoded handler if keybinding manager not available or lookup failed */
    if (fallback_handler) {
        return fallback_handler(NULL, ctx);
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Read a line of input from the user with line editing
 * 
 * This is the core readline function that replaces GNU readline when LLE is enabled.
 * 
 * Step 2 Implementation:
 * - Create terminal abstraction
 * - Enter raw terminal mode
 * - Create lle_buffer_t for line editing
 * - Read input events one at a time
 * - Use lle_buffer_insert_text() for character input
 * - Use lle_buffer_delete_text() for backspace
 * - Return on Enter key
 * - Exit raw mode and cleanup
 * 
 * @param prompt The prompt string to display to the user
 * @return Newly allocated string containing the line (caller must free), or NULL on error/EOF
 */
char *lle_readline(const char *prompt)
{
    lle_result_t result;
    
    /* Reset prompt display state for new input session */
    dc_reset_prompt_display_state();
    
    /* Get display controller from display_integration */
    void *display_controller = display_integration_get_controller();
    
    /* === STEP 1: Create terminal abstraction instance === */
    lle_terminal_abstraction_t *term = NULL;
    result = lle_terminal_abstraction_init(&term, (lusush_display_context_t *)display_controller);
    if (result != LLE_SUCCESS || term == NULL) {
        /* Failed to initialize terminal abstraction */
        return NULL;
    }
    
    /* === STEP 2: Get unix interface for raw mode === */
    if (term->unix_interface == NULL) {
        lle_terminal_abstraction_destroy(term);
        return NULL;
    }
    
    lle_unix_interface_t *unix_iface = term->unix_interface;
    
    /* === STEP 3: Enter raw mode === */
    result = lle_unix_interface_enter_raw_mode(unix_iface);
    if (result != LLE_SUCCESS) {
        /* Failed to enter raw mode */
        lle_terminal_abstraction_destroy(term);
        return NULL;
    }
    
    /* === STEP 4: Create buffer for line editing === */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 256);
    if (result != LLE_SUCCESS || buffer == NULL) {
        /* Failed to create buffer */
        lle_unix_interface_exit_raw_mode(unix_iface);
        lle_terminal_abstraction_destroy(term);
        return NULL;
    }
    
    /* === STEP 5: Create event system === */
    /* Step 3: Add event system for decoupled architecture */
    lle_event_system_t *event_system = NULL;
    result = lle_event_system_init(&event_system, (lle_memory_pool_t *)global_memory_pool);
    if (result != LLE_SUCCESS || event_system == NULL) {
        /* Failed to create event system */
        lle_buffer_destroy(buffer);
        lle_unix_interface_exit_raw_mode(unix_iface);
        lle_terminal_abstraction_destroy(term);
        return NULL;
    }
    
    /* === STEP 5.5: Create continuation state === */
    /* Step 6: Initialize shared multiline parser state */
    continuation_state_t continuation_state;
    continuation_state_init(&continuation_state);
    
    /* === STEP 5.6: Create kill buffer === */
    /* Step 5 enhancement: Initialize kill buffer for yank operations */
    char *kill_buffer = NULL;
    size_t kill_buffer_size = 0;
    
    /* === STEP 5.7: Get display controller for prompt display === */
    display_controller_t *display = display_integration_get_controller();
    
    /* === STEP 5.8: Initialize LLE display integration (Spec 08) === */
    lle_display_integration_t *lle_display_integ = lle_display_integration_get_global();
    if (!lle_display_integ && display) {
        /* Initialize display integration if not already initialized */
        result = lle_display_integration_init(
            &lle_display_integ,
            NULL,  /* editor context - not needed yet */
            display,
            (lle_memory_pool_t *)global_memory_pool
        );
        /* Note: Initialization failure is non-fatal - LLE can work without it */
    }
    
    /* === STEP 6: Register event handlers === */
    /* Step 4: Register handlers that will modify buffer and refresh display */
    bool done = false;
    char *final_line = NULL;
    /* === STEP 6.5: Initialize LLE editor (proper architecture) === */
    /* Create global editor instance if it doesn't exist */
    if (!global_lle_editor) {
        result = lle_editor_create(&global_lle_editor, global_memory_pool);
        if (result != LLE_SUCCESS || !global_lle_editor) {
            /* Failed to create editor - non-fatal, history won't work */
            global_lle_editor = NULL;
        } else {
            /* Initialize history subsystem with config from Lusush */
            lle_history_config_t hist_config;
            populate_history_config_from_lusush_config(&hist_config);
            
            result = lle_history_core_create(&global_lle_editor->history_system,
                                            global_lle_editor->lle_pool,
                                            &hist_config);
            
            if (result == LLE_SUCCESS && global_lle_editor->history_system) {
                /* Load existing history from LLE history file */
                const char *history_file = getenv("HOME");
                char history_path[1024];
                if (history_file) {
                    snprintf(history_path, sizeof(history_path), 
                            "%s/.lusush_history_lle", history_file);
                    lle_history_load_from_file(global_lle_editor->history_system,
                                              history_path);
                }
            }
        }
    }
    
    /* Set buffer in editor if editor exists */
    if (global_lle_editor) {
        global_lle_editor->buffer = buffer;
        
        /* Update cursor_manager's buffer reference to stay synchronized */
        if (global_lle_editor->cursor_manager) {
            global_lle_editor->cursor_manager->buffer = buffer;
        }
        
        /* CRITICAL: Reset history navigation position for new readline session */
        /* Each readline() call starts fresh - not in history navigation mode */
        global_lle_editor->history_navigation_pos = 0;
    }
    
    /* === STEP 6.6: Create keybinding manager (Group 1+ migration) === */
    /* Group 1: Navigation keys (LEFT, RIGHT, HOME, END) */
    lle_keybinding_manager_t *keybinding_manager = NULL;
    result = lle_keybinding_manager_create(&keybinding_manager, global_memory_pool);
    if (result != LLE_SUCCESS || keybinding_manager == NULL) {
        /* Failed to create keybinding manager - non-fatal, will use hardcoded fallbacks */
        keybinding_manager = NULL;
    } else {
        /* Bind Group 1 navigation keys to their action functions */
        /* These will be routed through keybinding manager instead of hardcoded handlers */
        lle_keybinding_manager_bind(keybinding_manager, "LEFT", lle_backward_char, "backward-char");
        lle_keybinding_manager_bind(keybinding_manager, "RIGHT", lle_forward_char, "forward-char");
        lle_keybinding_manager_bind(keybinding_manager, "HOME", lle_beginning_of_line, "beginning-of-line");
        lle_keybinding_manager_bind(keybinding_manager, "END", lle_end_of_line, "end-of-line");
        
        /* Bind Group 2 deletion keys to their action functions */
        /* These will be routed through keybinding manager instead of hardcoded handlers */
        lle_keybinding_manager_bind(keybinding_manager, "BACKSPACE", lle_backward_delete_char, "backward-delete-char");
        lle_keybinding_manager_bind(keybinding_manager, "DELETE", lle_delete_char, "delete-char");
        lle_keybinding_manager_bind(keybinding_manager, "C-d", lle_delete_char, "delete-char");
        
        /* Bind Group 3 kill/yank keys to their action functions */
        /* These will be routed through keybinding manager instead of hardcoded handlers */
        lle_keybinding_manager_bind(keybinding_manager, "C-k", lle_kill_line, "kill-line");
        lle_keybinding_manager_bind(keybinding_manager, "C-u", lle_unix_line_discard, "unix-line-discard");
        lle_keybinding_manager_bind(keybinding_manager, "C-w", lle_unix_word_rubout, "unix-word-rubout");
        lle_keybinding_manager_bind(keybinding_manager, "C-y", lle_yank, "yank");
        
        /* Bind Group 4 history & special keys to their action functions */
        /* These will be routed through keybinding manager instead of hardcoded handlers */
        /* Ctrl-A/B/E/F are duplicates of Group 1 navigation keys */
        lle_keybinding_manager_bind(keybinding_manager, "C-a", lle_beginning_of_line, "beginning-of-line");
        lle_keybinding_manager_bind(keybinding_manager, "C-b", lle_backward_char, "backward-char");
        lle_keybinding_manager_bind(keybinding_manager, "C-e", lle_end_of_line, "end-of-line");
        lle_keybinding_manager_bind(keybinding_manager, "C-f", lle_forward_char, "forward-char");
        /* History navigation */
        lle_keybinding_manager_bind(keybinding_manager, "C-n", lle_history_next, "history-next");
        lle_keybinding_manager_bind(keybinding_manager, "C-p", lle_history_previous, "history-previous");
        /* Smart arrows - context-aware (history vs multiline) */
        lle_keybinding_manager_bind(keybinding_manager, "UP", lle_smart_up_arrow, "smart-up-arrow");
        lle_keybinding_manager_bind(keybinding_manager, "DOWN", lle_smart_down_arrow, "smart-down-arrow");
        /* Special functions */
        lle_keybinding_manager_bind(keybinding_manager, "C-l", lle_clear_screen, "clear-screen");
        
        /* Ctrl-G: Context-aware abort action (requires readline_context_t access) */
        lle_keybinding_manager_bind_context(keybinding_manager, "C-g", lle_abort_line_context, "abort-line");
        
        /* Group 5: ENTER key - Context-aware action (requires readline_context_t access) */
        /* This uses the new dual-action architecture to provide ENTER with full readline context */
        /* See: docs/lle_implementation/DUAL_ACTION_ARCHITECTURE.md */
        lle_keybinding_manager_bind_context(keybinding_manager, "ENTER", lle_accept_line_context, "accept-line");
    }
    
    readline_context_t ctx = {
        .buffer = buffer,
        .done = &done,
        .final_line = &final_line,
        .term = term,
        .prompt = prompt,
        .continuation_state = &continuation_state,
        .kill_buffer = kill_buffer,
        .kill_buffer_size = kill_buffer_size,
        
        /* LLE Editor - proper architecture */
        .editor = global_lle_editor,
        
        /* Keybinding manager - Group 1+ migration */
        .keybinding_manager = keybinding_manager
    };
    
    /* CRITICAL: Reset per-readline-call flags on global editor
     * The editor is persistent across readline calls, but these flags
     * must be reset at the start of each new readline session.
     * Without this reset, abort_requested/eof_requested persist across
     * readline calls, causing all subsequent actions to immediately exit.
     */
    if (global_lle_editor) {
        global_lle_editor->abort_requested = false;
        global_lle_editor->eof_requested = false;
    }
    
    /* Register handler for character input */
    result = lle_event_handler_register(event_system, LLE_EVENT_KEY_PRESS,
                                       handle_character_input, &ctx, "character_input");
    if (result != LLE_SUCCESS) {
        lle_event_system_destroy(event_system);
        lle_buffer_destroy(buffer);
        lle_unix_interface_exit_raw_mode(unix_iface);
        lle_terminal_abstraction_destroy(term);
        return NULL;
    }
    
    /* === STEP 7: Display prompt === */
    /* Step 4: Set prompt in prompt_layer and display initial prompt */
    if (lle_display_integ && lle_display_integ->lusush_display) {
        display_controller_t *dc = lle_display_integ->lusush_display;
        if (dc->compositor && dc->compositor->prompt_layer && prompt) {
            /* Set the prompt content in the prompt_layer */
            prompt_layer_set_content(dc->compositor->prompt_layer, prompt);
        }
    }
    
    /* Initial display refresh to show prompt */
    refresh_display(&ctx);
    
    /* === STEP 8: Main input loop === */
    
    while (!done) {
        
        /* Read next input event */
        lle_input_event_t *event = NULL;
        result = lle_input_processor_read_next_event(
            term->input_processor,
            &event,
            100  /* 100ms timeout */
        );
        
        /* Handle timeout and null events - just continue */
        if (result == LLE_ERROR_TIMEOUT || event == NULL) {
            continue;
        }
        
        /* ALSO check event type for timeout (timeout can be returned as SUCCESS with type=TIMEOUT) */
        if (event->type == LLE_INPUT_TYPE_TIMEOUT) {
            continue;
        }
        
        /* Handle read errors */
        if (result != LLE_SUCCESS) {
            /* Error reading input - abort */
            done = true;
            final_line = NULL;
            continue;
        }
        
        /* === STEP 9: Convert input event to LLE event and dispatch === */
        /* Step 3: Use event system instead of direct buffer manipulation */
        
        switch (event->type) {
            case LLE_INPUT_TYPE_CHARACTER: {
                /* Regular character input */
                uint32_t codepoint = event->data.character.codepoint;
                
                /* Check for Enter key (newline) */
                if (codepoint == '\n' || codepoint == '\r') {
                    /* GROUP 5 MIGRATION: ENTER routed through keybinding manager (context-aware action) */
                    execute_keybinding_action(&ctx, "ENTER", handle_enter);
                    break;
                }
                
                /* NOTE: Ctrl-A through Ctrl-Z are now handled as SPECIAL_KEY events with modifiers
                 * See SPECIAL_KEY case below for Ctrl+letter handling.
                 * Ctrl-C (codepoint == 3) is still handled via SIGINT signal (src/signals.c). */
                
                /* Check for backspace */
                if (codepoint == 127 || codepoint == 8) {  /* DEL or BS */
                    execute_keybinding_action(&ctx, "BACKSPACE", handle_backspace);
                    break;
                }
                
                /* Regular character - create LLE event and dispatch */
                lle_event_t *lle_event = NULL;
                result = lle_event_create(event_system, LLE_EVENT_KEY_PRESS, 
                                         NULL, 0, &lle_event);
                if (result == LLE_SUCCESS && lle_event != NULL) {
                    /* Set event data */
                    lle_event->event_data.key.key_code = codepoint;
                    lle_event->event_data.key.modifiers = 0;
                    lle_event->event_data.key.is_special = false;
                    
                    /* Copy UTF-8 bytes */
                    size_t copy_len = event->data.character.byte_count;
                    if (copy_len > 7) copy_len = 7;  /* utf8_char is char[8] */
                    memcpy(lle_event->event_data.key.utf8_char,
                           event->data.character.utf8_bytes,
                           copy_len);
                    lle_event->event_data.key.utf8_char[copy_len] = '\0';
                    
                    /* Dispatch event - handler will modify buffer */
                    lle_event_dispatch(event_system, lle_event);
                }
                break;
            }
            
            case LLE_INPUT_TYPE_SPECIAL_KEY: {
                /* Special keys */
                /* GROUP 5 MIGRATION: ENTER routed through keybinding manager (context-aware action) */
                if (event->data.special_key.key == LLE_KEY_ENTER) {
                    execute_keybinding_action(&ctx, "ENTER", handle_enter);
                }
                /* GROUP 1 MIGRATION: Navigation keys routed through keybinding manager */
                else if (event->data.special_key.key == LLE_KEY_LEFT) {
                    execute_keybinding_action(&ctx, "LEFT", handle_arrow_left);
                }
                else if (event->data.special_key.key == LLE_KEY_RIGHT) {
                    execute_keybinding_action(&ctx, "RIGHT", handle_arrow_right);
                }
                /* History navigation with UP/DOWN arrows */
                else if (event->data.special_key.key == LLE_KEY_UP) {
                    execute_keybinding_action(&ctx, "UP", handle_arrow_up);
                }
                else if (event->data.special_key.key == LLE_KEY_DOWN) {
                    execute_keybinding_action(&ctx, "DOWN", handle_arrow_down);
                }
                /* GROUP 1 MIGRATION: Home/End keys routed through keybinding manager */
                else if (event->data.special_key.key == LLE_KEY_HOME) {
                    execute_keybinding_action(&ctx, "HOME", handle_home);
                }
                else if (event->data.special_key.key == LLE_KEY_END) {
                    execute_keybinding_action(&ctx, "END", handle_end);
                }
                /* Step 5: Delete key */
                else if (event->data.special_key.key == LLE_KEY_DELETE) {
                    execute_keybinding_action(&ctx, "DELETE", handle_delete);
                }
                /* Handle Ctrl+letter combinations (now SPECIAL_KEY events with keycode) */
                else if (event->data.special_key.key == LLE_KEY_UNKNOWN && 
                         (event->data.special_key.modifiers & LLE_MOD_CTRL)) {
                    uint32_t keycode = event->data.special_key.keycode;
                    
                    switch (keycode) {
                        case 'A':  /* Ctrl-A: Beginning of line */
                            execute_keybinding_action(&ctx, "C-a", handle_home);
                            break;
                        case 'B':  /* Ctrl-B: Back one character */
                            execute_keybinding_action(&ctx, "C-b", handle_arrow_left);
                            break;
                        case 'D':  /* Ctrl-D: EOF */
                            execute_keybinding_action(&ctx, "C-d", handle_eof);
                            break;
                        case 'E':  /* Ctrl-E: End of line */
                            execute_keybinding_action(&ctx, "C-e", handle_end);
                            break;
                        case 'F':  /* Ctrl-F: Forward one character */
                            execute_keybinding_action(&ctx, "C-f", handle_arrow_right);
                            break;
                        case 'G':  /* Ctrl-G: Abort/cancel line */
                            execute_keybinding_action(&ctx, "C-g", handle_abort);
                            break;
                        case 'K':  /* Ctrl-K: Kill to end of line */
                            execute_keybinding_action(&ctx, "C-k", handle_kill_to_end);
                            break;
                        case 'L':  /* Ctrl-L: Clear screen */
                            execute_keybinding_action(&ctx, "C-l", handle_clear_screen);
                            break;
                        case 'N':  /* Ctrl-N: Next history (always navigate history) */
                            execute_keybinding_action(&ctx, "C-n", NULL);
                            break;
                        case 'P':  /* Ctrl-P: Previous history (always navigate history) */
                            execute_keybinding_action(&ctx, "C-p", NULL);
                            break;
                        case 'U':  /* Ctrl-U: Kill entire line */
                            execute_keybinding_action(&ctx, "C-u", handle_kill_line);
                            break;
                        case 'W':  /* Ctrl-W: Kill word backwards */
                            execute_keybinding_action(&ctx, "C-w", handle_kill_word);
                            break;
                        case 'Y':  /* Ctrl-Y: Yank */
                            execute_keybinding_action(&ctx, "C-y", handle_yank);
                            break;
                        default:
                            /* Unknown Ctrl+letter - ignore */
                            break;
                    }
                }
                /* Other special keys ignored */
                break;
            }
            
            case LLE_INPUT_TYPE_EOF: {
                /* EOF received */
                handle_eof(NULL, &ctx);
                break;
            }
            
            case LLE_INPUT_TYPE_SIGNAL: {
                /* Signal received (Ctrl-C, etc.) */
                if (event->data.signal.signal_number == 2) {  /* SIGINT */
                    done = true;
                    final_line = NULL;
                }
                break;
            }
            
            case LLE_INPUT_TYPE_WINDOW_RESIZE: {
                /* Step 7: Window resize - refresh display with new dimensions */
                refresh_display(&ctx);
                break;
            }
            
            case LLE_INPUT_TYPE_ERROR: {
                /* Input error */
                done = true;
                final_line = NULL;
                break;
            }
            
            case LLE_INPUT_TYPE_TIMEOUT: {
                /* Timeout - continue loop */
                break;
            }
            
            default: {
                /* Unknown event type - ignore */
                break;
            }
        }
        
        /* Event processed - in Step 1 we don't free events (managed by input processor) */
    }
    
    /* === STEP 10: Exit raw mode and finalize input === */
    lle_unix_interface_exit_raw_mode(unix_iface);
    
    /* If we got a line, tell display system to finalize input.
     * This moves cursor to next line and resets display state.
     * The display system handles terminal I/O, not LLE.
     */
    if (final_line) {
        dc_finalize_input();
    }
    
    /* === STEP 11: Cleanup and return === */
    /* Step 5 enhancement: Free kill buffer */
    if (kill_buffer) {
        free(kill_buffer);
    }
    
    /* Group 1+ migration: Cleanup keybinding manager */
    if (keybinding_manager) {
        lle_keybinding_manager_destroy(keybinding_manager);
    }
    
    /* Step 6: Cleanup continuation state, destroy event system, buffer, and terminal */
    continuation_state_cleanup(&continuation_state);
    lle_event_system_destroy(event_system);
    lle_buffer_destroy(buffer);
    lle_terminal_abstraction_destroy(term);
    
    return final_line;
}

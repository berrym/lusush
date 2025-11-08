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
typedef struct {
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
    
    /* Insert character into buffer at cursor position */
    lle_result_t result = lle_buffer_insert_text(
        ctx->buffer,
        ctx->buffer->cursor.byte_offset,
        utf8_char,
        char_len
    );
    
    /* Refresh display after buffer modification */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}

/**
 * @brief Event handler for backspace
 * Step 4: Handler modifies buffer and refreshes display
 */
static lle_result_t handle_backspace(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (ctx->buffer->cursor.byte_offset > 0) {
        /* Find the start of the previous UTF-8 character by scanning backward */
        const char *data = ctx->buffer->data;
        size_t pos = ctx->buffer->cursor.byte_offset;
        size_t char_start = pos - 1;
        
        /* Scan backward to find UTF-8 character start (byte not starting with 10xxxxxx) */
        while (char_start > 0 && (data[char_start] & 0xC0) == 0x80) {
            char_start--;
        }
        
        /* Calculate how many bytes to delete (entire UTF-8 character) */
        size_t delete_len = pos - char_start;
        
        /* Delete the entire UTF-8 character */
        lle_result_t result = lle_buffer_delete_text(ctx->buffer, char_start, delete_len);
        
        /* Refresh display after buffer modification */
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
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
        
        if (result == LLE_SUCCESS) {
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
        /* Non-empty line - delete character at cursor (same as Delete key) */
        if (ctx->buffer->cursor.byte_offset < ctx->buffer->length) {
            /* Calculate UTF-8 character length at cursor position */
            size_t char_start = ctx->buffer->cursor.byte_offset;
            size_t char_end = char_start + 1;
            
            /* Scan forward past UTF-8 continuation bytes (10xxxxxx pattern) */
            while (char_end < ctx->buffer->length && 
                   (ctx->buffer->data[char_end] & 0xC0) == 0x80) {
                char_end++;
            }
            
            size_t char_length = char_end - char_start;
            
            /* Delete the entire UTF-8 character */
            lle_result_t result = lle_buffer_delete_text(
                ctx->buffer, 
                ctx->buffer->cursor.byte_offset, 
                char_length
            );
            
            if (result == LLE_SUCCESS) {
                refresh_display(ctx);
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
 * @brief Event handler for Ctrl-G (abort/cancel line)
 * Step 5 enhancement: Abort readline and return empty line to shell
 * 
 * This is the Emacs-style abort - it cancels the current input
 * and returns an empty line, causing the shell to display a fresh prompt.
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
        
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
        }
        
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Left arrow key
 * Step 5: Move cursor left one position
 */
static lle_result_t handle_arrow_left(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Move cursor left if not at beginning */
    if (ctx->buffer->cursor.byte_offset > 0) {
        /* Move back one UTF-8 character by scanning for character boundary */
        size_t new_offset = ctx->buffer->cursor.byte_offset - 1;
        
        /* Scan backward to find start of UTF-8 character
         * UTF-8 continuation bytes have pattern 10xxxxxx (0x80-0xBF)
         * Character start bytes are either:
         * - 0xxxxxxx (ASCII, 0x00-0x7F)
         * - 11xxxxxx (multi-byte start, 0xC0-0xFF)
         */
        while (new_offset > 0 && 
               (ctx->buffer->data[new_offset] & 0xC0) == 0x80) {
            new_offset--;
        }
        
        ctx->buffer->cursor.byte_offset = new_offset;
        refresh_display(ctx);
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Right arrow key
 * Step 5: Move cursor right one position
 */
static lle_result_t handle_arrow_right(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Move cursor right if not at end */
    if (ctx->buffer->cursor.byte_offset < ctx->buffer->length) {
        /* Move forward one UTF-8 character by scanning for next character boundary */
        size_t new_offset = ctx->buffer->cursor.byte_offset + 1;
        
        /* Skip UTF-8 continuation bytes (10xxxxxx pattern)
         * Stop at next character start byte:
         * - 0xxxxxxx (ASCII, 0x00-0x7F)
         * - 11xxxxxx (multi-byte start, 0xC0-0xFF)
         */
        while (new_offset < ctx->buffer->length && 
               (ctx->buffer->data[new_offset] & 0xC0) == 0x80) {
            new_offset++;
        }
        
        ctx->buffer->cursor.byte_offset = new_offset;
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
 */
static lle_result_t handle_delete(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Delete character at cursor if not at end */
    if (ctx->buffer->cursor.byte_offset < ctx->buffer->length) {
        /* Calculate UTF-8 character length at cursor position */
        size_t char_start = ctx->buffer->cursor.byte_offset;
        size_t char_end = char_start + 1;
        
        /* Scan forward past UTF-8 continuation bytes (10xxxxxx pattern) */
        while (char_end < ctx->buffer->length && 
               (ctx->buffer->data[char_end] & 0xC0) == 0x80) {
            char_end++;
        }
        
        size_t char_length = char_end - char_start;
        
        /* Delete the entire UTF-8 character */
        lle_result_t result = lle_buffer_delete_text(
            ctx->buffer, 
            ctx->buffer->cursor.byte_offset, 
            char_length
        );
        
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
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
 * @brief Handle UP arrow - navigate to previous history entry
 * Proper architecture: delegates to lle_history_previous() action function
 */
static lle_result_t handle_arrow_up(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Check if editor is available */
    if (!ctx->editor) {
        return LLE_SUCCESS;  /* No editor available */
    }
    
    /* Call the proper LLE action function */
    lle_result_t result = lle_history_previous(ctx->editor);
    
    /* Refresh display after history navigation */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}

/**
 * @brief Handle DOWN arrow - navigate to next history entry
 * Proper architecture: delegates to lle_history_next() action function
 */
static lle_result_t handle_arrow_down(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Check if editor is available */
    if (!ctx->editor) {
        return LLE_SUCCESS;  /* No editor available */
    }
    
    /* Call the proper LLE action function */
    lle_result_t result = lle_history_next(ctx->editor);
    
    /* Refresh display after history navigation */
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
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
        .editor = global_lle_editor
    };
    
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
                    handle_enter(NULL, &ctx);
                    break;
                }
                
                /* NOTE: Ctrl-A through Ctrl-Z are now handled as SPECIAL_KEY events with modifiers
                 * See SPECIAL_KEY case below for Ctrl+letter handling.
                 * Ctrl-C (codepoint == 3) is still handled via SIGINT signal (src/signals.c). */
                
                /* Check for backspace */
                if (codepoint == 127 || codepoint == 8) {  /* DEL or BS */
                    handle_backspace(NULL, &ctx);
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
                if (event->data.special_key.key == LLE_KEY_ENTER) {
                    handle_enter(NULL, &ctx);
                }
                /* Step 5: Arrow keys for cursor movement */
                else if (event->data.special_key.key == LLE_KEY_LEFT) {
                    handle_arrow_left(NULL, &ctx);
                }
                else if (event->data.special_key.key == LLE_KEY_RIGHT) {
                    handle_arrow_right(NULL, &ctx);
                }
                /* History navigation with UP/DOWN arrows */
                else if (event->data.special_key.key == LLE_KEY_UP) {
                    handle_arrow_up(NULL, &ctx);
                }
                else if (event->data.special_key.key == LLE_KEY_DOWN) {
                    handle_arrow_down(NULL, &ctx);
                }
                /* Step 5: Home/End keys */
                else if (event->data.special_key.key == LLE_KEY_HOME) {
                    handle_home(NULL, &ctx);
                }
                else if (event->data.special_key.key == LLE_KEY_END) {
                    handle_end(NULL, &ctx);
                }
                /* Step 5: Delete key */
                else if (event->data.special_key.key == LLE_KEY_DELETE) {
                    handle_delete(NULL, &ctx);
                }
                /* Handle Ctrl+letter combinations (now SPECIAL_KEY events with keycode) */
                else if (event->data.special_key.key == LLE_KEY_UNKNOWN && 
                         (event->data.special_key.modifiers & LLE_MOD_CTRL)) {
                    uint32_t keycode = event->data.special_key.keycode;
                    
                    switch (keycode) {
                        case 'A':  /* Ctrl-A: Beginning of line */
                            handle_home(NULL, &ctx);
                            break;
                        case 'B':  /* Ctrl-B: Back one character */
                            handle_arrow_left(NULL, &ctx);
                            break;
                        case 'D':  /* Ctrl-D: EOF */
                            handle_eof(NULL, &ctx);
                            break;
                        case 'E':  /* Ctrl-E: End of line */
                            handle_end(NULL, &ctx);
                            break;
                        case 'F':  /* Ctrl-F: Forward one character */
                            handle_arrow_right(NULL, &ctx);
                            break;
                        case 'G':  /* Ctrl-G: Abort/cancel line */
                            handle_abort(NULL, &ctx);
                            break;
                        case 'K':  /* Ctrl-K: Kill to end of line */
                            handle_kill_to_end(NULL, &ctx);
                            break;
                        case 'L':  /* Ctrl-L: Clear screen */
                            handle_clear_screen(NULL, &ctx);
                            break;
                        case 'U':  /* Ctrl-U: Kill entire line */
                            handle_kill_line(NULL, &ctx);
                            break;
                        case 'W':  /* Ctrl-W: Kill word backwards */
                            handle_kill_word(NULL, &ctx);
                            break;
                        case 'Y':  /* Ctrl-Y: Yank */
                            handle_yank(NULL, &ctx);
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
    
    /* Step 6: Cleanup continuation state, destroy event system, buffer, and terminal */
    continuation_state_cleanup(&continuation_state);
    lle_event_system_destroy(event_system);
    lle_buffer_destroy(buffer);
    lle_terminal_abstraction_destroy(term);
    
    return final_line;
}

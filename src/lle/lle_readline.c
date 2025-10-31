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
 * Implementation: Step 6 - Multiline Support
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
#include "input_continuation.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/* External global memory pool */
extern lusush_memory_pool_t *global_memory_pool;

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
 * @brief Refresh display after buffer modification
 * Step 4: Use display generator and client for rendering
 */
static void refresh_display(readline_context_t *ctx)
{
    /* Step 4: Use display APIs if available */
    if (ctx->term && ctx->term->display_generator && ctx->term->display_client) {
        /* Note: Display components may not be fully initialized if Lusush display is NULL */
        /* This call will use the proper API even if it doesn't fully render yet */
        lle_display_content_t *content = NULL;
        lle_result_t result = lle_display_generator_generate_content(
            ctx->term->display_generator,
            &content
        );
        
        if (result == LLE_SUCCESS && content != NULL) {
            /* Submit to Lusush display client */
            lle_lusush_display_client_submit_content(
                ctx->term->display_client,
                content
            );
        }
    }
    /* If display components are not available, no rendering occurs */
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
    
    /* Step 4: Refresh display after buffer modification */
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
        /* Delete one byte before cursor (Step 5 will improve to grapheme) */
        size_t delete_pos = ctx->buffer->cursor.byte_offset - 1;
        lle_result_t result = lle_buffer_delete_text(ctx->buffer, delete_pos, 1);
        
        /* Step 4: Refresh display after buffer modification */
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
    
    /* Step 6: Check for incomplete input using shared continuation parser */
    if (is_input_incomplete(ctx->buffer->data, ctx->continuation_state)) {
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
    
    /* Line complete */
    *ctx->done = true;
    *ctx->final_line = ctx->buffer->data ? strdup(ctx->buffer->data) : strdup("");
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-D (EOF)
 * Step 3: Handler signals EOF
 */
static lle_result_t handle_eof(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (ctx->buffer->length == 0) {
        /* EOF on empty line */
        *ctx->done = true;
        *ctx->final_line = NULL;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-C (interrupt)
 * Step 3: Handler signals interrupt
 */
static lle_result_t handle_interrupt(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Interrupted */
    *ctx->done = true;
    *ctx->final_line = NULL;
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Ctrl-G (abort/cancel line)
 * Step 5 enhancement: Clear buffer and reset to empty prompt
 */
static lle_result_t handle_abort(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    /* Clear the entire buffer */
    if (ctx->buffer->length > 0) {
        lle_result_t result = lle_buffer_delete_text(
            ctx->buffer,
            0,
            ctx->buffer->length
        );
        
        if (result == LLE_SUCCESS) {
            refresh_display(ctx);
        }
        
        return result;
    }
    
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
    
    /* Trigger display refresh which should clear and redraw */
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
        /* Move back one byte (Step 5: simple byte-based, UTF-8 grapheme in future) */
        ctx->buffer->cursor.byte_offset--;
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
        /* Move forward one byte (Step 5: simple byte-based, UTF-8 grapheme in future) */
        ctx->buffer->cursor.byte_offset++;
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
        /* Delete one byte at cursor (Step 5: simple byte-based, UTF-8 grapheme in future) */
        lle_result_t result = lle_buffer_delete_text(
            ctx->buffer, 
            ctx->buffer->cursor.byte_offset, 
            1
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
    
    /* Delete entire buffer contents */
    if (ctx->buffer->length > 0) {
        /* Save killed text to kill buffer */
        if (ctx->kill_buffer_size < ctx->buffer->length + 1) {
            char *new_buf = realloc(ctx->kill_buffer, ctx->buffer->length + 1);
            if (new_buf) {
                ctx->kill_buffer = new_buf;
                ctx->kill_buffer_size = ctx->buffer->length + 1;
            }
        }
        
        if (ctx->kill_buffer && ctx->kill_buffer_size >= ctx->buffer->length + 1) {
            memcpy(ctx->kill_buffer, ctx->buffer->data, ctx->buffer->length);
            ctx->kill_buffer[ctx->buffer->length] = '\0';
        }
        
        lle_result_t result = lle_buffer_delete_text(
            ctx->buffer,
            0,
            ctx->buffer->length
        );
        
        if (result == LLE_SUCCESS) {
            /* Cursor automatically moves to position 0 after deleting all content */
            refresh_display(ctx);
        }
        
        return result;
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
    
    /* === STEP 1: Create terminal abstraction instance === */
    lle_terminal_abstraction_t *term = NULL;
    result = lle_terminal_abstraction_init(&term, NULL);  /* NULL = no Lusush display yet */
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
    
    /* === STEP 6: Register event handlers === */
    /* Step 4: Register handlers that will modify buffer and refresh display */
    bool done = false;
    char *final_line = NULL;
    readline_context_t ctx = {
        .buffer = buffer,
        .done = &done,
        .final_line = &final_line,
        .term = term,
        .prompt = prompt,
        .continuation_state = &continuation_state,
        .kill_buffer = kill_buffer,
        .kill_buffer_size = kill_buffer_size
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
    /* Step 4: Initial display refresh to show prompt */
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
        
        /* Handle timeout - just continue */
        if (result == LLE_ERROR_TIMEOUT || event == NULL) {
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
                
                /* Check for Ctrl-D (EOF) */
                if (codepoint == 4) {  /* ASCII EOT */
                    handle_eof(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-A (beginning of line) */
                if (codepoint == 1) {  /* ASCII SOH */
                    handle_home(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-B (back one character) */
                if (codepoint == 2) {  /* ASCII STX */
                    handle_arrow_left(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-C (interrupt) */
                if (codepoint == 3) {  /* ASCII ETX */
                    handle_interrupt(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-E (end of line) */
                if (codepoint == 5) {  /* ASCII ENQ */
                    handle_end(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-F (forward one character) */
                if (codepoint == 6) {  /* ASCII ACK */
                    handle_arrow_right(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-G (abort/cancel line) */
                if (codepoint == 7) {  /* ASCII BEL */
                    handle_abort(NULL, &ctx);
                    break;
                }
                
                /* Check for backspace */
                if (codepoint == 127 || codepoint == 8) {  /* DEL or BS */
                    handle_backspace(NULL, &ctx);
                    break;
                }
                
                /* Step 5: Check for Ctrl-K (kill to end of line) */
                if (codepoint == 11) {  /* ASCII VT (Ctrl-K) */
                    handle_kill_to_end(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-L (clear screen) */
                if (codepoint == 12) {  /* ASCII FF */
                    handle_clear_screen(NULL, &ctx);
                    break;
                }
                
                /* Step 5: Check for Ctrl-U (kill entire line) */
                if (codepoint == 21) {  /* ASCII NAK (Ctrl-U) */
                    handle_kill_line(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-W (kill word backwards) */
                if (codepoint == 23) {  /* ASCII ETB */
                    handle_kill_word(NULL, &ctx);
                    break;
                }
                
                /* Check for Ctrl-Y (yank) */
                if (codepoint == 25) {  /* ASCII EM */
                    handle_yank(NULL, &ctx);
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
                /* Window resize - ignore for Step 1 */
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
    
    /* === STEP 10: Exit raw mode === */
    lle_unix_interface_exit_raw_mode(unix_iface);
    
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

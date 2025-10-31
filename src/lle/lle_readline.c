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
 * Implementation: Step 3 - Event System Integration
 * - Creates local terminal abstraction instance
 * - Uses terminal abstraction for raw mode
 * - Uses input processor for reading
 * - Uses lle_buffer_t for proper buffer management
 * - Uses event system to dispatch events to handlers
 * - Event handlers modify buffer (decoupled architecture)
 * - Returns on Enter key
 * 
 * NOTE: Creates own terminal, buffer, and event system for now.
 * Future steps will integrate with full LLE system initialization.
 */

#include "lle/terminal_abstraction.h"
#include "lle/buffer_management.h"
#include "lle/memory_management.h"
#include "lle/event_system.h"
#include "lle/error_handling.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/* External global memory pool */
extern lusush_memory_pool_t *global_memory_pool;

/* Event handler context for Step 3 */
typedef struct {
    lle_buffer_t *buffer;
    bool *done;
    char **final_line;
} readline_context_t;

/**
 * @brief Event handler for character input
 * Step 3: Handler modifies buffer instead of direct modification
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
    
    return result;
}

/**
 * @brief Event handler for backspace
 * Step 3: Handler modifies buffer
 */
static lle_result_t handle_backspace(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    if (ctx->buffer->cursor.byte_offset > 0) {
        /* Delete one byte before cursor (Step 5 will improve to grapheme) */
        size_t delete_pos = ctx->buffer->cursor.byte_offset - 1;
        lle_result_t result = lle_buffer_delete_text(ctx->buffer, delete_pos, 1);
        return result;
    }
    
    return LLE_SUCCESS;
}

/**
 * @brief Event handler for Enter key
 * Step 3: Handler signals completion
 */
static lle_result_t handle_enter(lle_event_t *event, void *user_data)
{
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
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
    
    /* === STEP 6: Register event handlers === */
    /* Step 3: Register handlers that will modify buffer */
    bool done = false;
    char *final_line = NULL;
    readline_context_t ctx = {
        .buffer = buffer,
        .done = &done,
        .final_line = &final_line
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
    /* Note: Prompt display comes in Step 4 (display integration). */
    (void)prompt;  /* Suppress unused warning */
    
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
                
                /* Check for Ctrl-C (interrupt) */
                if (codepoint == 3) {  /* ASCII ETX */
                    handle_interrupt(NULL, &ctx);
                    break;
                }
                
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
                /* Other special keys ignored in Step 3 */
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
    /* Step 3: Destroy event system, buffer, and terminal */
    lle_event_system_destroy(event_system);
    lle_buffer_destroy(buffer);
    lle_terminal_abstraction_destroy(term);
    
    return final_line;
}

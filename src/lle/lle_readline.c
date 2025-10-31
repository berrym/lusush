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
 * Implementation: Step 2 - Buffer Management Integration
 * - Creates local terminal abstraction instance
 * - Uses terminal abstraction for raw mode
 * - Uses input processor for reading
 * - Uses lle_buffer_t for proper buffer management
 * - Supports UTF-8, cursor tracking, undo/redo infrastructure
 * - Returns on Enter key
 * 
 * NOTE: Creates own terminal and memory pool for now.
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
    /* Step 2: Use proper lle_buffer_t instead of simple char array */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 256);
    if (result != LLE_SUCCESS || buffer == NULL) {
        /* Failed to create buffer */
        lle_unix_interface_exit_raw_mode(unix_iface);
        lle_terminal_abstraction_destroy(term);
        return NULL;
    }
    
    /* === STEP 5: Display prompt === */
    /* Note: Step 1 does not display the prompt. Display integration comes in Step 4. */
    /* The prompt parameter is acknowledged but not used in this minimal implementation. */
    (void)prompt;  /* Suppress unused warning */
    
    /* === STEP 6: Main input loop === */
    bool done = false;
    char *final_line = NULL;
    
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
        
        /* === STEP 7: Process input event === */
        switch (event->type) {
            case LLE_INPUT_TYPE_CHARACTER: {
                /* Regular character input */
                uint32_t codepoint = event->data.character.codepoint;
                
                /* Check for Enter key (newline) */
                if (codepoint == '\n' || codepoint == '\r') {
                    /* Line complete - get contents from buffer */
                    done = true;
                    final_line = buffer->data ? strdup(buffer->data) : strdup("");
                    break;
                }
                
                /* Check for Ctrl-D (EOF) */
                if (codepoint == 4) {  /* ASCII EOT */
                    if (buffer->length == 0) {
                        /* EOF on empty line */
                        done = true;
                        final_line = NULL;
                    }
                    break;
                }
                
                /* Check for Ctrl-C (interrupt) */
                if (codepoint == 3) {  /* ASCII ETX */
                    /* Interrupted */
                    done = true;
                    final_line = NULL;
                    break;
                }
                
                /* Check for backspace */
                if (codepoint == 127 || codepoint == 8) {  /* DEL or BS */
                    /* Step 2: Use lle_buffer_delete_text() for backspace */
                    if (buffer->cursor.byte_offset > 0) {
                        /* Delete one character before cursor */
                        /* For simplicity in Step 2, delete 1 byte (assumes ASCII) */
                        /* Step 5 will handle proper grapheme cluster deletion */
                        size_t delete_pos = buffer->cursor.byte_offset - 1;
                        result = lle_buffer_delete_text(buffer, delete_pos, 1);
                        (void)result;  /* Ignore errors for now */
                    }
                    break;
                }
                
                /* Add character to buffer */
                /* Step 2: Use lle_buffer_insert_text() */
                result = lle_buffer_insert_text(
                    buffer,
                    buffer->cursor.byte_offset,
                    event->data.character.utf8_bytes,
                    event->data.character.byte_count
                );
                (void)result;  /* Ignore errors for now */
                break;
            }
            
            case LLE_INPUT_TYPE_SPECIAL_KEY: {
                /* Special keys */
                if (event->data.special_key.key == LLE_KEY_ENTER) {
                    /* Enter key pressed */
                    done = true;
                    final_line = buffer->data ? strdup(buffer->data) : strdup("");
                }
                /* Other special keys ignored in Step 2 */
                break;
            }
            
            case LLE_INPUT_TYPE_EOF: {
                /* EOF received */
                if (buffer->length == 0) {
                    done = true;
                    final_line = NULL;
                } else {
                    /* Return partial line */
                    done = true;
                    final_line = buffer->data ? strdup(buffer->data) : strdup("");
                }
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
    
    /* === STEP 8: Exit raw mode === */
    lle_unix_interface_exit_raw_mode(unix_iface);
    
    /* === STEP 9: Cleanup and return === */
    /* Step 2: Destroy buffer using proper API */
    lle_buffer_destroy(buffer);
    lle_terminal_abstraction_destroy(term);
    
    return final_line;
}

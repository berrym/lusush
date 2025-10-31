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
 * Implementation: Step 1 - Minimal Implementation
 * - Creates local terminal abstraction instance
 * - Uses terminal abstraction for raw mode
 * - Uses input processor for reading
 * - Minimal local buffer for character accumulation
 * - Returns on Enter key
 * 
 * NOTE: Step 1 creates its own terminal instance for simplicity.
 * Future steps will integrate with full LLE system initialization.
 */

#include "lle/terminal_abstraction.h"
#include "lle/buffer_management.h"
#include "lle/event_system.h"
#include "lle/error_handling.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * @brief Read a line of input from the user with line editing
 * 
 * This is the core readline function that replaces GNU readline when LLE is enabled.
 * 
 * Step 1 Implementation:
 * - Verify LLE system is initialized
 * - Enter raw terminal mode
 * - Read input events one at a time
 * - Build simple line buffer (character accumulation)
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
    /* Note: Step 1 creates a minimal instance. Future steps will use global system. */
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
        return NULL;
    }
    
    /* === STEP 4: Simple line buffer for Step 1 === */
    /* Using a simple char buffer for minimal implementation */
    size_t buffer_capacity = 256;
    size_t buffer_length = 0;
    char *line_buffer = malloc(buffer_capacity);
    if (line_buffer == NULL) {
        lle_unix_interface_exit_raw_mode(unix_iface);
        return NULL;
    }
    memset(line_buffer, 0, buffer_capacity);
    
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
                    /* Line complete */
                    done = true;
                    final_line = strdup(line_buffer);
                    break;
                }
                
                /* Check for Ctrl-D (EOF) */
                if (codepoint == 4) {  /* ASCII EOT */
                    if (buffer_length == 0) {
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
                    if (buffer_length > 0) {
                        buffer_length--;
                        line_buffer[buffer_length] = '\0';
                    }
                    break;
                }
                
                /* Add character to buffer */
                if (buffer_length + event->data.character.byte_count < buffer_capacity - 1) {
                    memcpy(line_buffer + buffer_length, 
                           event->data.character.utf8_bytes,
                           event->data.character.byte_count);
                    buffer_length += event->data.character.byte_count;
                    line_buffer[buffer_length] = '\0';
                }
                break;
            }
            
            case LLE_INPUT_TYPE_SPECIAL_KEY: {
                /* Special keys */
                if (event->data.special_key.key == LLE_KEY_ENTER) {
                    /* Enter key pressed */
                    done = true;
                    final_line = strdup(line_buffer);
                }
                /* Other special keys ignored in Step 1 */
                break;
            }
            
            case LLE_INPUT_TYPE_EOF: {
                /* EOF received */
                if (buffer_length == 0) {
                    done = true;
                    final_line = NULL;
                } else {
                    /* Return partial line */
                    done = true;
                    final_line = strdup(line_buffer);
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
    free(line_buffer);
    lle_terminal_abstraction_destroy(term);
    
    return final_line;
}

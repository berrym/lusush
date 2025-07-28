/**
 * @file input_handler.c
 * @brief Key input handling utility functions for Lusush Line Editor (LLE)
 * 
 * This module implements the utility functions for key input handling,
 * providing support for key event manipulation, classification, string
 * conversion, and configuration management.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "input_handler.h"
#include "terminal_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

// ============================================================================
// Key Event Processing Functions (LLE-022)
// ============================================================================

/**
 * @brief Escape sequence mapping table
 */
typedef struct {
    const char *sequence;
    lle_key_type_t key_type;
    bool ctrl;
    bool alt;
    bool shift;
} lle_escape_mapping_t;

static const lle_escape_mapping_t escape_mappings[] = {
    // Arrow keys
    {"[A", LLE_KEY_ARROW_UP, false, false, false},
    {"[B", LLE_KEY_ARROW_DOWN, false, false, false},
    {"[C", LLE_KEY_ARROW_RIGHT, false, false, false},
    {"[D", LLE_KEY_ARROW_LEFT, false, false, false},
    
    // Home/End variants
    {"[H", LLE_KEY_HOME, false, false, false},
    {"[F", LLE_KEY_END, false, false, false},
    {"[1~", LLE_KEY_HOME, false, false, false},
    {"[4~", LLE_KEY_END, false, false, false},
    {"[7~", LLE_KEY_HOME, false, false, false},
    {"[8~", LLE_KEY_END, false, false, false},
    
    // Page Up/Down
    {"[5~", LLE_KEY_PAGE_UP, false, false, false},
    {"[6~", LLE_KEY_PAGE_DOWN, false, false, false},
    
    // Insert/Delete
    {"[2~", LLE_KEY_INSERT, false, false, false},
    {"[3~", LLE_KEY_DELETE, false, false, false},
    
    // Function keys
    {"OP", LLE_KEY_F1, false, false, false},
    {"OQ", LLE_KEY_F2, false, false, false},
    {"OR", LLE_KEY_F3, false, false, false},
    {"OS", LLE_KEY_F4, false, false, false},
    {"[15~", LLE_KEY_F5, false, false, false},
    {"[17~", LLE_KEY_F6, false, false, false},
    {"[18~", LLE_KEY_F7, false, false, false},
    {"[19~", LLE_KEY_F8, false, false, false},
    {"[20~", LLE_KEY_F9, false, false, false},
    {"[21~", LLE_KEY_F10, false, false, false},
    {"[23~", LLE_KEY_F11, false, false, false},
    {"[24~", LLE_KEY_F12, false, false, false},
    
    // Shift+Tab
    {"[Z", LLE_KEY_SHIFT_TAB, false, false, true},
    
    // Ctrl+Arrow keys
    {"[1;5A", LLE_KEY_ARROW_UP, true, false, false},
    {"[1;5B", LLE_KEY_ARROW_DOWN, true, false, false},
    {"[1;5C", LLE_KEY_CTRL_ARROW_RIGHT, true, false, false},
    {"[1;5D", LLE_KEY_CTRL_ARROW_LEFT, true, false, false},
    
    // Alt sequences (may start with different prefix)
    {"b", LLE_KEY_ALT_B, false, true, false},
    {"f", LLE_KEY_ALT_F, false, true, false},
    {"d", LLE_KEY_ALT_D, false, true, false},
    {".", LLE_KEY_ALT_DOT, false, true, false},
    {"_", LLE_KEY_ALT_UNDERSCORE, false, true, false},
    {"\x7f", LLE_KEY_ALT_BACKSPACE, false, true, false}, // Alt+Backspace
    
    {NULL, LLE_KEY_UNKNOWN, false, false, false} // Sentinel
};

/**
 * @brief Read raw bytes from terminal with timeout
 * @param fd File descriptor to read from
 * @param buffer Buffer to store data
 * @param buffer_size Maximum bytes to read
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes read, 0 on timeout, -1 on error
 */
static ssize_t read_with_timeout(int fd, char *buffer, size_t buffer_size, int timeout_ms) {
    fd_set readfds;
    struct timeval timeout;
    struct timeval *timeout_ptr;
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    if (timeout_ms == 0) {
        // No timeout - blocking read for piped input
        timeout_ptr = NULL;
    } else {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        timeout_ptr = &timeout;
    }
    
    int result = select(fd + 1, &readfds, NULL, NULL, timeout_ptr);
    if (result == -1) {
        return -1; // Error
    } else if (result == 0) {
        return 0; // Timeout
    }
    
    return read(fd, buffer, buffer_size);
}

/**
 * @brief Get current time in milliseconds
 * @return Current time in milliseconds
 */
static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
    return 0;
}

bool lle_input_read_key(lle_terminal_manager_t *tm, lle_key_event_t *event) {
    if (!tm || !event) return false;
    
    // Check for debug mode
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    // Initialize event
    lle_key_event_init(event);
    event->timestamp = get_current_time_ms();
    
    // Check if stdin is a TTY to determine timeout behavior
    bool is_tty = isatty(tm->stdin_fd);
    // For interactive TTY: no timeout for first character (wait for user input)
    // For piped input: no timeout (blocking read)
    int timeout_ms = 0; // Wait indefinitely for first character
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INPUT_READ_KEY] stdin_fd=%d, is_tty=%s, timeout=%dms (waiting for user input)\n", 
                tm->stdin_fd, is_tty ? "true" : "false", timeout_ms);
    }
    
    // Read first character - wait indefinitely for user input
    char buffer[16];
    ssize_t bytes_read = read_with_timeout(tm->stdin_fd, buffer, 1, timeout_ms);
    
    if (bytes_read == -1) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] Read error: %s\n", strerror(errno));
        }
        event->type = LLE_KEY_ERROR;
        return false;
    } else if (bytes_read == 0) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] EOF on input\n");
        }
        event->type = LLE_KEY_ERROR;
        return false;
    }
    
    unsigned char first_char = (unsigned char)buffer[0];
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INPUT_READ_KEY] Read character: 0x%02x ('%c')\n", 
                first_char, (first_char >= 32 && first_char <= 126) ? first_char : '?');
    }
    
    // Handle regular ASCII characters
    if (first_char >= 32 && first_char <= 126) {
        event->type = LLE_KEY_CHAR;
        event->character = first_char;
        event->raw_sequence[0] = first_char;
        event->sequence_length = 1;
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] Generated key type: LLE_KEY_CHAR\n");
        }
        return true;
    }
    
    // Handle control characters
    if (first_char < 32) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] Processing control character: 0x%02x\n", first_char);
        }
        switch (first_char) {
            case LLE_ASCII_CTRL_A: 
                event->type = LLE_KEY_CTRL_A; 
                if (debug_mode) fprintf(stderr, "[LLE_INPUT_READ_KEY] Detected Ctrl+A (0x01)\n");
                break;
            case LLE_ASCII_CTRL_B: event->type = LLE_KEY_CTRL_B; break;
            case LLE_ASCII_CTRL_C: event->type = LLE_KEY_CTRL_C; break;
            case LLE_ASCII_CTRL_D: event->type = LLE_KEY_CTRL_D; break;
            case LLE_ASCII_CTRL_E: 
                event->type = LLE_KEY_CTRL_E; 
                if (debug_mode) fprintf(stderr, "[LLE_INPUT_READ_KEY] Detected Ctrl+E (0x05)\n");
                break;
            case LLE_ASCII_CTRL_F: event->type = LLE_KEY_CTRL_F; break;
            case LLE_ASCII_CTRL_G: event->type = LLE_KEY_CTRL_G; break;
            case LLE_ASCII_CTRL_H: event->type = LLE_KEY_BACKSPACE; break; // Ctrl+H = Backspace
            case LLE_ASCII_CTRL_I: event->type = LLE_KEY_TAB; break; // Ctrl+I = Tab
            case LLE_ASCII_CTRL_J: event->type = LLE_KEY_ENTER; break; // Ctrl+J = Enter
            case LLE_ASCII_CTRL_K: event->type = LLE_KEY_CTRL_K; break;
            case LLE_ASCII_CTRL_L: event->type = LLE_KEY_CTRL_L; break;
            case LLE_ASCII_CTRL_M: event->type = LLE_KEY_ENTER; break; // Ctrl+M = Enter
            case LLE_ASCII_CTRL_N: event->type = LLE_KEY_CTRL_N; break;
            case LLE_ASCII_CTRL_O: event->type = LLE_KEY_CTRL_O; break;
            case LLE_ASCII_CTRL_P: event->type = LLE_KEY_CTRL_P; break;
            case LLE_ASCII_CTRL_Q: event->type = LLE_KEY_CTRL_Q; break;
            case LLE_ASCII_CTRL_R: event->type = LLE_KEY_CTRL_R; break;
            case LLE_ASCII_CTRL_S: event->type = LLE_KEY_CTRL_S; break;
            case LLE_ASCII_CTRL_T: event->type = LLE_KEY_CTRL_T; break;
            case LLE_ASCII_CTRL_U: event->type = LLE_KEY_CTRL_U; break;
            case LLE_ASCII_CTRL_V: event->type = LLE_KEY_CTRL_V; break;
            case LLE_ASCII_CTRL_W: event->type = LLE_KEY_CTRL_W; break;
            case LLE_ASCII_CTRL_X: event->type = LLE_KEY_CTRL_X; break;
            case LLE_ASCII_CTRL_Y: event->type = LLE_KEY_CTRL_Y; break;
            case LLE_ASCII_CTRL_Z: event->type = LLE_KEY_CTRL_Z; break;
            case LLE_ASCII_ESC:
                // Handle escape sequences
                event->type = LLE_KEY_ESCAPE;
                break;
            default:
                event->type = LLE_KEY_UNKNOWN;
                break;
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] Generated key type: %d (control character)\n", event->type);
        }
        
        event->character = first_char;
        event->raw_sequence[0] = first_char;
        event->sequence_length = 1;
        
        // Handle escape sequences specially
        if (first_char == LLE_ASCII_ESC) {
            // Read more characters to build escape sequence
            size_t seq_pos = 1;
            buffer[0] = first_char;
            
            while (seq_pos < sizeof(buffer) - 1) {
                bytes_read = read_with_timeout(tm->stdin_fd, &buffer[seq_pos], 1, 50); // Short timeout for sequence
                if (bytes_read <= 0) break;
                
                seq_pos++;
                
                // Check if we have a complete sequence
                buffer[seq_pos] = '\0';
                if (lle_input_parse_escape_sequence(buffer + 1, event)) {
                    // Copy raw sequence
                    memcpy(event->raw_sequence, buffer, seq_pos);
                    event->sequence_length = seq_pos;
                    return true;
                }
            }
            
            // If we couldn't parse as escape sequence, treat as plain ESC
            event->type = LLE_KEY_ESCAPE;
            event->raw_sequence[0] = LLE_ASCII_ESC;
            event->sequence_length = 1;
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] Generated key type: %d (escape sequence)\n", event->type);
        }
        return true;
    }
    
    // Handle high ASCII / DEL
    if (first_char == LLE_ASCII_DEL) {
        event->type = LLE_KEY_BACKSPACE; // DEL often means backspace
        event->character = first_char;
        event->raw_sequence[0] = first_char;
        event->sequence_length = 1;
        if (debug_mode) {
            fprintf(stderr, "[LLE_INPUT_READ_KEY] Generated key type: LLE_KEY_BACKSPACE (DEL)\n");
        }
        return true;
    }
    
    // Handle potential UTF-8 or other multi-byte sequences
    event->type = LLE_KEY_UNKNOWN;
    event->character = first_char;
    event->raw_sequence[0] = first_char;
    event->sequence_length = 1;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_INPUT_READ_KEY] Generated key type: LLE_KEY_UNKNOWN\n");
    }
    return true;
}

bool lle_input_parse_escape_sequence(const char *seq, lle_key_event_t *event) {
    if (!seq || !event) return false;
    
    // Check against our mapping table
    for (const lle_escape_mapping_t *mapping = escape_mappings; mapping->sequence; mapping++) {
        if (strcmp(seq, mapping->sequence) == 0) {
            event->type = mapping->key_type;
            event->ctrl = mapping->ctrl;
            event->alt = mapping->alt;
            event->shift = mapping->shift;
            return true;
        }
    }
    
    // Handle Alt+key sequences that start without '[' 
    if (strlen(seq) == 1) {
        for (const lle_escape_mapping_t *mapping = escape_mappings; mapping->sequence; mapping++) {
            if (mapping->alt && strcmp(seq, mapping->sequence) == 0) {
                event->type = mapping->key_type;
                event->ctrl = mapping->ctrl;
                event->alt = mapping->alt;
                event->shift = mapping->shift;
                return true;
            }
        }
    }
    
    return false; // Unknown sequence
}

bool lle_input_is_printable(const lle_key_event_t *event) {
    // This is a wrapper around the existing function for consistency with the API
    return lle_key_is_printable(event);
}

// ============================================================================
// Key Type Name Mappings
// ============================================================================

/**
 * @brief Key type to string name mappings
 */
static const char *key_type_names[] = {
    "CHAR",
    "BACKSPACE",
    "DELETE",
    "ENTER",
    "TAB",
    "ESCAPE",
    "ARROW_LEFT",
    "ARROW_RIGHT",
    "ARROW_UP",
    "ARROW_DOWN",
    "HOME",
    "END",
    "PAGE_UP",
    "PAGE_DOWN",
    "CTRL_A",
    "CTRL_B",
    "CTRL_C",
    "CTRL_D",
    "CTRL_E",
    "CTRL_F",
    "CTRL_G",
    "CTRL_H",
    "CTRL_I",
    "CTRL_J",
    "CTRL_K",
    "CTRL_L",
    "CTRL_M",
    "CTRL_N",
    "CTRL_O",
    "CTRL_P",
    "CTRL_Q",
    "CTRL_R",
    "CTRL_S",
    "CTRL_T",
    "CTRL_U",
    "CTRL_V",
    "CTRL_W",
    "CTRL_X",
    "CTRL_Y",
    "CTRL_Z",
    "ALT_B",
    "ALT_F",
    "ALT_D",
    "ALT_BACKSPACE",
    "ALT_DOT",
    "ALT_UNDERSCORE",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "INSERT",
    "CTRL_ARROW_LEFT",
    "CTRL_ARROW_RIGHT",
    "CTRL_DELETE",
    "CTRL_BACKSPACE",
    "SHIFT_TAB",
    "UNKNOWN",
    "INCOMPLETE",
    "TIMEOUT",
    "ERROR"
};

// ============================================================================
// Key Type Classification Functions
// ============================================================================

bool lle_key_is_printable(const lle_key_event_t *event) {
    if (!event) return false;
    
    // Only regular characters are printable
    if (event->type != LLE_KEY_CHAR) return false;
    
    // Check if character is printable ASCII
    return (event->character >= 32 && event->character <= 126);
}

bool lle_key_is_control(const lle_key_event_t *event) {
    if (!event) return false;
    
    // Control keys are those with ctrl modifier or control key types
    if (event->ctrl) return true;
    
    // Check for control key types
    return (event->type >= LLE_KEY_CTRL_A && event->type <= LLE_KEY_CTRL_Z);
}

bool lle_key_is_navigation(const lle_key_event_t *event) {
    if (!event) return false;
    
    switch (event->type) {
        case LLE_KEY_ARROW_LEFT:
        case LLE_KEY_ARROW_RIGHT:
        case LLE_KEY_ARROW_UP:
        case LLE_KEY_ARROW_DOWN:
        case LLE_KEY_HOME:
        case LLE_KEY_END:
        case LLE_KEY_PAGE_UP:
        case LLE_KEY_PAGE_DOWN:
        case LLE_KEY_CTRL_A:        // Beginning of line
        case LLE_KEY_CTRL_E:        // End of line
        case LLE_KEY_CTRL_B:        // Backward character
        case LLE_KEY_CTRL_F:        // Forward character
        case LLE_KEY_ALT_B:         // Backward word
        case LLE_KEY_ALT_F:         // Forward word
        case LLE_KEY_CTRL_ARROW_LEFT:
        case LLE_KEY_CTRL_ARROW_RIGHT:
            return true;
        default:
            return false;
    }
}

bool lle_key_is_editing(const lle_key_event_t *event) {
    if (!event) return false;
    
    switch (event->type) {
        case LLE_KEY_BACKSPACE:
        case LLE_KEY_DELETE:
        case LLE_KEY_INSERT:
        case LLE_KEY_CTRL_H:        // Backspace
        case LLE_KEY_CTRL_D:        // Delete character
        case LLE_KEY_CTRL_K:        // Kill to end of line
        case LLE_KEY_CTRL_U:        // Kill to beginning of line
        case LLE_KEY_CTRL_W:        // Kill word backward
        case LLE_KEY_CTRL_Y:        // Yank
        case LLE_KEY_CTRL_T:        // Transpose characters
        case LLE_KEY_ALT_D:         // Delete word forward
        case LLE_KEY_ALT_BACKSPACE: // Delete word backward
        case LLE_KEY_CTRL_DELETE:
        case LLE_KEY_CTRL_BACKSPACE:
            return true;
        default:
            return false;
    }
}

bool lle_key_is_function(const lle_key_event_t *event) {
    if (!event) return false;
    
    return (event->type >= LLE_KEY_F1 && event->type <= LLE_KEY_F12);
}

// ============================================================================
// Key Name and String Conversion Functions
// ============================================================================

const char *lle_key_type_to_string(lle_key_type_t key_type) {
    if (key_type < 0 || key_type >= LLE_KEY_TYPE_COUNT) {
        return NULL;
    }
    
    if (key_type < sizeof(key_type_names) / sizeof(key_type_names[0])) {
        return key_type_names[key_type];
    }
    
    return NULL;
}

lle_key_type_t lle_key_string_to_type(const char *key_name) {
    if (!key_name) return LLE_KEY_UNKNOWN;
    
    for (size_t i = 0; i < sizeof(key_type_names) / sizeof(key_type_names[0]); i++) {
        if (strcmp(key_name, key_type_names[i]) == 0) {
            return (lle_key_type_t)i;
        }
    }
    
    return LLE_KEY_UNKNOWN;
}

bool lle_key_event_to_string(const lle_key_event_t *event, char *buffer, size_t buffer_size) {
    if (!event || !buffer || buffer_size == 0) return false;
    
    // Build key description string
    char temp[256];
    temp[0] = '\0';
    
    // Add modifiers
    if (event->ctrl) strcat(temp, "Ctrl+");
    if (event->alt) strcat(temp, "Alt+");
    if (event->shift) strcat(temp, "Shift+");
    if (event->super) strcat(temp, "Super+");
    
    // Add key name
    if (event->type == LLE_KEY_CHAR && event->character > 0) {
        char char_str[8];
        if (isprint(event->character)) {
            snprintf(char_str, sizeof(char_str), "%c", event->character);
        } else {
            snprintf(char_str, sizeof(char_str), "\\x%02X", (unsigned char)event->character);
        }
        strcat(temp, char_str);
    } else {
        const char *key_name = lle_key_type_to_string(event->type);
        if (key_name) {
            strcat(temp, key_name);
        } else {
            strcat(temp, "UNKNOWN");
        }
    }
    
    // Copy to output buffer
    if (strlen(temp) >= buffer_size) {
        return false;
    }
    
    strcpy(buffer, temp);
    return true;
}

bool lle_key_string_to_event(const char *description, lle_key_event_t *event) {
    if (!description || !event) return false;
    
    // Initialize event
    lle_key_event_init(event);
    
    // Simple parser for basic descriptions
    char *desc_copy = strdup(description);
    if (!desc_copy) return false;
    
    char *token = strtok(desc_copy, "+");
    bool parsing_modifiers = true;
    
    while (token) {
        if (parsing_modifiers) {
            if (strcmp(token, "Ctrl") == 0) {
                event->ctrl = true;
            } else if (strcmp(token, "Alt") == 0) {
                event->alt = true;
            } else if (strcmp(token, "Shift") == 0) {
                event->shift = true;
            } else if (strcmp(token, "Super") == 0) {
                event->super = true;
            } else {
                // This is the key name
                parsing_modifiers = false;
                event->type = lle_key_string_to_type(token);
                if (event->type == LLE_KEY_UNKNOWN && strlen(token) == 1) {
                    event->type = LLE_KEY_CHAR;
                    event->character = token[0];
                }
            }
        }
        token = strtok(NULL, "+");
    }
    
    free(desc_copy);
    return true;
}

// ============================================================================
// Key Event Utility Functions
// ============================================================================

void lle_key_event_init(lle_key_event_t *event) {
    if (!event) return;
    
    memset(event, 0, sizeof(lle_key_event_t));
    event->type = LLE_KEY_UNKNOWN;
}

void lle_key_event_clear(lle_key_event_t *event) {
    if (!event) return;
    
    memset(event, 0, sizeof(lle_key_event_t));
    event->type = LLE_KEY_UNKNOWN;
}

bool lle_key_event_copy(lle_key_event_t *dest, const lle_key_event_t *src) {
    if (!dest || !src) return false;
    
    memcpy(dest, src, sizeof(lle_key_event_t));
    return true;
}

bool lle_key_event_equals(const lle_key_event_t *event1, const lle_key_event_t *event2) {
    if (!event1 || !event2) return false;
    
    // Compare key fields (ignore metadata like timestamp)
    return (event1->type == event2->type &&
            event1->character == event2->character &&
            event1->unicode == event2->unicode &&
            event1->ctrl == event2->ctrl &&
            event1->alt == event2->alt &&
            event1->shift == event2->shift &&
            event1->super == event2->super);
}

bool lle_key_event_matches_binding(const lle_key_event_t *event, const lle_key_binding_t *binding) {
    if (!event || !binding) return false;
    
    // Check key type match
    if (event->type != binding->key) return false;
    
    // Check modifier matches
    if (event->ctrl != binding->ctrl) return false;
    if (event->alt != binding->alt) return false;
    if (event->shift != binding->shift) return false;
    
    return true;
}

// ============================================================================
// Input Configuration Functions
// ============================================================================

bool lle_input_config_init(lle_input_config_t *config) {
    if (!config) return false;
    
    memset(config, 0, sizeof(lle_input_config_t));
    
    // Set default values
    config->escape_timeout_ms = LLE_DEFAULT_ESCAPE_TIMEOUT_MS;
    config->key_repeat_delay_ms = LLE_DEFAULT_KEY_REPEAT_DELAY_MS;
    config->key_repeat_rate_ms = LLE_DEFAULT_KEY_REPEAT_RATE_MS;
    
    // Set default behavior flags
    config->enable_ctrl_sequences = true;
    config->enable_alt_sequences = true;
    config->enable_mouse_input = false;
    config->enable_bracketed_paste = true;
    config->enable_focus_events = false;
    config->utf8_mode = true;
    config->compose_mode = false;
    config->debug_mode = false;
    config->log_all_keys = false;
    
    // Initialize binding storage
    config->bindings = NULL;
    config->binding_count = 0;
    config->binding_capacity = 0;
    config->key_event_count = 0;
    
    return true;
}

void lle_input_config_cleanup(lle_input_config_t *config) {
    if (!config) return;
    
    // Free bindings array
    if (config->bindings) {
        free(config->bindings);
        config->bindings = NULL;
    }
    
    // Clear all fields
    memset(config, 0, sizeof(lle_input_config_t));
}

bool lle_input_config_add_binding(lle_input_config_t *config, const lle_key_binding_t *binding) {
    if (!config || !binding) return false;
    
    // Check if we need to expand the bindings array
    if (config->binding_count >= config->binding_capacity) {
        size_t new_capacity = config->binding_capacity == 0 ? 16 : config->binding_capacity * 2;
        if (new_capacity > LLE_MAX_KEY_BINDINGS) {
            new_capacity = LLE_MAX_KEY_BINDINGS;
        }
        
        if (config->binding_count >= new_capacity) {
            return false; // Already at maximum capacity
        }
        
        lle_key_binding_t *new_bindings = realloc(config->bindings, 
                                                  new_capacity * sizeof(lle_key_binding_t));
        if (!new_bindings) return false;
        
        config->bindings = new_bindings;
        config->binding_capacity = new_capacity;
    }
    
    // Add the binding
    config->bindings[config->binding_count] = *binding;
    config->binding_count++;
    
    return true;
}

bool lle_input_config_remove_binding(lle_input_config_t *config, lle_key_type_t key, 
                                      bool ctrl, bool alt, bool shift) {
    if (!config || !config->bindings) return false;
    
    // Find the binding to remove
    for (size_t i = 0; i < config->binding_count; i++) {
        lle_key_binding_t *binding = &config->bindings[i];
        if (binding->key == key && 
            binding->ctrl == ctrl && 
            binding->alt == alt && 
            binding->shift == shift) {
            
            // Remove by shifting remaining elements
            for (size_t j = i; j < config->binding_count - 1; j++) {
                config->bindings[j] = config->bindings[j + 1];
            }
            config->binding_count--;
            return true;
        }
    }
    
    return false; // Binding not found
}

const lle_key_binding_t *lle_input_config_find_binding(const lle_input_config_t *config, 
                                                        const lle_key_event_t *event) {
    if (!config || !event || !config->bindings) return NULL;
    
    // Search for matching binding
    for (size_t i = 0; i < config->binding_count; i++) {
        const lle_key_binding_t *binding = &config->bindings[i];
        if (lle_key_event_matches_binding(event, binding)) {
            return binding;
        }
    }
    
    return NULL;
}
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

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
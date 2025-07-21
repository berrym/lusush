/**
 * @file input_handler.h
 * @brief Key input handling structures and definitions for Lusush Line Editor (LLE)
 * 
 * This module defines the key input handling system for LLE, providing comprehensive
 * support for keyboard input, special keys, modifier combinations, and escape sequence
 * processing. The system is designed to handle all essential editing keys and be
 * extensible for future key combinations.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef LLE_INPUT_HANDLER_H
#define LLE_INPUT_HANDLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "terminal_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length for escape sequences
 */
#define LLE_ESCAPE_SEQUENCE_MAX 16

/**
 * @brief Maximum length for key name strings
 */
#define LLE_KEY_NAME_MAX 32

/**
 * @brief Key type enumeration covering all essential editing keys
 * 
 * This enumeration defines all the key types that the line editor
 * needs to handle for basic and advanced editing operations.
 */
typedef enum {
    // Character input
    LLE_KEY_CHAR = 0,            // Regular printable character
    
    // Basic editing keys
    LLE_KEY_BACKSPACE,           // Backspace (delete before cursor)
    LLE_KEY_DELETE,              // Delete (delete at cursor)
    LLE_KEY_ENTER,               // Enter/Return (execute command)
    LLE_KEY_TAB,                 // Tab (completion)
    LLE_KEY_ESCAPE,              // Escape key
    
    // Navigation keys
    LLE_KEY_ARROW_LEFT,          // Left arrow (move cursor left)
    LLE_KEY_ARROW_RIGHT,         // Right arrow (move cursor right)
    LLE_KEY_ARROW_UP,            // Up arrow (history previous)
    LLE_KEY_ARROW_DOWN,          // Down arrow (history next)
    LLE_KEY_HOME,                // Home (beginning of line)
    LLE_KEY_END,                 // End (end of line)
    LLE_KEY_PAGE_UP,             // Page Up (scroll up)
    LLE_KEY_PAGE_DOWN,           // Page Down (scroll down)
    
    // Control key combinations
    LLE_KEY_CTRL_A,              // Ctrl+A (beginning of line)
    LLE_KEY_CTRL_B,              // Ctrl+B (backward character)
    LLE_KEY_CTRL_C,              // Ctrl+C (interrupt)
    LLE_KEY_CTRL_D,              // Ctrl+D (delete character/EOF)
    LLE_KEY_CTRL_E,              // Ctrl+E (end of line)
    LLE_KEY_CTRL_F,              // Ctrl+F (forward character)
    LLE_KEY_CTRL_G,              // Ctrl+G (abort)
    LLE_KEY_CTRL_H,              // Ctrl+H (backspace)
    LLE_KEY_CTRL_I,              // Ctrl+I (tab)
    LLE_KEY_CTRL_J,              // Ctrl+J (newline)
    LLE_KEY_CTRL_K,              // Ctrl+K (kill to end of line)
    LLE_KEY_CTRL_L,              // Ctrl+L (clear screen)
    LLE_KEY_CTRL_M,              // Ctrl+M (return)
    LLE_KEY_CTRL_N,              // Ctrl+N (next history)
    LLE_KEY_CTRL_O,              // Ctrl+O (operate)
    LLE_KEY_CTRL_P,              // Ctrl+P (previous history)
    LLE_KEY_CTRL_Q,              // Ctrl+Q (flow control)
    LLE_KEY_CTRL_R,              // Ctrl+R (reverse search)
    LLE_KEY_CTRL_S,              // Ctrl+S (forward search)
    LLE_KEY_CTRL_T,              // Ctrl+T (transpose characters)
    LLE_KEY_CTRL_U,              // Ctrl+U (kill to beginning of line)
    LLE_KEY_CTRL_V,              // Ctrl+V (literal next)
    LLE_KEY_CTRL_W,              // Ctrl+W (kill word backward)
    LLE_KEY_CTRL_X,              // Ctrl+X (prefix key)
    LLE_KEY_CTRL_Y,              // Ctrl+Y (yank)
    LLE_KEY_CTRL_Z,              // Ctrl+Z (suspend)
    
    // Alt/Meta key combinations
    LLE_KEY_ALT_B,               // Alt+B (backward word)
    LLE_KEY_ALT_F,               // Alt+F (forward word)
    LLE_KEY_ALT_D,               // Alt+D (delete word forward)
    LLE_KEY_ALT_BACKSPACE,       // Alt+Backspace (delete word backward)
    LLE_KEY_ALT_DOT,             // Alt+. (insert last argument)
    LLE_KEY_ALT_UNDERSCORE,      // Alt+_ (insert last argument)
    
    // Function keys
    LLE_KEY_F1,                  // F1
    LLE_KEY_F2,                  // F2
    LLE_KEY_F3,                  // F3
    LLE_KEY_F4,                  // F4
    LLE_KEY_F5,                  // F5
    LLE_KEY_F6,                  // F6
    LLE_KEY_F7,                  // F7
    LLE_KEY_F8,                  // F8
    LLE_KEY_F9,                  // F9
    LLE_KEY_F10,                 // F10
    LLE_KEY_F11,                 // F11
    LLE_KEY_F12,                 // F12
    
    // Extended keys
    LLE_KEY_INSERT,              // Insert key
    LLE_KEY_CTRL_ARROW_LEFT,     // Ctrl+Left (word backward)
    LLE_KEY_CTRL_ARROW_RIGHT,    // Ctrl+Right (word forward)
    LLE_KEY_CTRL_DELETE,         // Ctrl+Delete (delete word forward)
    LLE_KEY_CTRL_BACKSPACE,      // Ctrl+Backspace (delete word backward)
    LLE_KEY_SHIFT_TAB,           // Shift+Tab (reverse completion)
    
    // Special cases
    LLE_KEY_UNKNOWN,             // Unknown or unrecognized key
    LLE_KEY_INCOMPLETE,          // Incomplete escape sequence
    LLE_KEY_TIMEOUT,             // Input timeout
    LLE_KEY_ERROR,               // Input error
    
    // Count for validation
    LLE_KEY_TYPE_COUNT
} lle_key_type_t;

/**
 * @brief Key event structure containing all input information
 * 
 * This structure encapsulates a complete key event including the key type,
 * character data, modifier states, and additional metadata for processing.
 */
typedef struct {
    lle_key_type_t type;         // Type of key pressed
    char character;              // Character for printable keys (0 for special keys)
    uint32_t unicode;            // Unicode codepoint for extended characters
    
    // Modifier key states
    bool ctrl;                   // Ctrl key held
    bool alt;                    // Alt/Meta key held
    bool shift;                  // Shift key held
    bool super;                  // Super/Windows key held (future use)
    
    // Raw input data
    char raw_sequence[LLE_ESCAPE_SEQUENCE_MAX]; // Raw escape sequence
    size_t sequence_length;      // Length of raw sequence
    
    // Metadata
    uint64_t timestamp;          // Event timestamp (microseconds)
    bool is_repeat;              // True if this is a key repeat event
    uint32_t repeat_count;       // Number of repeats for this key
    
    // Processing flags
    bool processed;              // True if event has been processed
    bool consumed;               // True if event should not propagate
    bool synthetic;              // True if event was generated internally
} lle_key_event_t;

/**
 * @brief Key binding structure for mapping keys to actions
 */
typedef struct {
    lle_key_type_t key;          // Key type
    bool ctrl;                   // Ctrl modifier required
    bool alt;                    // Alt modifier required
    bool shift;                  // Shift modifier required
    const char *action_name;     // Name of action to perform
    void *action_data;           // Optional action-specific data
} lle_key_binding_t;

/**
 * @brief Input handler configuration
 */
typedef struct {
    // Timeout settings
    int escape_timeout_ms;       // Timeout for escape sequences (default: 100ms)
    int key_repeat_delay_ms;     // Delay before key repeat starts (default: 500ms)
    int key_repeat_rate_ms;      // Rate of key repeat (default: 50ms)
    
    // Behavior flags
    bool enable_ctrl_sequences;  // Enable Ctrl+key combinations
    bool enable_alt_sequences;   // Enable Alt+key combinations
    bool enable_mouse_input;     // Enable mouse event processing
    bool enable_bracketed_paste; // Enable bracketed paste mode
    bool enable_focus_events;    // Enable focus in/out events
    
    // Character encoding
    bool utf8_mode;              // Enable UTF-8 character processing
    bool compose_mode;           // Enable compose key sequences
    
    // Key bindings
    lle_key_binding_t *bindings; // Array of key bindings
    size_t binding_count;        // Number of key bindings
    size_t binding_capacity;     // Capacity of bindings array
    
    // Debug and diagnostics
    bool debug_mode;             // Enable debug output
    bool log_all_keys;           // Log all key events
    size_t key_event_count;      // Total key events processed
} lle_input_config_t;

/**
 * @brief Input handler state structure
 */
typedef struct {
    // Configuration
    lle_input_config_t config;   // Input handler configuration
    
    // Current state
    lle_key_event_t last_event;  // Last processed key event
    char escape_buffer[LLE_ESCAPE_SEQUENCE_MAX]; // Buffer for escape sequences
    size_t escape_buffer_pos;    // Current position in escape buffer
    uint64_t escape_start_time;  // When current escape sequence started
    
    // Key repeat state
    lle_key_type_t repeat_key;   // Currently repeating key
    uint64_t repeat_start_time;  // When repeat started
    uint64_t last_repeat_time;   // Last repeat event time
    uint32_t repeat_count;       // Current repeat count
    
    // Statistics
    uint64_t total_events;       // Total events processed
    uint64_t char_events;        // Character events
    uint64_t special_events;     // Special key events
    uint64_t escape_sequences;   // Escape sequences processed
    uint64_t unknown_events;     // Unknown/unhandled events
    uint64_t error_events;       // Error events
    
    // Internal state
    bool initialized;            // Handler is initialized
    bool active;                 // Handler is active and processing
    int terminal_fd;             // Terminal file descriptor
    void *terminal_manager;      // Pointer to terminal manager
} lle_input_handler_t;

// ============================================================================
// Key Type Classification Functions
// ============================================================================

/**
 * Check if key event represents a printable character
 * @param event Key event to check
 * @return true if character is printable, false otherwise
 */
bool lle_key_is_printable(const lle_key_event_t *event);

/**
 * Check if key event represents a control character
 * @param event Key event to check
 * @return true if character is a control character, false otherwise
 */
bool lle_key_is_control(const lle_key_event_t *event);

/**
 * Check if key event represents a navigation key
 * @param event Key event to check
 * @return true if key is for navigation, false otherwise
 */
bool lle_key_is_navigation(const lle_key_event_t *event);

/**
 * Check if key event represents an editing key
 * @param event Key event to check
 * @return true if key is for editing, false otherwise
 */
bool lle_key_is_editing(const lle_key_event_t *event);

/**
 * Check if key event represents a function key
 * @param event Key event to check
 * @return true if key is a function key, false otherwise
 */
bool lle_key_is_function(const lle_key_event_t *event);

// ============================================================================
// Key Name and String Conversion Functions
// ============================================================================

/**
 * Convert key type to string name
 * @param key_type Key type to convert
 * @return String name of key type, NULL for invalid type
 */
const char *lle_key_type_to_string(lle_key_type_t key_type);

/**
 * Convert string name to key type
 * @param key_name String name of key
 * @return Key type, LLE_KEY_UNKNOWN for invalid name
 */
lle_key_type_t lle_key_string_to_type(const char *key_name);

/**
 * Get descriptive name for key event
 * @param event Key event
 * @param buffer Buffer to store name
 * @param buffer_size Size of buffer
 * @return true on success, false on error
 */
bool lle_key_event_to_string(const lle_key_event_t *event, char *buffer, size_t buffer_size);

/**
 * Parse key description string into key event
 * @param description Key description (e.g., "Ctrl+C", "Alt+F4")
 * @param event Output key event
 * @return true on success, false on parse error
 */
bool lle_key_string_to_event(const char *description, lle_key_event_t *event);

// ============================================================================
// Key Event Utility Functions
// ============================================================================

/**
 * Initialize key event structure
 * @param event Key event to initialize
 */
void lle_key_event_init(lle_key_event_t *event);

/**
 * Clear key event structure
 * @param event Key event to clear
 */
void lle_key_event_clear(lle_key_event_t *event);

/**
 * Copy key event structure
 * @param dest Destination event
 * @param src Source event
 * @return true on success, false on error
 */
bool lle_key_event_copy(lle_key_event_t *dest, const lle_key_event_t *src);

/**
 * Compare two key events for equality
 * @param event1 First event
 * @param event2 Second event
 * @return true if events are equal, false otherwise
 */
bool lle_key_event_equals(const lle_key_event_t *event1, const lle_key_event_t *event2);

/**
 * Check if key event matches key binding
 * @param event Key event to check
 * @param binding Key binding to match against
 * @return true if event matches binding, false otherwise
 */
bool lle_key_event_matches_binding(const lle_key_event_t *event, const lle_key_binding_t *binding);

// ============================================================================
// Key Event Processing Functions (LLE-022)
// ============================================================================

/**
 * Read key from terminal and convert to key event
 * @param tm Pointer to terminal manager structure
 * @param event Pointer to key event structure to populate
 * @return true on success, false on error or timeout
 */
bool lle_input_read_key(lle_terminal_manager_t *tm, lle_key_event_t *event);

/**
 * Parse escape sequence into key event
 * @param seq Escape sequence string (without leading ESC)
 * @param event Pointer to key event structure to populate
 * @return true if sequence was recognized, false otherwise
 */
bool lle_input_parse_escape_sequence(const char *seq, lle_key_event_t *event);

/**
 * Check if key event represents printable character
 * @param event Pointer to key event structure
 * @return true if printable, false otherwise
 */
bool lle_input_is_printable(const lle_key_event_t *event);

// ============================================================================
// Input Configuration Functions
// ============================================================================

/**
 * Initialize input configuration with defaults
 * @param config Configuration structure to initialize
 * @return true on success, false on error
 */
bool lle_input_config_init(lle_input_config_t *config);

/**
 * Cleanup input configuration
 * @param config Configuration structure to cleanup
 */
void lle_input_config_cleanup(lle_input_config_t *config);

/**
 * Add key binding to configuration
 * @param config Configuration structure
 * @param binding Key binding to add
 * @return true on success, false on error
 */
bool lle_input_config_add_binding(lle_input_config_t *config, const lle_key_binding_t *binding);

/**
 * Remove key binding from configuration
 * @param config Configuration structure
 * @param key Key type to remove binding for
 * @param ctrl Ctrl modifier
 * @param alt Alt modifier
 * @param shift Shift modifier
 * @return true if binding was removed, false if not found
 */
bool lle_input_config_remove_binding(lle_input_config_t *config, lle_key_type_t key, 
                                      bool ctrl, bool alt, bool shift);

/**
 * Find key binding in configuration
 * @param config Configuration structure
 * @param event Key event to find binding for
 * @return Pointer to binding if found, NULL otherwise
 */
const lle_key_binding_t *lle_input_config_find_binding(const lle_input_config_t *config, 
                                                        const lle_key_event_t *event);

// ============================================================================
// Constants and Default Values
// ============================================================================

/**
 * @brief Default escape sequence timeout in milliseconds
 */
#define LLE_DEFAULT_ESCAPE_TIMEOUT_MS 100

/**
 * @brief Default key repeat delay in milliseconds
 */
#define LLE_DEFAULT_KEY_REPEAT_DELAY_MS 500

/**
 * @brief Default key repeat rate in milliseconds
 */
#define LLE_DEFAULT_KEY_REPEAT_RATE_MS 50

/**
 * @brief Maximum number of key bindings
 */
#define LLE_MAX_KEY_BINDINGS 256

/**
 * @brief Common ASCII control characters
 */
#define LLE_ASCII_CTRL_A 0x01
#define LLE_ASCII_CTRL_B 0x02
#define LLE_ASCII_CTRL_C 0x03
#define LLE_ASCII_CTRL_D 0x04
#define LLE_ASCII_CTRL_E 0x05
#define LLE_ASCII_CTRL_F 0x06
#define LLE_ASCII_CTRL_G 0x07
#define LLE_ASCII_CTRL_H 0x08
#define LLE_ASCII_CTRL_I 0x09
#define LLE_ASCII_CTRL_J 0x0A
#define LLE_ASCII_CTRL_K 0x0B
#define LLE_ASCII_CTRL_L 0x0C
#define LLE_ASCII_CTRL_M 0x0D
#define LLE_ASCII_CTRL_N 0x0E
#define LLE_ASCII_CTRL_O 0x0F
#define LLE_ASCII_CTRL_P 0x10
#define LLE_ASCII_CTRL_Q 0x11
#define LLE_ASCII_CTRL_R 0x12
#define LLE_ASCII_CTRL_S 0x13
#define LLE_ASCII_CTRL_T 0x14
#define LLE_ASCII_CTRL_U 0x15
#define LLE_ASCII_CTRL_V 0x16
#define LLE_ASCII_CTRL_W 0x17
#define LLE_ASCII_CTRL_X 0x18
#define LLE_ASCII_CTRL_Y 0x19
#define LLE_ASCII_CTRL_Z 0x1A

#define LLE_ASCII_ESC 0x1B
#define LLE_ASCII_DEL 0x7F
#define LLE_ASCII_BACKSPACE 0x08
#define LLE_ASCII_TAB 0x09
#define LLE_ASCII_NEWLINE 0x0A
#define LLE_ASCII_RETURN 0x0D

#ifdef __cplusplus
}
#endif

#endif // LLE_INPUT_HANDLER_H
// src/lle/foundation/input/input_processor.h
//
// LLE Input Processor (Week 10)
//
// Simple keyboard input processor that connects terminal input to buffer operations.
// Week 10 scope: Basic character input, cursor movement, simple key bindings.
//
// TODO Phase 1 Month 2: Implement full Spec 06 Input Parsing System
// - Complete escape sequence parser (see docs/lle_specification/06_input_parsing_complete.md)
// - Mouse input processing
// - Widget hook integration  
// - Keybinding engine integration
// - Adaptive terminal integration
// - Event system integration
// - UTF-8 multi-byte sequence handling
// - Parser state machine
// - Input stream management
//
// TODO Phase 1 Month 3: Advanced key binding features
// - Custom keymaps
// - Mode-specific bindings (insert/command/visual modes)
// - Key sequence macros
// - Rebindable keys

#ifndef LLE_FOUNDATION_INPUT_PROCESSOR_H
#define LLE_FOUNDATION_INPUT_PROCESSOR_H

#include "../buffer/buffer_manager.h"
#include "../display/display_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

// Key codes for special keys (terminal escape sequences)
typedef enum {
    LLE_KEY_CHAR = 0,           // Regular character (value in ch field)
    LLE_KEY_BACKSPACE = 127,    // Backspace
    LLE_KEY_DELETE = 1000,      // Delete key
    LLE_KEY_ARROW_UP,           // Arrow up
    LLE_KEY_ARROW_DOWN,         // Arrow down
    LLE_KEY_ARROW_LEFT,         // Arrow left
    LLE_KEY_ARROW_RIGHT,        // Arrow right
    LLE_KEY_HOME,               // Home key
    LLE_KEY_END,                // End key
    LLE_KEY_PAGE_UP,            // Page up
    LLE_KEY_PAGE_DOWN,          // Page down
    LLE_KEY_ENTER,              // Enter/return
    LLE_KEY_TAB,                // Tab
    LLE_KEY_ESCAPE,             // Escape
    
    // Ctrl combinations
    LLE_KEY_CTRL_A,             // Ctrl+A (beginning of line)
    LLE_KEY_CTRL_E,             // Ctrl+E (end of line)
    LLE_KEY_CTRL_K,             // Ctrl+K (kill to end of line)
    LLE_KEY_CTRL_U,             // Ctrl+U (kill to beginning of line)
    LLE_KEY_CTRL_D,             // Ctrl+D (delete forward / EOF)
    LLE_KEY_CTRL_L,             // Ctrl+L (clear screen)
    LLE_KEY_CTRL_C,             // Ctrl+C (interrupt)
    
    LLE_KEY_UNKNOWN = 9999      // Unknown/unsupported key
} lle_key_code_t;

// Input event (parsed key press) - Week 10 simple version
// TODO Phase 1 Month 2: Migrate to full lle_input_event_t from terminal.h (Spec 06)
typedef struct {
    lle_key_code_t key;         // Key code
    char ch;                    // Character (for LLE_KEY_CHAR)
    bool ctrl;                  // Ctrl modifier
    bool alt;                   // Alt modifier (TODO: Phase 1 Month 2)
    bool shift;                 // Shift modifier (TODO: Phase 1 Month 2)
} lle_key_event_t;  // Renamed to avoid conflict with terminal.h

// Input processor state - Week 10 simple version
// TODO Phase 1 Month 2: Migrate to full lle_input_processor_t from terminal.h (Spec 06)
typedef struct {
    // Terminal state
    int input_fd;               // Input file descriptor
    struct termios orig_termios; // Original terminal settings
    struct termios raw_termios;  // Raw mode terminal settings
    bool raw_mode_enabled;      // Raw mode active
    
    // Integration references (not owned)
    lle_buffer_manager_t *buffer_manager;
    lle_display_buffer_renderer_t *renderer;
    
    // Input buffer for escape sequence parsing
    char input_buffer[32];      // Buffer for reading escape sequences
    size_t input_buffer_len;    // Current length
    
    // Statistics
    uint64_t keys_processed;
    uint64_t chars_inserted;
    uint64_t chars_deleted;
    uint64_t cursor_moves;
    
    // State
    bool initialized;
    bool running;
} lle_simple_input_processor_t;  // Renamed to avoid conflict with terminal.h

// Error codes
typedef enum {
    LLE_INPUT_OK = 0,
    LLE_INPUT_ERR_NULL_PTR = -1,
    LLE_INPUT_ERR_NOT_INIT = -2,
    LLE_INPUT_ERR_TERMIOS = -3,
    LLE_INPUT_ERR_READ = -4,
    LLE_INPUT_ERR_UNKNOWN_KEY = -5,
    LLE_INPUT_ERR_EOF = -6,
    LLE_INPUT_ERR_TIMEOUT = -7,  // Timeout waiting for escape sequence bytes
} lle_input_error_t;

// Initialize input processor
int lle_simple_input_init(lle_simple_input_processor_t *processor,
                         int input_fd,
                         lle_buffer_manager_t *buffer_manager,
                         lle_display_buffer_renderer_t *renderer);

// Enable raw terminal mode (for character-by-character input)
int lle_simple_input_enable_raw_mode(lle_simple_input_processor_t *processor);

// Disable raw terminal mode (restore original settings)
int lle_simple_input_disable_raw_mode(lle_simple_input_processor_t *processor);

// Read and parse next input event
int lle_simple_input_read_event(lle_simple_input_processor_t *processor,
                                lle_key_event_t *event);

// Process input event (execute corresponding action)
int lle_simple_input_process_event(lle_simple_input_processor_t *processor,
                                   const lle_key_event_t *event);

// Main input loop (read event → process → render until exit)
int lle_simple_input_run(lle_simple_input_processor_t *processor);

// Action handlers (can be called directly or via process_event)
int lle_input_action_insert_char(lle_simple_input_processor_t *processor, char ch);
int lle_input_action_backspace(lle_simple_input_processor_t *processor);
int lle_input_action_delete(lle_simple_input_processor_t *processor);
int lle_input_action_move_left(lle_simple_input_processor_t *processor);
int lle_input_action_move_right(lle_simple_input_processor_t *processor);
int lle_input_action_move_up(lle_simple_input_processor_t *processor);
int lle_input_action_move_down(lle_simple_input_processor_t *processor);
int lle_input_action_move_home(lle_simple_input_processor_t *processor);
int lle_input_action_move_end(lle_simple_input_processor_t *processor);
int lle_input_action_page_up(lle_simple_input_processor_t *processor);
int lle_input_action_page_down(lle_simple_input_processor_t *processor);
int lle_input_action_newline(lle_simple_input_processor_t *processor);

// Ctrl key actions
int lle_input_action_beginning_of_line(lle_simple_input_processor_t *processor);  // Ctrl+A
int lle_input_action_end_of_line(lle_simple_input_processor_t *processor);        // Ctrl+E
int lle_input_action_kill_line(lle_simple_input_processor_t *processor);          // Ctrl+K
int lle_input_action_kill_backward(lle_simple_input_processor_t *processor);      // Ctrl+U
int lle_input_action_clear_screen(lle_simple_input_processor_t *processor);       // Ctrl+L

// Get statistics
void lle_simple_input_get_stats(const lle_simple_input_processor_t *processor,
                                uint64_t *keys_processed,
                                uint64_t *chars_inserted,
                                uint64_t *chars_deleted,
                                uint64_t *cursor_moves);

// Cleanup
void lle_simple_input_cleanup(lle_simple_input_processor_t *processor);

// Utility: Convert error code to string
const char* lle_input_error_string(int error_code);

// Utility: Convert key code to string (for debugging)
const char* lle_key_code_string(lle_key_code_t key);

#endif // LLE_FOUNDATION_INPUT_PROCESSOR_H

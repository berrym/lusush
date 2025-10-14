// src/lle/validation/terminal/state.h

#ifndef LLE_VALIDATION_TERMINAL_STATE_H
#define LLE_VALIDATION_TERMINAL_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

// Terminal state - internal source of truth
typedef struct {
    // Cursor position (0-based)
    uint16_t cursor_row;
    uint16_t cursor_col;
    
    // Screen dimensions
    uint16_t rows;
    uint16_t cols;
    
    // Scroll region
    uint16_t scroll_top;
    uint16_t scroll_bottom;
    
    // Terminal mode flags
    bool application_keypad;
    bool application_cursor;
    bool auto_wrap;
    bool origin_mode;
    
    // Validation tracking
    uint64_t last_update_ns;  // Nanosecond timestamp
    uint32_t update_count;
} lle_terminal_state_t;

// Terminal capabilities (detected once at init)
typedef struct {
    bool has_color;
    bool has_256_color;
    bool has_true_color;
    bool has_unicode;
    bool has_mouse;
    bool has_bracketed_paste;
    
    // Terminal type
    enum {
        TERM_VT100,
        TERM_XTERM,
        TERM_XTERM_256,
        TERM_RXVT,
        TERM_SCREEN,
        TERM_TMUX,
        TERM_UNKNOWN
    } type;
    
    // Detection metadata
    uint32_t detection_time_ms;
    bool detection_successful;
} lle_terminal_caps_t;

// Terminal validation context
typedef struct {
    // Original terminal settings (restore on exit)
    struct termios original_termios;
    
    // Raw mode settings
    struct termios raw_termios;
    
    // Internal state (authority)
    lle_terminal_state_t state;
    
    // Capabilities (detected once)
    lle_terminal_caps_t caps;
    
    // File descriptors
    int input_fd;   // stdin
    int output_fd;  // stdout
    
    // Validation metrics
    uint64_t total_updates;
    uint64_t total_update_time_ns;
} lle_terminal_validation_t;

// Initialize terminal validation
int lle_terminal_validation_init(lle_terminal_validation_t *term);

// Update internal state (never query terminal)
void lle_terminal_state_update_cursor(lle_terminal_validation_t *term,
                                       uint16_t row, uint16_t col);

// One-time capability detection (at initialization)
int lle_terminal_detect_capabilities(lle_terminal_validation_t *term,
                                      uint32_t timeout_ms);

// Cleanup
void lle_terminal_validation_cleanup(lle_terminal_validation_t *term);

#endif // LLE_VALIDATION_TERMINAL_STATE_H

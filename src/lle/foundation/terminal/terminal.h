// src/lle/foundation/terminal/terminal.h
//
// LLE Terminal Abstraction Layer (Production)
//
// This module implements the terminal state abstraction validated in Phase 0.
// Key architectural principles:
// - Internal state is the single source of truth
// - Zero terminal queries during operation (except SIGWINCH)
// - One-time capability detection at initialization
// - Thread-safe state updates

#ifndef LLE_FOUNDATION_TERMINAL_H
#define LLE_FOUNDATION_TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

// Error codes
typedef enum {
    LLE_TERM_OK = 0,
    LLE_TERM_ERR_INVALID_FD = -1,
    LLE_TERM_ERR_TERMIOS = -2,
    LLE_TERM_ERR_WINSIZE = -3,
    LLE_TERM_ERR_TIMEOUT = -4,
    LLE_TERM_ERR_NOT_TTY = -5,
    LLE_TERM_ERR_NULL_PTR = -6,
    LLE_TERM_ERR_ALREADY_INIT = -7,
} lle_term_error_t;

// Terminal type detection
typedef enum {
    LLE_TERM_TYPE_VT100,
    LLE_TERM_TYPE_XTERM,
    LLE_TERM_TYPE_XTERM_256,
    LLE_TERM_TYPE_KONSOLE,
    LLE_TERM_TYPE_GNOME,
    LLE_TERM_TYPE_ALACRITTY,
    LLE_TERM_TYPE_KITTY,
    LLE_TERM_TYPE_RXVT,
    LLE_TERM_TYPE_SCREEN,
    LLE_TERM_TYPE_TMUX,
    LLE_TERM_TYPE_UNKNOWN
} lle_term_type_t;

// Terminal capabilities (detected once at initialization)
typedef struct {
    // Color support
    bool has_color;              // Basic 8/16 color
    bool has_256_color;          // 256 color palette
    bool has_true_color;         // 24-bit RGB color
    
    // Text rendering
    bool has_unicode;            // UTF-8 support
    bool has_bold;               // Bold text
    bool has_underline;          // Underline text
    bool has_italic;             // Italic text
    
    // Interactive features
    bool has_mouse;              // Mouse support
    bool has_bracketed_paste;    // Bracketed paste mode
    bool has_focus_events;       // Focus in/out events
    
    // Terminal type
    lle_term_type_t type;
    
    // Detection metadata
    uint32_t detection_time_ms;
    bool detection_successful;
    bool detection_timed_out;
} lle_term_capabilities_t;

// Terminal state (internal source of truth)
typedef struct {
    // Cursor position (0-based)
    uint16_t cursor_row;
    uint16_t cursor_col;
    
    // Screen dimensions
    uint16_t rows;
    uint16_t cols;
    
    // Scroll region (0-based, inclusive)
    uint16_t scroll_top;
    uint16_t scroll_bottom;
    
    // Terminal mode flags
    bool application_keypad;     // Application keypad mode
    bool application_cursor;     // Application cursor keys
    bool auto_wrap;              // Automatic line wrapping
    bool origin_mode;            // Origin mode (scroll region)
    bool bracketed_paste_active; // Bracketed paste enabled
    bool mouse_tracking_active;  // Mouse tracking enabled
    
    // State tracking
    uint64_t last_update_ns;     // Last update timestamp
    uint32_t update_count;       // Total state updates
    bool size_changed;           // Window size changed flag
} lle_term_state_t;

// Terminal context (main handle)
typedef struct {
    // Terminal settings
    struct termios original_termios;  // Original settings (restore on exit)
    struct termios raw_termios;       // Raw mode settings
    
    // Internal state (authority)
    lle_term_state_t state;
    
    // Capabilities (detected once)
    lle_term_capabilities_t caps;
    
    // File descriptors
    int input_fd;    // Typically STDIN_FILENO
    int output_fd;   // Typically STDOUT_FILENO
    
    // Initialization state
    bool initialized;
    bool raw_mode_active;
    
    // Performance metrics
    uint64_t total_updates;
    uint64_t total_update_time_ns;
} lle_term_t;

// Initialize terminal abstraction
// This performs one-time capability detection and setup.
// Returns LLE_TERM_OK on success, error code on failure.
int lle_term_init(lle_term_t *term, int input_fd, int output_fd);

// Enter raw mode (disable canonical mode, echo, signals)
int lle_term_enter_raw_mode(lle_term_t *term);

// Exit raw mode (restore original terminal settings)
int lle_term_exit_raw_mode(lle_term_t *term);

// Update cursor position (internal state only, NO terminal query)
void lle_term_update_cursor(lle_term_t *term, uint16_t row, uint16_t col);

// Update window size (called on SIGWINCH)
// This is the ONLY time we query terminal (via ioctl TIOCGWINSZ)
int lle_term_update_size(lle_term_t *term);

// Update scroll region
void lle_term_update_scroll_region(lle_term_t *term,
                                    uint16_t top, uint16_t bottom);

// Enable/disable terminal features
int lle_term_enable_bracketed_paste(lle_term_t *term, bool enable);
int lle_term_enable_mouse_tracking(lle_term_t *term, bool enable);

// Query internal state (NO terminal queries, just return cached state)
const lle_term_state_t* lle_term_get_state(const lle_term_t *term);
const lle_term_capabilities_t* lle_term_get_capabilities(const lle_term_t *term);

// Cleanup terminal (restore original settings)
void lle_term_cleanup(lle_term_t *term);

// Utility: Convert error code to string
const char* lle_term_error_string(int error_code);

#endif // LLE_FOUNDATION_TERMINAL_H

// src/lle/foundation/terminal/terminal.c
//
// LLE Terminal Abstraction Layer Implementation

#include "terminal.h"
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>

// Get nanosecond timestamp
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Detect terminal capabilities (forward declaration)
static int detect_capabilities(lle_term_t *term, uint32_t timeout_ms);

int lle_term_init(lle_term_t *term, int input_fd, int output_fd) {
    if (!term) {
        return LLE_TERM_ERR_NULL_PTR;
    }
    
    if (term->initialized) {
        return LLE_TERM_ERR_ALREADY_INIT;
    }
    
    // Zero-initialize structure
    memset(term, 0, sizeof(*term));
    
    // Validate file descriptors
    if (!isatty(input_fd)) {
        return LLE_TERM_ERR_NOT_TTY;
    }
    
    term->input_fd = input_fd;
    term->output_fd = output_fd;
    
    // Save original terminal settings
    if (tcgetattr(input_fd, &term->original_termios) != 0) {
        return LLE_TERM_ERR_TERMIOS;
    }
    
    // Setup raw mode configuration (don't activate yet)
    term->raw_termios = term->original_termios;
    term->raw_termios.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    term->raw_termios.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    term->raw_termios.c_oflag &= ~(OPOST);
    term->raw_termios.c_cflag |= (CS8);
    term->raw_termios.c_cc[VMIN] = 0;   // Non-blocking read
    term->raw_termios.c_cc[VTIME] = 0;  // No timeout
    
    // Get initial window size (ONE-TIME ioctl, not repeated)
    struct winsize ws;
    if (ioctl(output_fd, TIOCGWINSZ, &ws) != 0) {
        // Use default fallback if ioctl fails
        term->state.rows = 24;
        term->state.cols = 80;
    } else {
        term->state.rows = ws.ws_row > 0 ? ws.ws_row : 24;
        term->state.cols = ws.ws_col > 0 ? ws.ws_col : 80;
    }
    
    // Initialize scroll region to full screen
    term->state.scroll_top = 0;
    term->state.scroll_bottom = term->state.rows - 1;
    
    // Initialize cursor to home position
    term->state.cursor_row = 0;
    term->state.cursor_col = 0;
    
    // Set default mode flags
    term->state.auto_wrap = true;
    term->state.application_keypad = false;
    term->state.application_cursor = false;
    term->state.origin_mode = false;
    term->state.bracketed_paste_active = false;
    term->state.mouse_tracking_active = false;
    
    // Detect terminal capabilities (one-time, 50ms timeout)
    detect_capabilities(term, 50);
    
    term->initialized = true;
    term->raw_mode_active = false;
    
    return LLE_TERM_OK;
}

int lle_term_enter_raw_mode(lle_term_t *term) {
    if (!term || !term->initialized) {
        return LLE_TERM_ERR_NULL_PTR;
    }
    
    if (term->raw_mode_active) {
        return LLE_TERM_OK;  // Already in raw mode
    }
    
    if (tcsetattr(term->input_fd, TCSAFLUSH, &term->raw_termios) != 0) {
        return LLE_TERM_ERR_TERMIOS;
    }
    
    term->raw_mode_active = true;
    return LLE_TERM_OK;
}

int lle_term_exit_raw_mode(lle_term_t *term) {
    if (!term || !term->initialized) {
        return LLE_TERM_ERR_NULL_PTR;
    }
    
    if (!term->raw_mode_active) {
        return LLE_TERM_OK;  // Already in normal mode
    }
    
    if (tcsetattr(term->input_fd, TCSAFLUSH, &term->original_termios) != 0) {
        return LLE_TERM_ERR_TERMIOS;
    }
    
    term->raw_mode_active = false;
    return LLE_TERM_OK;
}

void lle_term_update_cursor(lle_term_t *term, uint16_t row, uint16_t col) {
    if (!term || !term->initialized) {
        return;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Update internal state (NO TERMINAL QUERY)
    // This is the single source of truth
    term->state.cursor_row = row;
    term->state.cursor_col = col;
    term->state.update_count++;
    
    uint64_t end = get_timestamp_ns();
    uint64_t elapsed = end - start;
    
    // Track performance metrics
    term->state.last_update_ns = elapsed;
    term->total_update_time_ns += elapsed;
    term->total_updates++;
}

int lle_term_update_size(lle_term_t *term) {
    if (!term || !term->initialized) {
        return LLE_TERM_ERR_NULL_PTR;
    }
    
    // This is the ONLY time we query terminal during operation
    // Called in response to SIGWINCH signal
    struct winsize ws;
    if (ioctl(term->output_fd, TIOCGWINSZ, &ws) != 0) {
        return LLE_TERM_ERR_WINSIZE;
    }
    
    // Update internal state
    uint16_t old_rows = term->state.rows;
    uint16_t old_cols = term->state.cols;
    
    term->state.rows = ws.ws_row > 0 ? ws.ws_row : old_rows;
    term->state.cols = ws.ws_col > 0 ? ws.ws_col : old_cols;
    
    // Update scroll region to match new size
    term->state.scroll_bottom = term->state.rows - 1;
    
    // Set flag to indicate size changed
    term->state.size_changed = true;
    
    return LLE_TERM_OK;
}

void lle_term_update_scroll_region(lle_term_t *term,
                                    uint16_t top, uint16_t bottom) {
    if (!term || !term->initialized) {
        return;
    }
    
    // Validate scroll region
    if (top >= term->state.rows || bottom >= term->state.rows || top > bottom) {
        return;  // Invalid region, ignore
    }
    
    term->state.scroll_top = top;
    term->state.scroll_bottom = bottom;
    term->state.update_count++;
}

int lle_term_enable_bracketed_paste(lle_term_t *term, bool enable) {
    if (!term || !term->initialized) {
        return LLE_TERM_ERR_NULL_PTR;
    }
    
    if (!term->caps.has_bracketed_paste) {
        return LLE_TERM_OK;  // Not supported, ignore
    }
    
    // Send escape sequence to enable/disable bracketed paste
    const char *seq = enable ? "\x1b[?2004h" : "\x1b[?2004l";
    ssize_t written = write(term->output_fd, seq, strlen(seq));
    
    if (written != (ssize_t)strlen(seq)) {
        return LLE_TERM_ERR_INVALID_FD;
    }
    
    term->state.bracketed_paste_active = enable;
    return LLE_TERM_OK;
}

int lle_term_enable_mouse_tracking(lle_term_t *term, bool enable) {
    if (!term || !term->initialized) {
        return LLE_TERM_ERR_NULL_PTR;
    }
    
    if (!term->caps.has_mouse) {
        return LLE_TERM_OK;  // Not supported, ignore
    }
    
    // Send escape sequence to enable/disable mouse tracking
    // Using SGR mouse mode (1006) which supports larger coordinates
    const char *seq = enable ? "\x1b[?1006h\x1b[?1000h" 
                             : "\x1b[?1000l\x1b[?1006l";
    ssize_t written = write(term->output_fd, seq, strlen(seq));
    
    if (written != (ssize_t)strlen(seq)) {
        return LLE_TERM_ERR_INVALID_FD;
    }
    
    term->state.mouse_tracking_active = enable;
    return LLE_TERM_OK;
}

const lle_term_state_t* lle_term_get_state(const lle_term_t *term) {
    if (!term || !term->initialized) {
        return NULL;
    }
    return &term->state;
}

const lle_term_capabilities_t* lle_term_get_capabilities(const lle_term_t *term) {
    if (!term || !term->initialized) {
        return NULL;
    }
    return &term->caps;
}

void lle_term_cleanup(lle_term_t *term) {
    if (!term || !term->initialized) {
        return;
    }
    
    // Disable mouse tracking if active
    if (term->state.mouse_tracking_active) {
        lle_term_enable_mouse_tracking(term, false);
    }
    
    // Disable bracketed paste if active
    if (term->state.bracketed_paste_active) {
        lle_term_enable_bracketed_paste(term, false);
    }
    
    // Restore original terminal settings
    if (term->raw_mode_active) {
        lle_term_exit_raw_mode(term);
    }
    
    term->initialized = false;
}

const char* lle_term_error_string(int error_code) {
    switch (error_code) {
        case LLE_TERM_OK:
            return "Success";
        case LLE_TERM_ERR_INVALID_FD:
            return "Invalid file descriptor";
        case LLE_TERM_ERR_TERMIOS:
            return "Terminal I/O control error";
        case LLE_TERM_ERR_WINSIZE:
            return "Window size query failed";
        case LLE_TERM_ERR_TIMEOUT:
            return "Operation timed out";
        case LLE_TERM_ERR_NOT_TTY:
            return "Not a terminal device";
        case LLE_TERM_ERR_NULL_PTR:
            return "Null pointer argument";
        case LLE_TERM_ERR_ALREADY_INIT:
            return "Already initialized";
        default:
            return "Unknown error";
    }
}

// Forward declaration for capability detection (implemented in capability.c)
extern int lle_term_detect_capabilities(lle_term_t *term, uint32_t timeout_ms);

// Wrapper for internal use
static int detect_capabilities(lle_term_t *term, uint32_t timeout_ms) {
    return lle_term_detect_capabilities(term, timeout_ms);
}

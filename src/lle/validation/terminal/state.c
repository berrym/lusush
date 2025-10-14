// src/lle/validation/terminal/state.c

#include "state.h"
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

int lle_terminal_validation_init(lle_terminal_validation_t *term) {
    memset(term, 0, sizeof(*term));
    
    term->input_fd = STDIN_FILENO;
    term->output_fd = STDOUT_FILENO;
    
    // Save original terminal settings
    if (tcgetattr(term->input_fd, &term->original_termios) != 0) {
        return -1;
    }
    
    // Setup raw mode
    term->raw_termios = term->original_termios;
    term->raw_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    term->raw_termios.c_iflag &= ~(IXON | ICRNL);
    term->raw_termios.c_cc[VMIN] = 0;
    term->raw_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(term->input_fd, TCSANOW, &term->raw_termios) != 0) {
        return -1;
    }
    
    // Get initial window size (ONE-TIME query, not repeated)
    struct winsize ws;
    if (ioctl(term->output_fd, TIOCGWINSZ, &ws) == 0) {
        term->state.rows = ws.ws_row;
        term->state.cols = ws.ws_col;
    } else {
        // Fallback defaults
        term->state.rows = 24;
        term->state.cols = 80;
    }
    
    // Initialize scroll region to full screen
    term->state.scroll_top = 0;
    term->state.scroll_bottom = term->state.rows - 1;
    
    // Detect capabilities
    lle_terminal_detect_capabilities(term, 50);  // 50ms timeout
    
    return 0;
}

void lle_terminal_state_update_cursor(lle_terminal_validation_t *term,
                                       uint16_t row, uint16_t col) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Update internal state (NO TERMINAL QUERY)
    term->state.cursor_row = row;
    term->state.cursor_col = col;
    term->state.update_count++;
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Track performance
    uint64_t update_time_ns = 
        (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
        (uint64_t)(end.tv_nsec - start.tv_nsec);
    
    term->state.last_update_ns = update_time_ns;
    term->total_update_time_ns += update_time_ns;
    term->total_updates++;
}

void lle_terminal_validation_cleanup(lle_terminal_validation_t *term) {
    // Restore original terminal settings
    tcsetattr(term->input_fd, TCSANOW, &term->original_termios);
}

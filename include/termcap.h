/**
 * termcap.h - Portable Terminal Capability Management for LUSUSH
 *
 * This module provides portable terminal handling using standard POSIX
 * terminal capabilities. Works across Linux, macOS, and BSD systems
 * without requiring external dependencies.
 *
 * Features:
 * - Terminal size detection using TIOCGWINSZ
 * - Cursor position detection using standard escape sequences
 * - Portable terminal control using VT100/ANSI sequences
 * - Bottom-line protection for shell applications
 * - Cross-platform Unix compatibility
 *
 * Usage:
 *   termcap_init();                    // Initialize terminal capabilities
 *   termcap_get_cursor_pos(&row, &col); // Get current cursor position
 *   termcap_ensure_bottom_margin();    // Ensure safe bottom margin
 *   termcap_cleanup();                 // Cleanup terminal state
 */

#ifndef TERMCAP_H
#define TERMCAP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Terminal information structure */
typedef struct {
    int rows;           /* Terminal height */
    int cols;           /* Terminal width */
    bool is_tty;        /* Is a terminal */
    const char *term_type; /* Terminal type from $TERM */
} terminal_info_t;

/* Initialize and cleanup */
int termcap_init(void);
void termcap_cleanup(void);

/* Terminal information */
const terminal_info_t *termcap_get_info(void);
void termcap_update_size(void);

/* Cursor operations using standard ANSI/VT100 sequences */
int termcap_get_cursor_pos(int *row, int *col);
int termcap_save_cursor(void);
int termcap_restore_cursor(void);

/* Bottom-line protection for shells */
int termcap_ensure_bottom_margin(void);
bool termcap_is_at_bottom_line(void);
int termcap_create_safe_margin(void);

/* Platform detection */
bool termcap_is_iterm2(void);
bool termcap_is_tmux(void);
bool termcap_is_screen(void);

/* Error codes */
#define TERMCAP_OK              0
#define TERMCAP_ERROR          -1
#define TERMCAP_NOT_TERMINAL   -2
#define TERMCAP_TIMEOUT        -3

/* Default values */
#define TERMCAP_DEFAULT_ROWS   24
#define TERMCAP_DEFAULT_COLS   80
#define TERMCAP_TIMEOUT_MS     100

#ifdef __cplusplus
}
#endif

#endif /* TERMCAP_H */
/**
 * termcap.c - Portable Terminal Capability Implementation for LUSUSH
 *
 * This module provides portable terminal handling using standard POSIX
 * terminal capabilities. Works across Linux, macOS, and BSD systems
 * without requiring external dependencies.
 */

#include "../include/termcap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>



/* Global terminal information */
static terminal_info_t term_info = {0};
static bool initialized = false;
static struct termios orig_termios;
static bool termios_saved = false;

/* Initialize terminal capabilities */
int termcap_init(void) {
    if (initialized) {
        return TERMCAP_OK;
    }

    /* Check if we're dealing with a terminal */
    term_info.is_tty = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    if (!term_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    /* Get terminal type */
    const char *term_env = getenv("TERM");
    term_info.term_type = term_env ? term_env : "unknown";

    /* Save original terminal settings */
    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        termios_saved = true;
    }

    /* Get initial terminal size */
    termcap_update_size();

    initialized = true;
    return TERMCAP_OK;
}

/* Cleanup terminal state */
void termcap_cleanup(void) {
    if (!initialized) {
        return;
    }

    /* Restore original terminal settings */
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        termios_saved = false;
    }

    initialized = false;
}

/* Get terminal information */
const terminal_info_t *termcap_get_info(void) {
    if (!initialized) {
        termcap_init();
    }
    return &term_info;
}

/* Update terminal size using TIOCGWINSZ */
void termcap_update_size(void) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_info.rows = ws.ws_row;
        term_info.cols = ws.ws_col;
    } else {
        /* Fallback to defaults */
        term_info.rows = TERMCAP_DEFAULT_ROWS;
        term_info.cols = TERMCAP_DEFAULT_COLS;
    }

    /* Sanity check */
    if (term_info.rows <= 0) term_info.rows = TERMCAP_DEFAULT_ROWS;
    if (term_info.cols <= 0) term_info.cols = TERMCAP_DEFAULT_COLS;
}

/* Get cursor position using ANSI escape sequence */
int termcap_get_cursor_pos(int *row, int *col) {
    if (!term_info.is_tty || !row || !col) {
        return TERMCAP_ERROR;
    }

    /* Send cursor position query */
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return TERMCAP_ERROR;
    }

    /* Read response with timeout */
    fd_set readfds;
    struct timeval timeout;
    char response[32];
    int response_len = 0;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = TERMCAP_TIMEOUT_MS * 1000;

    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) <= 0) {
        return TERMCAP_TIMEOUT;
    }

    /* Read response */
    response_len = read(STDIN_FILENO, response, sizeof(response) - 1);
    if (response_len <= 0) {
        return TERMCAP_ERROR;
    }

    response[response_len] = '\0';

    /* Parse response: ESC[row;colR */
    if (sscanf(response, "\x1b[%d;%dR", row, col) == 2) {
        return TERMCAP_OK;
    }

    return TERMCAP_ERROR;
}

/* Save cursor position */
int termcap_save_cursor(void) {
    if (!term_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    /* ANSI escape sequence to save cursor */
    if (write(STDOUT_FILENO, "\x1b\x37", 2) == 2) {
        return TERMCAP_OK;
    }

    return TERMCAP_ERROR;
}

/* Restore cursor position */
int termcap_restore_cursor(void) {
    if (!term_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    /* ANSI escape sequence to restore cursor */
    if (write(STDOUT_FILENO, "\x1b\x38", 2) == 2) {
        return TERMCAP_OK;
    }

    return TERMCAP_ERROR;
}

/* Check if cursor is at bottom line */
bool termcap_is_at_bottom_line(void) {
    int row, col;
    
    if (termcap_get_cursor_pos(&row, &col) != TERMCAP_OK) {
        return false; /* Assume not at bottom if we can't detect */
    }

    return (row >= term_info.rows);
}

/* Ensure there's a safe margin at the bottom of the terminal */
int termcap_ensure_bottom_margin(void) {
    if (!term_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    static bool margin_created = false;
    if (margin_created) {
        return TERMCAP_OK;
    }

    /* Save current cursor position */
    if (termcap_save_cursor() != TERMCAP_OK) {
        return TERMCAP_ERROR;
    }

    /* Move to bottom of screen and create margin */
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "\x1b[%d;1H", term_info.rows);
    write(STDOUT_FILENO, cmd, strlen(cmd));
    write(STDOUT_FILENO, "\n", 1);

    /* Restore cursor position */
    if (termcap_restore_cursor() != TERMCAP_OK) {
        return TERMCAP_ERROR;
    }

    margin_created = true;
    return TERMCAP_OK;
}

/* Create safe margin by moving up if at bottom */
int termcap_create_safe_margin(void) {
    if (!term_info.is_tty) {
        return TERMCAP_NOT_TERMINAL;
    }

    int row, col;
    if (termcap_get_cursor_pos(&row, &col) != TERMCAP_OK) {
        /* If we can't get position, apply conservative protection */
        return termcap_ensure_bottom_margin();
    }

    /* If we're at or near the bottom, create space */
    if (row >= term_info.rows - 1) {
        /* Move up one line and create margin */
        write(STDOUT_FILENO, "\x1b[A", 3);  /* Move up */
        write(STDOUT_FILENO, "\n", 1);      /* Create newline */
        return TERMCAP_OK;
    }

    return TERMCAP_OK;
}

/* Check if terminal is iTerm2 */
bool termcap_is_iterm2(void) {
    const char *term_program = getenv("TERM_PROGRAM");
    const char *iterm_session = getenv("ITERM_SESSION_ID");
    
    return (iterm_session != NULL) || 
           (term_program && strstr(term_program, "iTerm"));
}

/* Check if terminal is tmux */
bool termcap_is_tmux(void) {
    const char *tmux = getenv("TMUX");
    return (tmux != NULL) || 
           (term_info.term_type && strstr(term_info.term_type, "tmux"));
}

/* Check if terminal is screen */
bool termcap_is_screen(void) {
    return (term_info.term_type && strstr(term_info.term_type, "screen"));
}
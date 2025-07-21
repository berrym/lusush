/**
 * @file terminal_manager.c
 * @brief Lusush Line Editor - Terminal Manager Implementation
 *
 * Terminal interface and capability management implementation for the
 * Lusush Line Editor. Provides terminal state management, raw mode control,
 * and terminal capability detection for optimal line editing experience.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "terminal_manager.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>

/**
 * @brief Get terminal size using ioctl
 *
 * Uses the TIOCGWINSZ ioctl to get the current terminal size.
 * Falls back to environment variables if ioctl fails.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
static bool lle_get_terminal_size_ioctl(lle_terminal_manager_t *tm) {
    struct winsize ws;
    
    if (ioctl(tm->stdout_fd, TIOCGWINSZ, &ws) == -1) {
        return false;
    }
    
    if (ws.ws_col == 0 || ws.ws_row == 0) {
        return false;
    }
    
    tm->geometry.width = ws.ws_col;
    tm->geometry.height = ws.ws_row;
    return true;
}

/**
 * @brief Get terminal size from environment variables
 *
 * Fallback method to get terminal size from COLUMNS and LINES
 * environment variables when ioctl is not available.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
static bool lle_get_terminal_size_env(lle_terminal_manager_t *tm) {
    const char *cols_str = getenv("COLUMNS");
    const char *lines_str = getenv("LINES");
    
    if (!cols_str || !lines_str) {
        return false;
    }
    
    int cols = atoi(cols_str);
    int lines = atoi(lines_str);
    
    if (cols <= 0 || lines <= 0) {
        return false;
    }
    
    tm->geometry.width = (size_t)cols;
    tm->geometry.height = (size_t)lines;
    return true;
}

/**
 * @brief Detect terminal capabilities by querying terminal
 *
 * Detects various terminal capabilities through environment variables
 * and terminal queries. Updates the capabilities flags accordingly.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_detect_capabilities(lle_terminal_manager_t *tm) {
    if (!tm) {
        return false;
    }
    
    tm->capabilities = 0;
    
    // Check for color support
    const char *term = getenv("TERM");
    if (term) {
        if (strstr(term, "color") || strstr(term, "xterm") || 
            strstr(term, "screen") || strstr(term, "tmux")) {
            tm->capabilities |= LLE_TERM_CAP_COLORS;
        }
        
        if (strstr(term, "256color") || strstr(term, "xterm-256")) {
            tm->capabilities |= LLE_TERM_CAP_256_COLORS;
        }
    }
    
    // Check COLORTERM for additional color support
    const char *colorterm = getenv("COLORTERM");
    if (colorterm) {
        if (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit")) {
            tm->capabilities |= LLE_TERM_CAP_COLORS | LLE_TERM_CAP_256_COLORS;
        }
    }
    
    // Most modern terminals support these features
    if (isatty(tm->stdout_fd)) {
        tm->capabilities |= LLE_TERM_CAP_CURSOR_MOVEMENT;
        tm->capabilities |= LLE_TERM_CAP_CLEAR_SCREEN;
    }
    
    // Check for UTF-8 support
    const char *lang = getenv("LANG");
    const char *lc_all = getenv("LC_ALL");
    const char *lc_ctype = getenv("LC_CTYPE");
    
    if ((lang && strstr(lang, "UTF-8")) ||
        (lc_all && strstr(lc_all, "UTF-8")) ||
        (lc_ctype && strstr(lc_ctype, "UTF-8"))) {
        tm->capabilities |= LLE_TERM_CAP_UTF8;
    }
    
    // Modern terminals often support these features
    if (term && (strstr(term, "xterm") || strstr(term, "screen") || 
                 strstr(term, "tmux") || strstr(term, "alacritty") ||
                 strstr(term, "kitty"))) {
        tm->capabilities |= LLE_TERM_CAP_ALTERNATE_SCREEN;
        tm->capabilities |= LLE_TERM_CAP_BRACKETED_PASTE;
        tm->capabilities |= LLE_TERM_CAP_MOUSE;
    }
    
    tm->capabilities_initialized = true;
    return true;
}

/**
 * @brief Enter raw mode for character-by-character input
 *
 * Configures the terminal for raw mode input, saving the original
 * terminal state for later restoration.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_enter_raw_mode(lle_terminal_manager_t *tm) {
    if (!tm || tm->in_raw_mode) {
        return false;
    }
    
    if (!isatty(tm->stdin_fd)) {
        return false;
    }
    
    // Allocate space for original termios if not already done
    if (!tm->saved_state.original_termios) {
        tm->saved_state.original_termios = malloc(sizeof(struct termios));
        if (!tm->saved_state.original_termios) {
            return false;
        }
    }
    
    struct termios *orig_termios = (struct termios *)tm->saved_state.original_termios;
    
    // Get current terminal attributes
    if (tcgetattr(tm->stdin_fd, orig_termios) == -1) {
        return false;
    }
    
    struct termios raw = *orig_termios;
    
    // Configure raw mode
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    // Apply raw mode settings
    if (tcsetattr(tm->stdin_fd, TCSAFLUSH, &raw) == -1) {
        return false;
    }
    
    tm->in_raw_mode = true;
    tm->saved_state.was_raw_mode = false;
    tm->saved_state.needs_restoration = true;
    
    return true;
}

/**
 * @brief Exit raw mode and restore normal terminal behavior
 *
 * Restores the terminal to its original state before raw mode
 * was entered.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_exit_raw_mode(lle_terminal_manager_t *tm) {
    if (!tm || !tm->in_raw_mode || !tm->saved_state.original_termios) {
        return false;
    }
    
    struct termios *orig_termios = (struct termios *)tm->saved_state.original_termios;
    
    // Restore original terminal attributes
    if (tcsetattr(tm->stdin_fd, TCSAFLUSH, orig_termios) == -1) {
        return false;
    }
    
    tm->in_raw_mode = false;
    tm->saved_state.needs_restoration = false;
    
    return true;
}

/**
 * @brief Get current terminal size and update geometry
 *
 * Updates the terminal geometry information by querying the current
 * terminal size through various methods.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_get_size(lle_terminal_manager_t *tm) {
    if (!tm) {
        return false;
    }
    
    // Try ioctl first
    if (lle_get_terminal_size_ioctl(tm)) {
        tm->geometry_valid = true;
        return true;
    }
    
    // Fall back to environment variables
    if (lle_get_terminal_size_env(tm)) {
        tm->geometry_valid = true;
        return true;
    }
    
    // Use default size as last resort
    tm->geometry.width = LLE_DEFAULT_TERMINAL_WIDTH;
    tm->geometry.height = LLE_DEFAULT_TERMINAL_HEIGHT;
    tm->geometry_valid = false;
    
    return false;
}

/**
 * @brief Initialize terminal manager with default settings
 *
 * Performs complete terminal manager initialization including file
 * descriptor setup, capability detection, and geometry detection.
 *
 * @param tm Pointer to terminal manager structure
 * @return LLE_TERM_INIT_SUCCESS on success, error code on failure
 */
lle_terminal_init_result_t lle_terminal_init(lle_terminal_manager_t *tm) {
    if (!tm) {
        return LLE_TERM_INIT_ERROR_INVALID_FD;
    }
    
    // Initialize structure to known state
    memset(tm, 0, sizeof(lle_terminal_manager_t));
    
    // Set up standard file descriptors
    tm->stdin_fd = STDIN_FILENO;
    tm->stdout_fd = STDOUT_FILENO;
    tm->stderr_fd = STDERR_FILENO;
    
    // Verify file descriptors are valid TTYs
    if (!isatty(tm->stdin_fd) || !isatty(tm->stdout_fd)) {
        return LLE_TERM_INIT_ERROR_NOT_TTY;
    }
    
    // Detect terminal capabilities
    if (!lle_terminal_detect_capabilities(tm)) {
        return LLE_TERM_INIT_ERROR_CAPABILITIES;
    }
    
    // Get terminal geometry
    if (!lle_terminal_get_size(tm)) {
        return LLE_TERM_INIT_ERROR_GEOMETRY;
    }
    
    // Validate the resulting geometry
    if (!lle_validate_terminal_geometry(&tm->geometry)) {
        return LLE_TERM_INIT_ERROR_GEOMETRY;
    }
    
    return LLE_TERM_INIT_SUCCESS;
}

/**
 * @brief Clean up terminal manager and restore original state
 *
 * Performs complete cleanup including exiting raw mode, restoring
 * terminal state, and freeing allocated resources.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_cleanup(lle_terminal_manager_t *tm) {
    if (!tm) {
        return false;
    }
    
    bool success = true;
    
    // Exit raw mode if currently in it
    if (tm->in_raw_mode) {
        if (!lle_terminal_exit_raw_mode(tm)) {
            success = false;
        }
    }
    
    // Free saved terminal state
    if (tm->saved_state.original_termios) {
        free(tm->saved_state.original_termios);
        tm->saved_state.original_termios = NULL;
    }
    
    // Reset structure to clean state
    memset(tm, 0, sizeof(lle_terminal_manager_t));
    
    return success;
}

/**
 * @brief Check if terminal has specific capability
 *
 * Checks whether the terminal supports a specific capability.
 * Capabilities must be detected first using lle_terminal_detect_capabilities.
 *
 * @param tm Pointer to terminal manager structure
 * @param capability Capability flag to check
 * @return true if capability is supported, false otherwise
 */
bool lle_terminal_has_capability(const lle_terminal_manager_t *tm, lle_terminal_capabilities_t capability) {
    if (!tm || !tm->capabilities_initialized) {
        return false;
    }
    
    return (tm->capabilities & capability) != 0;
}

/**
 * @brief Validate terminal manager structure
 *
 * Validates that a terminal manager structure is in a consistent
 * state with proper values and initialization.
 *
 * @param tm Pointer to terminal manager structure
 * @return true if structure is valid, false otherwise
 */
bool lle_terminal_manager_is_valid(const lle_terminal_manager_t *tm) {
    if (!tm) {
        return false;
    }
    
    // Check file descriptors are reasonable
    if (tm->stdin_fd < 0 || tm->stdout_fd < 0 || tm->stderr_fd < 0) {
        return false;
    }
    
    // Check geometry if marked as valid
    if (tm->geometry_valid && !lle_validate_terminal_geometry(&tm->geometry)) {
        return false;
    }
    
    // Check raw mode consistency
    if (tm->in_raw_mode && !tm->saved_state.needs_restoration) {
        return false;
    }
    
    return true;
}

/**
 * @brief Get human-readable description of terminal capabilities
 *
 * Generates a human-readable string describing the detected terminal
 * capabilities for debugging and diagnostic purposes.
 *
 * @param tm Pointer to terminal manager structure
 * @param buffer Buffer to store description
 * @param buffer_size Size of buffer in bytes
 * @return Number of characters written, or -1 on error
 */
int lle_terminal_get_capabilities_string(const lle_terminal_manager_t *tm, char *buffer, size_t buffer_size) {
    if (!tm || !buffer || buffer_size == 0) {
        return -1;
    }
    
    if (!tm->capabilities_initialized) {
        return snprintf(buffer, buffer_size, "Capabilities not initialized");
    }
    
    int written = 0;
    int result;
    
    result = snprintf(buffer + written, buffer_size - written, "Terminal capabilities: ");
    if (result < 0 || (size_t)result >= buffer_size - written) return -1;
    written += result;
    
    if (tm->capabilities & LLE_TERM_CAP_COLORS) {
        result = snprintf(buffer + written, buffer_size - written, "colors ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (tm->capabilities & LLE_TERM_CAP_256_COLORS) {
        result = snprintf(buffer + written, buffer_size - written, "256colors ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (tm->capabilities & LLE_TERM_CAP_CURSOR_MOVEMENT) {
        result = snprintf(buffer + written, buffer_size - written, "cursor ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (tm->capabilities & LLE_TERM_CAP_UTF8) {
        result = snprintf(buffer + written, buffer_size - written, "utf8 ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (tm->capabilities & LLE_TERM_CAP_MOUSE) {
        result = snprintf(buffer + written, buffer_size - written, "mouse ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (tm->capabilities & LLE_TERM_CAP_BRACKETED_PASTE) {
        result = snprintf(buffer + written, buffer_size - written, "paste ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (written > 0 && buffer[written - 1] == ' ') {
        buffer[written - 1] = '\0';
        written--;
    }
    
    return written;
}
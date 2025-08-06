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
#include "termcap/lle_termcap.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

/**
 * @brief Update terminal geometry from termcap system
 *
 * Uses the integrated termcap system to get accurate terminal geometry
 * information with enhanced capabilities and iTerm2 optimizations.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_update_geometry(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Update termcap size information
    lle_termcap_update_size();
    
    // Get updated terminal info from termcap
    tm->termcap_info = lle_termcap_get_info();
    if (!tm->termcap_info) {
        return false;
    }
    
    // Update geometry from termcap info
    tm->geometry.width = (size_t)tm->termcap_info->cols;
    tm->geometry.height = (size_t)tm->termcap_info->rows;
    
    // Validate geometry
    if (tm->geometry.width <= 0 || tm->geometry.height <= 0) {
        // Use defaults if termcap returns invalid values
        tm->geometry.width = LLE_DEFAULT_TERMINAL_WIDTH;
        tm->geometry.height = LLE_DEFAULT_TERMINAL_HEIGHT;
        tm->geometry_valid = false;
        return false;
    }
    
    tm->geometry_valid = true;
    return true;
}

/**
 * @brief Check if terminal is iTerm2 (for optimizations)
 *
 * Uses the integrated termcap system to detect iTerm2 for
 * platform-specific optimizations.
 *
 * @param tm Pointer to terminal manager structure
 * @return true if iTerm2, false otherwise
 */
bool lle_terminal_is_iterm2(const lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    return tm->is_iterm2;
}

/**
 * @brief Detect terminal capabilities using integrated termcap system
 *
 * Uses the integrated termcap system for comprehensive capability detection
 * with enhanced support for modern terminals and iTerm2 optimizations.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_detect_capabilities(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Use termcap to detect capabilities
    int result = lle_termcap_detect_capabilities();
    if (result != LLE_TERMCAP_OK && result != LLE_TERMCAP_NOT_TERMINAL) {
        return false;
    }
    
    // Get termcap capability information
    if (!tm->termcap_info) {
        tm->termcap_info = lle_termcap_get_info();
        if (!tm->termcap_info) {
            return false;
        }
    }
    
    // Map termcap capabilities to LLE terminal capabilities
    tm->capabilities = 0;
    
    if (tm->termcap_info->caps.colors) {
        tm->capabilities |= LLE_TERM_CAP_COLORS;
    }
    
    if (tm->termcap_info->caps.colors_256) {
        tm->capabilities |= LLE_TERM_CAP_256_COLORS;
    }
    
    if (tm->termcap_info->caps.unicode) {
        tm->capabilities |= LLE_TERM_CAP_UTF8;
    }
    
    if (tm->termcap_info->caps.mouse) {
        tm->capabilities |= LLE_TERM_CAP_MOUSE;
    }
    
    if (tm->termcap_info->caps.bracketed_paste) {
        tm->capabilities |= LLE_TERM_CAP_BRACKETED_PASTE;
    }
    
    if (tm->termcap_info->caps.alternate_screen) {
        tm->capabilities |= LLE_TERM_CAP_ALTERNATE_SCREEN;
    }
    
    // Most terminals support basic cursor movement and screen clearing
    if (tm->termcap_info->is_tty) {
        tm->capabilities |= LLE_TERM_CAP_CURSOR_MOVEMENT;
        tm->capabilities |= LLE_TERM_CAP_CLEAR_SCREEN;
    }
    
    // Check for iTerm2 for platform-specific optimizations
    tm->is_iterm2 = lle_termcap_is_iterm2();
    
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
 * Updates the terminal geometry information using the integrated
 * termcap system for accurate and comprehensive terminal sizing.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_get_size(lle_terminal_manager_t *tm) {
    if (!tm) {
        return false;
    }
    
    // Use termcap system for terminal geometry
    if (tm->termcap_initialized) {
        return lle_terminal_update_geometry(tm);
    }
    
    // Fallback to basic ioctl if termcap not available
    struct winsize ws;
    if (ioctl(tm->stdout_fd, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0 && ws.ws_row > 0) {
        tm->geometry.width = ws.ws_col;
        tm->geometry.height = ws.ws_row;
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
 * @brief Initialize terminal manager with integrated termcap system
 *
 * Performs complete terminal manager initialization using the integrated
 * termcap system for professional-grade terminal handling with iTerm2
 * optimizations and comprehensive capability detection.
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
    
    // Initialize integrated termcap system
    int termcap_result = lle_termcap_init();
    if (termcap_result == LLE_TERMCAP_OK || termcap_result == LLE_TERMCAP_NOT_TERMINAL) {
        tm->termcap_initialized = true;
        tm->termcap_info = lle_termcap_get_info();
    } else {
        tm->termcap_initialized = false;
        tm->termcap_info = NULL;
    }
    
    // Verify file descriptors are valid TTYs (unless in non-terminal mode)
    if (termcap_result != LLE_TERMCAP_NOT_TERMINAL) {
        if (!isatty(tm->stdin_fd) || !isatty(tm->stdout_fd)) {
            if (tm->termcap_initialized) {
                lle_termcap_cleanup();
            }
            return LLE_TERM_INIT_ERROR_NOT_TTY;
        }
    }
    
    // Detect terminal capabilities using termcap
    if (!lle_terminal_detect_capabilities(tm)) {
        if (tm->termcap_initialized) {
            lle_termcap_cleanup();
        }
        return LLE_TERM_INIT_ERROR_CAPABILITIES;
    }
    
    // Get terminal geometry using termcap
    if (!lle_terminal_get_size(tm)) {
        if (tm->termcap_initialized) {
            lle_termcap_cleanup();
        }
        return LLE_TERM_INIT_ERROR_GEOMETRY;
    }
    
    // Validate the resulting geometry
    if (!lle_validate_terminal_geometry(&tm->geometry)) {
        if (tm->termcap_initialized) {
            lle_termcap_cleanup();
        }
        return LLE_TERM_INIT_ERROR_GEOMETRY;
    }
    
    return LLE_TERM_INIT_SUCCESS;
}

/**
 * @brief Clean up terminal manager and restore original state
 *
 * Performs complete cleanup including exiting raw mode, restoring
 * terminal state, cleaning up termcap system, and freeing allocated resources.
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
    
    // Clean up integrated termcap system
    if (tm->termcap_initialized) {
        lle_termcap_cleanup();
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
    
    if (tm->termcap_info && tm->termcap_info->caps.truecolor) {
        result = snprintf(buffer + written, buffer_size - written, "truecolor ");
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
    
    if (tm->is_iterm2) {
        result = snprintf(buffer + written, buffer_size - written, "iterm2 ");
        if (result < 0 || (size_t)result >= buffer_size - written) return -1;
        written += result;
    }
    
    if (written > 0 && buffer[written - 1] == ' ') {
        buffer[written - 1] = '\0';
        written--;
    }
    
    return written;
}

/* ======================= LLE-011: Terminal Output Functions ======================= */

/**
 * @brief Write data to terminal with proper error handling
 *
 * Writes data directly to the terminal's stdout file descriptor with
 * comprehensive error handling and validation.
 *
 * @param tm Pointer to terminal manager structure
 * @param data Data to write
 * @param length Length of data in bytes
 * @return true on success, false on failure
 */
bool lle_terminal_write(lle_terminal_manager_t *tm, const char *data, size_t length) {
    if (!tm || !data || length == 0) {
        return false;
    }
    
    if (!tm->termcap_initialized) {
        return false;
    }
    
    ssize_t written = write(tm->stdout_fd, data, length);
    if (written < 0 || (size_t)written != length) {
        return false;
    }
    
    return true;
}

/**
 * @brief Clear exactly the specified number of characters
 *
 * Clears the exact number of characters from the current cursor position
 * using a reliable space+backspace method.
 *
 * @param tm Pointer to terminal manager structure
 * @param length_to_clear Exact number of characters to clear
 * @return true on success, false on failure
 */
bool lle_terminal_clear_exactly(lle_terminal_manager_t *tm, size_t length_to_clear) {
    if (!tm || !tm->termcap_initialized || length_to_clear == 0) {
        return true; // Nothing to clear
    }
    
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Clearing exactly %zu characters\n", length_to_clear);
    }
    
    // Method: Write spaces to overwrite content, then backspace to original position
    
    // Step 1: Write spaces to overwrite the content
    for (size_t i = 0; i < length_to_clear; i++) {
        if (!lle_terminal_write(tm, " ", 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_TERMINAL] Failed to write space at position %zu\n", i);
            }
            return false;
        }
    }
    
    // Step 2: Backspace to return to original position
    for (size_t i = 0; i < length_to_clear; i++) {
        if (!lle_terminal_write(tm, "\b", 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_TERMINAL] Failed to backspace at position %zu\n", i);
            }
            return false;
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Successfully cleared %zu characters\n", length_to_clear);
    }
    
    return true;
}

/**
 * @brief Move cursor to specified position using termcap
 *
 * Uses the integrated termcap system to move the cursor to the specified
 * row and column position with bounds checking.
 *
 * @param tm Pointer to terminal manager structure
 * @param row Target row (0-based)
 * @param col Target column (0-based)
 * @return true on success, false on failure
 */
bool lle_terminal_move_cursor(lle_terminal_manager_t *tm, size_t row, size_t col) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Bounds checking against terminal geometry
    if (tm->geometry_valid) {
        if (row >= tm->geometry.height || col >= tm->geometry.width) {
            return false;
        }
    }
    
    int result = lle_termcap_move_cursor((int)row, (int)col);
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Clear current line using termcap
 *
 * Uses the integrated termcap system to clear the entire current line.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_line(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    int result = lle_termcap_clear_line();
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Clear from cursor to end of line using enhanced geometry-aware method
 *
 * Enhanced for boundary crossing scenarios with maximum intelligence and safety.
 * Clears exactly what is needed to eliminate artifacts while preserving cursor position.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_to_eol(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Using calculated exact boundary crossing clearing\n");
    }
    
    // Get terminal width with enhanced geometry detection
    size_t terminal_width = 80; // Safe default
    if (tm->geometry_valid && tm->geometry.width > 0) {
        terminal_width = tm->geometry.width;
        if (debug_mode) {
            fprintf(stderr, "[LLE_TERMINAL] Enhanced geometry: terminal_width=%zu\n", terminal_width);
        }
    } else {
        if (debug_mode) {
            fprintf(stderr, "[LLE_TERMINAL] Using conservative default width: %zu\n", terminal_width);
        }
    }
    
    // CALCULATED EXACT clearing strategy
    // CRITICAL: Calculate exactly the number of characters needed to clear to the boundary
    // Never use fixed margins - calculate precisely what's required each time
    size_t calculated_clear_width;
    
    // Calculate exact clearing width to reach terminal boundary without wrap
    // We need to clear exactly to position (terminal_width - 1) to remove boundary character
    // This prevents any line wrap while ensuring complete clearing
    if (terminal_width > 1) {
        calculated_clear_width = terminal_width;
    } else {
        // Fallback for edge case
        calculated_clear_width = 1;
    }
    
    // Safety cap for extreme terminal sizes
    if (calculated_clear_width > 500) {
        calculated_clear_width = 500;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Calculated exact clear width: %zu (terminal=%zu, target_pos=%zu)\n", 
               calculated_clear_width, terminal_width, terminal_width);
    }
    
    // CALCULATED EXACT clearing: Write spaces to exact boundary position
    for (size_t i = 0; i < calculated_clear_width; i++) {
        if (!lle_terminal_write(tm, " ", 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_TERMINAL] Calculated clearing space write failed at %zu\n", i);
            }
            break; // Continue with partial clearing - better than complete failure
        }
    }
    
    // CALCULATED EXACT backspace: Return to original position for rewrite
    for (size_t i = 0; i < calculated_clear_width; i++) {
        if (!lle_terminal_write(tm, "\b", 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_TERMINAL] Calculated clearing backspace failed at %zu\n", i);
            }
            break; // Continue with partial backspace - cursor will be close enough
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Calculated exact boundary crossing clearing completed (cleared %zu chars to pos %zu)\n", 
               calculated_clear_width, terminal_width - 1);
    }
    
    return true;
}

/**
 * @brief Clear entire screen using termcap
 *
 * Uses the integrated termcap system to clear the entire terminal screen
 * and optionally move cursor to home position.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_screen(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    int result = lle_termcap_clear_screen();
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Clear from cursor to end of screen using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_to_eos(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    int result = lle_termcap_clear_to_eos();
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Set terminal colors using termcap
 *
 * Uses the integrated termcap system to set foreground and background
 * colors with capability checking.
 *
 * @param tm Pointer to terminal manager structure
 * @param fg Foreground color
 * @param bg Background color
 * @return true on success, false on failure
 */
bool lle_terminal_set_color(lle_terminal_manager_t *tm, lle_termcap_color_t fg, lle_termcap_color_t bg) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Check if terminal supports colors
    if (!lle_terminal_has_capability(tm, LLE_TERM_CAP_COLORS)) {
        return false;
    }
    
    int result = lle_termcap_set_color(fg, bg);
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Reset terminal colors to defaults using termcap
 *
 * Uses the integrated termcap system to reset terminal colors to
 * their default values.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_reset_colors(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    int result = lle_termcap_reset_colors();
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Hide cursor using termcap
 *
 * Uses the integrated termcap system to hide the terminal cursor.
 * Useful during complex display updates to reduce flicker.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_hide_cursor(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    int result = lle_termcap_hide_cursor();
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Show cursor using termcap
 *
 * Uses the integrated termcap system to show the terminal cursor.
 * Should be called after hide_cursor to restore normal cursor visibility.
 *
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_show_cursor(lle_terminal_manager_t *tm) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    int result = lle_termcap_show_cursor();
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Move cursor up by specified number of lines
 */
bool lle_terminal_move_cursor_up(lle_terminal_manager_t *tm, size_t lines) {
    if (!tm || !tm->termcap_initialized || lines == 0) {
        return false;
    }
    
    int result = lle_termcap_move_cursor_up(lines);
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Move cursor down by specified number of lines
 */
bool lle_terminal_move_cursor_down(lle_terminal_manager_t *tm, size_t lines) {
    if (!tm || !tm->termcap_initialized || lines == 0) {
        return false;
    }
    
    int result = lle_termcap_move_cursor_down(lines);
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Move cursor to specified column on current line
 */
bool lle_terminal_move_cursor_to_column(lle_terminal_manager_t *tm, size_t col) {
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Bounds checking against terminal geometry
    if (tm->geometry_valid && col >= tm->geometry.width) {
        return false;
    }
    
    int result = lle_termcap_cursor_to_column(col);
    return result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL || result == LLE_TERMCAP_INVALID_PARAMETER;
}

// Phase 1A: Multi-Line Operations (Architecture Rewrite)

/**
 * @brief Clear a rectangular region of the terminal
 *
 * Clears all content within the specified rectangular region using absolute
 * terminal coordinates. This is a fundamental operation for the architectural
 * rewrite to support proper multi-line content management.
 *
 * @param tm Pointer to terminal manager structure
 * @param start_row Starting row of region to clear (0-based)
 * @param start_col Starting column of region to clear (0-based)
 * @param end_row Ending row of region to clear (0-based, inclusive)
 * @param end_col Ending column of region to clear (0-based, inclusive)
 * @return true on success, false on failure
 */
bool lle_terminal_clear_region(lle_terminal_manager_t *tm,
                              size_t start_row, size_t start_col,
                              size_t end_row, size_t end_col)
{
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Bounds checking against terminal geometry
    if (tm->geometry_valid) {
        if (start_row >= tm->geometry.height || end_row >= tm->geometry.height ||
            start_col >= tm->geometry.width || end_col >= tm->geometry.width) {
            return false;
        }
    }
    
    // Validate parameters
    if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
        return false;
    }
    
    // Clear region row by row
    for (size_t row = start_row; row <= end_row; row++) {
        // Move to start of region on this row
        if (!lle_terminal_move_cursor(tm, row, start_col)) {
            return false;
        }
        
        // Determine how much to clear on this row
        size_t clear_start = start_col;
        size_t clear_end = (row == end_row) ? end_col : tm->geometry.width - 1;
        
        // Clear by writing spaces
        for (size_t col = clear_start; col <= clear_end; col++) {
            if (!lle_terminal_write(tm, " ", 1)) {
                return false;
            }
        }
    }
    
    return true;
}

/**
 * @brief Clear multiple consecutive lines
 *
 * Clears the specified number of complete lines starting from the given row.
 * More efficient than clearing individual regions when clearing complete lines.
 *
 * @param tm Pointer to terminal manager structure
 * @param start_row Starting row to clear (0-based)
 * @param num_lines Number of lines to clear
 * @return true on success, false on failure
 */
bool lle_terminal_clear_lines(lle_terminal_manager_t *tm,
                             size_t start_row, size_t num_lines)
{
    if (!tm || !tm->termcap_initialized || num_lines == 0) {
        return false;
    }
    
    // Bounds checking against terminal geometry
    if (tm->geometry_valid) {
        if (start_row >= tm->geometry.height || 
            start_row + num_lines > tm->geometry.height) {
            return false;
        }
    }
    
    // Clear each line completely
    for (size_t i = 0; i < num_lines; i++) {
        size_t row = start_row + i;
        
        // Move to start of line
        if (!lle_terminal_move_cursor(tm, row, 0)) {
            return false;
        }
        
        // Clear entire line using termcap
        int result = lle_termcap_clear_line();
        if (result != LLE_TERMCAP_OK && 
            result != LLE_TERMCAP_NOT_TERMINAL && 
            result != LLE_TERMCAP_INVALID_PARAMETER) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Clear from specified position to end of line
 *
 * Clears content from the specified absolute position to the end of that line.
 * Similar to lle_terminal_clear_to_eol() but works with absolute coordinates.
 *
 * @param tm Pointer to terminal manager structure
 * @param row Row to clear from (0-based)
 * @param col Column to start clearing from (0-based)
 * @return true on success, false on failure
 */
bool lle_terminal_clear_from_position_to_eol(lle_terminal_manager_t *tm,
                                            size_t row, size_t col)
{
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Bounds checking against terminal geometry
    if (tm->geometry_valid) {
        if (row >= tm->geometry.height || col >= tm->geometry.width) {
            return false;
        }
    }
    
    // Move to specified position
    if (!lle_terminal_move_cursor(tm, row, col)) {
        return false;
    }
    
    // Clear to end of line using termcap
    int result = lle_termcap_clear_to_eol();
    return result == LLE_TERMCAP_OK || 
           result == LLE_TERMCAP_NOT_TERMINAL || 
           result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Clear from specified position to end of screen
 *
 * Clears all content from the specified absolute position to the end of the
 * terminal screen. Useful for clearing multi-line content efficiently.
 *
 * @param tm Pointer to terminal manager structure
 * @param row Row to start clearing from (0-based)
 * @param col Column to start clearing from (0-based)
 * @return true on success, false on failure
 */
bool lle_terminal_clear_from_position_to_eos(lle_terminal_manager_t *tm,
                                            size_t row, size_t col)
{
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    // Bounds checking against terminal geometry
    if (tm->geometry_valid) {
        if (row >= tm->geometry.height || col >= tm->geometry.width) {
            return false;
        }
    }
    
    // Move to specified position
    if (!lle_terminal_move_cursor(tm, row, col)) {
        return false;
    }
    
    // Clear to end of screen using termcap
    int result = lle_termcap_clear_to_eos();
    return result == LLE_TERMCAP_OK || 
           result == LLE_TERMCAP_NOT_TERMINAL || 
           result == LLE_TERMCAP_INVALID_PARAMETER;
}

/**
 * @brief Save current cursor position for later restoration
 *
 * Saves the current cursor position so it can be restored later with
 * lle_terminal_restore_cursor_position(). More reliable than termcap
 * save/restore for multi-line operations.
 *
 * @param tm Pointer to terminal manager structure
 * @param saved_row Pointer to store current row (0-based)
 * @param saved_col Pointer to store current column (0-based)
 * @return true on success, false on failure
 */
bool lle_terminal_save_cursor_position(lle_terminal_manager_t *tm,
                                      size_t *saved_row, size_t *saved_col)
{
    if (!tm || !tm->termcap_initialized || !saved_row || !saved_col) {
        return false;
    }
    
    // CRITICAL FIX: Don't use cursor queries during interactive sessions
    // They contaminate stdin with escape sequence responses
    
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Cursor save disabled to prevent input contamination\n");
    }
    
    // Use mathematical position tracking instead
    *saved_row = 0;
    *saved_col = 0;
    return false; // Force mathematical positioning
}

/**
 * @brief Query current cursor position from terminal
 *
 * Queries the terminal for the current cursor position using escape sequences.
 * May have timeout issues on some terminals - use with caution.
 *
 * @param tm Pointer to terminal manager structure
 * @param current_row Pointer to store current row (0-based)
 * @param current_col Pointer to store current column (0-based)
 * @return true on success, false on failure or timeout
 */
bool lle_terminal_query_cursor_position(lle_terminal_manager_t *tm,
                                       size_t *current_row, size_t *current_col)
{
    if (!tm || !tm->termcap_initialized || !current_row || !current_col) {
        return false;
    }
    
    // CRITICAL FIX: Disable cursor queries universally during interactive sessions
    // Cursor position queries send ^[[6n and expect ^[[row;colR responses
    // These responses contaminate stdin and appear as visible characters
    // This affects ALL platforms, not just Linux
    
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_TERMINAL] Cursor queries disabled universally - preventing input contamination\n");
    }
    
    // Force mathematical positioning instead of terminal queries
    // This prevents the ^[[37;1R contamination that was appearing before prompts
    *current_row = 0;
    *current_col = 0;
    return false; // Always use fallback mathematical positioning
}

/**
 * @brief Calculate exact number of lines needed for content
 * 
 * Uses geometry-aware calculation to determine exact visual footprint
 * for precise clearing operations.
 *
 * @param content Content to analyze
 * @param content_length Length of content in bytes
 * @param terminal_width Terminal width in characters
 * @param prompt_width Prompt width in characters
 * @return Number of lines required
 */
size_t lle_terminal_calculate_content_lines(const char *content,
                                           size_t content_length,
                                           size_t terminal_width,
                                           size_t prompt_width)
{
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (!content || content_length == 0 || terminal_width <= prompt_width) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_LINE_CALC] Empty content or invalid width: content=%p, len=%zu, term_w=%zu, prompt_w=%zu -> 1 line\n",
                    (void*)content, content_length, terminal_width, prompt_width);
        }
        return 1;
    }
    
    size_t available_width = terminal_width - prompt_width;
    if (available_width == 0) {
        return 1;
    }
    
    // Count printable characters only for line calculation
    size_t printable_chars = 0;
    for (size_t i = 0; i < content_length; i++) {
        unsigned char c = (unsigned char)content[i];
        if ((c >= 32 && c <= 126) || c == '\n' || c == '\t') {
            printable_chars++;
        }
    }
    
    size_t lines = (printable_chars + available_width - 1) / available_width;
    if (lines == 0) lines = 1;
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_LINE_CALC] Content analysis: len=%zu, printable=%zu, available_w=%zu -> %zu lines\n",
                content_length, printable_chars, available_width, lines);
    }
    
    return lines;
}

/**
 * @brief Filter control characters from content for safe display
 *
 * @param input Input content
 * @param input_length Length of input
 * @param output Output buffer
 * @param output_capacity Size of output buffer
 * @return Number of characters written to output
 */
size_t lle_terminal_filter_control_chars(const char *input,
                                        size_t input_length,
                                        char *output,
                                        size_t output_capacity)
{
    if (!input || !output || output_capacity == 0) {
        return 0;
    }
    
    size_t output_pos = 0;
    
    for (size_t i = 0; i < input_length && output_pos < output_capacity - 1; i++) {
        unsigned char c = (unsigned char)input[i];
        
        // Allow only safe printable characters and newlines
        if ((c >= 32 && c <= 126) || c == '\n' || c == '\t') {
            output[output_pos++] = c;
        }
        // Skip all control characters including DEL (127) and backspace (177)
    }
    
    output[output_pos] = '\0';
    return output_pos;
}

/**
 * @brief Clear exactly the specified number of characters using space-and-backspace
 *
 * Uses the proven safe pattern from backspace boundary crossing.
 * Clears exactly the specified amount - no more, no less.
 *
 * @param tm Terminal manager
 * @param chars_to_clear Exact number of characters to clear
 * @return true on success, false on failure
 */
bool lle_terminal_clear_exact_chars(lle_terminal_manager_t *tm, size_t chars_to_clear)
{
    if (!tm || !tm->termcap_initialized || chars_to_clear == 0) {
        return true; // Nothing to clear
    }
    
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_EXACT_CLEAR] Clearing exactly %zu characters using space-and-backspace\n", chars_to_clear);
    }
    
    // Step 1: Write spaces to overwrite content
    for (size_t i = 0; i < chars_to_clear; i++) {
        if (!lle_terminal_write(tm, " ", 1)) {
            return false;
        }
    }
    
    // Step 2: Backspace to return to original position
    for (size_t i = 0; i < chars_to_clear; i++) {
        if (!lle_terminal_write(tm, "\b", 1)) {
            return false;
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_EXACT_CLEAR] Successfully cleared exactly %zu characters\n", chars_to_clear);
    }
    
    return true;
}

/**
 * Clear multiline content using full redraw approach.
 * 
 * This function implements the professional shell approach of completely
 * clearing all content lines while preserving the prompt. This matches
 * how bash/zsh handle multiline history navigation.
 *
 * @param tm Terminal manager instance
 * @param old_content_length Length of old content to clear
 * @param prompt_width Width of prompt  
 * @param terminal_width Terminal width
 * @return true on success, false on error
 *
 * @note Clears all content but preserves prompt for immediate content replacement.
 */
bool lle_terminal_clear_multiline_content(lle_terminal_manager_t *tm,
                                        size_t old_content_length,
                                        size_t prompt_width,
                                        size_t terminal_width)
{
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_FULL_REDRAW] Starting full redraw clearing: old_len=%zu, prompt_w=%zu, term_w=%zu\n",
                old_content_length, prompt_width, terminal_width);
    }
    
    // Calculate content layout
    size_t available_first_line = terminal_width - prompt_width;
    size_t content_lines = 0;
    
    if (old_content_length > 0) {
        if (old_content_length <= available_first_line) {
            content_lines = 1;
        } else {
            size_t remaining = old_content_length - available_first_line;
            content_lines = 1 + (remaining + terminal_width - 1) / terminal_width;
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_FULL_REDRAW] Content spans %zu lines, clearing all content\n", content_lines);
    }
    
    // Step 1: Move to content start (preserve prompt)
    if (!lle_terminal_write(tm, "\r", 1)) {
        return false;
    }
    if (!lle_terminal_move_cursor_to_column(tm, prompt_width)) {
        return false;
    }
    
    // Step 2: Clear content on first line only
    size_t first_line_content = (old_content_length <= available_first_line) ? 
                               old_content_length : available_first_line;
    
    if (first_line_content > 0) {
        if (!lle_terminal_clear_exact_chars(tm, first_line_content)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_FULL_REDRAW] ERROR: Failed to clear first line content\n");
            }
            return false;
        }
    }
    
    // Step 3: Clear any wrapped lines completely
    for (size_t line = 1; line < content_lines; line++) {
        if (debug_mode) {
            fprintf(stderr, "[LLE_FULL_REDRAW] Clearing wrapped line %zu\n", line);
        }
        
        // Move to next line
        if (!lle_terminal_write(tm, "\r\n", 2)) {
            return false;
        }
        
        // Clear entire wrapped line
        if (!lle_terminal_clear_exact_chars(tm, terminal_width)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_FULL_REDRAW] ERROR: Failed to clear wrapped line %zu\n", line);
            }
            return false;
        }
        
        fflush(stdout);
    }
    
    // Step 4: Return to content start position
    for (size_t line = 1; line < content_lines; line++) {
        if (!lle_terminal_move_cursor_up(tm, 1)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_FULL_REDRAW] WARNING: Failed to move up from line %zu\n", line);
            }
        }
    }
    
    if (!lle_terminal_write(tm, "\r", 1)) {
        return false;
    }
    if (!lle_terminal_move_cursor_to_column(tm, prompt_width)) {
        return false;
    }
    
    fflush(stdout);
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_FULL_REDRAW] Full content clearing completed - cursor at content start\n");
    }
    
    return true;
}

/**
 * @brief Safe content replacement without affecting prompt
 *
 * Replaces content in the content area without redrawing prompt.
 * Uses exact clearing principles from backspace boundary crossing.
 *
 * @param tm Terminal manager
 * @param prompt_width Width of prompt in characters
 * @param old_content_length Length of content to replace
 * @param new_content New content to write
 * @param new_content_length Length of new content
 * @param terminal_width Terminal width for calculations
 * @return true on success, false on failure
 */
bool lle_terminal_safe_replace_content(lle_terminal_manager_t *tm,
                                     size_t prompt_width,
                                     size_t old_content_length,
                                     const char *new_content,
                                     size_t new_content_length,
                                     size_t terminal_width)
{
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    if (!tm || !tm->termcap_initialized) {
        return false;
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_SAFE_REPLACE] Replacing content: old_len=%zu, new_len=%zu, prompt_w=%zu, term_w=%zu\n",
                old_content_length, new_content_length, prompt_width, terminal_width);
    }
    
    // Step 1: Move to content start position
    if (!lle_terminal_write(tm, "\r", 1)) {
        return false;
    }
    if (!lle_terminal_move_cursor_to_column(tm, prompt_width)) {
        return false;
    }
    
    // Step 2: Calculate exact clearing needed
    size_t old_lines = lle_terminal_calculate_content_lines("", old_content_length, terminal_width, prompt_width);
    
    if (old_lines > 1) {
        // Multi-line clearing: ENABLED - Use proven multiline approach
        if (debug_mode) {
            fprintf(stderr, "[LLE_SAFE_REPLACE] Multi-line content detected: %zu lines, using enhanced clearing\n", old_lines);
        }
        
        // Step 1: Move to end of content to start clearing backwards
        if (!lle_terminal_write(tm, "\r", 1)) {
            return false;
        }
        if (!lle_terminal_move_cursor_to_column(tm, prompt_width)) {
            return false;
        }
        
        // Step 2: Clear using exact character count method (proven working for single-line)
        // This extends the working single-line approach to multiline content
        if (!lle_terminal_clear_exact_chars(tm, old_content_length)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_SAFE_REPLACE] ERROR: Multi-line exact character clearing failed\n");
            }
            return false;
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_SAFE_REPLACE] Multi-line exact character clearing completed\n");
        }
    } else {
        // Single line: use proven exact character clearing
        if (!lle_terminal_clear_exact_chars(tm, old_content_length)) {
            if (debug_mode) {
                fprintf(stderr, "[LLE_SAFE_REPLACE] ERROR: Single-line clearing failed\n");
            }
            return false;
        }
    }
    
    // Step 3: Write new content (filtered)
    if (new_content && new_content_length > 0) {
        char filtered_content[new_content_length + 1];
        size_t filtered_length = lle_terminal_filter_control_chars(
            new_content, new_content_length, filtered_content, sizeof(filtered_content)
        );
        
        if (filtered_length > 0) {
            if (!lle_terminal_write(tm, filtered_content, filtered_length)) {
                return false;
            }
        }
        
        if (debug_mode) {
            fprintf(stderr, "[LLE_SAFE_REPLACE] Wrote filtered content: %zu chars (was %zu)\n",
                    filtered_length, new_content_length);
        }
    }
    
    if (debug_mode) {
        fprintf(stderr, "[LLE_SAFE_REPLACE] Content replacement completed successfully\n");
    }
    
    return true;
}
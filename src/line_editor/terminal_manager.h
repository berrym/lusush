#ifndef LLE_TERMINAL_MANAGER_H
#define LLE_TERMINAL_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "cursor_math.h"
#include "termcap/lle_termcap.h"

/**
 * @file terminal_manager.h
 * @brief Lusush Line Editor - Terminal Manager
 *
 * Terminal interface and capability management for the Lusush Line Editor.
 * Provides terminal state management, raw mode control, and terminal
 * capability detection for optimal line editing experience.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief Terminal capability flags for feature detection
 */
typedef enum {
    LLE_TERM_CAP_COLORS = 1 << 0,        /**< Terminal supports colors */
    LLE_TERM_CAP_CURSOR_MOVEMENT = 1 << 1, /**< Terminal supports cursor movement */
    LLE_TERM_CAP_CLEAR_SCREEN = 1 << 2,   /**< Terminal supports screen clearing */
    LLE_TERM_CAP_ALTERNATE_SCREEN = 1 << 3, /**< Terminal supports alternate screen */
    LLE_TERM_CAP_MOUSE = 1 << 4,          /**< Terminal supports mouse reporting */
    LLE_TERM_CAP_BRACKETED_PASTE = 1 << 5, /**< Terminal supports bracketed paste */
    LLE_TERM_CAP_UTF8 = 1 << 6,           /**< Terminal supports UTF-8 encoding */
    LLE_TERM_CAP_256_COLORS = 1 << 7      /**< Terminal supports 256 colors */
} lle_terminal_capabilities_t;

/**
 * @brief Terminal state information for restoration
 */
typedef struct {
    void *original_termios;     /**< Original terminal attributes (platform specific) */
    bool was_raw_mode;          /**< Whether terminal was in raw mode before */
    bool needs_restoration;     /**< Whether terminal state needs restoration */
    uint32_t original_flags;    /**< Original terminal flags */
} lle_terminal_state_t;

/**
 * @brief Terminal manager structure for comprehensive terminal control
 *
 * This structure manages all aspects of terminal interaction including
 * geometry, capabilities, state, and file descriptors. It provides the
 * foundation for sophisticated terminal control and line editing.
 *
 * State management:
 * - Tracks terminal geometry and changes
 * - Manages raw mode state and restoration
 * - Caches capability detection results
 * - Handles file descriptor management
 * - Integrates with LLE termcap system for professional terminal handling
 */
typedef struct {
    lle_terminal_geometry_t geometry;      /**< Current terminal dimensions and prompt info */
    lle_terminal_state_t saved_state;      /**< Saved state for restoration */
    const lle_terminal_info_t *termcap_info; /**< Terminal info from integrated termcap system */
    uint32_t capabilities;                 /**< Terminal capability flags (lle_terminal_capabilities_t) */
    bool capabilities_initialized;         /**< Whether capabilities have been detected */
    bool termcap_initialized;             /**< Whether termcap system is initialized */
    bool in_raw_mode;                     /**< Whether terminal is in raw mode */
    bool geometry_valid;                  /**< Whether geometry information is current */
    bool is_iterm2;                       /**< Whether running in iTerm2 (for optimizations) */
    int stdin_fd;                         /**< Standard input file descriptor */
    int stdout_fd;                        /**< Standard output file descriptor */
    int stderr_fd;                        /**< Standard error file descriptor */
} lle_terminal_manager_t;

/**
 * @brief Terminal initialization result codes
 */
typedef enum {
    LLE_TERM_INIT_SUCCESS = 0,            /**< Terminal initialized successfully */
    LLE_TERM_INIT_ERROR_INVALID_FD = -1,  /**< Invalid file descriptor */
    LLE_TERM_INIT_ERROR_NOT_TTY = -2,     /**< File descriptor is not a TTY */
    LLE_TERM_INIT_ERROR_CAPABILITIES = -3, /**< Failed to detect capabilities */
    LLE_TERM_INIT_ERROR_RAW_MODE = -4,    /**< Failed to enter raw mode */
    LLE_TERM_INIT_ERROR_GEOMETRY = -5     /**< Failed to get terminal geometry */
} lle_terminal_init_result_t;

// Task LLE-009: Terminal manager structure functions

/**
 * @brief Initialize terminal manager with default settings
 * @param tm Pointer to terminal manager structure
 * @return LLE_TERM_INIT_SUCCESS on success, error code on failure
 */
lle_terminal_init_result_t lle_terminal_init(lle_terminal_manager_t *tm);

/**
 * @brief Clean up terminal manager and restore original state
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_cleanup(lle_terminal_manager_t *tm);

/**
 * @brief Get current terminal size and update geometry
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_get_size(lle_terminal_manager_t *tm);

/**
 * @brief Detect terminal capabilities and update flags
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_detect_capabilities(lle_terminal_manager_t *tm);

/**
 * @brief Check if terminal has specific capability
 * @param tm Pointer to terminal manager structure
 * @param capability Capability flag to check
 * @return true if capability is supported, false otherwise
 */
bool lle_terminal_has_capability(const lle_terminal_manager_t *tm, lle_terminal_capabilities_t capability);

/**
 * @brief Enter raw mode for character-by-character input
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_enter_raw_mode(lle_terminal_manager_t *tm);

/**
 * @brief Exit raw mode and restore normal terminal behavior
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_exit_raw_mode(lle_terminal_manager_t *tm);

/**
 * @brief Validate terminal manager structure
 * @param tm Pointer to terminal manager structure
 * @return true if structure is valid, false otherwise
 */
bool lle_terminal_manager_is_valid(const lle_terminal_manager_t *tm);

/**
 * @brief Get human-readable description of terminal capabilities
 * @param tm Pointer to terminal manager structure
 * @param buffer Buffer to store description
 * @param buffer_size Size of buffer in bytes
 * @return Number of characters written, or -1 on error
 */
int lle_terminal_get_capabilities_string(const lle_terminal_manager_t *tm, char *buffer, size_t buffer_size);

/**
 * @brief Update terminal geometry from termcap system
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_update_geometry(lle_terminal_manager_t *tm);

/**
 * @brief Check if terminal is iTerm2 (for optimizations)
 * @param tm Pointer to terminal manager structure
 * @return true if iTerm2, false otherwise
 */
bool lle_terminal_is_iterm2(const lle_terminal_manager_t *tm);

// Task LLE-010: Terminal initialization functions (COMPLETED)
// Task LLE-011: Terminal output functions

/**
 * @brief Write data to terminal with proper error handling
 * @param tm Pointer to terminal manager structure
 * @param data Data to write
 * @param length Length of data in bytes
 * @return true on success, false on failure
 */
bool lle_terminal_write(lle_terminal_manager_t *tm, const char *data, size_t length);

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
bool lle_terminal_clear_exactly(lle_terminal_manager_t *tm, size_t length_to_clear);

/**
 * @brief Move cursor to specified position using termcap
 * @param tm Pointer to terminal manager structure
 * @param row Target row (0-based)
 * @param col Target column (0-based)
 * @return true on success, false on failure
 */
bool lle_terminal_move_cursor(lle_terminal_manager_t *tm, size_t row, size_t col);

/**
 * @brief Clear current line using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_line(lle_terminal_manager_t *tm);

/**
 * @brief Clear from cursor to end of line using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_to_eol(lle_terminal_manager_t *tm);

/**
 * @brief Clear entire screen using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_screen(lle_terminal_manager_t *tm);

/**
 * @brief Clear from cursor to end of screen using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_clear_to_eos(lle_terminal_manager_t *tm);

/**
 * @brief Set terminal colors using termcap
 * @param tm Pointer to terminal manager structure
 * @param fg Foreground color
 * @param bg Background color
 * @return true on success, false on failure
 */
bool lle_terminal_set_color(lle_terminal_manager_t *tm, lle_termcap_color_t fg, lle_termcap_color_t bg);

/**
 * @brief Reset terminal colors to defaults using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_reset_colors(lle_terminal_manager_t *tm);

/**
 * @brief Hide cursor using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_hide_cursor(lle_terminal_manager_t *tm);

/**
 * @brief Show cursor using termcap
 * @param tm Pointer to terminal manager structure
 * @return true on success, false on failure
 */
bool lle_terminal_show_cursor(lle_terminal_manager_t *tm);

/**
 * @brief Move cursor up by specified number of lines
 * @param tm Pointer to terminal manager structure
 * @param lines Number of lines to move up
 * @return true on success, false on failure
 */
bool lle_terminal_move_cursor_up(lle_terminal_manager_t *tm, size_t lines);

/**
 * @brief Move cursor down by specified number of lines
 * @param tm Pointer to terminal manager structure
 * @param lines Number of lines to move down
 * @return true on success, false on failure
 */
bool lle_terminal_move_cursor_down(lle_terminal_manager_t *tm, size_t lines);

/**
 * @brief Move cursor to specified column on current line
 * @param tm Pointer to terminal manager structure
 * @param col Target column (0-based)
 * @return true on success, false on failure
 */
bool lle_terminal_move_cursor_to_column(lle_terminal_manager_t *tm, size_t col);

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
                              size_t end_row, size_t end_col);

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
                             size_t start_row, size_t num_lines);

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
                                            size_t row, size_t col);

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
                                            size_t row, size_t col);

// Safe Content Replacement Functions (Based on Proven Patterns)

/**
 * @brief Safely replace content area without affecting prompt
 *
 * This function uses proven safe terminal operations to replace content
 * in the terminal without causing display corruption. It handles both
 * single-line and multiline content correctly.
 *
 * @param tm Pointer to terminal manager structure
 * @param prompt_width Width of the prompt (content starts after this)
 * @param old_content_length Length of existing content to clear
 * @param new_content New content to display (can be NULL for clearing only)
 * @param new_content_length Length of new content
 * @param max_lines Maximum lines the content might span
 * @return true on success, false on failure
 */
bool lle_terminal_safe_replace_content(lle_terminal_manager_t *tm,
                                     size_t prompt_width,
                                     size_t old_content_length,
                                     const char *new_content,
                                     size_t new_content_length,
                                     size_t max_lines);

/**
 * @brief Filter control characters from text for safe display
 *
 * Removes potentially harmful control characters from text content
 * before displaying it. Based on proven safe character filtering.
 *
 * @param input Input text that may contain control characters
 * @param input_length Length of input text
 * @param output Buffer to store filtered text
 * @param output_capacity Maximum capacity of output buffer
 * @return Length of filtered text, 0 on error
 */
size_t lle_terminal_filter_control_chars(const char *input,
                                        size_t input_length,
                                        char *output,
                                        size_t output_capacity);

/**
 * @brief Calculate lines needed for content with given width
 *
 * Calculates how many terminal lines the given content will occupy
 * when displayed with the specified terminal width.
 *
 * @param content Content to measure
 * @param content_length Length of content
 * @param terminal_width Width of terminal
 * @param prompt_width Width occupied by prompt
 * @return Number of lines needed
 */
size_t lle_terminal_calculate_content_lines(const char *content,
                                           size_t content_length,
                                           size_t terminal_width,
                                           size_t prompt_width);

/**
 * @brief Filter control characters from content for safe display
 */
size_t lle_terminal_filter_control_chars(const char *input,
                                        size_t input_length,
                                        char *output,
                                        size_t output_capacity);

/**
 * @brief Clear exactly the specified number of characters using space-and-backspace
 */
bool lle_terminal_clear_exact_chars(lle_terminal_manager_t *tm, size_t chars_to_clear);

/**
 * @brief Safe content replacement without affecting prompt
 */
bool lle_terminal_safe_replace_content(lle_terminal_manager_t *tm,
                                     size_t prompt_width,
                                     size_t old_content_length,
                                     const char *new_content,
                                     size_t new_content_length,
                                     size_t terminal_width);

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
                                      size_t *saved_row, size_t *saved_col);

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
                                       size_t *current_row, size_t *current_col);

#endif // LLE_TERMINAL_MANAGER_H

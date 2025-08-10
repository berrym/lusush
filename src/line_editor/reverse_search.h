/**
 * @file reverse_search.h
 * @brief Reliable Ctrl+R reverse search interface for Lusush Line Editor (LLE)
 * 
 * This module provides comprehensive reverse search functionality using the proven
 * exact backspace replication pattern that was successful for tab completion.
 * Bypasses broken display state integration system in favor of direct terminal operations.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2025
 */

#ifndef LLE_REVERSE_SEARCH_H
#define LLE_REVERSE_SEARCH_H

#include <stdbool.h>
#include <stddef.h>

// Include actual type definitions instead of forward declarations
#include "display.h"
#include "text_buffer.h"
#include "command_history.h"

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Search direction for navigation
 */
typedef enum {
    LLE_SEARCH_BACKWARD = 0,    /**< Search backward through history (Ctrl+R) */
    LLE_SEARCH_FORWARD = 1      /**< Search forward through history (Ctrl+S) */
} lle_search_direction_t;

/**
 * @brief Reverse search state structure
 * 
 * Contains all state information for active reverse search session.
 * This follows the proven pattern used in the successful tab completion system.
 */
typedef struct {
    bool active;                        /**< Whether search mode is active */
    char query[256];                    /**< Current search query */
    char *original_line;                /**< Saved original line content */
    int current_match_index;            /**< Current match index in history */
    lle_search_direction_t search_direction; /**< Current search direction */
    lle_display_state_t *display;       /**< Display state for operations */
} lle_reverse_search_state_t;

// ============================================================================
// Core API Functions
// ============================================================================

/**
 * @brief Initialize the reverse search system
 * 
 * Sets up global state and prepares the reverse search system for use.
 * Should be called once during line editor initialization.
 * 
 * @return true on success, false on error
 */
bool lle_reverse_search_init(void);

/**
 * @brief Clean up the reverse search system
 * 
 * Frees any allocated memory and resets global state.
 * Should be called during line editor cleanup.
 */
void lle_reverse_search_cleanup(void);

/**
 * @brief Check if reverse search is currently active
 * 
 * @return true if reverse search mode is active, false otherwise
 */
bool lle_reverse_search_is_active(void);

// ============================================================================
// Search Session Management
// ============================================================================

/**
 * @brief Enter reverse search mode
 * 
 * Initiates a new reverse search session. Saves the current line content,
 * clears the buffer using exact backspace replication, and displays the
 * initial search prompt.
 * 
 * @param display Display state for terminal operations
 * @param buffer Current text buffer content to save
 * @param history Command history to search through
 * @return true on success, false on error
 * 
 * @note Uses proven exact backspace replication pattern from tab completion
 * @note Bypasses broken display state integration system
 */
bool lle_reverse_search_enter(lle_display_state_t *display,
                             const lle_text_buffer_t *buffer,
                             const lle_history_t *history);

/**
 * @brief Exit reverse search mode
 * 
 * Terminates the current reverse search session. Can either accept the
 * current match or restore the original line content.
 * 
 * @param display Display state for terminal operations
 * @param accept_match true to keep current match, false to restore original line
 * @return true on success, false on error
 * 
 * @note Uses reliable line restoration pattern
 */
bool lle_reverse_search_exit(lle_display_state_t *display, bool accept_match);

// ============================================================================
// Search Input Handling
// ============================================================================

/**
 * @brief Add character to search query
 * 
 * Adds a character to the current search query and immediately searches
 * for matches. Updates the display with the first match found.
 * 
 * @param display Display state for terminal operations
 * @param buffer Text buffer to update with matches
 * @param history Command history to search
 * @param c Character to add to search query
 * @return true on success, false on error
 * 
 * @note Uses exact backspace replication for buffer updates
 * @note Searches from most recent history entry backward
 */
bool lle_reverse_search_add_char(lle_display_state_t *display,
                                lle_text_buffer_t *buffer,
                                const lle_history_t *history,
                                char c);

/**
 * @brief Remove character from search query (backspace)
 * 
 * Removes the last character from the search query and re-searches
 * with the shortened query. Updates display accordingly.
 * 
 * @param display Display state for terminal operations
 * @param buffer Text buffer to update with matches
 * @param history Command history to search
 * @return true on success, false on error
 * 
 * @note Handles empty query gracefully
 * @note Uses proven buffer management patterns
 */
bool lle_reverse_search_backspace(lle_display_state_t *display,
                                 lle_text_buffer_t *buffer,
                                 const lle_history_t *history);

// ============================================================================
// Search Navigation
// ============================================================================

/**
 * @brief Navigate to next match in specified direction
 * 
 * Finds the next match in the specified direction (backward/forward)
 * relative to the current match. Updates buffer and display.
 * 
 * @param display Display state for terminal operations
 * @param buffer Text buffer to update with matches
 * @param history Command history to search
 * @param direction Search direction (backward or forward)
 * @return true if match found, false if no more matches
 * 
 * @note Supports both Ctrl+R (backward) and Ctrl+S (forward) navigation
 * @note Shows appropriate prompt indicators for direction
 * @note Handles search exhaustion gracefully
 */
bool lle_reverse_search_next_match(lle_display_state_t *display,
                                  lle_text_buffer_t *buffer,
                                  const lle_history_t *history,
                                  lle_search_direction_t direction);

// ============================================================================
// State Query Functions
// ============================================================================

/**
 * @brief Get current search query
 * 
 * @return Current search query string, or NULL if not in search mode
 */
const char *lle_reverse_search_get_query(void);

/**
 * @brief Get current match index
 * 
 * @return Index of current match in history, or -1 if no match
 */
int lle_reverse_search_get_match_index(void);

/**
 * @brief Get current search direction
 * 
 * @return Current search direction (backward or forward)
 */
lle_search_direction_t lle_reverse_search_get_direction(void);

// ============================================================================
// Integration Macros
// ============================================================================

/**
 * @brief Convenience macro for entering reverse search
 * 
 * Typical usage in main input loop:
 * ```c
 * case LLE_KEY_CTRL_R:
 *     LLE_REVERSE_SEARCH_ENTER(display, buffer, history);
 *     break;
 * ```
 */
#define LLE_REVERSE_SEARCH_ENTER(display, buffer, history) \
    lle_reverse_search_enter((display), (buffer), (history))

/**
 * @brief Convenience macro for accepting search match
 * 
 * Typical usage for Enter key in search mode:
 * ```c
 * if (lle_reverse_search_is_active()) {
 *     LLE_REVERSE_SEARCH_ACCEPT(display);
 * }
 * ```
 */
#define LLE_REVERSE_SEARCH_ACCEPT(display) \
    lle_reverse_search_exit((display), true)

/**
 * @brief Convenience macro for cancelling search
 * 
 * Typical usage for Ctrl+G or Escape in search mode:
 * ```c
 * if (lle_reverse_search_is_active()) {
 *     LLE_REVERSE_SEARCH_CANCEL(display);
 * }
 * ```
 */
#define LLE_REVERSE_SEARCH_CANCEL(display) \
    lle_reverse_search_exit((display), false)

/**
 * @brief Convenience macro for repeat backward search
 * 
 * Typical usage for repeated Ctrl+R:
 * ```c
 * if (lle_reverse_search_is_active()) {
 *     LLE_REVERSE_SEARCH_NEXT_BACKWARD(display, buffer, history);
 * }
 * ```
 */
#define LLE_REVERSE_SEARCH_NEXT_BACKWARD(display, buffer, history) \
    lle_reverse_search_next_match((display), (buffer), (history), LLE_SEARCH_BACKWARD)

/**
 * @brief Convenience macro for forward search
 * 
 * Typical usage for Ctrl+S:
 * ```c
 * if (lle_reverse_search_is_active()) {
 *     LLE_REVERSE_SEARCH_NEXT_FORWARD(display, buffer, history);
 * }
 * ```
 */
#define LLE_REVERSE_SEARCH_NEXT_FORWARD(display, buffer, history) \
    lle_reverse_search_next_match((display), (buffer), (history), LLE_SEARCH_FORWARD)

#endif // LLE_REVERSE_SEARCH_H
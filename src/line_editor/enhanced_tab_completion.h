/*
 * Enhanced Tab Completion Header for Lusush Line Editor (LLE)
 * 
 * This header defines an enhanced tab completion system that fixes cross-platform
 * issues and provides consistent behavior across different terminals and operating
 * systems. It improves upon the original completion system with better state
 * management, word boundary detection, and completion cycling.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#ifndef LLE_ENHANCED_TAB_COMPLETION_H
#define LLE_ENHANCED_TAB_COMPLETION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Required includes for proper type definitions
#include "text_buffer.h" 
#include "completion.h"
#include "display_state_integration.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Enhanced Completion Type Enumerations
// ============================================================================

/**
 * @brief Types of completions supported by the enhanced system
 */
typedef enum {
    LLE_ENHANCED_COMPLETION_FILE = 0,      /**< File and directory completion */
    LLE_ENHANCED_COMPLETION_COMMAND = 1,   /**< Shell command completion */
    LLE_ENHANCED_COMPLETION_VARIABLE = 2,  /**< Environment variable completion */
    LLE_ENHANCED_COMPLETION_PATH = 3       /**< Path-specific completion */
} lle_enhanced_completion_type_t;

// ============================================================================
// Enhanced Completion Information Structure
// ============================================================================

/**
 * @brief Information about the current completion session
 */
typedef struct {
    bool active;                        /**< Whether completion session is active */
    lle_enhanced_completion_type_t completion_type; /**< Type of completion */
    int current_index;                  /**< Current completion index */
    int total_count;                    /**< Total number of completions */
    bool has_cycled;                    /**< Whether user has cycled through completions */
    char original_word[512];            /**< Original word being completed */
    char current_completion[512];       /**< Currently applied completion */
    size_t word_start_pos;              /**< Start position of word in buffer */
    size_t word_end_pos;                /**< End position of word in buffer */
} lle_enhanced_completion_info_t;

// ============================================================================
// Core Enhanced Tab Completion Functions
// ============================================================================

/**
 * @brief Initialize the enhanced tab completion system
 * 
 * This function initializes the enhanced tab completion system and sets up
 * internal state management. It should be called once during LLE initialization.
 * 
 * @return true on successful initialization
 * 
 * @note This function automatically detects debug mode from environment variables
 *       LLE_DEBUG_COMPLETION or LLE_DEBUG and provides detailed logging when enabled.
 */
bool lle_enhanced_tab_completion_init(void);

/**
 * @brief Clean up the enhanced tab completion system
 * 
 * Releases all resources and resets completion state. This should be called
 * during LLE cleanup to ensure proper resource management.
 */
void lle_enhanced_tab_completion_cleanup(void);

/**
 * @brief Handle a tab completion request
 * 
 * This is the main function for processing tab completion requests. It handles
 * both initial completion requests and cycling through multiple completions.
 * The function automatically detects whether this is a new completion session
 * or a continuation of an existing one.
 * 
 * @param buffer Text buffer containing the current line
 * @param completions Completion list to populate and manage
 * @return true if completion was handled and buffer was modified
 * 
 * @note This function replaces the original tab completion logic and provides
 *       improved state management and cross-platform compatibility.
 */
bool lle_enhanced_tab_completion_handle(lle_text_buffer_t *buffer,
                                      lle_completion_list_t *completions,
                                      lle_display_integration_t *display_integration);

/**
 * @brief Reset the current completion session
 * 
 * This function ends any active completion session and resets all internal
 * state. It should be called when the user performs an action that should
 * terminate completion (like moving the cursor away from the completion area).
 */
void lle_enhanced_tab_completion_reset(void);

/**
 * @brief Check if a completion session is currently active
 * 
 * @return true if there is an active completion session
 * 
 * @note This can be used by other parts of the editor to determine whether
 *       certain key events should be handled differently during completion.
 */
bool lle_enhanced_tab_completion_is_active(void);

/**
 * @brief Get information about the current completion session
 * 
 * Returns detailed information about the current completion session including
 * the type of completion, current index, total count, and other relevant data.
 * 
 * @return Pointer to completion information structure
 * 
 * @note The returned pointer points to static data that is updated on each call.
 *       If no completion session is active, all fields will be zeroed.
 */
const lle_enhanced_completion_info_t *lle_enhanced_tab_completion_get_info(void);

/**
 * @brief Get detailed debug information about the completion system
 * 
 * Returns a formatted string containing comprehensive debug information about
 * the current completion session, internal state, and any active operations.
 * 
 * @return Pointer to static debug information string
 * 
 * @note The returned string is updated on each call and should be copied if
 *       persistence is needed. This function is primarily for debugging and
 *       troubleshooting completion issues.
 */
const char *lle_enhanced_tab_completion_get_debug_info(void);

// ============================================================================
// Integration Helpers
// ============================================================================

/**
 * @brief Integration pattern for replacing original tab completion
 * 
 * Replace the original tab completion logic in line_editor.c:
 * 
 * // OLD CODE (in LLE_KEY_TAB case):
 * // [complex static variable management and cycling logic]
 * 
 * // NEW CODE:
 * if (lle_enhanced_tab_completion_handle(editor->buffer, editor->completions, editor->state_integration)) {
 *     // Display update handled by tab completion function
 *     needs_display_update = false;
 *     
 *     // Optional: Show completion info for debugging
 *     if (debug_mode) {
 *         const lle_enhanced_completion_info_t *info = 
 *             lle_enhanced_tab_completion_get_info();
 *         fprintf(stderr, "[TAB_COMPLETION] %d/%d: %s\n",
 *                info->current_index + 1, info->total_count,
 *                info->current_completion);
 *     }
 * } else {
 *     needs_display_update = false;
 * }
 */

/**
 * @brief Integration pattern for cursor movement handling
 * 
 * Add to cursor movement handlers to reset completion when appropriate:
 * 
 * // In cursor movement cases (arrows, home, end, etc.):
 * if (lle_enhanced_tab_completion_is_active()) {
 *     lle_enhanced_tab_completion_reset();
 * }
 * 
 * // Then proceed with normal cursor movement
 */

/**
 * @brief Integration pattern for initialization
 * 
 * Add to LLE initialization:
 * 
 * // During editor initialization:
 * if (!lle_enhanced_tab_completion_init()) {
 *     // Handle initialization failure
 * }
 * 
 * // During editor cleanup:
 * lle_enhanced_tab_completion_cleanup();
 */

// ============================================================================
// Configuration and Feature Flags
// ============================================================================

/**
 * @brief Environment variables recognized by the enhanced completion system
 * 
 * LLE_DEBUG_COMPLETION=1    Enable detailed completion debugging
 * LLE_DEBUG=1              Enable general LLE debugging (includes completion)
 */

// ============================================================================
// Cross-Platform Compatibility Notes
// ============================================================================

/**
 * @brief Cross-platform improvements implemented
 * 
 * The enhanced tab completion system addresses several cross-platform issues:
 * 
 * 1. **Completion Cycling on Linux/Konsole**: Fixed state management that
 *    caused cycling to fail on some Linux terminals.
 * 
 * 2. **Word Boundary Detection**: Improved detection of word boundaries that
 *    works consistently across different shell contexts.
 * 
 * 3. **Session State Management**: Robust session tracking that prevents
 *    completion state corruption during rapid key presses.
 * 
 * 4. **Memory Management**: Proper cleanup of completion state to prevent
 *    memory leaks during long editing sessions.
 * 
 * 5. **Terminal Event Handling**: Better handling of terminal-specific
 *    key event timing and sequencing.
 */

// ============================================================================
// Performance Characteristics
// ============================================================================

/**
 * @brief Performance targets for enhanced completion
 * 
 * - Initial completion generation: < 50ms
 * - Completion cycling: < 1ms
 * - Memory usage: < 100KB for typical completion sets
 * - File system operations: Cached where possible
 * - Completion set size: Efficiently handles 1000+ completions
 */

// ============================================================================
// Error Handling and Edge Cases
// ============================================================================

/**
 * @brief Error conditions handled by the enhanced system
 * 
 * - Invalid buffer states (NULL pointers, corrupted data)
 * - File system errors during completion generation
 * - Memory allocation failures
 * - Cursor position outside buffer bounds
 * - Empty or whitespace-only input
 * - Very long completion lists
 * - Rapid successive tab presses
 * - Terminal resize during completion
 */

// ============================================================================
// Completion Menu Navigation Functions
// ============================================================================

/**
 * @brief Navigate up in the completion menu
 * 
 * Moves the selection to the previous completion item in the menu.
 * If at the first item, wraps to the last item.
 * 
 * @param buffer Text buffer to update with selected completion
 * @param completions Completion list to navigate
 * @param display_integration Display integration for visual updates
 * @return true if navigation succeeded, false if no active completion session
 */
bool lle_enhanced_tab_completion_navigate_up(lle_text_buffer_t *buffer,
                                            lle_completion_list_t *completions,
                                            lle_display_integration_t *display_integration);

/**
 * @brief Navigate down in the completion menu
 * 
 * Moves the selection to the next completion item in the menu.
 * If at the last item, wraps to the first item.
 * 
 * @param buffer Text buffer to update with selected completion
 * @param completions Completion list to navigate
 * @param display_integration Display integration for visual updates
 * @return true if navigation succeeded, false if no active completion session
 */
bool lle_enhanced_tab_completion_navigate_down(lle_text_buffer_t *buffer,
                                             lle_completion_list_t *completions,
                                             lle_display_integration_t *display_integration);

/**
 * @brief Navigate left in the completion menu
 * 
 * Moves the selection to the previous page of completions or previous completion
 * depending on menu layout. Provides horizontal navigation in multi-column menus.
 * 
 * @param buffer Text buffer to update with selected completion
 * @param completions Completion list to navigate
 * @param display_integration Display integration for visual updates
 * @return true if navigation succeeded, false if no active completion session
 */
bool lle_enhanced_tab_completion_navigate_left(lle_text_buffer_t *buffer,
                                             lle_completion_list_t *completions,
                                             lle_display_integration_t *display_integration);

/**
 * @brief Navigate right in the completion menu
 * 
 * Moves the selection to the next page of completions or next completion
 * depending on menu layout. Provides horizontal navigation in multi-column menus.
 * 
 * @param buffer Text buffer to update with selected completion
 * @param completions Completion list to navigate
 * @param display_integration Display integration for visual updates
 * @return true if navigation succeeded, false if no active completion session
 */
bool lle_enhanced_tab_completion_navigate_right(lle_text_buffer_t *buffer,
                                              lle_completion_list_t *completions,
                                              lle_display_integration_t *display_integration);

/**
 * @brief Accept currently selected completion and close menu
 * 
 * Applies the currently selected completion to the buffer and ends the
 * completion session, closing the menu.
 * 
 * @param buffer Text buffer to update with selected completion
 * @param completions Completion list containing current selection
 * @param display_integration Display integration for visual updates
 * @return true if completion was accepted, false if no active completion session
 */
bool lle_enhanced_tab_completion_accept(lle_text_buffer_t *buffer,
                                      lle_completion_list_t *completions,
                                      lle_display_integration_t *display_integration);

#ifdef __cplusplus
}
#endif

#endif /* LLE_ENHANCED_TAB_COMPLETION_H */
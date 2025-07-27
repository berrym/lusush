#ifndef LLE_INTEGRATION_H
#define LLE_INTEGRATION_H

/**
 * @file lle_integration.h
 * @brief Lusush Line Editor Integration - Linenoise Replacement Interface
 *
 * This module provides the integration layer for replacing linenoise with LLE
 * throughout Lusush. It maintains API compatibility while providing enhanced
 * functionality through the complete LLE system.
 *
 * The integration preserves existing function signatures and behavior while
 * internally using LLE for superior multiline support, Unicode handling,
 * syntax highlighting, and comprehensive history management.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdbool.h>
#include <stddef.h>

// Include LLE core components
#include "line_editor.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Global LLE Integration State
// ============================================================================

/**
 * @brief Global LLE instance for linenoise compatibility
 *
 * This global instance maintains the line editor state across function calls,
 * providing compatibility with linenoise's stateless API model while leveraging
 * LLE's stateful advantages.
 */
extern lle_line_editor_t *lle_global_editor;

// ============================================================================
// Linenoise Compatibility API
// ============================================================================

/**
 * @brief Initialize LLE integration system
 *
 * Initializes the global LLE instance and sets up the integration layer.
 * This should be called once during shell startup before any line editing
 * operations.
 *
 * @return true on success, false on failure
 *
 * @note This function is called automatically on first use if not called explicitly
 * @note Safe to call multiple times - subsequent calls are ignored
 */
bool lle_integration_init(void);

/**
 * @brief Shutdown LLE integration system
 *
 * Destroys the global LLE instance and cleans up all resources. This should
 * be called during shell shutdown to ensure proper cleanup.
 *
 * @note Safe to call multiple times or with uninitialized system
 * @note All pending history is saved before shutdown
 */
void lle_integration_shutdown(void);

/**
 * @brief Check if LLE integration is initialized
 *
 * Returns whether the LLE integration system has been properly initialized
 * and is ready for use.
 *
 * @return true if initialized, false otherwise
 */
bool lle_integration_is_initialized(void);

/**
 * @brief Read a line of input with LLE (linenoise replacement)
 *
 * This function replaces linenoise() calls throughout Lusush. It provides
 * the same interface while internally using LLE for enhanced functionality.
 *
 * Features provided:
 * - Multiline prompt support with ANSI escape codes
 * - Full Unicode support with proper cursor movement
 * - Command history navigation with Up/Down arrows
 * - Tab completion with multiple providers
 * - Syntax highlighting (if enabled)
 * - Undo/redo operations
 * - All standard editing keys
 *
 * @param prompt Prompt string to display (supports ANSI escape codes)
 * @return Dynamically allocated string containing user input, or NULL on EOF/error
 *
 * @note The returned string must be freed with lle_integration_free()
 * @note Returns NULL on EOF (Ctrl+D) or error
 * @note Automatically initializes LLE integration on first call
 * @note Compatible with existing linenoise() usage patterns
 */
char *lle_integration_readline(const char *prompt);

/**
 * @brief Free memory allocated by LLE integration (linenoiseFree replacement)
 *
 * Frees memory allocated by lle_integration_readline() and other LLE
 * integration functions. This replaces linenoiseFree() calls.
 *
 * @param ptr Pointer to memory to free (can be NULL)
 *
 * @note Safe to call with NULL pointer
 * @note Only frees memory allocated by LLE integration functions
 */
void lle_integration_free(void *ptr);

/**
 * @brief Add a line to command history (linenoiseHistoryAdd replacement)
 *
 * Adds the specified line to the command history managed by LLE.
 * This replaces linenoiseHistoryAdd() calls.
 *
 * @param line Line to add to history (must not be NULL or empty)
 * @return 0 on success, -1 on failure (for linenoise compatibility)
 *
 * @note Empty lines and whitespace-only lines are ignored
 * @note Duplicate consecutive entries may be filtered
 * @note History is automatically saved to disk
 */
int lle_integration_history_add(const char *line);

/**
 * @brief Set maximum history length (linenoiseHistorySetMaxLen replacement)
 *
 * Sets the maximum number of entries to keep in command history.
 * This replaces linenoiseHistorySetMaxLen() calls.
 *
 * @param len Maximum number of history entries (0 for unlimited)
 * @return 1 on success, 0 on failure (for linenoise compatibility)
 *
 * @note Setting to 0 uses default limit (typically 1000)
 * @note Existing history may be truncated if new limit is smaller
 */
int lle_integration_history_set_max_len(int len);

/**
 * @brief Save history to file (linenoiseHistorySave replacement)
 *
 * Saves the current command history to the specified file.
 * This replaces linenoiseHistorySave() calls.
 *
 * @param filename Path to history file to save
 * @return 0 on success, -1 on failure (for linenoise compatibility)
 *
 * @note Creates file if it doesn't exist
 * @note Overwrites existing file content
 * @note Uses UTF-8 encoding for international commands
 */
int lle_integration_history_save(const char *filename);

/**
 * @brief Load history from file (linenoiseHistoryLoad replacement)
 *
 * Loads command history from the specified file.
 * This replaces linenoiseHistoryLoad() calls.
 *
 * @param filename Path to history file to load
 * @return 0 on success, -1 on failure (for linenoise compatibility)
 *
 * @note Existing history is preserved - new entries are added
 * @note Handles UTF-8 encoded files correctly
 * @note Creates empty history if file doesn't exist
 */
int lle_integration_history_load(const char *filename);

/**
 * @brief Print all history entries (linenoiseHistoryPrint replacement)
 *
 * Prints all command history entries with line numbers.
 * This replaces linenoiseHistoryPrint() calls.
 *
 * @note Prints to stdout with standard formatting
 * @note Uses 1-based numbering for consistency with shells
 * @note Handles Unicode content correctly
 */
void lle_integration_history_print(void);

/**
 * @brief Get history entry by index (linenoiseHistoryGet replacement)
 *
 * Retrieves a specific history entry by its index.
 * This replaces linenoiseHistoryGet() calls.
 *
 * @param index 0-based index of history entry to retrieve
 * @return Pointer to history entry string, or NULL if index invalid
 *
 * @note Returned pointer is valid until next history operation
 * @note Index 0 is the oldest entry, increasing index = newer entries
 * @note Does not allocate memory - returns internal pointer
 */
char *lle_integration_history_get(int index);

/**
 * @brief Remove duplicate history entries (linenoiseHistoryRemoveDups replacement)
 *
 * Removes duplicate entries from command history, keeping the most recent
 * occurrence of each unique command.
 *
 * @return Number of entries removed
 *
 * @note Preserves chronological order of remaining entries
 * @note Case-sensitive comparison
 * @note Updates history file automatically
 */
int lle_integration_history_remove_dups(void);

/**
 * @brief Enable or disable history deduplication (linenoiseHistoryNoDups replacement)
 *
 * Controls automatic deduplication of history entries.
 * When enabled, duplicate commands are moved to end instead of creating duplicates.
 *
 * @param flag true to enable deduplication, false to disable
 *
 * @note This is the hist_no_dups feature implemented in LLE
 * @note Can be toggled at runtime
 * @note Affects future history additions, not existing entries
 */
void lle_integration_history_no_dups(bool flag);

// ============================================================================
// Enhanced History API for POSIX Compliance
// ============================================================================

/**
 * @brief Get number of history entries
 *
 * Returns the current number of entries in command history.
 * This is an enhancement over standard linenoise.
 *
 * @return Number of history entries
 */
size_t lle_integration_history_count(void);

/**
 * @brief Clear all history entries
 *
 * Removes all entries from command history.
 * This is an enhancement for the history builtin.
 *
 * @return true on success, false on failure
 */
bool lle_integration_history_clear(void);

/**
 * @brief Delete specific history entry
 *
 * Removes a specific history entry by index.
 * This is an enhancement for POSIX fc command compliance.
 *
 * @param index 0-based index of entry to delete
 * @return true on success, false on failure
 */
bool lle_integration_history_delete(int index);

/**
 * @brief Get history entry range
 *
 * Retrieves multiple history entries within a specified range.
 * This is an enhancement for POSIX fc command compliance.
 *
 * @param start Start index (0-based, inclusive)
 * @param end End index (0-based, inclusive) 
 * @param reverse true to return in reverse order
 * @param entries Pointer to array that will be allocated and filled
 * @param count Pointer to variable that will receive entry count
 * @return true on success, false on failure
 *
 * @note Caller must free the returned entries array
 * @note Individual entry strings should NOT be freed
 */
bool lle_integration_history_get_range(int start, int end, bool reverse, 
                                      char ***entries, size_t *count);

// ============================================================================
// Configuration and Feature Control
// ============================================================================

/**
 * @brief Enable or disable multiline editing mode
 *
 * Controls whether LLE allows multiline input. This enhances the editing
 * experience beyond what linenoise provides.
 *
 * @param enable true to enable multiline mode, false to disable
 * @return true on success, false on failure
 */
bool lle_integration_set_multiline_mode(bool enable);

/**
 * @brief Enable or disable syntax highlighting
 *
 * Controls whether LLE applies syntax highlighting to user input.
 * This is a major enhancement over linenoise.
 *
 * @param enable true to enable syntax highlighting, false to disable
 * @return true on success, false on failure
 */
bool lle_integration_set_syntax_highlighting(bool enable);

/**
 * @brief Enable or disable auto-completion
 *
 * Controls whether LLE provides tab completion functionality.
 * This enhances the basic completion provided by linenoise.
 *
 * @param enable true to enable auto-completion, false to disable
 * @return true on success, false on failure
 */
bool lle_integration_set_auto_completion(bool enable);

/**
 * @brief Enable or disable undo/redo functionality
 *
 * Controls whether LLE provides undo/redo operations.
 * This is a major enhancement over linenoise.
 *
 * @param enable true to enable undo/redo, false to disable
 * @return true on success, false on failure
 */
bool lle_integration_set_undo_enabled(bool enable);

// ============================================================================
// Error Handling and Diagnostics
// ============================================================================

/**
 * @brief Get last error message from LLE integration
 *
 * Returns a human-readable description of the last error that occurred
 * in the LLE integration layer.
 *
 * @return Error message string, or NULL if no error
 *
 * @note Error message is valid until next LLE integration operation
 * @note Used for debugging and error reporting
 */
const char *lle_integration_get_last_error(void);

/**
 * @brief Get integration statistics
 *
 * Provides statistics about LLE integration usage for debugging and
 * performance monitoring.
 *
 * @param readline_calls Pointer to receive number of readline calls
 * @param history_operations Pointer to receive number of history operations
 * @param memory_allocated Pointer to receive bytes currently allocated
 * @return true on success, false on failure
 */
bool lle_integration_get_statistics(size_t *readline_calls, 
                                   size_t *history_operations,
                                   size_t *memory_allocated);

#ifdef __cplusplus
}
#endif

#endif /* LLE_INTEGRATION_H */
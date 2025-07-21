/**
 * @file lle_history.h
 * @brief Command history data structures for Lusush Line Editor (LLE)
 * 
 * This module defines the data structures and interfaces for managing command
 * history in the line editor. Provides efficient storage, navigation, and
 * retrieval of previously entered commands with configurable size limits.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#ifndef LLE_HISTORY_H
#define LLE_HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants and Configuration
// ============================================================================

/**
 * @brief Default maximum number of history entries
 */
#define LLE_HISTORY_DEFAULT_MAX_ENTRIES 1000

/**
 * @brief Minimum allowed history size
 */
#define LLE_HISTORY_MIN_ENTRIES 10

/**
 * @brief Maximum allowed history size
 */
#define LLE_HISTORY_MAX_ENTRIES 50000

/**
 * @brief Invalid history position marker
 */
#define LLE_HISTORY_INVALID_POSITION ((size_t)-1)

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Command history entry
 * 
 * Represents a single command in the history with timestamp and metadata.
 */
typedef struct {
    char *command;              /**< Command text (null-terminated) */
    size_t length;              /**< Command length in bytes */
    uint64_t timestamp;         /**< Entry timestamp (Unix time) */
    bool persistent;            /**< Should survive session restart */
} lle_history_entry_t;

/**
 * @brief Command history structure
 * 
 * Main structure for managing command history with efficient navigation
 * and configurable size limits. Uses circular buffer for memory efficiency.
 */
typedef struct {
    lle_history_entry_t *entries;  /**< History entries array */
    size_t count;                   /**< Number of current entries */
    size_t capacity;                /**< Array capacity */
    size_t current;                 /**< Current position for navigation */
    size_t max_entries;             /**< Maximum entries to keep */
    size_t oldest_index;            /**< Index of oldest entry (circular buffer) */
    bool is_full;                   /**< Whether buffer is at capacity */
    bool navigation_mode;           /**< Whether in navigation mode */
    char *temp_buffer;              /**< Temporary buffer for current edit */
    size_t temp_length;             /**< Length of temporary buffer */
} lle_history_t;

/**
 * @brief History navigation direction
 */
typedef enum {
    LLE_HISTORY_PREV = 0,       /**< Navigate to previous (older) entry */
    LLE_HISTORY_NEXT,           /**< Navigate to next (newer) entry */
    LLE_HISTORY_FIRST,          /**< Navigate to first (oldest) entry */
    LLE_HISTORY_LAST            /**< Navigate to last (newest) entry */
} lle_history_direction_t;

/**
 * @brief History search options
 */
typedef struct {
    const char *pattern;        /**< Search pattern */
    bool case_sensitive;        /**< Case sensitive search */
    bool prefix_match;          /**< Match only at beginning of command */
    bool regex_enabled;         /**< Enable regular expression matching */
    size_t max_results;         /**< Maximum results to return */
} lle_history_search_options_t;

/**
 * @brief History search result
 */
typedef struct {
    size_t index;               /**< Index in history */
    char *command;              /**< Matching command */
    size_t match_offset;        /**< Offset of match in command */
    size_t match_length;        /**< Length of match */
} lle_history_search_result_t;

/**
 * @brief History statistics
 */
typedef struct {
    size_t total_entries;       /**< Total entries added (including removed) */
    size_t current_entries;     /**< Current number of entries */
    size_t memory_usage;        /**< Current memory usage in bytes */
    size_t average_length;      /**< Average command length */
    size_t longest_command;     /**< Length of longest command */
    uint64_t oldest_timestamp;  /**< Timestamp of oldest entry */
    uint64_t newest_timestamp;  /**< Timestamp of newest entry */
} lle_history_stats_t;

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * @brief Create a new history structure
 * 
 * Allocates and initializes a new command history with the specified
 * maximum number of entries.
 * 
 * @param max_entries Maximum number of entries to keep (0 for default)
 * @return Pointer to new history structure, or NULL on failure
 */
lle_history_t *lle_history_create(size_t max_entries);

/**
 * @brief Initialize an existing history structure
 * 
 * Initializes a stack-allocated history structure with the specified
 * maximum number of entries.
 * 
 * @param history History structure to initialize
 * @param max_entries Maximum number of entries to keep (0 for default)
 * @return true on success, false on failure
 */
bool lle_history_init(lle_history_t *history, size_t max_entries);

/**
 * @brief Destroy a history structure
 * 
 * Frees all memory associated with the history structure, including
 * all stored commands.
 * 
 * @param history History structure to destroy
 */
void lle_history_destroy(lle_history_t *history);

/**
 * @brief Clean up a stack-allocated history structure
 * 
 * Cleans up a stack-allocated history structure without freeing the
 * structure itself. Use this for histories initialized with lle_history_init().
 * 
 * @param history History structure to clean up
 */
void lle_history_cleanup(lle_history_t *history);

/**
 * @brief Clear all history entries
 * 
 * Removes all entries from the history but keeps the structure intact
 * for future use.
 * 
 * @param history History structure to clear
 * @return true on success, false on failure
 */
bool lle_history_clear(lle_history_t *history);

/**
 * @brief Add a command to history
 * 
 * Adds a new command to the history. Duplicate consecutive commands
 * are typically ignored unless force_add is true.
 * 
 * @param history History structure
 * @param command Command text to add
 * @param force_add Add even if duplicate of last entry
 * @return true on success, false on failure
 */
bool lle_history_add(lle_history_t *history, const char *command, bool force_add);

/**
 * @brief Save history to file
 * 
 * Saves all history entries to the specified file. Each entry is written
 * as a separate line with timestamp metadata if available.
 * 
 * @param history History structure
 * @param filename Path to file where history should be saved
 * @return true on success, false on failure
 */
bool lle_history_save(const lle_history_t *history, const char *filename);

/**
 * @brief Load history from file
 * 
 * Loads history entries from the specified file. Existing history entries
 * are preserved unless clear_existing is true.
 * 
 * @param history History structure
 * @param filename Path to file containing history data
 * @param clear_existing Whether to clear existing entries before loading
 * @return true on success, false on failure
 */
bool lle_history_load(lle_history_t *history, const char *filename, bool clear_existing);

/**
 * @brief Get history entry at specified index
 * 
 * Returns a pointer to the history entry at the given index.
 * Index 0 is the oldest entry, count-1 is the newest.
 * 
 * @param history History structure
 * @param index Entry index
 * @return Pointer to entry, or NULL if index invalid
 */
const lle_history_entry_t *lle_history_get(const lle_history_t *history, size_t index);

/**
 * @brief Get current navigation entry
 * 
 * Returns the entry at the current navigation position.
 * 
 * @param history History structure
 * @return Pointer to current entry, or NULL if no current entry
 */
const lle_history_entry_t *lle_history_current(const lle_history_t *history);

/**
 * @brief Navigate to previous/next entry
 * 
 * Moves the current position in the specified direction and returns
 * the entry at the new position.
 * 
 * @param history History structure
 * @param direction Navigation direction
 * @return Pointer to entry at new position, or NULL if no entry
 */
const lle_history_entry_t *lle_history_navigate(lle_history_t *history, 
                                                lle_history_direction_t direction);

/**
 * @brief Reset navigation to end of history
 * 
 * Resets navigation position to after the newest entry, ending
 * navigation mode.
 * 
 * @param history History structure
 */
void lle_history_reset_navigation(lle_history_t *history);

/**
 * @brief Search history for pattern
 * 
 * Searches through history entries for commands matching the given
 * pattern and options.
 * 
 * @param history History structure
 * @param options Search options
 * @param results Array to store results (caller allocated)
 * @param max_results Maximum number of results to return
 * @return Number of results found
 */
size_t lle_history_search(const lle_history_t *history,
                          const lle_history_search_options_t *options,
                          lle_history_search_result_t *results,
                          size_t max_results);

/**
 * @brief Get history statistics
 * 
 * Returns statistical information about the history structure.
 * 
 * @param history History structure
 * @param stats Statistics structure to fill
 * @return true on success, false on failure
 */
bool lle_history_get_stats(const lle_history_t *history, lle_history_stats_t *stats);

/**
 * @brief Set temporary buffer for current edit
 * 
 * Sets a temporary buffer to preserve the current line being edited
 * when navigating through history.
 * 
 * @param history History structure
 * @param buffer Current edit buffer content
 * @param length Length of buffer content
 * @return true on success, false on failure
 */
bool lle_history_set_temp_buffer(lle_history_t *history, 
                                 const char *buffer, 
                                 size_t length);

/**
 * @brief Get temporary buffer content
 * 
 * Returns the content of the temporary buffer for restoring
 * the current edit when navigation ends.
 * 
 * @param history History structure
 * @param length Pointer to store buffer length (optional)
 * @return Pointer to temporary buffer, or NULL if none set
 */
const char *lle_history_get_temp_buffer(const lle_history_t *history, 
                                        size_t *length);

/**
 * @brief Check if history is empty
 * 
 * @param history History structure
 * @return true if history is empty, false otherwise
 */
bool lle_history_is_empty(const lle_history_t *history);

/**
 * @brief Check if history is full
 * 
 * @param history History structure
 * @return true if history has reached maximum capacity, false otherwise
 */
bool lle_history_is_full(const lle_history_t *history);

/**
 * @brief Get number of entries in history
 * 
 * @param history History structure
 * @return Number of entries in history
 */
size_t lle_history_size(const lle_history_t *history);

/**
 * @brief Get maximum number of entries
 * 
 * @param history History structure
 * @return Maximum number of entries configured
 */
size_t lle_history_max_size(const lle_history_t *history);

/**
 * @brief Set maximum number of entries
 * 
 * Changes the maximum number of entries. If the new limit is smaller
 * than the current number of entries, oldest entries are removed.
 * 
 * @param history History structure
 * @param max_entries New maximum number of entries
 * @return true on success, false on failure
 */
bool lle_history_set_max_size(lle_history_t *history, size_t max_entries);

#ifdef __cplusplus
}
#endif

#endif // LLE_HISTORY_H
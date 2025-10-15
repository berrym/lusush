// src/lle/foundation/history/history.h
//
// LLE Command History System
//
// This module implements persistent command history with search, deduplication,
// and timestamp tracking. Designed for efficient storage and fast lookup.

#ifndef LLE_FOUNDATION_HISTORY_H
#define LLE_FOUNDATION_HISTORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Maximum line length for history entries
#define LLE_HISTORY_MAX_LINE 8192

// Default history size (entries)
#define LLE_HISTORY_DEFAULT_SIZE 10000

// History entry
typedef struct {
    char *line;              // Command line (dynamically allocated)
    size_t line_len;         // Length of command line
    time_t timestamp;        // When command was added
    uint32_t index;          // Global index (monotonically increasing)
} lle_history_entry_t;

// History state
typedef struct {
    lle_history_entry_t *entries;  // Array of history entries
    size_t capacity;                // Maximum number of entries
    size_t count;                   // Current number of entries
    size_t head;                    // Index of oldest entry (circular buffer)
    uint32_t next_index;            // Next global index to assign
    
    // File persistence
    char *history_file;             // Path to history file
    bool auto_save;                 // Auto-save on add
    bool modified;                  // Unsaved changes
    
    // Search state
    size_t search_pos;              // Current search position
    char *search_pattern;           // Current search pattern
    bool search_active;             // Is search active
    
    // Configuration
    bool ignore_duplicates;         // Skip duplicate entries
    bool ignore_space;              // Skip lines starting with space
    size_t max_line_length;         // Maximum line length to store
} lle_history_t;

// Error codes
typedef enum {
    LLE_HISTORY_OK = 0,
    LLE_HISTORY_ERR_NULL_PTR = -1,
    LLE_HISTORY_ERR_INVALID_SIZE = -2,
    LLE_HISTORY_ERR_ALLOC_FAILED = -3,
    LLE_HISTORY_ERR_FILE_OPEN = -4,
    LLE_HISTORY_ERR_FILE_READ = -5,
    LLE_HISTORY_ERR_FILE_WRITE = -6,
    LLE_HISTORY_ERR_LINE_TOO_LONG = -7,
    LLE_HISTORY_ERR_NOT_INIT = -8,
    LLE_HISTORY_ERR_EMPTY = -9,
    LLE_HISTORY_ERR_NOT_FOUND = -10
} lle_history_error_t;

// Initialize history system
int lle_history_init(lle_history_t *history,
                     size_t capacity,
                     const char *history_file);

// Add entry to history
int lle_history_add(lle_history_t *history, const char *line);

// Get entry by index (0 = most recent)
const lle_history_entry_t* lle_history_get(const lle_history_t *history,
                                           size_t index);

// Get count of entries
size_t lle_history_count(const lle_history_t *history);

// Clear all entries
int lle_history_clear(lle_history_t *history);

// Load history from file
int lle_history_load(lle_history_t *history);

// Save history to file
int lle_history_save(const lle_history_t *history);

// Search operations
int lle_history_search_start(lle_history_t *history, const char *pattern);
const lle_history_entry_t* lle_history_search_next(lle_history_t *history);
const lle_history_entry_t* lle_history_search_prev(lle_history_t *history);
int lle_history_search_end(lle_history_t *history);

// Get most recent entry
const lle_history_entry_t* lle_history_get_recent(const lle_history_t *history);

// Get entry by global index
const lle_history_entry_t* lle_history_get_by_index(const lle_history_t *history,
                                                    uint32_t global_index);

// Check if line is duplicate of most recent
bool lle_history_is_duplicate(const lle_history_t *history, const char *line);

// Configuration
void lle_history_set_ignore_duplicates(lle_history_t *history, bool ignore);
void lle_history_set_ignore_space(lle_history_t *history, bool ignore);
void lle_history_set_auto_save(lle_history_t *history, bool auto_save);
void lle_history_set_max_line_length(lle_history_t *history, size_t max_len);

// Cleanup
void lle_history_cleanup(lle_history_t *history);

// Utility: Convert error code to string
const char* lle_history_error_string(int error_code);

#endif // LLE_FOUNDATION_HISTORY_H

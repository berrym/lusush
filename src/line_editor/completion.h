/*
 * Lusush Line Editor - Completion Framework
 * 
 * This file defines the completion system architecture for the Lusush Line Editor.
 * It provides a flexible framework for tab completion with support for multiple
 * completion providers, prioritized results, and extensible architecture.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#ifndef LLE_COMPLETION_H
#define LLE_COMPLETION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lle_completion_item lle_completion_item_t;
typedef struct lle_completion_list lle_completion_list_t;
typedef struct lle_completion_context lle_completion_context_t;

/**
 * Completion priority levels for sorting results
 */
typedef enum {
    LLE_COMPLETION_PRIORITY_LOW = 0,        // Generic completions
    LLE_COMPLETION_PRIORITY_NORMAL = 50,    // Standard completions
    LLE_COMPLETION_PRIORITY_HIGH = 100,     // Preferred completions
    LLE_COMPLETION_PRIORITY_EXACT = 200     // Exact matches
} lle_completion_priority_t;

/**
 * Individual completion item
 * 
 * Represents a single completion suggestion with text, optional description,
 * and priority for sorting.
 */
struct lle_completion_item {
    char *text;                    // Completion text (required)
    char *description;             // Optional description (can be NULL)
    int priority;                  // Completion priority for sorting
    
    // Internal fields
    size_t text_len;               // Length of completion text
    size_t desc_len;               // Length of description (0 if NULL)
    bool allocated;                // True if memory was allocated for this item
};

/**
 * Collection of completion items
 * 
 * Dynamic array of completion items with selection tracking.
 * Supports efficient insertion, sorting, and navigation.
 */
struct lle_completion_list {
    lle_completion_item_t *items;  // Array of completion items
    size_t count;                  // Number of items in list
    size_t capacity;               // Allocated capacity
    size_t selected;               // Currently selected item index
    
    // State tracking
    bool sorted;                   // True if list is sorted by priority
    bool owns_memory;              // True if list manages item memory
    
    // Statistics
    size_t total_text_len;         // Total length of all completion text
    size_t max_text_len;           // Length of longest completion text
    size_t max_desc_len;           // Length of longest description
};

/**
 * Completion context information
 * 
 * Provides context about the current completion request including
 * input state, cursor position, and completion preferences.
 */
struct lle_completion_context {
    const char *input;             // Full input line
    size_t input_len;              // Length of input line
    size_t cursor_pos;             // Current cursor position
    
    // Word boundaries
    size_t word_start;             // Start of word being completed
    size_t word_end;               // End of word being completed
    const char *word;              // Pointer to word being completed
    size_t word_len;               // Length of word being completed
    
    // Completion preferences
    bool case_sensitive;           // Case sensitive matching
    bool include_hidden;           // Include hidden files/entries
    size_t max_results;            // Maximum number of results
    
    // Context type hints
    bool at_command_start;         // True if completing command name
    bool in_quotes;                // True if inside quoted string
    char quote_char;               // Quote character (' or ")
};

/**
 * Completion provider function type
 * 
 * Function signature for completion providers. Providers analyze the
 * completion context and populate the completion list with relevant suggestions.
 * 
 * @param context Completion context information
 * @param completions List to populate with completion items
 * @return true on success, false on error
 */
typedef bool (*lle_completion_provider_t)(
    const lle_completion_context_t *context,
    lle_completion_list_t *completions
);

/**
 * Completion provider registration
 * 
 * Structure for registering completion providers with priorities
 * and conditions for when they should be invoked.
 */
typedef struct {
    const char *name;              // Provider name (for debugging)
    lle_completion_provider_t provider; // Provider function
    int priority;                  // Provider priority (higher = earlier)
    
    // Conditions
    bool (*should_run)(const lle_completion_context_t *context);
    
    // Configuration
    bool enabled;                  // True if provider is enabled
    size_t max_results;            // Maximum results from this provider
} lle_completion_provider_registration_t;

/* ========================================================================
 * Completion List Management
 * ======================================================================== */

/**
 * Create a new completion list
 * 
 * @param initial_capacity Initial capacity for completion items
 * @return New completion list or NULL on error
 */
lle_completion_list_t *lle_completion_list_create(size_t initial_capacity);

/**
 * Initialize an existing completion list
 * 
 * @param list List to initialize
 * @param initial_capacity Initial capacity for completion items
 * @return true on success, false on error
 */
bool lle_completion_list_init(lle_completion_list_t *list, size_t initial_capacity);

/**
 * Clear all items from completion list
 * 
 * @param list Completion list to clear
 */
void lle_completion_list_clear(lle_completion_list_t *list);

/**
 * Destroy completion list and free memory
 * 
 * @param list List to destroy (can be NULL)
 */
void lle_completion_list_destroy(lle_completion_list_t *list);

/**
 * Add completion item to list
 * 
 * @param list Completion list
 * @param text Completion text (copied)
 * @param description Optional description (copied, can be NULL)
 * @param priority Completion priority
 * @return true on success, false on error
 */
bool lle_completion_list_add(
    lle_completion_list_t *list,
    const char *text,
    const char *description,
    int priority
);

/**
 * Add completion item with explicit lengths
 * 
 * @param list Completion list
 * @param text Completion text
 * @param text_len Length of completion text
 * @param description Optional description (can be NULL)
 * @param desc_len Length of description (ignored if description is NULL)
 * @param priority Completion priority
 * @return true on success, false on error
 */
bool lle_completion_list_add_len(
    lle_completion_list_t *list,
    const char *text,
    size_t text_len,
    const char *description,
    size_t desc_len,
    int priority
);

/**
 * Sort completion list by priority and text
 * 
 * @param list Completion list to sort
 */
void lle_completion_list_sort(lle_completion_list_t *list);

/**
 * Get currently selected completion item
 * 
 * @param list Completion list
 * @return Selected item or NULL if list is empty
 */
const lle_completion_item_t *lle_completion_list_get_selected(const lle_completion_list_t *list);

/**
 * Set selected completion item by index
 * 
 * @param list Completion list
 * @param index Index to select
 * @return true if index is valid, false otherwise
 */
bool lle_completion_list_set_selected(lle_completion_list_t *list, size_t index);

/**
 * Move selection to next item (wrap around)
 * 
 * @param list Completion list
 * @return true if selection changed, false if list is empty
 */
bool lle_completion_list_select_next(lle_completion_list_t *list);

/**
 * Move selection to previous item (wrap around)
 * 
 * @param list Completion list
 * @return true if selection changed, false if list is empty
 */
bool lle_completion_list_select_prev(lle_completion_list_t *list);

/* ========================================================================
 * Completion Context Management
 * ======================================================================== */

/**
 * Create completion context from input and cursor position
 * 
 * @param input Input string
 * @param cursor_pos Cursor position in input
 * @return New completion context or NULL on error
 */
lle_completion_context_t *lle_completion_context_create(const char *input, size_t cursor_pos);

/**
 * Initialize completion context
 * 
 * @param context Context to initialize
 * @param input Input string
 * @param cursor_pos Cursor position in input
 * @return true on success, false on error
 */
bool lle_completion_context_init(lle_completion_context_t *context, const char *input, size_t cursor_pos);

/**
 * Destroy completion context
 * 
 * @param context Context to destroy (can be NULL)
 */
void lle_completion_context_destroy(lle_completion_context_t *context);

/* ========================================================================
 * Completion Item Utilities
 * ======================================================================== */

/**
 * Create completion item
 * 
 * @param text Completion text
 * @param description Optional description
 * @param priority Completion priority
 * @return New completion item or NULL on error
 */
lle_completion_item_t *lle_completion_item_create(
    const char *text,
    const char *description,
    int priority
);

/**
 * Initialize completion item
 * 
 * @param item Item to initialize
 * @param text Completion text
 * @param description Optional description
 * @param priority Completion priority
 * @return true on success, false on error
 */
bool lle_completion_item_init(
    lle_completion_item_t *item,
    const char *text,
    const char *description,
    int priority
);

/**
 * Clear completion item and free its memory
 * 
 * @param item Item to clear
 */
void lle_completion_item_clear(lle_completion_item_t *item);

/**
 * Destroy completion item
 * 
 * @param item Item to destroy (can be NULL)
 */
void lle_completion_item_destroy(lle_completion_item_t *item);

/* ========================================================================
 * Provider Management (Extended API)
 * ======================================================================== */

/**
 * Register completion provider
 * 
 * @param registration Provider registration info
 * @return true on success, false on error
 */
bool lle_completion_register_provider(const lle_completion_provider_registration_t *registration);

/**
 * Unregister completion provider by name
 * 
 * @param name Provider name
 * @return true if provider was found and removed, false otherwise
 */
bool lle_completion_unregister_provider(const char *name);

/**
 * Get list of registered provider names
 * 
 * @param names Array to fill with provider names (caller allocated)
 * @param max_names Maximum number of names to return
 * @return Number of provider names returned
 */
size_t lle_completion_get_provider_names(const char **names, size_t max_names);

/* ========================================================================
 * Utility Functions
 * ======================================================================== */

/**
 * Check if text starts with prefix (case sensitive or insensitive)
 * 
 * @param text Text to check
 * @param prefix Prefix to match
 * @param case_sensitive True for case sensitive matching
 * @return true if text starts with prefix
 */
bool lle_completion_text_matches(const char *text, const char *prefix, bool case_sensitive);

/**
 * Calculate common prefix of completion texts
 * 
 * @param list Completion list
 * @param common_prefix Buffer to store common prefix
 * @param max_len Maximum length of common prefix buffer
 * @return Length of common prefix
 */
size_t lle_completion_find_common_prefix(
    const lle_completion_list_t *list,
    char *common_prefix,
    size_t max_len
);

/**
 * Filter completion list by prefix
 * 
 * @param list Completion list to filter
 * @param prefix Prefix to match
 * @param case_sensitive True for case sensitive matching
 * @return Number of items remaining after filtering
 */
size_t lle_completion_filter_by_prefix(
    lle_completion_list_t *list,
    const char *prefix,
    bool case_sensitive
);

/* ========================================================================
 * File Completion Functions (LLE-030)
 * ======================================================================== */

/**
 * @brief Extract word being completed from input at cursor position
 *
 * Analyzes the input string at the given cursor position to extract the
 * word currently being completed. Uses shell-aware word boundary detection
 * that recognizes command separators, redirections, and other shell syntax.
 * Handles both absolute and relative paths correctly.
 *
 * @param input Input string to analyze
 * @param cursor_pos Current cursor position in input (clamped to input length)
 * @param word Buffer to store extracted word (null-terminated)
 * @param word_size Size of word buffer including space for null terminator
 * @param word_start Pointer to store start position of extracted word
 * @return true on success, false on error or invalid parameters
 *
 * @note Word boundaries include: space, tab, pipe, redirections, semicolon
 * @note Handles quoted strings and escaped characters appropriately
 * @note word_start will point to first character of the extracted word
 */
bool lle_completion_extract_word(
    const char *input,
    size_t cursor_pos,
    char *word,
    size_t word_size,
    size_t *word_start
);

/**
 * @brief Complete file names and directories for shell command line
 *
 * Provides comprehensive file and directory completion by analyzing the
 * current word being typed and searching the appropriate directory for
 * matching entries. Handles both relative and absolute paths, prioritizes
 * directories over files, and provides exact match detection.
 *
 * @param input Full input string containing the command line
 * @param cursor_pos Current cursor position in input string
 * @param completions Completion list to populate with file/directory matches
 * @return true on success (even if no matches found), false on error
 *
 * @note Automatically extracts the word being completed from input
 * @note Hidden files are only shown if prefix starts with '.'
 * @note Directories are marked with trailing '/' and given higher priority
 * @note Exact matches receive highest priority for selection
 * @note Completions are automatically sorted by priority and name
 * @note Returns true even when no completions found (not an error condition)
 */
bool lle_complete_files(
    const char *input,
    size_t cursor_pos,
    lle_completion_list_t *completions
);

#ifdef __cplusplus
}
#endif

#endif /* LLE_COMPLETION_H */
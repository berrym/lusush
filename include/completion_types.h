/*
 * Lusush - Completion Type Classification System
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * completion_types.h - Completion Type Classification System
 *
 * Provides type metadata for completions to enable:
 * - Visual categorization in interactive menus
 * - Type-specific indicators (symbols/colors)
 * - Intelligent ranking and grouping
 * - Enhanced display integration
 *
 * Part of pragmatic completion system (Spec 12 minimal + Spec 23 menu).
 */

#ifndef COMPLETION_TYPES_H
#define COMPLETION_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include "readline_integration.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// COMPLETION TYPE ENUMERATION
// ============================================================================

/**
 * Completion type classification for categorization and display
 */
typedef enum {
    COMPLETION_TYPE_BUILTIN,      // Shell built-in commands (cd, echo, etc.)
    COMPLETION_TYPE_COMMAND,      // External commands from PATH
    COMPLETION_TYPE_FILE,         // Regular files
    COMPLETION_TYPE_DIRECTORY,    // Directories
    COMPLETION_TYPE_VARIABLE,     // Shell/environment variables
    COMPLETION_TYPE_ALIAS,        // Command aliases
    COMPLETION_TYPE_HISTORY,      // History entries
    COMPLETION_TYPE_UNKNOWN       // Unclassified/fallback
} completion_type_t;

// ============================================================================
// COMPLETION ITEM WITH METADATA
// ============================================================================

/**
 * Completion item with type metadata for categorization and display
 */
typedef struct completion_item {
    char *text;                   // Completion text (owned by this struct)
    char *suffix;                 // Suffix to append (space, /, etc.)
    completion_type_t type;       // Completion type
    const char *type_indicator;   // Visual indicator (symbol/emoji)
    int relevance_score;          // Relevance ranking (0-1000)
    char *description;            // Optional description (may be NULL)
    
    // Internal memory management flags
    bool owns_text;               // Whether this struct owns text memory
    bool owns_description;        // Whether this struct owns description memory
} completion_item_t;

/**
 * Completion result with classified items
 */
typedef struct completion_result {
    completion_item_t *items;     // Array of completion items
    size_t count;                 // Number of items in array
    size_t capacity;              // Allocated capacity
    
    // Category statistics for display
    size_t builtin_count;         // Number of builtin completions
    size_t command_count;         // Number of command completions
    size_t file_count;            // Number of file completions
    size_t directory_count;       // Number of directory completions
    size_t variable_count;        // Number of variable completions
    size_t alias_count;           // Number of alias completions
    size_t history_count;         // Number of history completions
} completion_result_t;

// ============================================================================
// TYPE INFORMATION AND METADATA
// ============================================================================

/**
 * Type metadata for classification and display
 */
typedef struct {
    completion_type_t type;       // Completion type
    const char *type_name;        // Human-readable name ("Command", "File", etc.)
    const char *category_name;    // Category name for grouping ("COMMANDS", "FILES")
    const char *indicator;        // Visual indicator (symbol/emoji)
    int default_priority;         // Default priority for ranking (0-1000)
} completion_type_info_t;

/**
 * Get type information for a completion type
 * @param type completion type
 * @return type information structure (static, do not free)
 */
const completion_type_info_t* completion_type_get_info(completion_type_t type);

/**
 * Get category name for a completion type
 * @param type completion type
 * @return category name (e.g., "COMMANDS", "FILES", "VARIABLES")
 */
const char* completion_type_get_category(completion_type_t type);

/**
 * Get visual indicator for a completion type
 * @param type completion type
 * @return indicator string (symbol or emoji)
 */
const char* completion_type_get_indicator(completion_type_t type);

// ============================================================================
// COMPLETION ITEM MANAGEMENT
// ============================================================================

/**
 * Create a new completion item
 * @param text completion text (will be duplicated)
 * @param suffix suffix to append (may be NULL)
 * @param type completion type
 * @param relevance_score relevance score (0-1000)
 * @return newly allocated completion item (must be freed with completion_item_free)
 */
completion_item_t* completion_item_create(const char *text,
                                         const char *suffix,
                                         completion_type_t type,
                                         int relevance_score);

/**
 * Create completion item with description
 * @param text completion text (will be duplicated)
 * @param suffix suffix to append (may be NULL)
 * @param type completion type
 * @param relevance_score relevance score (0-1000)
 * @param description optional description (will be duplicated if not NULL)
 * @return newly allocated completion item (must be freed with completion_item_free)
 */
completion_item_t* completion_item_create_with_description(const char *text,
                                                          const char *suffix,
                                                          completion_type_t type,
                                                          int relevance_score,
                                                          const char *description);

/**
 * Free a completion item
 * @param item item to free
 */
void completion_item_free(completion_item_t *item);

// ============================================================================
// COMPLETION RESULT MANAGEMENT
// ============================================================================

/**
 * Create a new completion result structure
 * @param initial_capacity initial capacity for items array
 * @return newly allocated completion result (must be freed with completion_result_free)
 */
completion_result_t* completion_result_create(size_t initial_capacity);

/**
 * Add a completion item to the result
 * Takes ownership of the item
 * @param result completion result
 * @param item completion item to add
 * @return true on success, false on error
 */
bool completion_result_add_item(completion_result_t *result, 
                               completion_item_t *item);

/**
 * Add a simple completion to the result
 * Creates completion item internally
 * @param result completion result
 * @param text completion text
 * @param suffix suffix to append (may be NULL)
 * @param type completion type
 * @param relevance_score relevance score (0-1000)
 * @return true on success, false on error
 */
bool completion_result_add(completion_result_t *result,
                          const char *text,
                          const char *suffix,
                          completion_type_t type,
                          int relevance_score);

/**
 * Sort completion result by type and relevance
 * Groups items by category, then sorts by relevance within each category
 * @param result completion result to sort
 */
void completion_result_sort(completion_result_t *result);

/**
 * Free a completion result and all its items
 * @param result completion result to free
 */
void completion_result_free(completion_result_t *result);

// ============================================================================
// CONVERSION UTILITIES
// ============================================================================

/**
 * Convert typed completion result to readline compatibility structure
 * @param result typed completion result
 * @param lc readline compatibility structure to populate
 * @return number of completions added
 */
size_t completion_result_to_lusush_completions(completion_result_t *result,
                                              lusush_completions_t *lc);

/**
 * Create completion result from existing readline compatibility structure
 * Attempts to classify items by analyzing their text and context
 * @param lc readline compatibility structure
 * @return newly allocated completion result (must be freed)
 */
completion_result_t* completion_result_from_lusush_completions(
    lusush_completions_t *lc);

// ============================================================================
// CLASSIFICATION HELPERS
// ============================================================================

/**
 * Classify a completion text into a type
 * Uses heuristics based on text content and context
 * @param text completion text
 * @param is_command_position whether completing at command position
 * @return classified type
 */
completion_type_t completion_classify_text(const char *text, 
                                          bool is_command_position);

/**
 * Check if text is a builtin command
 * @param text text to check
 * @return true if builtin, false otherwise
 */
bool completion_is_builtin(const char *text);

/**
 * Check if text is an alias
 * @param text text to check
 * @return true if alias, false otherwise
 */
bool completion_is_alias(const char *text);

/**
 * Check if text refers to a directory
 * @param text text to check
 * @return true if directory, false otherwise
 */
bool completion_is_directory(const char *text);

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

/**
 * Print completion result statistics
 * @param result completion result
 */
void completion_result_print_stats(const completion_result_t *result);

/**
 * Get count of items for a specific type
 * @param result completion result
 * @param type completion type
 * @return count of items of that type
 */
size_t completion_result_count_by_type(const completion_result_t *result,
                                      completion_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* COMPLETION_TYPES_H */

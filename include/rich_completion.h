/*
 * Lusush - A modern shell with rich completion system
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Rich Completion System Header - Fish-inspired completions with descriptions
 * 
 * This module enhances the existing completion system with:
 * - Rich descriptions for completions (like Fish shell)
 * - Categorized completions (commands, files, variables, etc.)
 * - Multi-column display with descriptions
 * - Context-aware completion metadata
 * - Performance-optimized with intelligent caching
 * - Seamless integration with existing completion system
 */

#ifndef RICH_COMPLETION_H
#define RICH_COMPLETION_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPES AND STRUCTURES
// ============================================================================

/**
 * Categories of completions for better organization and display
 */
typedef enum {
    COMPLETION_COMMAND = 1,       // External commands from PATH
    COMPLETION_BUILTIN = 2,       // Shell builtin commands
    COMPLETION_ALIAS = 3,         // User-defined aliases
    COMPLETION_FUNCTION = 4,      // Shell functions
    COMPLETION_FILE = 5,          // Regular files
    COMPLETION_DIRECTORY = 6,     // Directories
    COMPLETION_VARIABLE = 7,      // Environment and shell variables
    COMPLETION_OPTION = 8,        // Command line options/flags
    COMPLETION_ARGUMENT = 9,      // Command arguments
    COMPLETION_HOSTNAME = 10,     // Network hostnames
    COMPLETION_USERNAME = 11,     // System usernames
    COMPLETION_SERVICE = 12,      // System services
    COMPLETION_PACKAGE = 13,      // Software packages
    COMPLETION_UNKNOWN = 99       // Unknown/uncategorized
} completion_category_t;

/**
 * Context information for determining what type of completion is needed
 */
typedef enum {
    CONTEXT_COMMAND,              // First word - looking for commands
    CONTEXT_ARGUMENT,             // Command arguments
    CONTEXT_FILE,                 // File/directory completion
    CONTEXT_VARIABLE,             // Variable completion ($VAR)
    CONTEXT_OPTION,               // Command option completion (--option)
    CONTEXT_MIXED                 // Mixed context - try multiple types
} completion_context_t;

/**
 * Rich completion item with metadata
 */
typedef struct {
    char *completion;             // The completion text itself
    char *description;            // Human-readable description
    char *detail;                // Additional detail information
    completion_category_t category; // Category of this completion
    const char *display_color;    // ANSI color for display
    int priority;                 // Priority for sorting (higher = first)
    bool exact_match;            // True if this is an exact match
    int fuzzy_score;             // Fuzzy matching score (0-100)
    time_t last_used;            // When this completion was last used
    int usage_count;             // How often this completion is used
} rich_completion_item_t;

/**
 * List of rich completion items
 */
#define MAX_RICH_COMPLETIONS 500
typedef struct {
    rich_completion_item_t *items[MAX_RICH_COMPLETIONS];
    size_t count;                 // Number of items
    size_t capacity;             // Maximum capacity
    completion_context_t context; // Context for these completions
    bool sorted;                 // Whether list is currently sorted
} rich_completion_list_t;

/**
 * Configuration for rich completion display and behavior
 */
typedef struct {
    bool enabled;                      // Master enable/disable
    bool show_descriptions;           // Show completion descriptions
    bool show_file_details;          // Show file sizes, types, etc.
    bool show_command_types;         // Show [builtin], [alias], etc.
    bool show_usage_stats;           // Show usage frequency
    bool group_by_category;          // Group completions by type
    int max_description_length;     // Maximum length of descriptions
    int max_completions_displayed;  // Maximum number to display at once
    int columns;                    // Number of columns for display
    bool colorized_output;          // Use colors in output
    
    // Color configuration
    const char *description_color;   // Color for descriptions
    const char *category_color;      // Color for category labels
    const char *file_color;         // Color for files
    const char *directory_color;    // Color for directories
    const char *command_color;      // Color for commands
    const char *builtin_color;      // Color for builtins
    const char *variable_color;     // Color for variables
    const char *option_color;       // Color for options
    const char *highlight_color;    // Color for matched portions
    const char *reset_color;        // Reset color sequence
} rich_completion_config_t;

/**
 * Statistics for rich completion system
 */
typedef struct {
    int completions_generated;       // Total completions generated
    int completions_selected;        // How many were selected by user
    int cache_hits;                 // Cache performance
    int cache_misses;
    double avg_generation_time_ms;  // Performance metrics
    int descriptions_loaded;        // How many descriptions were loaded
    int external_command_queries;   // Expensive operations count
} rich_completion_stats_t;

/**
 * Completion provider function type
 * Custom providers can be registered to extend completion capabilities
 */
typedef rich_completion_list_t* (*completion_provider_t)(const char *text, completion_context_t context);

// ============================================================================
// CORE API FUNCTIONS
// ============================================================================

/**
 * Initialize the rich completion system
 * Must be called before any other rich completion functions
 * 
 * @return true on success, false on failure
 */
bool lusush_rich_completion_init(void);

/**
 * Cleanup the rich completion system and free all resources
 */
void lusush_rich_completion_cleanup(void);

/**
 * Generate rich completions for given input and context
 * This is the main function for getting completions with metadata
 * 
 * @param text The text to complete
 * @param context The completion context
 * @return Rich completion list, or NULL on failure
 *         Caller must free with lusush_free_rich_completions()
 */
rich_completion_list_t* lusush_get_rich_completions(const char *text, completion_context_t context);

/**
 * Free a rich completion list and all its items
 * 
 * @param list The list to free
 */
void lusush_free_rich_completions(rich_completion_list_t *list);

/**
 * Get the best single completion from a rich completion list
 * Useful for auto-completion without showing a menu
 * 
 * @param list The completion list
 * @return Best completion item, or NULL if none suitable
 */
rich_completion_item_t* lusush_get_best_rich_completion(const rich_completion_list_t *list);

// ============================================================================
// DISPLAY AND FORMATTING
// ============================================================================

/**
 * Display rich completions in a formatted multi-column layout
 * Similar to Fish shell's completion display
 * 
 * @param list The completion list to display
 */
void lusush_display_rich_completions(const rich_completion_list_t *list);

/**
 * Display rich completions in a simple list format
 * Alternative display style for narrow terminals
 * 
 * @param list The completion list to display
 */
void lusush_display_rich_completions_simple(const rich_completion_list_t *list);

/**
 * Generate a formatted string representation of completions
 * Useful for integration with existing display systems
 * 
 * @param list The completion list
 * @param format_style Display format style
 * @return Formatted string (caller must free), or NULL on failure
 */
typedef enum {
    FORMAT_COLUMNS,     // Multi-column format
    FORMAT_LIST,        // Simple list format
    FORMAT_MENU,        // Menu format with numbers
    FORMAT_COMPACT      // Compact single-line format
} completion_format_style_t;

char* lusush_format_rich_completions(const rich_completion_list_t *list, completion_format_style_t format_style);

/**
 * Get terminal width for optimal display formatting
 * 
 * @return Terminal width in characters, or default if unavailable
 */
int lusush_get_terminal_width_for_completions(void);

// ============================================================================
// SORTING AND FILTERING
// ============================================================================

/**
 * Sort completion list by various criteria
 * 
 * @param list The list to sort
 * @param criteria Sorting criteria
 */
typedef enum {
    SORT_ALPHABETICAL,    // A-Z order
    SORT_RELEVANCE,       // By relevance/fuzzy score
    SORT_FREQUENCY,       // By usage frequency
    SORT_CATEGORY,        // By category first, then alphabetical
    SORT_MIXED           // Smart mixed sorting
} completion_sort_criteria_t;

void lusush_sort_rich_completions(rich_completion_list_t *list, completion_sort_criteria_t criteria);

/**
 * Filter completion list based on various criteria
 * 
 * @param list The list to filter (modified in place)
 * @param filter_func Function to determine if item should be kept
 * @param user_data User data passed to filter function
 */
typedef bool (*completion_filter_func_t)(const rich_completion_item_t *item, void *user_data);
void lusush_filter_rich_completions(rich_completion_list_t *list, completion_filter_func_t filter_func, void *user_data);

/**
 * Remove duplicate completions from list
 * 
 * @param list The list to deduplicate (modified in place)
 */
void lusush_deduplicate_rich_completions(rich_completion_list_t *list);

/**
 * Limit completion list to maximum number of items
 * Keeps the highest priority items
 * 
 * @param list The list to limit (modified in place)
 * @param max_items Maximum number of items to keep
 */
void lusush_limit_rich_completions(rich_completion_list_t *list, size_t max_items);

// ============================================================================
// CONFIGURATION AND MANAGEMENT
// ============================================================================

/**
 * Configure the rich completion system
 * 
 * @param config New configuration settings
 */
void lusush_configure_rich_completion(const rich_completion_config_t *config);

/**
 * Get current rich completion configuration
 * 
 * @return Pointer to current configuration (read-only)
 */
const rich_completion_config_t* lusush_get_rich_completion_config(void);

/**
 * Get default rich completion configuration
 * 
 * @return Default configuration structure
 */
rich_completion_config_t lusush_get_default_rich_completion_config(void);

/**
 * Enable or disable rich completions globally
 * 
 * @param enabled true to enable, false to disable
 */
void lusush_set_rich_completion_enabled(bool enabled);

/**
 * Check if rich completions are currently enabled
 * 
 * @return true if enabled, false otherwise
 */
bool lusush_are_rich_completions_enabled(void);

// ============================================================================
// CUSTOM COMPLETION PROVIDERS
// ============================================================================

/**
 * Register a custom completion provider
 * Allows plugins and extensions to contribute completions
 * 
 * @param provider The provider function
 * @param priority Priority level (higher = checked first)
 * @param name Human-readable name for the provider
 * @return true on success, false on failure
 */
bool lusush_register_completion_provider(completion_provider_t provider, int priority, const char *name);

/**
 * Unregister a custom completion provider
 * 
 * @param provider The provider function to remove
 * @return true on success, false if not found
 */
bool lusush_unregister_completion_provider(completion_provider_t provider);

/**
 * List all registered completion providers
 * 
 * @return Array of provider names (caller must free), or NULL
 */
char** lusush_list_completion_providers(void);

// ============================================================================
// CACHING AND PERFORMANCE
// ============================================================================

/**
 * Clear all cached completion data
 * Useful when system state has changed significantly
 */
void lusush_clear_completion_cache(void);

/**
 * Preload completions for better performance
 * Generates and caches completions for common patterns
 * 
 * @param patterns Array of completion patterns to preload
 * @param pattern_count Number of patterns
 */
void lusush_preload_rich_completions(const char **patterns, size_t pattern_count);

/**
 * Get completion cache statistics
 * 
 * @return Pointer to cache statistics (read-only)
 */
const rich_completion_stats_t* lusush_get_rich_completion_stats(void);

/**
 * Reset completion statistics
 */
void lusush_reset_rich_completion_stats(void);

// ============================================================================
// INTEGRATION WITH EXISTING SYSTEMS
// ============================================================================

/**
 * Convert basic lusush_completions_t to rich completion list
 * Allows integration with existing completion code
 * 
 * @param basic_completions The basic completion structure
 * @param context Completion context for categorization
 * @return Rich completion list, or NULL on failure
 */
rich_completion_list_t* lusush_convert_to_rich_completions(const void *basic_completions, completion_context_t context);

/**
 * Extract basic completion strings from rich completion list
 * For compatibility with systems that expect simple string arrays
 * 
 * @param rich_list The rich completion list
 * @param count Output parameter for number of completions
 * @return Array of completion strings (caller must free), or NULL
 */
char** lusush_extract_completion_strings(const rich_completion_list_t *rich_list, size_t *count);

/**
 * Update usage statistics when a completion is selected
 * Helps improve future completion ranking
 * 
 * @param completion The completion text that was selected
 * @param context The context where it was selected
 */
void lusush_record_completion_usage(const char *completion, completion_context_t context);

// ============================================================================
// DEBUGGING AND UTILITIES
// ============================================================================

/**
 * Print detailed information about a completion list
 * Useful for debugging completion issues
 * 
 * @param list The completion list to analyze
 */
void lusush_debug_rich_completions(const rich_completion_list_t *list);

/**
 * Validate a rich completion list for consistency
 * Checks for common issues and data integrity
 * 
 * @param list The list to validate
 * @return true if list is valid, false if issues found
 */
bool lusush_validate_rich_completion_list(const rich_completion_list_t *list);

/**
 * Get human-readable name for completion category
 * 
 * @param category The completion category
 * @return String name of the category (do not free)
 */
const char* lusush_get_completion_category_name(completion_category_t category);

/**
 * Get human-readable name for completion context
 * 
 * @param context The completion context
 * @return String name of the context (do not free)
 */
const char* lusush_get_completion_context_name(completion_context_t context);

/**
 * Enable or disable debug output for rich completions
 * 
 * @param enabled true to enable debug output
 */
void lusush_set_rich_completion_debug(bool enabled);

/**
 * Check if debug output is enabled
 * 
 * @return true if debug is enabled
 */
bool lusush_is_rich_completion_debug_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* RICH_COMPLETION_H */
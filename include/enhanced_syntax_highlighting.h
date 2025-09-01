/*
 * Lusush - A modern shell with enhanced syntax highlighting
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Enhanced Syntax Highlighting System Header - Fish-inspired intelligent highlighting
 * 
 * This module provides advanced syntax highlighting with:
 * - Real-time command validation and coloring
 * - Error highlighting for invalid commands/syntax  
 * - Context-aware highlighting (strings, variables, operators)
 * - Performance-optimized with incremental updates
 * - Fish-like intelligence for command existence checking
 * - Integration with existing readline system
 */

#ifndef ENHANCED_SYNTAX_HIGHLIGHTING_H
#define ENHANCED_SYNTAX_HIGHLIGHTING_H

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
 * Token types for syntax highlighting
 */
typedef enum {
    TOKEN_UNKNOWN = 0,           // Unknown token type
    TOKEN_COMMAND,               // Valid external command
    TOKEN_COMMAND_INVALID,       // Invalid/non-existent command
    TOKEN_BUILTIN,               // Shell builtin command
    TOKEN_KEYWORD,               // Shell keyword (if, while, etc.)
    TOKEN_STRING,                // Quoted string
    TOKEN_COMMAND_SUBSTITUTION,  // Command substitution (`cmd` or $(cmd))
    TOKEN_VARIABLE,              // Variable reference ($var)
    TOKEN_OPERATOR,              // Shell operator (|, &, ;, etc.)
    TOKEN_COMMENT,               // Comment (# ...)
    TOKEN_NUMBER,                // Numeric literal
    TOKEN_PATH_VALID,            // Valid file/directory path
    TOKEN_PATH_INVALID,          // Invalid file/directory path
    TOKEN_ARGUMENT,              // Generic argument
    TOKEN_OPTION,                // Command line option (-v, --verbose)
    TOKEN_REDIRECT,              // Redirection operator (>, <, >>)
    TOKEN_PIPE,                  // Pipe operator
    TOKEN_BACKGROUND,            // Background operator (&)
    TOKEN_ERROR                  // Syntax error
} token_type_t;

/**
 * Token structure for parsed command line
 */
typedef struct {
    token_type_t type;           // Type of this token
    size_t start;               // Start position in original line
    size_t end;                 // End position in original line
    bool is_error;              // Whether this token represents an error
    char *error_message;        // Error message if applicable
} highlight_token_t;

/**
 * Color configuration for syntax highlighting
 */
typedef struct {
    const char *command_valid;      // Color for valid commands
    const char *command_invalid;    // Color for invalid commands
    const char *command_builtin;    // Color for builtin commands
    const char *keyword;           // Color for shell keywords
    const char *string;            // Color for strings
    const char *variable;          // Color for variables
    const char *operator;          // Color for operators
    const char *comment;           // Color for comments
    const char *number;            // Color for numbers
    const char *path_valid;        // Color for valid paths
    const char *path_invalid;      // Color for invalid paths
    const char *quote;             // Color for quote characters
    const char *redirect;          // Color for redirection
    const char *pipe;              // Color for pipes
    const char *background;        // Color for background operator
    const char *error;             // Color for errors
    const char *reset;             // Reset color sequence
} highlight_colors_t;

/**
 * Configuration for enhanced syntax highlighting
 */
typedef struct {
    bool enabled;                    // Master enable/disable
    bool realtime_highlighting;      // Enable real-time highlighting during typing
    bool colorize_output;           // Enable colored output
    bool highlight_errors;          // Highlight syntax errors
    bool highlight_paths;           // Highlight and validate file paths
    bool check_command_existence;   // Check if commands actually exist
    bool smart_quotes;              // Smart quote matching and highlighting
    bool highlight_variables;      // Highlight variable references
    bool highlight_operators;      // Highlight shell operators
    bool show_error_messages;      // Show error messages for invalid syntax
    int cache_timeout_seconds;     // How long to cache command existence checks
    int max_line_length;           // Maximum line length to highlight
    double performance_threshold_ms; // Skip highlighting if it takes longer than this
} enhanced_highlight_config_t;

/**
 * Statistics structure for monitoring performance
 */
typedef struct {
    int lines_highlighted;          // Total lines highlighted
    int realtime_updates;          // Real-time highlighting updates
    int cache_hits;                // Command existence cache hits
    int cache_misses;              // Command existence cache misses
    int total_tokens_processed;    // Total tokens parsed and highlighted
    int command_validations;       // Command existence checks performed
    int path_validations;          // Path existence checks performed
    double avg_highlighting_time_ms; // Average time to highlight a line
    int syntax_errors_found;       // Syntax errors detected
    int performance_skips;         // Times highlighting was skipped for performance
} highlight_stats_t;

/**
 * Error information structure
 */
typedef struct {
    int position;                  // Character position of error
    int length;                    // Length of error token
    char *message;                 // Error message
    token_type_t expected_type;    // What type was expected
    token_type_t actual_type;      // What type was found
} syntax_error_t;

// ============================================================================
// CORE API FUNCTIONS
// ============================================================================

/**
 * Initialize the enhanced syntax highlighting system
 * Must be called before any other highlighting functions
 * 
 * @return true on success, false on failure
 */
bool lusush_enhanced_highlighting_init(void);

/**
 * Cleanup the enhanced syntax highlighting system and free all resources
 */
void lusush_enhanced_highlighting_cleanup(void);

/**
 * Generate highlighted version of command line
 * This is the main function for getting syntax-highlighted text
 * 
 * @param line The command line to highlight
 * @return Highlighted line with ANSI color codes (caller must free), or NULL on error
 */
char* lusush_generate_highlighted_line(const char *line);

/**
 * Real-time highlighting callback for readline integration
 * Called during interactive editing to update highlighting as user types
 */
void lusush_highlight_line_realtime(void);

/**
 * Parse command line into tokens for analysis
 * 
 * @param line The command line to parse
 * @param tokens Output array of tokens (caller must free)
 * @return Number of tokens parsed, or -1 on error
 */
int lusush_parse_command_line(const char *line, highlight_token_t **tokens);

/**
 * Free array of tokens
 * 
 * @param tokens Array of tokens to free
 * @param count Number of tokens in array
 */
void lusush_free_tokens(highlight_token_t *tokens, int count);

// ============================================================================
// CONFIGURATION AND MANAGEMENT
// ============================================================================

/**
 * Configure the enhanced syntax highlighting system
 * 
 * @param config New configuration settings
 */
void lusush_configure_enhanced_highlighting(const enhanced_highlight_config_t *config);

/**
 * Get current syntax highlighting configuration
 * 
 * @return Pointer to current configuration (read-only)
 */
const enhanced_highlight_config_t* lusush_get_enhanced_highlighting_config(void);

/**
 * Get default syntax highlighting configuration
 * 
 * @return Default configuration structure
 */
enhanced_highlight_config_t lusush_get_default_enhanced_highlighting_config(void);

/**
 * Set custom colors for syntax highlighting
 * 
 * @param colors Color configuration structure
 */
void lusush_set_highlighting_colors(const highlight_colors_t *colors);

/**
 * Get current highlighting colors
 * 
 * @return Pointer to current colors (read-only)
 */
const highlight_colors_t* lusush_get_highlighting_colors(void);

/**
 * Get default highlighting colors
 * 
 * @return Default color configuration
 */
highlight_colors_t lusush_get_default_highlighting_colors(void);

/**
 * Enable or disable syntax highlighting globally
 * 
 * @param enabled true to enable, false to disable
 */
void lusush_set_enhanced_highlighting_enabled(bool enabled);

/**
 * Check if syntax highlighting is currently enabled
 * 
 * @return true if enabled, false otherwise
 */
bool lusush_is_enhanced_highlighting_enabled(void);

/**
 * Enable or disable real-time highlighting
 * 
 * @param enabled true to enable real-time highlighting
 */
void lusush_set_realtime_highlighting_enabled(bool enabled);

/**
 * Check if real-time highlighting is enabled
 * 
 * @return true if enabled, false otherwise
 */
bool lusush_is_realtime_highlighting_enabled(void);

// ============================================================================
// VALIDATION AND ERROR DETECTION
// ============================================================================

/**
 * Validate command line for syntax errors
 * 
 * @param line The command line to validate
 * @param error_message Output parameter for error message (optional)
 * @return true if valid, false if syntax errors found
 */
bool lusush_validate_command_line(const char *line, char **error_message);

/**
 * Find all syntax errors in command line
 * 
 * @param line The command line to check
 * @param errors Output array of errors (caller must free)
 * @return Number of errors found, or -1 on failure
 */
int lusush_find_syntax_errors(const char *line, syntax_error_t **errors);

/**
 * Free array of syntax errors
 * 
 * @param errors Array of errors to free
 * @param count Number of errors in array
 */
void lusush_free_syntax_errors(syntax_error_t *errors, int count);

/**
 * Check if a command exists and is executable
 * Uses caching for performance
 * 
 * @param command The command name to check
 * @return true if command exists, false otherwise
 */
bool lusush_check_command_exists(const char *command);

/**
 * Check if a path exists and is accessible
 * 
 * @param path The file/directory path to check
 * @return true if path exists, false otherwise
 */
bool lusush_check_path_exists(const char *path);

// ============================================================================
// PERFORMANCE AND CACHING
// ============================================================================

/**
 * Clear all highlighting caches
 * Useful when system state has changed significantly
 */
void lusush_clear_highlighting_cache(void);

/**
 * Preload command existence cache for better performance
 * 
 * @param commands Array of command names to preload
 * @param count Number of commands
 */
void lusush_preload_command_cache(const char **commands, size_t count);

/**
 * Get syntax highlighting statistics
 * 
 * @return Pointer to current statistics (read-only)
 */
const highlight_stats_t* lusush_get_highlighting_stats(void);

/**
 * Reset syntax highlighting statistics
 */
void lusush_reset_highlighting_stats(void);

/**
 * Print detailed syntax highlighting statistics to stdout
 * Useful for debugging and performance analysis
 */
void lusush_print_highlighting_stats(void);

// ============================================================================
// THEME INTEGRATION
// ============================================================================

/**
 * Load highlighting colors from theme
 * Integrates with Lusush's existing theme system
 * 
 * @param theme_name Name of theme to load colors from
 * @return true on success, false if theme not found
 */
bool lusush_load_highlighting_theme(const char *theme_name);

/**
 * Save current highlighting colors as theme
 * 
 * @param theme_name Name to save theme as
 * @return true on success, false on failure
 */
bool lusush_save_highlighting_theme(const char *theme_name);

/**
 * List available highlighting themes
 * 
 * @return Array of theme names (caller must free), or NULL
 */
char** lusush_list_highlighting_themes(void);

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

/**
 * Register custom token type for highlighting
 * Allows plugins to add custom syntax highlighting
 * 
 * @param type_name Name of the token type
 * @param pattern Regex pattern to match
 * @param color Color to use for highlighting
 * @return Token type ID, or -1 on failure
 */
int lusush_register_custom_token_type(const char *type_name, const char *pattern, const char *color);

/**
 * Unregister custom token type
 * 
 * @param token_type The token type ID to remove
 * @return true on success, false if not found
 */
bool lusush_unregister_custom_token_type(int token_type);

/**
 * Set custom highlighting function
 * Allows complete customization of highlighting logic
 * 
 * @param highlighter Custom highlighting function
 * @return true on success, false on failure
 */
typedef char* (*custom_highlighter_t)(const char *line, const highlight_colors_t *colors);
bool lusush_set_custom_highlighter(custom_highlighter_t highlighter);

/**
 * Remove custom highlighting function and restore default
 * 
 * @return true on success, false otherwise
 */
bool lusush_remove_custom_highlighter(void);

// ============================================================================
// INTEGRATION HOOKS
// ============================================================================

/**
 * Hook called when line highlighting is complete
 * Allows monitoring and post-processing of highlighted lines
 */
typedef void (*line_highlighted_hook_t)(const char *original_line, const char *highlighted_line, const highlight_token_t *tokens, int token_count);

/**
 * Hook called when syntax error is detected
 * Allows custom error handling and reporting
 */
typedef void (*syntax_error_hook_t)(const char *line, const syntax_error_t *error);

/**
 * Set hook for line highlighting events
 * 
 * @param hook Function to call when line is highlighted
 */
void lusush_set_line_highlighted_hook(line_highlighted_hook_t hook);

/**
 * Set hook for syntax error events
 * 
 * @param hook Function to call when syntax error is found
 */
void lusush_set_syntax_error_hook(syntax_error_hook_t hook);

// ============================================================================
// DEBUGGING AND UTILITIES
// ============================================================================

/**
 * Enable or disable debug output for syntax highlighting
 * 
 * @param enabled true to enable debug output
 */
void lusush_set_highlighting_debug(bool enabled);

/**
 * Check if debug output is enabled
 * 
 * @return true if debug is enabled
 */
bool lusush_is_highlighting_debug_enabled(void);

/**
 * Get detailed debug information about last highlighting operation
 * 
 * @return Debug information string (caller must free), or NULL
 */
char* lusush_get_highlighting_debug_info(void);

/**
 * Analyze command line and print detailed token information
 * Useful for debugging highlighting issues
 * 
 * @param line The command line to analyze
 */
void lusush_analyze_command_line(const char *line);

/**
 * Test highlighting performance with sample text
 * 
 * @param test_lines Array of test lines
 * @param line_count Number of test lines
 * @param iterations Number of times to repeat test
 * @return Average highlighting time per line in milliseconds
 */
double lusush_benchmark_highlighting(const char **test_lines, size_t line_count, int iterations);

/**
 * Get human-readable name for token type
 * 
 * @param type The token type
 * @return String name of the token type (do not free)
 */
const char* lusush_get_token_type_name(token_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* ENHANCED_SYNTAX_HIGHLIGHTING_H */
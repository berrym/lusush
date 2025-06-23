#ifndef INPUT_MODERN_H
#define INPUT_MODERN_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Modern Input System for Lusush Shell
 * 
 * Provides clean, efficient input handling with multiline support,
 * history integration, and proper POSIX compliance.
 * Designed to work seamlessly with the modern tokenizer and parser.
 */

// Input context structure for managing input state
typedef struct input_context {
    // Core input state
    bool interactive;           // Whether we're in interactive mode
    bool multiline_enabled;     // Whether multiline input is enabled
    FILE *input_stream;         // Input stream (stdin or file)
    
    // Multiline state tracking
    int control_depth;          // Depth of control structures (if/for/while/case)
    int brace_depth;            // Depth of brace groups { }
    int paren_depth;            // Depth of parentheses groups ( )
    int bracket_depth;          // Depth of bracket groups [ ]
    bool in_single_quote;       // Inside single quotes
    bool in_double_quote;       // Inside double quotes
    bool in_here_doc;           // Inside here document
    char *here_doc_delimiter;   // Here document delimiter
    bool escaped;               // Last character was backslash
    bool in_function_def;       // Inside function definition
    bool in_arithmetic;         // Inside arithmetic expansion $(( ))
    bool in_command_subst;      // Inside command substitution $( )
    int command_subst_depth;    // Depth of nested command substitutions
    bool line_continuation;     // Explicit line continuation with backslash
    
    // Buffer management
    char *buffer;               // Current input buffer
    size_t buffer_size;         // Size of allocated buffer
    size_t buffer_used;         // Amount of buffer currently used
    char *prompt;               // Current prompt string
    char *continuation_prompt;  // Prompt for continuation lines
    
    // History integration
    bool history_enabled;       // Whether to add to history
    char *history_line;         // Line to add to history (converted format)
    
    // Error handling
    bool has_error;             // Whether an error occurred
    char *error_message;        // Error message if any
} input_context_t;

// Core input functions
input_context_t *input_context_new(FILE *input_stream, bool interactive);
void input_context_free(input_context_t *context);

// Main input function - returns complete command line ready for parsing
char *input_get_command_line(input_context_t *context);

// Multiline input support
bool input_is_complete_command(input_context_t *context, const char *line);
bool input_needs_continuation(input_context_t *context, const char *line);
void input_update_state(input_context_t *context, const char *line);
void input_reset_state(input_context_t *context);

// Prompt management
void input_set_prompt(input_context_t *context, const char *prompt);
void input_set_continuation_prompt(input_context_t *context, const char *prompt);
const char *input_get_current_prompt(input_context_t *context);

// History integration
void input_set_history_enabled(input_context_t *context, bool enabled);
char *input_prepare_for_history(input_context_t *context, const char *command);

// Buffer management
void input_clear_buffer(input_context_t *context);
bool input_append_to_buffer(input_context_t *context, const char *line);
const char *input_get_buffer_content(input_context_t *context);

// Utility functions
bool input_is_interactive(input_context_t *context);
bool input_has_error(input_context_t *context);
const char *input_get_error_message(input_context_t *context);
void input_clear_error(input_context_t *context);

// Character classification for multiline detection
bool input_is_control_keyword(const char *word);
bool input_is_opening_control(const char *word);
bool input_is_closing_control(const char *word);
bool input_is_continuation_word(const char *word);

// Line processing utilities
char *input_trim_whitespace(const char *line);
char *input_remove_comments(const char *line);
bool input_is_empty_or_comment(const char *line);

// Integration with linenoise for interactive mode
char *input_get_line_interactive(input_context_t *context);
char *input_get_line_non_interactive(input_context_t *context);

// Debug and testing utilities
void input_debug_state(input_context_t *context);
void input_set_debug_mode(input_context_t *context, bool debug);

#endif /* INPUT_MODERN_H */
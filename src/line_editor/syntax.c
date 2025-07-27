/*
 * Lusush Line Editor - Syntax Highlighting Framework Implementation
 * 
 * This file implements the syntax highlighting system for the Lusush Line Editor.
 * It provides real-time syntax highlighting with support for shell command syntax,
 * efficient region-based highlighting storage, and extensible architecture.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Resize syntax regions array if needed
 */
static bool lle_syntax_resize_regions(lle_syntax_highlight_t *highlight, size_t needed_capacity) {
    if (!highlight) return false;
    
    if (needed_capacity <= highlight->capacity) {
        return true; // Already have enough capacity
    }
    
    // Prevent excessive memory usage
    if (needed_capacity > LLE_SYNTAX_MAX_REGIONS) {
        return false;
    }
    
    // Calculate new capacity (double current or minimum needed)
    size_t new_capacity = highlight->capacity * 2;
    if (new_capacity < needed_capacity) {
        new_capacity = needed_capacity;
    }
    if (new_capacity > LLE_SYNTAX_MAX_REGIONS) {
        new_capacity = LLE_SYNTAX_MAX_REGIONS;
    }
    
    // Reallocate memory
    lle_syntax_region_t *new_regions = realloc(highlight->regions, 
                                               new_capacity * sizeof(lle_syntax_region_t));
    if (!new_regions) {
        return false;
    }
    
    highlight->regions = new_regions;
    highlight->capacity = new_capacity;
    return true;
}

/**
 * @brief Add a syntax region to the highlighting data
 */
static bool lle_syntax_add_region(lle_syntax_highlight_t *highlight,
                                  size_t start, size_t length,
                                  lle_syntax_type_t type) {
    if (!highlight || length == 0) return false;
    
    // Ensure we have capacity
    if (!lle_syntax_resize_regions(highlight, highlight->count + 1)) {
        return false;
    }
    
    // Add the region
    lle_syntax_region_t *region = &highlight->regions[highlight->count];
    region->start = start;
    region->length = length;
    region->type = type;
    region->char_start = start; // Will be computed properly for Unicode later
    region->char_length = length;
    
    highlight->count++;
    return true;
}

/**
 * @brief Check if character is a shell word separator
 */
static bool lle_syntax_is_word_separator(char c) {
    return isspace(c) || c == '|' || c == '&' || c == ';' || 
           c == '(' || c == ')' || c == '<' || c == '>' ||
           c == '"' || c == '\'' || c == '`' || c == '$' ||
           c == '#' || c == '\0';
}

/**
 * @brief Check if string is a shell keyword
 */
static bool lle_syntax_is_shell_keyword(const char *word, size_t length) {
    if (!word || length == 0) return false;
    
    // Common shell keywords
    static const char *keywords[] = {
        "if", "then", "else", "elif", "fi",
        "for", "while", "do", "done", "until",
        "case", "esac", "in", "function",
        "local", "export", "readonly", "unset",
        "return", "exit", "break", "continue",
        "true", "false", "test",
        NULL
    };
    
    for (int i = 0; keywords[i] != NULL; i++) {
        size_t keyword_len = strlen(keywords[i]);
        if (keyword_len == length && memcmp(word, keywords[i], length) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Check if string is a shell built-in command
 */
static bool lle_syntax_is_shell_builtin(const char *word, size_t length) {
    if (!word || length == 0) return false;
    
    // Common shell built-in commands
    static const char *builtins[] = {
        "cd", "pwd", "echo", "printf", "read", "test",
        "exec", "eval", "source", "alias", "unalias",
        "history", "fc", "jobs", "bg", "fg", "kill",
        "wait", "trap", "shift", "set", "unset",
        "type", "which", "command", "builtin",
        "enable", "disable", "help", "times",
        "ulimit", "umask", "getopts", "let",
        "declare", "typeset", "readonly", "export",
        "local", "logout", "exit", "return",
        NULL
    };
    
    for (int i = 0; builtins[i] != NULL; i++) {
        size_t builtin_len = strlen(builtins[i]);
        if (builtin_len == length && memcmp(word, builtins[i], length) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Check if character is a shell operator
 */
static bool lle_syntax_is_operator_char(char c) {
    return c == '|' || c == '&' || c == '<' || c == '>' ||
           c == ';' || c == '(' || c == ')' || c == '!' ||
           c == '=' || c == '+' || c == '-' || c == '*' ||
           c == '/' || c == '%' || c == '^' || c == '~';
}

/**
 * @brief Check if character is a number
 */
static bool lle_syntax_is_number_start(char c) {
    return isdigit(c);
}

/**
 * @brief Parse a number and return its end position
 */
static size_t lle_syntax_parse_number(const char *text, size_t start, size_t max_length) {
    if (!text || start >= max_length) return start;
    
    size_t pos = start;
    
    // Parse digits
    while (pos < max_length && isdigit(text[pos])) {
        pos++;
    }
    
    // Handle decimal point
    if (pos < max_length && text[pos] == '.') {
        pos++;
        while (pos < max_length && isdigit(text[pos])) {
            pos++;
        }
    }
    
    // Handle scientific notation
    if (pos < max_length && (text[pos] == 'e' || text[pos] == 'E')) {
        pos++;
        if (pos < max_length && (text[pos] == '+' || text[pos] == '-')) {
            pos++;
        }
        while (pos < max_length && isdigit(text[pos])) {
            pos++;
        }
    }
    
    return pos;
}

/**
 * @brief Parse a quoted string and return its end position
 */
static size_t lle_syntax_parse_string(const char *text, size_t start, size_t max_length, char quote_char) {
    if (!text || start >= max_length) return start;
    
    size_t pos = start + 1; // Skip opening quote
    
    while (pos < max_length && text[pos] != '\0') {
        if (text[pos] == quote_char) {
            return pos + 1; // Include closing quote
        }
        
        // Handle escape sequences
        if (text[pos] == '\\' && pos + 1 < max_length) {
            pos += 2; // Skip escaped character
        } else {
            pos++;
        }
    }
    
    return pos; // Unterminated string goes to end
}



/**
 * @brief Parse command substitution and return its end position
 */
static size_t lle_syntax_parse_command_substitution(const char *text, size_t start, size_t max_length) {
    if (!text || start >= max_length) return start;
    
    if (text[start] == '$' && start + 1 < max_length && text[start + 1] == '(') {
        // Handle $(...) syntax
        size_t pos = start + 2; // Skip $(
        int paren_count = 1;
        
        while (pos < max_length && paren_count > 0) {
            if (text[pos] == '(') {
                paren_count++;
            } else if (text[pos] == ')') {
                paren_count--;
            }
            pos++;
        }
        return pos;
    } else if (text[start] == '`') {
        // Handle `command` syntax
        size_t pos = start + 1; // Skip opening backtick
        while (pos < max_length && text[pos] != '`' && text[pos] != '\0') {
            if (text[pos] == '\\' && pos + 1 < max_length) {
                pos += 2; // Skip escaped character
            } else {
                pos++;
            }
        }
        if (pos < max_length && text[pos] == '`') {
            pos++; // Include closing backtick
        }
        return pos;
    }
    
    return start;
}

/**
 * @brief Parse parameter expansion and return its end position
 */
static size_t lle_syntax_parse_parameter_expansion(const char *text, size_t start, size_t max_length) {
    if (!text || start >= max_length || text[start] != '$') return start;
    
    size_t pos = start + 1; // Skip $
    
    if (pos >= max_length) return pos;
    
    // Handle ${...} syntax with parameter expansion
    if (text[pos] == '{') {
        pos++; // Skip {
        int brace_count = 1;
        
        while (pos < max_length && brace_count > 0) {
            if (text[pos] == '{') {
                brace_count++;
            } else if (text[pos] == '}') {
                brace_count--;
            }
            pos++;
        }
        return pos;
    }
    
    // Regular variable name
    while (pos < max_length && (isalnum(text[pos]) || text[pos] == '_')) {
        pos++;
    }
    
    // Handle special variables like $?, $!, $$, etc.
    if (pos == start + 1 && pos < max_length) {
        char special = text[pos];
        if (special == '?' || special == '!' || special == '$' || 
            special == '*' || special == '@' || special == '#' ||
            isdigit(special)) {
            pos++;
        }
    }
    
    return pos;
}

/**
 * @brief Parse redirection operators and return end position
 */
static size_t lle_syntax_parse_redirection(const char *text, size_t start, size_t max_length) {
    if (!text || start >= max_length) return start;
    
    size_t pos = start;
    char c = text[pos];
    
    if (c == '<') {
        pos++;
        if (pos < max_length && text[pos] == '<') {
            pos++; // <<
            if (pos < max_length && text[pos] == '<') {
                pos++; // <<<
            }
        } else if (pos < max_length && text[pos] == '&') {
            pos++; // <&
        }
    } else if (c == '>') {
        pos++;
        if (pos < max_length && text[pos] == '>') {
            pos++; // >>
        } else if (pos < max_length && text[pos] == '&') {
            pos++; // >&
        } else if (pos < max_length && text[pos] == '|') {
            pos++; // >|
        }
    } else if (c == '|') {
        pos++;
        if (pos < max_length && text[pos] == '&') {
            pos++; // |&
        }
    }
    
    return pos;
}

/**
 * @brief Highlight shell syntax in the given text
 */
static bool lle_syntax_highlight_shell_syntax(lle_syntax_highlight_t *highlight,
                                              const char *text, size_t length) {
    if (!highlight || !text) return false;
    
    size_t pos = 0;
    bool in_command_position = true; // First word is typically a command
    
    while (pos < length && text[pos] != '\0') {
        char c = text[pos];
        
        // Skip whitespace
        if (isspace(c)) {
            pos++;
            continue;
        }
        
        // Handle comments
        if (c == '#') {
            size_t comment_start = pos;
            while (pos < length && text[pos] != '\0' && text[pos] != '\n') {
                pos++;
            }
            if (!lle_syntax_add_region(highlight, comment_start, pos - comment_start, LLE_SYNTAX_COMMENT)) {
                return false;
            }
            continue;
        }
        
        // Handle quoted strings
        if (c == '"' || c == '\'' || c == '`') {
            size_t string_start = pos;
            pos = lle_syntax_parse_string(text, pos, length, c);
            lle_syntax_type_t string_type = (c == '`') ? LLE_SYNTAX_COMMAND : LLE_SYNTAX_STRING;
            if (!lle_syntax_add_region(highlight, string_start, pos - string_start, string_type)) {
                return false;
            }
            in_command_position = false;
            continue;
        }
        
        // Handle command substitution
        if (c == '$' && pos + 1 < length && text[pos + 1] == '(') {
            size_t cmd_start = pos;
            pos = lle_syntax_parse_command_substitution(text, pos, length);
            if (!lle_syntax_add_region(highlight, cmd_start, pos - cmd_start, LLE_SYNTAX_COMMAND)) {
                return false;
            }
            in_command_position = false;
            continue;
        }
        
        // Handle variables and parameter expansion
        if (c == '$') {
            size_t var_start = pos;
            pos = lle_syntax_parse_parameter_expansion(text, pos, length);
            if (!lle_syntax_add_region(highlight, var_start, pos - var_start, LLE_SYNTAX_VARIABLE)) {
                return false;
            }
            in_command_position = false;
            continue;
        }
        
        // Handle numbers
        if (lle_syntax_is_number_start(c)) {
            size_t num_start = pos;
            pos = lle_syntax_parse_number(text, pos, length);
            if (!lle_syntax_add_region(highlight, num_start, pos - num_start, LLE_SYNTAX_NUMBER)) {
                return false;
            }
            in_command_position = false;
            continue;
        }
        
        // Handle redirection operators
        if (c == '<' || c == '>' || (c == '|' && pos + 1 < length && text[pos + 1] == '&')) {
            size_t op_start = pos;
            pos = lle_syntax_parse_redirection(text, pos, length);
            if (!lle_syntax_add_region(highlight, op_start, pos - op_start, LLE_SYNTAX_OPERATOR)) {
                return false;
            }
            in_command_position = false; // After redirection, expect filename/path
            continue;
        }
        
        // Handle other operators
        if (lle_syntax_is_operator_char(c)) {
            size_t op_start = pos;
            while (pos < length && lle_syntax_is_operator_char(text[pos]) && 
                   text[pos] != '<' && text[pos] != '>') {
                pos++;
            }
            if (!lle_syntax_add_region(highlight, op_start, pos - op_start, LLE_SYNTAX_OPERATOR)) {
                return false;
            }
            in_command_position = (c == '|' || c == '&' || c == ';'); // Reset command position after these
            continue;
        }
        
        // Handle words (potential commands, keywords, or arguments)
        if (isalnum(c) || c == '_' || c == '.' || c == '/' || c == '-' || c == '~') {
            size_t word_start = pos;
            while (pos < length && !lle_syntax_is_word_separator(text[pos])) {
                pos++;
            }
            size_t word_length = pos - word_start;
            
            // Determine word type
            lle_syntax_type_t word_type = LLE_SYNTAX_NORMAL;
            if (lle_syntax_is_shell_keyword(text + word_start, word_length)) {
                word_type = LLE_SYNTAX_KEYWORD;
                in_command_position = true; // Keywords often start command contexts
            } else if (in_command_position) {
                // Check if it's a built-in command first
                if (lle_syntax_is_shell_builtin(text + word_start, word_length)) {
                    word_type = LLE_SYNTAX_KEYWORD; // Treat builtins as keywords for highlighting
                } else {
                    word_type = LLE_SYNTAX_COMMAND;
                }
                in_command_position = false;
            } else {
                // Check if it looks like a path
                bool has_slash = false;
                bool starts_with_tilde = (text[word_start] == '~');
                bool has_dot = false;
                for (size_t i = word_start; i < pos; i++) {
                    if (text[i] == '/') {
                        has_slash = true;
                    } else if (text[i] == '.') {
                        has_dot = true;
                    }
                }
                if (has_slash || starts_with_tilde || (has_dot && word_length > 2)) {
                    word_type = LLE_SYNTAX_PATH;
                }
            }
            
            if (word_type != LLE_SYNTAX_NORMAL) {
                if (!lle_syntax_add_region(highlight, word_start, word_length, word_type)) {
                    return false;
                }
            }
            continue;
        }
        
        // Skip other characters
        pos++;
    }
    
    return true;
}

// ============================================================================
// Core API Implementation
// ============================================================================

lle_syntax_highlighter_t *lle_syntax_create(void) {
    lle_syntax_highlighter_t *highlighter = malloc(sizeof(lle_syntax_highlighter_t));
    if (!highlighter) {
        return NULL;
    }
    
    if (!lle_syntax_init(highlighter, true, 0)) {
        free(highlighter);
        return NULL;
    }
    
    return highlighter;
}

bool lle_syntax_init(lle_syntax_highlighter_t *highlighter, 
                     bool enable_shell, 
                     size_t max_length) {
    if (!highlighter) return false;
    
    // Initialize the highlight structure
    highlighter->current = malloc(sizeof(lle_syntax_highlight_t));
    if (!highlighter->current) {
        return false;
    }
    
    // Initialize regions array
    highlighter->current->regions = malloc(LLE_SYNTAX_DEFAULT_CAPACITY * sizeof(lle_syntax_region_t));
    if (!highlighter->current->regions) {
        free(highlighter->current);
        highlighter->current = NULL;
        return false;
    }
    
    highlighter->current->count = 0;
    highlighter->current->capacity = LLE_SYNTAX_DEFAULT_CAPACITY;
    highlighter->current->text_length = 0;
    highlighter->current->is_dirty = false;
    
    // Set configuration
    highlighter->enable_shell_syntax = enable_shell;
    highlighter->enable_string_highlighting = true;
    highlighter->enable_comment_highlighting = true;
    highlighter->enable_variable_highlighting = true;
    highlighter->max_highlight_length = max_length;
    highlighter->incremental_update = true;
    highlighter->initialized = true;
    highlighter->last_cursor_pos = 0;
    
    return true;
}

void lle_syntax_destroy(lle_syntax_highlighter_t *highlighter) {
    if (!highlighter) return;
    
    lle_syntax_cleanup(highlighter);
    free(highlighter);
}

void lle_syntax_cleanup(lle_syntax_highlighter_t *highlighter) {
    if (!highlighter) return;
    
    if (highlighter->current) {
        free(highlighter->current->regions);
        free(highlighter->current);
        highlighter->current = NULL;
    }
    
    highlighter->initialized = false;
}

bool lle_syntax_highlight_text(lle_syntax_highlighter_t *highlighter,
                               const char *text,
                               size_t length) {
    if (!highlighter || !text || !highlighter->initialized) return false;
    
    // Check length limits
    if (highlighter->max_highlight_length > 0 && length > highlighter->max_highlight_length) {
        length = highlighter->max_highlight_length;
    }
    
    // Clear existing regions
    highlighter->current->count = 0;
    highlighter->current->text_length = length;
    highlighter->current->is_dirty = false;
    
    // Apply syntax highlighting based on configuration
    if (highlighter->enable_shell_syntax) {
        if (!lle_syntax_highlight_shell_syntax(highlighter->current, text, length)) {
            return false;
        }
    }
    
    return true;
}

bool lle_syntax_update_region(lle_syntax_highlighter_t *highlighter,
                              const char *text,
                              size_t text_length,
                              size_t change_start,
                              size_t change_length) {
    if (!highlighter || !text || !highlighter->initialized) return false;
    
    // Suppress unused parameter warnings
    (void)change_start;
    (void)change_length;
    
    // For now, perform full re-highlighting
    // TODO: Implement incremental updates for better performance
    return lle_syntax_highlight_text(highlighter, text, text_length);
}

lle_syntax_type_t lle_syntax_get_type_at_position(const lle_syntax_highlighter_t *highlighter,
                                                  size_t position) {
    if (!highlighter || !highlighter->initialized || !highlighter->current) {
        return LLE_SYNTAX_NORMAL;
    }
    
    // Search through regions to find the one containing this position
    for (size_t i = 0; i < highlighter->current->count; i++) {
        const lle_syntax_region_t *region = &highlighter->current->regions[i];
        if (position >= region->start && position < region->start + region->length) {
            return region->type;
        }
    }
    
    return LLE_SYNTAX_NORMAL;
}

const lle_syntax_region_t *lle_syntax_get_regions(const lle_syntax_highlighter_t *highlighter,
                                                  size_t *region_count) {
    if (!highlighter || !highlighter->initialized || !highlighter->current) {
        if (region_count) *region_count = 0;
        return NULL;
    }
    
    if (region_count) {
        *region_count = highlighter->current->count;
    }
    
    return highlighter->current->regions;
}

// ============================================================================
// Configuration Functions
// ============================================================================

bool lle_syntax_configure_shell(lle_syntax_highlighter_t *highlighter, bool enable) {
    if (!highlighter || !highlighter->initialized) return false;
    
    highlighter->enable_shell_syntax = enable;
    highlighter->current->is_dirty = true;
    return true;
}

bool lle_syntax_configure_strings(lle_syntax_highlighter_t *highlighter, bool enable) {
    if (!highlighter || !highlighter->initialized) return false;
    
    highlighter->enable_string_highlighting = enable;
    highlighter->current->is_dirty = true;
    return true;
}

bool lle_syntax_configure_variables(lle_syntax_highlighter_t *highlighter, bool enable) {
    if (!highlighter || !highlighter->initialized) return false;
    
    highlighter->enable_variable_highlighting = enable;
    highlighter->current->is_dirty = true;
    return true;
}

bool lle_syntax_configure_comments(lle_syntax_highlighter_t *highlighter, bool enable) {
    if (!highlighter || !highlighter->initialized) return false;
    
    highlighter->enable_comment_highlighting = enable;
    highlighter->current->is_dirty = true;
    return true;
}

bool lle_syntax_set_max_length(lle_syntax_highlighter_t *highlighter, size_t max_length) {
    if (!highlighter || !highlighter->initialized) return false;
    
    highlighter->max_highlight_length = max_length;
    return true;
}

// ============================================================================
// Utility Functions
// ============================================================================

bool lle_syntax_is_enabled(const lle_syntax_highlighter_t *highlighter) {
    if (!highlighter || !highlighter->initialized) return false;
    
    return highlighter->enable_shell_syntax || 
           highlighter->enable_string_highlighting ||
           highlighter->enable_comment_highlighting ||
           highlighter->enable_variable_highlighting;
}

bool lle_syntax_is_dirty(const lle_syntax_highlighter_t *highlighter) {
    if (!highlighter || !highlighter->initialized || !highlighter->current) {
        return false;
    }
    
    return highlighter->current->is_dirty;
}

bool lle_syntax_clear_regions(lle_syntax_highlighter_t *highlighter) {
    if (!highlighter || !highlighter->initialized || !highlighter->current) {
        return false;
    }
    
    highlighter->current->count = 0;
    highlighter->current->text_length = 0;
    highlighter->current->is_dirty = false;
    return true;
}

const char *lle_syntax_type_name(lle_syntax_type_t type) {
    switch (type) {
        case LLE_SYNTAX_NORMAL:     return "normal";
        case LLE_SYNTAX_KEYWORD:    return "keyword";
        case LLE_SYNTAX_COMMAND:    return "command";
        case LLE_SYNTAX_STRING:     return "string";
        case LLE_SYNTAX_COMMENT:    return "comment";
        case LLE_SYNTAX_NUMBER:     return "number";
        case LLE_SYNTAX_OPERATOR:   return "operator";
        case LLE_SYNTAX_VARIABLE:   return "variable";
        case LLE_SYNTAX_PATH:       return "path";
        case LLE_SYNTAX_ERROR:      return "error";
        default:                    return "unknown";
    }
}
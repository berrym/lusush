/*
 * Lusush - A modern shell with enhanced syntax highlighting
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Enhanced Syntax Highlighting System - Fish-inspired intelligent highlighting
 * 
 * This module provides advanced syntax highlighting with:
 * - Real-time command validation and coloring
 * - Error highlighting for invalid commands/syntax  
 * - Context-aware highlighting (strings, variables, operators)
 * - Performance-optimized with incremental updates
 * - Fish-like intelligence for command existence checking
 * - Integration with existing readline system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "../include/enhanced_syntax_highlighting.h"
#include "../include/readline_integration.h"
#include "../include/builtins.h"
#include "../include/completion.h"
#include "../include/config.h"
#include "../include/symtable.h"
#include "../include/alias.h"
#include "../include/strings.h"

// ============================================================================
// GLOBAL STATE AND CONFIGURATION
// ============================================================================

static bool initialized = false;
static enhanced_highlight_config_t config;
static char *last_highlighted_line = NULL;
static highlight_token_t *last_tokens = NULL;
static int last_token_count = 0;
static time_t last_highlight_time = 0;

// Color definitions - can be customized via configuration
static highlight_colors_t default_colors = {
    .command_valid = "\033[1;32m",      // Bright green
    .command_invalid = "\033[1;31m",    // Bright red  
    .command_builtin = "\033[1;36m",    // Bright cyan
    .keyword = "\033[1;34m",            // Bright blue
    .string = "\033[1;33m",             // Bright yellow
    .variable = "\033[1;35m",           // Bright magenta
    .operator = "\033[1;31m",           // Bright red
    .comment = "\033[1;30m",            // Gray
    .number = "\033[1;36m",             // Bright cyan
    .path_valid = "\033[4;32m",         // Underlined green
    .path_invalid = "\033[4;31m",       // Underlined red
    .quote = "\033[1;33m",              // Bright yellow
    .redirect = "\033[1;35m",           // Bright magenta
    .pipe = "\033[1;34m",               // Bright blue
    .background = "\033[1;37m",         // Bright white
    .error = "\033[41;1;37m",           // Red background, white text
    .reset = "\033[0m"                  // Reset all formatting
};

static highlight_colors_t current_colors;

// Performance optimization structures
typedef struct {
    char *command;
    bool exists;
    time_t cache_time;
    bool is_valid;
} command_existence_cache_entry_t;

#define COMMAND_CACHE_SIZE 256
static command_existence_cache_entry_t cmd_cache[COMMAND_CACHE_SIZE];
static int cmd_cache_initialized = 0;

// Statistics for performance monitoring
static highlight_stats_t stats = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Hash function for command cache
 */
static unsigned int hash_command(const char *command) {
    unsigned int hash = 5381;
    int c;
    
    while ((c = *command++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash % COMMAND_CACHE_SIZE;
}

/**
 * Check if a command exists (with caching)
 */
static bool check_command_exists(const char *command) {
    if (!command || !*command) {
        return false;
    }
    
    if (!cmd_cache_initialized) {
        memset(cmd_cache, 0, sizeof(cmd_cache));
        cmd_cache_initialized = 1;
    }
    
    unsigned int index = hash_command(command);
    command_existence_cache_entry_t *entry = &cmd_cache[index];
    
    // Check if cache entry is valid and matches
    if (entry->is_valid && entry->command && 
        strcmp(entry->command, command) == 0) {
        // Check if cache entry is still fresh (30 seconds)
        if (time(NULL) - entry->cache_time < 30) {
            return entry->exists;
        }
    }
    
    // Cache miss or expired - check command existence
    bool exists = false;
    
    // Check if it's a builtin command
    if (is_builtin_command(command)) {
        exists = true;
    }
    // Check if it's an alias
    else if (is_alias(command)) {
        exists = true;
    }
    // Check if it's executable in PATH
    else {
        char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            char *path_dir = strtok(path_copy, ":");
            
            while (path_dir && !exists) {
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", path_dir, command);
                
                struct stat st;
                if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode) && 
                    (st.st_mode & S_IXUSR)) {
                    exists = true;
                }
                
                path_dir = strtok(NULL, ":");
            }
            
            free(path_copy);
        }
    }
    
    // Update cache
    free(entry->command);
    entry->command = strdup(command);
    entry->exists = exists;
    entry->cache_time = time(NULL);
    entry->is_valid = true;
    
    return exists;
}

/**
 * Check if a character is a shell operator
 */
static bool is_shell_operator(char c) {
    return c == '|' || c == '&' || c == ';' || c == '>' || c == '<' || 
           c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']';
}

/**
 * Check if a word is a shell keyword
 */
static bool is_shell_keyword(const char *word) {
    static const char *keywords[] = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "while", 
        "until", "do", "done", "function", "select", "time", "in", "break", 
        "continue", "return", "exit", "export", "readonly", "local", "declare",
        "typeset", "unset", "shift", "eval", "exec", "trap", "test", "[", "[[",
        NULL
    };
    
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * Check if a path exists and is accessible
 */
static bool check_path_exists(const char *path) {
    if (!path || !*path) {
        return false;
    }
    
    struct stat st;
    return stat(path, &st) == 0;
}

/**
 * Get current time in milliseconds for performance measurement
 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// ============================================================================
// TOKENIZER
// ============================================================================

/**
 * Tokenize the command line for syntax highlighting
 */
static int tokenize_line(const char *line, highlight_token_t **tokens) {
    if (!line || !tokens) {
        return 0;
    }
    
    size_t line_len = strlen(line);
    if (line_len == 0) {
        return 0;
    }
    
    // Allocate token array (estimate maximum needed)
    int max_tokens = line_len / 2 + 10;
    *tokens = calloc(max_tokens, sizeof(highlight_token_t));
    if (!*tokens) {
        return 0;
    }
    
    int token_count = 0;
    size_t pos = 0;
    
    while (pos < line_len && token_count < max_tokens - 1) {
        // Skip whitespace
        while (pos < line_len && isspace(line[pos])) {
            pos++;
        }
        
        if (pos >= line_len) {
            break;
        }
        
        highlight_token_t *token = &(*tokens)[token_count];
        token->start = pos;
        
        char current = line[pos];
        
        // Handle comments
        if (current == '#') {
            token->type = TOKEN_COMMENT;
            token->start = pos;
            // Comment extends to end of line
            while (pos < line_len) {
                pos++;
            }
            token->end = pos;
            token_count++;
            break;
        }
        
        // Handle strings
        if (current == '"' || current == '\'' || current == '`') {
            char quote_char = current;
            token->type = (current == '`') ? TOKEN_COMMAND_SUBSTITUTION : TOKEN_STRING;
            pos++; // Skip opening quote
            
            // Find closing quote
            while (pos < line_len && line[pos] != quote_char) {
                if (line[pos] == '\\' && pos + 1 < line_len) {
                    pos += 2; // Skip escaped character
                } else {
                    pos++;
                }
            }
            
            if (pos < line_len) {
                pos++; // Skip closing quote
            }
            
            token->end = pos;
            token_count++;
            continue;
        }
        
        // Handle variables
        if (current == '$') {
            token->type = TOKEN_VARIABLE;
            pos++; // Skip $
            
            // Handle ${var} syntax
            if (pos < line_len && line[pos] == '{') {
                pos++; // Skip {
                while (pos < line_len && line[pos] != '}') {
                    pos++;
                }
                if (pos < line_len) {
                    pos++; // Skip }
                }
            } else {
                // Handle $var syntax
                while (pos < line_len && (isalnum(line[pos]) || line[pos] == '_')) {
                    pos++;
                }
            }
            
            token->end = pos;
            token_count++;
            continue;
        }
        
        // Handle operators
        if (is_shell_operator(current)) {
            token->type = TOKEN_OPERATOR;
            
            // Handle multi-character operators
            if (current == '>' && pos + 1 < line_len && line[pos + 1] == '>') {
                pos += 2; // >>
            } else if (current == '<' && pos + 1 < line_len && line[pos + 1] == '<') {
                pos += 2; // <<
            } else if (current == '&' && pos + 1 < line_len && line[pos + 1] == '&') {
                pos += 2; // &&
            } else if (current == '|' && pos + 1 < line_len && line[pos + 1] == '|') {
                pos += 2; // ||
            } else {
                pos++; // Single character operator
            }
            
            token->end = pos;
            token_count++;
            continue;
        }
        
        // Handle numbers
        if (isdigit(current)) {
            token->type = TOKEN_NUMBER;
            while (pos < line_len && (isdigit(line[pos]) || line[pos] == '.')) {
                pos++;
            }
            token->end = pos;
            token_count++;
            continue;
        }
        
        // Handle words (commands, arguments, paths)
        if (isalpha(current) || current == '/' || current == '.' || current == '~') {
            size_t word_start = pos;
            
            // Collect the word
            while (pos < line_len && !isspace(line[pos]) && 
                   !is_shell_operator(line[pos]) && line[pos] != '"' && 
                   line[pos] != '\'' && line[pos] != '`' && line[pos] != '$') {
                pos++;
            }
            
            // Extract word for analysis
            size_t word_len = pos - word_start;
            char word[word_len + 1];
            strncpy(word, line + word_start, word_len);
            word[word_len] = '\0';
            
            // Determine token type
            bool is_first_word = (token_count == 0);
            for (int i = 0; i < token_count; i++) {
                if ((*tokens)[i].type != TOKEN_COMMENT && 
                    ((*tokens)[i].type == TOKEN_OPERATOR && 
                     (line[(*tokens)[i].start] == '|' || line[(*tokens)[i].start] == '&' || 
                      line[(*tokens)[i].start] == ';'))) {
                    is_first_word = true;
                    break;
                }
            }
            
            if (is_first_word || 
                (token_count > 0 && (*tokens)[token_count-1].type == TOKEN_OPERATOR)) {
                // This is a command position
                if (is_shell_keyword(word)) {
                    token->type = TOKEN_KEYWORD;
                } else if (check_command_exists(word)) {
                    if (is_builtin_command(word)) {
                        token->type = TOKEN_BUILTIN;
                    } else {
                        token->type = TOKEN_COMMAND;
                    }
                } else {
                    token->type = TOKEN_COMMAND_INVALID;
                }
            } else {
                // This is an argument position
                if (word[0] == '/' || word[0] == '~' || word[0] == '.') {
                    // Looks like a path
                    if (check_path_exists(word)) {
                        token->type = TOKEN_PATH_VALID;
                    } else {
                        token->type = TOKEN_PATH_INVALID;
                    }
                } else {
                    token->type = TOKEN_ARGUMENT;
                }
            }
            
            token->end = pos;
            token_count++;
            continue;
        }
        
        // Handle everything else as argument
        token->type = TOKEN_ARGUMENT;
        while (pos < line_len && !isspace(line[pos])) {
            pos++;
        }
        token->end = pos;
        token_count++;
    }
    
    return token_count;
}

// ============================================================================
// HIGHLIGHTING ENGINE
// ============================================================================

/**
 * Apply color to a token based on its type
 */
static const char* get_token_color(token_type_t type) {
    switch (type) {
        case TOKEN_COMMAND:
            return current_colors.command_valid;
        case TOKEN_COMMAND_INVALID:
            return current_colors.command_invalid;
        case TOKEN_BUILTIN:
            return current_colors.command_builtin;
        case TOKEN_KEYWORD:
            return current_colors.keyword;
        case TOKEN_STRING:
        case TOKEN_COMMAND_SUBSTITUTION:
            return current_colors.string;
        case TOKEN_VARIABLE:
            return current_colors.variable;
        case TOKEN_OPERATOR:
            return current_colors.operator;
        case TOKEN_COMMENT:
            return current_colors.comment;
        case TOKEN_NUMBER:
            return current_colors.number;
        case TOKEN_PATH_VALID:
            return current_colors.path_valid;
        case TOKEN_PATH_INVALID:
            return current_colors.path_invalid;
        case TOKEN_ARGUMENT:
        default:
            return NULL; // No special color
    }
}

/**
 * Generate highlighted version of command line
 */
char* lusush_generate_highlighted_line(const char *line) {
    if (!initialized || !config.enabled || !line) {
        return strdup(line ? line : "");
    }
    
    double start_time = get_time_ms();
    
    // Check if we can use cached result
    if (last_highlighted_line && strcmp(line, last_highlighted_line) == 0 &&
        time(NULL) - last_highlight_time < config.cache_timeout_seconds) {
        stats.cache_hits++;
        return strdup(line); // Return original for now, could cache highlighted version
    }
    
    stats.cache_misses++;
    
    // Tokenize the line
    highlight_token_t *tokens = NULL;
    int token_count = tokenize_line(line, &tokens);
    
    if (token_count == 0) {
        free(tokens);
        return strdup(line);
    }
    
    // Calculate required buffer size (original + color codes)
    size_t line_len = strlen(line);
    size_t buffer_size = line_len * 3 + token_count * 20 + 100; // Generous estimate
    char *highlighted = malloc(buffer_size);
    if (!highlighted) {
        free(tokens);
        return strdup(line);
    }
    
    size_t output_pos = 0;
    size_t input_pos = 0;
    
    // Apply highlighting token by token
    for (int i = 0; i < token_count; i++) {
        highlight_token_t *token = &tokens[i];
        
        // Copy any text before this token (whitespace, etc.)
        while (input_pos < token->start && input_pos < line_len) {
            highlighted[output_pos++] = line[input_pos++];
        }
        
        // Apply color for this token
        const char *color = get_token_color(token->type);
        if (color && config.colorize_output) {
            size_t color_len = strlen(color);
            if (output_pos + color_len < buffer_size) {
                strcpy(highlighted + output_pos, color);
                output_pos += color_len;
            }
        }
        
        // Copy the token text
        while (input_pos < token->end && input_pos < line_len && output_pos < buffer_size - 1) {
            highlighted[output_pos++] = line[input_pos++];
        }
        
        // Reset color if we applied one
        if (color && config.colorize_output) {
            size_t reset_len = strlen(current_colors.reset);
            if (output_pos + reset_len < buffer_size) {
                strcpy(highlighted + output_pos, current_colors.reset);
                output_pos += reset_len;
            }
        }
    }
    
    // Copy any remaining text
    while (input_pos < line_len && output_pos < buffer_size - 1) {
        highlighted[output_pos++] = line[input_pos++];
    }
    
    highlighted[output_pos] = '\0';
    
    // Update cache
    free(last_highlighted_line);
    last_highlighted_line = strdup(line);
    
    free(last_tokens);
    last_tokens = tokens;
    last_token_count = token_count;
    last_highlight_time = time(NULL);
    
    // Update statistics
    stats.lines_highlighted++;
    double elapsed = get_time_ms() - start_time;
    stats.avg_highlighting_time_ms = (stats.avg_highlighting_time_ms * (stats.lines_highlighted - 1) + elapsed) / stats.lines_highlighted;
    stats.total_tokens_processed += token_count;
    
    return highlighted;
}

/**
 * Real-time highlighting callback for readline
 */
void lusush_highlight_line_realtime(void) {
    if (!initialized || !config.enabled || !config.realtime_highlighting) {
        return;
    }
    
    // This would be called by readline's redisplay mechanism
    // Implementation depends on readline integration details
    
    const char *line_buffer = rl_line_buffer;
    if (!line_buffer) {
        return;
    }
    
    // For now, we'll just validate this is being called
    // Full implementation would require deeper readline integration
    stats.realtime_updates++;
}

// ============================================================================
// API FUNCTIONS
// ============================================================================

/**
 * Initialize enhanced syntax highlighting system
 */
bool lusush_enhanced_highlighting_init(void) {
    if (initialized) {
        return true;
    }
    
    // Set default configuration
    config.enabled = true;
    config.realtime_highlighting = true;
    config.colorize_output = true;
    config.highlight_errors = true;
    config.highlight_paths = true;
    config.check_command_existence = true;
    config.cache_timeout_seconds = 30;
    
    // Copy default colors
    current_colors = default_colors;
    
    // Initialize command cache
    if (!cmd_cache_initialized) {
        memset(cmd_cache, 0, sizeof(cmd_cache));
        cmd_cache_initialized = 1;
    }
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    
    initialized = true;
    return true;
}

/**
 * Cleanup enhanced syntax highlighting system
 */
void lusush_enhanced_highlighting_cleanup(void) {
    if (!initialized) {
        return;
    }
    
    // Clean up cache
    free(last_highlighted_line);
    free(last_tokens);
    
    // Clean up command cache
    for (int i = 0; i < COMMAND_CACHE_SIZE; i++) {
        free(cmd_cache[i].command);
        memset(&cmd_cache[i], 0, sizeof(cmd_cache[i]));
    }
    
    memset(&config, 0, sizeof(config));
    memset(&stats, 0, sizeof(stats));
    
    last_highlighted_line = NULL;
    last_tokens = NULL;
    last_token_count = 0;
    last_highlight_time = 0;
    cmd_cache_initialized = 0;
    initialized = false;
}

/**
 * Configure enhanced syntax highlighting
 */
void lusush_configure_enhanced_highlighting(const enhanced_highlight_config_t *new_config) {
    if (!new_config) return;
    
    config = *new_config;
    
    // Invalidate cache when configuration changes
    free(last_highlighted_line);
    last_highlighted_line = NULL;
    last_highlight_time = 0;
}

/**
 * Set custom colors for syntax highlighting
 */
void lusush_set_highlighting_colors(const highlight_colors_t *colors) {
    if (!colors) return;
    
    current_colors = *colors;
}

/**
 * Get current syntax highlighting configuration
 */
const enhanced_highlight_config_t* lusush_get_enhanced_highlighting_config(void) {
    return &config;
}

/**
 * Enable or disable syntax highlighting
 */
void lusush_set_enhanced_highlighting_enabled(bool enabled) {
    config.enabled = enabled;
    
    if (!enabled) {
        // Clear cache when disabling
        free(last_highlighted_line);
        last_highlighted_line = NULL;
    }
}

/**
 * Check if syntax highlighting is enabled
 */
bool lusush_is_enhanced_highlighting_enabled(void) {
    return initialized && config.enabled;
}

/**
 * Clear highlighting cache
 */
void lusush_clear_highlighting_cache(void) {
    free(last_highlighted_line);
    last_highlighted_line = NULL;
    last_highlight_time = 0;
    
    // Clear command existence cache too
    for (int i = 0; i < COMMAND_CACHE_SIZE; i++) {
        cmd_cache[i].is_valid = false;
    }
}

/**
 * Get syntax highlighting statistics
 */
const highlight_stats_t* lusush_get_highlighting_stats(void) {
    return &stats;
}

/**
 * Reset syntax highlighting statistics
 */
void lusush_reset_highlighting_stats(void) {
    memset(&stats, 0, sizeof(stats));
}

/**
 * Print syntax highlighting statistics
 */
void lusush_print_highlighting_stats(void) {
    printf("Enhanced Syntax Highlighting Statistics:\n");
    printf("  Lines highlighted: %d\n", stats.lines_highlighted);
    printf("  Realtime updates: %d\n", stats.realtime_updates);
    printf("  Cache hits: %d\n", stats.cache_hits);
    printf("  Cache misses: %d\n", stats.cache_misses);
    printf("  Tokens processed: %d\n", stats.total_tokens_processed);
    printf("  Command validations: %d\n", stats.command_validations);
    printf("  Path validations: %d\n", stats.path_validations);
    printf("  Avg highlighting time: %.2f ms\n", stats.avg_highlighting_time_ms);
    
    if (stats.cache_hits + stats.cache_misses > 0) {
        printf("  Cache hit rate: %.1f%%\n",
               (100.0 * stats.cache_hits) / (stats.cache_hits + stats.cache_misses));
    }
}

/**
 * Validate command line for errors (without highlighting)
 */
bool lusush_validate_command_line(const char *line, char **error_message) {
    if (!line || !initialized) {
        return true; // No errors if no input
    }
    
    highlight_token_t *tokens = NULL;
    int token_count = tokenize_line(line, &tokens);
    
    bool valid = true;
    static char error_buffer[256];
    error_buffer[0] = '\0';
    
    // Check for basic syntax errors
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_COMMAND_INVALID) {
            // Extract command name for error message
            size_t cmd_len = tokens[i].end - tokens[i].start;
            char cmd_name[cmd_len + 1];
            strncpy(cmd_name, line + tokens[i].start, cmd_len);
            cmd_name[cmd_len] = '\0';
            
            snprintf(error_buffer, sizeof(error_buffer), 
                    "Command not found: %s", cmd_name);
            valid = false;
            break;
        }
    }
    
    free(tokens);
    
    if (error_message) {
        *error_message = error_buffer[0] ? error_buffer : NULL;
    }
    
    return valid;
}

/**
 * Get default highlighting colors
 */
highlight_colors_t lusush_get_default_highlighting_colors(void) {
    return default_colors;
}
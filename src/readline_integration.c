/*
 * Lusush - A modern shell with GNU Readline integration
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
 *
 * Lusush Shell - Complete GNU Readline Integration
 * 
 * This module provides complete readline integration with ALL Lusush features:
 * - Theme integration with colored prompts
 * - Tab completion using existing completion system
 * - History with deduplication (hist_no_dups)
 * - Syntax highlighting
 * - Key bindings and shortcuts
 * - Multiline support
 * - Git integration in prompts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../include/readline_integration.h"
#include "../include/completion.h"
#include "../include/config.h"
#include "../include/themes.h"
#include "../include/prompt.h"
#include "../include/symtable.h"
#include "../include/posix_history.h"
#include "../include/lusush.h"
#include "../include/init.h"

// Global state
static bool initialized = false;
static char *current_prompt = NULL;
static char *history_file_path = NULL;
static bool syntax_highlighting_enabled = true;
static bool debug_enabled = false;

// Syntax highlighting colors
static const char *keyword_color = "\033[1;34m";    // Bright blue
static const char *command_color = "\033[1;32m";    // Bright green
static const char *string_color = "\033[1;33m";     // Bright yellow
static const char *variable_color = "\033[1;35m";   // Bright magenta
static const char *operator_color = "\033[1;31m";   // Bright red
static const char *comment_color = "\033[1;30m";    // Gray
static const char *number_color = "\033[1;36m";     // Bright cyan
static const char *reset_color = "\033[0m";         // Reset

// Hooks
static lusush_pre_input_hook_t pre_input_hook = NULL;
static lusush_post_input_hook_t post_input_hook = NULL;
static lusush_prompt_callback_t prompt_callback = NULL;

// History deduplication support
static char **history_cache = NULL;
static int history_cache_size = 0;
static int history_cache_capacity = 0;

// Forward declarations
static void setup_readline_config(void);
static void setup_key_bindings(void);
static char **lusush_tab_completion(const char *text, int start, int end);
static char *lusush_completion_entry_function(const char *text, int state);
static void apply_syntax_highlighting(void);
static void lusush_simple_syntax_display(void);
static char **lusush_git_subcommand_completion(const char *text);
static char **lusush_directory_only_completion(const char *text);
static bool lusush_is_shell_keyword(const char *word, size_t length);
static bool lusush_is_shell_builtin(const char *word, size_t length);
static bool lusush_is_word_separator(char c);
static void lusush_custom_redisplay(void);
static void lusush_output_colored_line(const char *line, int cursor_pos);
static int lusush_getc(FILE *stream);
static int lusush_abort_line(int count, int key);
static int lusush_clear_screen_and_redisplay(int count, int key);
static int lusush_previous_history(int count, int key);
static int lusush_next_history(int count, int key);


// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

bool lusush_readline_init(void) {
    if (initialized) return true;
    
    // Initialize readline
    rl_readline_name = "lusush";
    
    // SELECTIVE completion setup - enable TAB completion while preserving arrow keys
    // Critical: Only TAB should trigger completion, never arrow keys
    rl_attempted_completion_function = lusush_tab_completion;
    rl_completion_entry_function = lusush_completion_entry_function;
    rl_ignore_completion_duplicates = 1;
    rl_filename_completion_desired = 0;
    
    // Setup history
    using_history();
    stifle_history(config.history_size > 0 ? config.history_size : 1000);
    
    // Set history file path
    if (!history_file_path) {
        char *home = getenv("HOME");
        if (home) {
            size_t path_len = strlen(home) + 20;
            history_file_path = malloc(path_len);
            if (history_file_path) {
                snprintf(history_file_path, path_len, "%s/.lusush_history", home);
            }
        }
        if (!history_file_path) {
            history_file_path = strdup(".lusush_history");
        }
    }
    
    // Load existing history
    lusush_history_load();
    
    // Setup readline configuration
    setup_readline_config();
    
    // Setup key bindings
    setup_key_bindings();
    
    // Initialize completion system
    lusush_completion_setup();
    
    // Enable syntax highlighting by default
    lusush_syntax_highlighting_set_enabled(true);
    
    // Initialize history cache for deduplication
    history_cache_capacity = 100;
    history_cache = malloc(history_cache_capacity * sizeof(char*));
    if (history_cache) {
        memset(history_cache, 0, history_cache_capacity * sizeof(char*));
    }
    
    initialized = true;
    return true;
}

void lusush_readline_cleanup(void) {
    if (!initialized) return;
    
    // Save history
    lusush_history_save();
    
    // Clear history
    clear_history();
    
    // Free history cache
    if (history_cache) {
        for (int i = 0; i < history_cache_size; i++) {
            free(history_cache[i]);
        }
        free(history_cache);
        history_cache = NULL;
        history_cache_size = 0;
        history_cache_capacity = 0;
    }
    
    // Free current prompt
    if (current_prompt) {
        free(current_prompt);
        current_prompt = NULL;
    }
    
    // Free history file path
    if (history_file_path) {
        free(history_file_path);
        history_file_path = NULL;
    }
    
    initialized = false;
}

// ============================================================================
// MAIN READLINE INTERFACE
// ============================================================================

char *lusush_readline_with_prompt(const char *prompt) {
    if (!initialized) {
        lusush_readline_init();
    }
    
    // Call pre-input hook
    if (pre_input_hook) {
        pre_input_hook();
    }
    
    // Generate prompt using Lusush theming if no specific prompt provided
    const char *actual_prompt = prompt;
    if (!prompt || (prompt_callback && strcmp(prompt, "$ ") == 0)) {
        char *themed_prompt = lusush_generate_prompt();
        if (themed_prompt) {
            actual_prompt = themed_prompt;
        }
    }
    
    // Custom redisplay is now properly implemented for syntax highlighting
    // The redisplay function is set in lusush_syntax_highlighting_set_enabled()
    
    // Get input from readline
    char *line = readline(actual_prompt);
    
    // Debug: Track what commands are being entered in interactive mode
    if (debug_enabled && line && *line) {
        fprintf(stderr, "[READLINE_DEBUG] Command entered: '%s'\n", line);
        fflush(stderr);
    }
    
    // Call post-input hook
    if (post_input_hook) {
        post_input_hook(line);
    }
    
    // Add to history if non-empty and in interactive mode
    if (line && *line && is_interactive_shell()) {
        lusush_history_add(line);
    }
    
    // Ensure output is visible before returning
    fflush(stdout);
    fflush(stderr);
    
    return line;
}

char *lusush_readline(void) {
    return lusush_readline_with_prompt(NULL);
}

bool lusush_readline_available(void) {
    return true;
}

const char *lusush_readline_version(void) {
    return rl_library_version;
}

// ============================================================================
// HISTORY MANAGEMENT WITH DEDUPLICATION
// ============================================================================

static bool is_duplicate_in_cache(const char *line) {
    if (!history_cache || !line) return false;
    
    for (int i = 0; i < history_cache_size; i++) {
        if (history_cache[i] && strcmp(history_cache[i], line) == 0) {
            return true;
        }
    }
    return false;
}

static void add_to_history_cache(const char *line) {
    if (!history_cache || !line) return;
    
    // Check if we need to expand cache
    if (history_cache_size >= history_cache_capacity) {
        int new_capacity = history_cache_capacity * 2;
        char **new_cache = realloc(history_cache, new_capacity * sizeof(char*));
        if (new_cache) {
            history_cache = new_cache;
            // Initialize new entries to NULL
            for (int i = history_cache_capacity; i < new_capacity; i++) {
                history_cache[i] = NULL;
            }
            history_cache_capacity = new_capacity;
        }
    }
    
    // Add to cache
    if (history_cache_size < history_cache_capacity) {
        history_cache[history_cache_size] = strdup(line);
        history_cache_size++;
    }
}

void lusush_history_add(const char *line) {
    if (!line || !*line) return;
    
    // Skip whitespace-only lines
    const char *trimmed = line;
    while (*trimmed && isspace(*trimmed)) trimmed++;
    if (!*trimmed) return;
    
    // Check for duplicates if hist_no_dups is enabled
    if (config.history_no_dups) {
        // Check recent history (last 50 entries)
        HIST_ENTRY *entry;
        for (int i = 1; i <= 50 && i <= history_length; i++) {
            entry = history_get(history_length - i + 1);
            if (entry && entry->line && strcmp(entry->line, line) == 0) {
                return; // Skip duplicate
            }
        }
        
        // Check cache for more comprehensive deduplication
        if (is_duplicate_in_cache(line)) {
            return;
        }
    }
    
    // Add to readline history
    add_history(line);
    
    // Add to our cache for deduplication
    add_to_history_cache(line);
    
    // Limit history size
    if (config.history_size > 0 && history_length > config.history_size) {
        HIST_ENTRY *entry = remove_history(0);
        if (entry) {
            free(entry->line);
            free(entry->data);
            free(entry);
        }
    }
}

void lusush_history_save(void) {
    if (history_file_path) {
        write_history(history_file_path);
    }
}

bool lusush_history_load(void) {
    if (!history_file_path) return false;
    
    struct stat st;
    if (stat(history_file_path, &st) != 0) {
        return false; // File doesn't exist, that's okay
    }
    
    if (read_history(history_file_path) != 0) {
        return false;
    }
    
    // Populate cache from loaded history for deduplication
    HIST_ENTRY **hist_list = history_list();
    if (hist_list) {
        for (int i = 0; hist_list[i]; i++) {
            add_to_history_cache(hist_list[i]->line);
        }
    }
    
    return true;
}

void lusush_history_clear(void) {
    clear_history();
    
    // Clear cache
    if (history_cache) {
        for (int i = 0; i < history_cache_size; i++) {
            free(history_cache[i]);
            history_cache[i] = NULL;
        }
        history_cache_size = 0;
    }
}

const char *lusush_history_get(int index) {
    HIST_ENTRY *entry = history_get(index + 1);
    return entry ? entry->line : NULL;
}

int lusush_history_length(void) {
    return history_length;
}

int lusush_history_remove_duplicates(void) {
    int removed = 0;
    HIST_ENTRY **hist_list = history_list();
    
    if (!hist_list) return 0;
    
    // Simple O(n²) deduplication - remove later duplicates
    for (int i = 0; hist_list[i]; i++) {
        for (int j = i + 1; hist_list[j]; j++) {
            if (strcmp(hist_list[i]->line, hist_list[j]->line) == 0) {
                HIST_ENTRY *entry = remove_history(j);
                if (entry) {
                    free(entry->line);
                    free(entry->data);
                    free(entry);
                    removed++;
                    // Refresh list after removal
                    hist_list = history_list();
                    j--; // Check this position again
                }
            }
        }
    }
    
    return removed;
}

void lusush_history_set_max_length(int max_length) {
    stifle_history(max_length);
}

bool lusush_history_set_file(const char *filepath) {
    if (!filepath) return false;
    
    free(history_file_path);
    history_file_path = strdup(filepath);
    return history_file_path != NULL;
}

// ============================================================================
// TAB COMPLETION INTEGRATION
// ============================================================================

void lusush_completion_setup(void) {
    rl_attempted_completion_function = lusush_tab_completion;
    rl_completion_entry_function = lusush_completion_entry_function;
    
    // Configure completion behavior for cycling
    rl_completion_append_character = ' ';
    rl_completion_suppress_append = 0;
    rl_filename_completion_desired = 1;
    rl_filename_quoting_desired = 1;
    rl_completion_query_items = 50;
    
    // Configure tab completion to cycle through matches
    rl_bind_key('\t', rl_complete);
    rl_variable_bind("menu-complete-display-prefix", "on");
    
    // Better completion behavior
    rl_variable_bind("show-all-if-ambiguous", "on");
    rl_variable_bind("completion-ignore-case", "on");
    rl_variable_bind("visible-stats", "on");
    rl_variable_bind("mark-directories", "on");
    rl_variable_bind("mark-symlinked-directories", "on");
    rl_variable_bind("menu-complete-display-prefix", "on");
}

static char **lusush_tab_completion(const char *text, int start, int end __attribute__((unused))) {
    char **matches = NULL;
    
    // Get the full line buffer for context-aware completion
    const char *line_buffer = rl_line_buffer;
    
    // Extract command for context-aware completion
    const char *cmd_start = line_buffer;
    while (isspace(*cmd_start)) cmd_start++;
    const char *cmd_end = cmd_start;
    while (*cmd_end && !isspace(*cmd_end)) cmd_end++;
    size_t cmd_len = cmd_end - cmd_start;
    
    // Context-aware completion for specific commands
    if (cmd_len == 3 && memcmp(cmd_start, "git", 3) == 0 && start > 4) {
        matches = lusush_git_subcommand_completion(text);
        if (matches) return matches;
    }
    
    if (cmd_len == 2 && memcmp(cmd_start, "cd", 2) == 0 && start > 3) {
        matches = lusush_directory_only_completion(text);
        if (matches) return matches;
    }
    
    // Use standard Lusush completion system for other cases
    lusush_completions_t completions = {0, NULL};
    
    // Call existing Lusush completion callback
    lusush_completion_callback(line_buffer, &completions);
    
    // Convert to readline format with proper cycling support
    if (completions.len > 0) {
        // Add 2 extra slots: one for the common prefix, one for NULL terminator
        matches = malloc((completions.len + 2) * sizeof(char*));
        if (matches) {
            // First element is the substitution text (common prefix)
            if (completions.len == 1) {
                // Single match - use it as substitution
                matches[0] = strdup(completions.cvec[0]);
            } else {
                // Multiple matches - find common prefix
                size_t prefix_len = strlen(completions.cvec[0]);
                for (size_t i = 1; i < completions.len; i++) {
                    size_t j = 0;
                    while (j < prefix_len && 
                           j < strlen(completions.cvec[i]) &&
                           completions.cvec[0][j] == completions.cvec[i][j]) {
                        j++;
                    }
                    prefix_len = j;
                }
                
                if (prefix_len > strlen(text)) {
                    // Use common prefix if it's longer than current text
                    matches[0] = malloc(prefix_len + 1);
                    if (matches[0]) {
                        strncpy(matches[0], completions.cvec[0], prefix_len);
                        matches[0][prefix_len] = '\0';
                    }
                } else {
                    // No useful common prefix - use original text
                    matches[0] = strdup(text);
                }
            }
            
            // Copy all completions for display/cycling
            for (size_t i = 0; i < completions.len; i++) {
                matches[i + 1] = completions.cvec[i]; // Transfer ownership
            }
            matches[completions.len + 1] = NULL;
            
            // Don't free the strings since we transferred ownership
            free(completions.cvec);
        }
    }
    
    return matches;
}

static char *lusush_completion_entry_function(const char *text, int state) {
    (void)text; (void)state; // Suppress unused parameter warnings
    // This is called by readline for default completions when our
    // attempted_completion_function returns NULL
    return NULL; // Let readline handle default file completion
}

char *lusush_completion_generator(const char *text, int state) {
    (void)text; (void)state; // Suppress unused parameter warnings
    // Legacy compatibility - redirects to main completion system
    return NULL;
}

char **lusush_completion_matches(const char *text, int start, int end) {
    return lusush_tab_completion(text, start, end);
}

/**
 * @brief Git subcommand completion
 */
static char **lusush_git_subcommand_completion(const char *text) {
    // Performance optimization: prioritize most common git commands
    static const char *git_commands[] = {
        "status", "add", "commit", "push", "pull", "checkout", "branch",
        "log", "diff", "merge", "fetch", "clone", "init", "reset", "rebase",
        "tag", "config", "remote", "show", "stash", "cherry-pick", "revert", 
        "bisect", "grep", "mv", "rm", "clean", "describe", "shortlog", 
        "archive", "bundle", "blame", "show-branch", "format-patch",
        NULL
    };
    
    char **matches = NULL;
    int match_count = 0;
    size_t text_len = strlen(text);
    
    // Performance optimization: early exit for empty text
    if (text_len == 0) {
        // Return most common commands for empty completion
        matches = malloc(8 * sizeof(char*));
        if (matches) {
            matches[0] = strdup("");
            matches[1] = strdup("status");
            matches[2] = strdup("add");
            matches[3] = strdup("commit");
            matches[4] = strdup("push");
            matches[5] = strdup("pull");
            matches[6] = strdup("checkout");
            matches[7] = NULL;
        }
        return matches;
    }
    
    // Count matching commands
    for (int i = 0; git_commands[i]; i++) {
        if (strncmp(git_commands[i], text, text_len) == 0) {
            match_count++;
        }
    }
    
    if (match_count == 0) return NULL;
    
    // Allocate matches array
    matches = malloc((match_count + 2) * sizeof(char*));
    if (!matches) return NULL;
    
    // Find common prefix for substitution
    matches[0] = strdup(text);
    int match_index = 1;
    
    // Add matching commands
    for (int i = 0; git_commands[i]; i++) {
        if (strncmp(git_commands[i], text, text_len) == 0) {
            matches[match_index++] = strdup(git_commands[i]);
        }
    }
    matches[match_index] = NULL;
    
    return matches;
}

/**
 * @brief Directory-only completion for cd command
 */
static char **lusush_directory_only_completion(const char *text) {
    // Performance optimization: limit directory scanning for large directories
    char **all_matches = rl_completion_matches(text, rl_filename_completion_function);
    if (!all_matches) return NULL;
    
    // Optimized filter to keep only directories with size limit
    int max_dirs = 100; // Limit for performance
    char **dir_matches = malloc((max_dirs + 2) * sizeof(char*));
    if (!dir_matches) {
        for (int i = 0; all_matches[i]; i++) free(all_matches[i]);
        free(all_matches);
        return NULL;
    }
    
    dir_matches[0] = strdup(all_matches[0]); // Keep substitution text
    int dir_count = 1;
    
    for (int i = 1; all_matches[i] && dir_count < max_dirs; i++) {
        // Check if it's a directory
        char full_path[1024];
        if (text[0] == '/' || text[0] == '~') {
            snprintf(full_path, sizeof(full_path), "%s", all_matches[i]);
        } else {
            snprintf(full_path, sizeof(full_path), "%s", all_matches[i]);
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            dir_matches[dir_count++] = strdup(all_matches[i]);
        }
    }
    dir_matches[dir_count] = NULL;
    
    // Clean up original matches
    for (int i = 0; all_matches[i]; i++) free(all_matches[i]);
    free(all_matches);
    
    return dir_count > 1 ? dir_matches : NULL;
}

// ============================================================================
// PROMPT INTEGRATION WITH LUSUSH THEMES
// ============================================================================

char *lusush_generate_prompt(void) {
    if (prompt_callback) {
        return prompt_callback();
    }
    
    // Use Lusush prompt building system
    build_prompt();
    
    // Get the generated prompt from symbol table
    const char *ps1 = symtable_get_global_default("PS1", "$ ");
    
    // Free previous prompt
    if (current_prompt) {
        free(current_prompt);
    }
    
    // Make a copy for readline
    current_prompt = strdup(ps1);
    return current_prompt;
}

void lusush_prompt_update(void) {
    // Force rebuild of prompt
    rebuild_prompt();
    
    // Update current_prompt
    lusush_generate_prompt();
}

void lusush_prompt_set_callback(lusush_prompt_callback_t callback) {
    prompt_callback = callback;
}

// ============================================================================
// SYNTAX HIGHLIGHTING
// ============================================================================

void lusush_syntax_highlighting_set_enabled(bool enabled) {
    syntax_highlighting_enabled = enabled;
    
    // Always use standard functions to prevent display issues
    // Real-time highlighting disabled to prevent prompt spam
    rl_redisplay_function = rl_redisplay;
    rl_getc_function = rl_getc;
}

bool lusush_syntax_highlighting_is_enabled(void) {
    return syntax_highlighting_enabled;
}

static void apply_syntax_highlighting(void) {
    // Only apply during normal interactive editing
    if (!rl_line_buffer || !*rl_line_buffer) {
        rl_redisplay();
        return;
    }
    
    // Check if we're in a safe state for custom display
    if (rl_readline_state & (RL_STATE_ISEARCH | RL_STATE_NSEARCH | 
                            RL_STATE_SEARCH | RL_STATE_COMPLETING |
                            RL_STATE_VICMDONCE | RL_STATE_VIMOTION)) {
        rl_redisplay();
        return;
    }
    
    // Use a much simpler and safer approach
    lusush_simple_syntax_display();
}

// Helper function to check if a word is a shell keyword
static bool lusush_is_shell_keyword(const char *word, size_t length) {
    static const char *keywords[] = {
        "if", "then", "else", "elif", "fi",
        "for", "while", "until", "do", "done",
        "case", "esac", "in", "select",
        "function", "time", "!", "[[", "]]"
    };
    
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i]) == length && 
            strncmp(word, keywords[i], length) == 0) {
            return true;
        }
    }
    return false;
}

// Helper function to check if a word is a shell builtin
static bool lusush_is_shell_builtin(const char *word, size_t length) {
    static const char *builtins[] = {
        "echo", "cd", "pwd", "export", "set", "unset",
        "alias", "unalias", "history", "exit", "return",
        "source", ".", "exec", "eval", "test", "[",
        "printf", "read", "shift", "trap", "ulimit",
        "umask", "wait", "jobs", "fg", "bg", "kill",
        "type", "which", "command", "builtin", "enable",
        "help", "let", "local", "readonly", "declare",
        "typeset", "fc", "theme", "config"
    };
    
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strlen(builtins[i]) == length && 
            strncmp(word, builtins[i], length) == 0) {
            return true;
        }
    }
    return false;
}

// Helper function to check if a character separates words
static bool lusush_is_word_separator(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '|' || c == '&' || c == ';' || c == '(' || c == ')' ||
           c == '<' || c == '>' || c == '\0';
}

// Simple and safe syntax display
static void lusush_simple_syntax_display(void) {
    if (!rl_line_buffer) {
        rl_redisplay();
        return;
    }
    
    // Clear line and redraw with colors
    printf("\r\033[K");
    
    // Print prompt
    if (rl_prompt) {
        printf("%s", rl_prompt);
    }
    
    // Print line with syntax highlighting
    lusush_output_colored_line(rl_line_buffer, rl_point);
    
    // Move cursor to correct position
    if (rl_point > 0) {
        printf("\r");
        if (rl_prompt) {
            printf("%s", rl_prompt);
        }
        // Simple cursor positioning - just move forward
        for (int i = 0; i < rl_point && i < (int)strlen(rl_line_buffer); i++) {
            printf("\033[C");
        }
    }
    
    fflush(stdout);
}

// Show syntax highlighted version of command when user presses Enter
void lusush_show_command_syntax_preview(const char *command) {
    // Disable preview since we now have real-time highlighting
    (void)command; // Suppress unused parameter warning
    return;
}

// Output the line with syntax highlighting colors
static void lusush_output_colored_line(const char *line, int cursor_pos __attribute__((unused))) {
    if (!line || !*line) {
        return;
    }
    
    size_t len = strlen(line);
    size_t i = 0;
    bool in_string = false;
    bool in_single_quote __attribute__((unused)) = false;
    char string_char = '\0';
    
    while (i < len) {
        char c = line[i];
        
        // Handle string literals
        if (!in_string && (c == '"' || c == '\'')) {
            in_string = true;
            string_char = c;
            if (c == '\'') {
                in_single_quote = true;
            }
            printf("%s%c", string_color, c);
            i++;
            continue;
        } else if (in_string && c == string_char) {
            printf("%c%s", c, reset_color);
            in_string = false;
            in_single_quote = false;
            string_char = '\0';
            i++;
            continue;
        } else if (in_string) {
            printf("%c", c);
            i++;
            continue;
        }
        
        // Handle comments
        if (c == '#') {
            printf("%s", comment_color);
            while (i < len) {
                printf("%c", line[i]);
                i++;
            }
            printf("%s", reset_color);
            break;
        }
        
        // Handle variables
        if (c == '$') {
            printf("%s%c", variable_color, c);
            i++;
            while (i < len && (isalnum(line[i]) || line[i] == '_' || 
                              line[i] == '{' || line[i] == '}')) {
                printf("%c", line[i]);
                i++;
            }
            printf("%s", reset_color);
            continue;
        }
        
        // Handle operators
        if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>') {
            printf("%s%c", operator_color, c);
            printf("%s", reset_color);
            i++;
            continue;
        }
        
        // Handle word separators
        if (lusush_is_word_separator(c)) {
            printf("%c", c);
            i++;
            continue;
        }
        
        // Handle words (commands, keywords)
        if (isalnum(c) || c == '_' || c == '-' || c == '.') {
            size_t word_start = i;
            while (i < len && (isalnum(line[i]) || line[i] == '_' || 
                              line[i] == '-' || line[i] == '.')) {
                i++;
            }
            
            size_t word_len = i - word_start;
            const char *word = line + word_start;
            
            // Check if it's a keyword
            if (lusush_is_shell_keyword(word, word_len)) {
                printf("%s", keyword_color);
            }
            // Check if it's a builtin command
            else if (lusush_is_shell_builtin(word, word_len)) {
                printf("%s", command_color);
            }
            // Check if it's a number
            else if (word_len > 0 && isdigit(word[0])) {
                bool is_number = true;
                for (size_t j = 0; j < word_len; j++) {
                    if (!isdigit(word[j]) && word[j] != '.') {
                        is_number = false;
                        break;
                    }
                }
                if (is_number) {
                    printf("%s", number_color);
                }
            }
            
            // Print the word
            for (size_t j = 0; j < word_len; j++) {
                printf("%c", word[j]);
            }
            printf("%s", reset_color);
            continue;
        }
        
        // Default: print character as-is
        printf("%c", c);
        i++;
    }
}



// Custom redisplay function - disabled for safety
static void lusush_custom_redisplay(void) {
    // Always use standard redisplay to prevent display corruption
    rl_redisplay();
}



int lusush_syntax_highlight_line(void) {
    // Simple hook that doesn't interfere with readline
    return 0;
}

void lusush_syntax_highlighting_configure(const char *commands_color,
                                         const char *strings_color,
                                         const char *comments_color,
                                         const char *keywords_color) {
    // Update color configurations if provided
    if (commands_color) command_color = commands_color;
    if (strings_color) string_color = strings_color;
    if (comments_color) comment_color = comments_color;
    if (keywords_color) keyword_color = keywords_color;
}

// ============================================================================
// KEY BINDINGS AND SHORTCUTS
// ============================================================================

// Custom function to abort current line (Ctrl+G)
static int lusush_abort_line(int count, int key) {
    (void)count; (void)key;
    
    // DIAGNOSTIC: Print to stderr to verify function is called
    fprintf(stderr, "[KEY_DEBUG] lusush_abort_line called - Ctrl+G pressed\n");
    
    // Clear displayed line properly
    printf("\r\033[K");  // Move to start of line and clear to end of line
    fflush(stdout);
    
    // Clear readline's internal buffer
    rl_replace_line("", 0);
    rl_point = 0;
    rl_end = 0;
    
    // Display fresh prompt
    rl_on_new_line();
    rl_redisplay();
    
    fprintf(stderr, "[KEY_DEBUG] lusush_next_history completed\n");
    return 0;
}

    // Ctrl-R uses standard readline reverse search

// Custom function to clear screen (Ctrl+L)
static int lusush_clear_screen_and_redisplay(int count, int key) {
    (void)count; (void)key;
    
    // Use standard readline clear screen approach
    return rl_clear_screen(count, key);
}

// Custom history navigation with artifact prevention
static int lusush_previous_history(int count, int key) {
    (void)count; (void)key;
    
    // Move to previous history entry
    rl_get_previous_history(1, 0);
    
    // Force complete redisplay to prevent artifacts
    rl_forced_update_display();
    rl_redisplay();
    return 0;
}

static int lusush_next_history(int count, int key) {
    (void)count; (void)key;
    
    // Move to next history entry
    rl_get_next_history(1, 0);
    
    // Force complete redisplay to prevent artifacts
    rl_forced_update_display();
    rl_redisplay();
    return 0;
}

static void setup_key_bindings(void) {
    // DISABLE tab completion to prevent interference with arrow keys
    rl_bind_key('\t', rl_insert);  // TAB just inserts tab character
    // rl_bind_key(27, rl_complete); // Disable Alt+Tab completion
    
    rl_bind_key('\n', rl_newline);   // Enter
    rl_bind_key('\r', rl_newline);   // Return
    
    // Ctrl key bindings
    rl_bind_key(1, rl_beg_of_line);   // Ctrl-A: beginning of line
    rl_bind_key(5, rl_end_of_line);   // Ctrl-E: end of line
    rl_bind_key(7, lusush_abort_line); // Ctrl-G: abort/cancel line
    rl_bind_key(12, lusush_clear_screen_and_redisplay); // Ctrl-L: clear screen
    rl_bind_key(21, rl_unix_line_discard); // Ctrl-U: kill line
    rl_bind_key(11, rl_kill_line);    // Ctrl-K: kill to end
    rl_bind_key(23, rl_unix_word_rubout); // Ctrl-W: kill word
    // Ctrl-R uses standard readline reverse search (rl_reverse_search_history)
    
    // History navigation with artifact prevention
    rl_bind_key(16, lusush_previous_history); // Ctrl-P
    rl_bind_key(14, lusush_next_history);     // Ctrl-N
    
    // Let readline handle arrow keys natively - custom bindings cause [A [B artifacts
    // Default readline arrow key handling will work for history navigation
    
    // Enable vi or emacs mode based on config
    if (false) { // vi_mode not implemented yet
        rl_editing_mode = 0; // vi mode
    } else {
        rl_editing_mode = 1; // emacs mode (default)
    }
}

void lusush_keybindings_setup(void) {
    setup_key_bindings();
}

int lusush_keybinding_add(int key, rl_command_func_t *function) {
    return rl_bind_key(key, function);
}

int lusush_keybinding_remove(int key) {
    return rl_unbind_key(key);
}

// ============================================================================
// CONFIGURATION AND SETTINGS
// ============================================================================

static void setup_readline_config(void) {
    // TAB-ONLY completion configuration - preserve arrow key protection
    rl_completion_append_character = ' ';  // Standard completion append
    rl_attempted_completion_over = 0;      // Allow completion processing
    rl_filename_completion_desired = 1;    // Enable filename completion for TAB
    rl_filename_quoting_desired = 1;       // Enable quote handling
    rl_inhibit_completion = 0;             // Allow completion but only for TAB
    
    // History configuration
    // rl_history_search_delimiter_chars = " \t\n;&()|<>"; // Not available in all readline versions
    
    // TAB completion configuration - optimized for performance
    // CRITICAL: Protect arrow keys while enabling TAB completion
    rl_completion_query_items = 50;       // Optimized query threshold for large sets
    rl_completion_suppress_quote = 0;     // Allow quote handling for TAB
    rl_completion_quote_character = '"';  // Standard quote character
    rl_completion_suppress_append = 0;    // Allow append for TAB completion
    
    // Input configuration - minimal signal handling
    rl_catch_signals = 0; // Let shell handle signals for child processes like git
    rl_catch_sigwinch = 1; // Handle window resize only
    
    // Redisplay configuration - Use standard redisplay for safety
    // Syntax highlighting framework exists but is disabled to prevent output issues
    rl_redisplay_function = rl_redisplay;
    
    // CRITICAL VARIABLES: Enable TAB completion, protect arrow keys
    rl_variable_bind("disable-completion", "off");      // MASTER SWITCH: Enable completion for TAB
    rl_variable_bind("show-all-if-unmodified", "off");  // CRITICAL: Prevents arrow key completion
    rl_variable_bind("show-all-if-ambiguous", "off");   // CRITICAL: Prevents "display all possibilities"
    rl_variable_bind("visible-stats", "off");           // Disable to prevent interference
    rl_variable_bind("page-completions", "on");         // Enable paging for large completion lists
    rl_variable_bind("completion-query-items", "50");   // Optimized threshold for large sets
    rl_variable_bind("completion-display-width", "4");  // Optimize column display
    rl_variable_bind("print-completions-horizontally", "on"); // Better layout for many items
    
    // Multi-line prompt handling for Ctrl+R positioning
    rl_variable_bind("horizontal-scroll-mode", "off");  // Use vertical scrolling for long lines
    rl_variable_bind("enable-meta-key", "on");          // Better key handling

    
    // Custom getc function for better input handling
    rl_getc_function = lusush_getc;
}

void lusush_readline_configure(bool show_completions_immediately,
                              bool case_insensitive_completion,
                              bool colored_completion_prefix) {
   (void)colored_completion_prefix; // Suppress unused parameter warnings
    if (show_completions_immediately) {
        rl_completion_query_items = 0;
        rl_variable_bind("show-all-if-ambiguous", "on");
    } else {
        rl_completion_query_items = 50;
        rl_variable_bind("show-all-if-ambiguous", "off");
    }
    
    if (case_insensitive_completion) {
        rl_variable_bind("completion-ignore-case", "on");
    } else {
        rl_variable_bind("completion-ignore-case", "off");
    }
    
    // Force redisplay after configuration changes
    rl_forced_update_display();
}

void lusush_multiline_set_enabled(bool enabled) {
    if (enabled) {
        rl_variable_bind("enable-bracketed-paste", "on");
    } else {
        rl_variable_bind("enable-bracketed-paste", "off");
    }
}


// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void lusush_clear_screen(void) {
    rl_clear_screen(0, 0);
}

void lusush_refresh_line(void) {
    rl_forced_update_display();
}

int lusush_get_cursor_position(void) {
    return rl_point;
}

bool lusush_set_cursor_position(int position) {
    if (position >= 0 && position <= rl_end) {
        rl_point = position;
        return true;
    }
    return false;
}

const char *lusush_get_line_buffer(void) {
    return rl_line_buffer;
}

void lusush_set_line_buffer(const char *new_content) {
    if (new_content) {
        rl_replace_line(new_content, 0);
        rl_point = strlen(new_content);
    }
}

// ============================================================================
// ERROR HANDLING AND DEBUGGING
// ============================================================================

const char *lusush_readline_get_error(void) {
    return NULL; // Readline doesn't provide specific error messages
}

void lusush_readline_set_debug(bool enabled) {
    debug_enabled = enabled;
}

bool lusush_readline_is_debug_enabled(void) {
    return debug_enabled;
}

// ============================================================================
// COMPATIBILITY LAYER
// ============================================================================

void lusush_add_completion(lusush_completions_t *lc, const char *completion) {
    if (!lc || !completion) return;
    
    // Resize if needed
    if (lc->len % 10 == 0) {
        char **new_cvec = realloc(lc->cvec, (lc->len + 10) * sizeof(char*));
        if (!new_cvec) return;
        lc->cvec = new_cvec;
    }
    
    lc->cvec[lc->len++] = strdup(completion);
}

void lusush_free_completions(lusush_completions_t *lc) {
    if (!lc) return;
    
    for (size_t i = 0; i < lc->len; i++) {
        free(lc->cvec[i]);
    }
    free(lc->cvec);
    lc->cvec = NULL;
    lc->len = 0;
}

// ============================================================================
// INTEGRATION HOOKS
// ============================================================================

void lusush_set_pre_input_hook(lusush_pre_input_hook_t hook) {
    pre_input_hook = hook;
}

void lusush_set_post_input_hook(lusush_post_input_hook_t hook) {
    post_input_hook = hook;
}

// ============================================================================
// CUSTOM INPUT HANDLING
// ============================================================================

static int lusush_getc(FILE *stream) {
    int c = getc(stream);
    
    // Handle EOF properly
    if (c == EOF) {
        if (feof(stream)) {
            // Real EOF
            return EOF;
        } else if (ferror(stream)) {
            // Error reading
            clearerr(stream);
            return EOF;
        }
    }
    
    // Real-time highlighting disabled to prevent display issues
    // Character-based triggering causes prompt spam
    
    return c;
}
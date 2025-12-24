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
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "readline_integration.h"
#include "display_integration.h"
#include "ht.h"
#include "lusush.h"
#include "prompt.h"
#include "themes.h"
#include "completion.h"
#include "config.h"
#include "symtable.h"
#include "lle/lle_readline.h"
#include "posix_history.h"

#include "lusush.h"
#include "init.h"
#include "input.h"

// Forward declarations
bool is_emacs_mode_enabled(void);
bool is_vi_mode_enabled(void);
static void update_readline_editing_mode(void);

// Global state
static bool initialized = false;
static char *current_prompt = NULL;
static char *history_file_path = NULL;
static bool debug_enabled = false;

// Hooks
static lusush_pre_input_hook_t pre_input_hook = NULL;
static lusush_post_input_hook_t post_input_hook = NULL;
static lusush_prompt_callback_t prompt_callback = NULL;

// History deduplication support
static char **history_cache = NULL;
static int history_cache_size = 0;

// Forward declarations for new layered display integration functions
static char *lusush_readline_with_layered_display(const char *prompt);
static int history_cache_capacity = 0;

// Forward declarations
static void setup_readline_config(void);
static void setup_key_bindings(void);
static char **lusush_tab_completion(const char *text, int start, int end);
static char *lusush_completion_entry_function(const char *text, int state);
static char **lusush_git_subcommand_completion(const char *text);
static char **lusush_config_subcommand_completion(const char *text);
static char **lusush_display_subcommand_completion(const char *text, const char *line_buffer);
static char **lusush_history_subcommand_completion(const char *text);
static char **lusush_alias_completion(const char *text);
static char **lusush_path_completion(const char *text);
static char **lusush_directory_only_completion(const char *text);
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
    
    // Free current prompt with safety check
    if (current_prompt) {
        // Safety: Check if this pointer was already freed or is invalid
        // by checking if it's the same as what we might have allocated
        char *temp_prompt = current_prompt;
        current_prompt = NULL;  // Clear pointer first to prevent double free
        free(temp_prompt);
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
    
    // Enhanced Performance Monitoring: Start timing for readline operation
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    // Call pre-input hook
    if (pre_input_hook) {
        pre_input_hook();
    }
    
    // Generate prompt using Lusush theming if no specific prompt provided
    const char *actual_prompt = prompt;
    char *themed_prompt = NULL;
    if (!prompt || (prompt_callback && strcmp(prompt, "$ ") == 0)) {
        themed_prompt = lusush_generate_prompt();
        if (themed_prompt) {
            actual_prompt = themed_prompt;
        }
    }
    
    // MINIMAL INTEGRATION: Only enhance primary prompts, preserve established input.c flow
    char *line = NULL;
    
    // Line editor selection: Use LLE if configured, otherwise GNU readline
    // Note: config.use_lle requires shell restart to take effect
    if (config.use_lle) {
        // Use LLE (Lusush Line Editor)
        line = lle_readline(actual_prompt);
    } else {
        // Use GNU readline (default)
        // CRITICAL FIX: Always use standard readline to prevent history corruption
        // Layered display deviation was causing history access isolation
        // Keep single unified readline path for consistent history behavior
        line = readline(actual_prompt);
    }
    
    // Note: Do not free themed_prompt here - it's managed by readline cleanup system
    // The themed_prompt becomes current_prompt and will be freed in lusush_readline_cleanup()
    
    // Handle EOF with ignoreeof option
    if (!line && shell_opts.ignoreeof && is_interactive_shell()) {
        printf("Use \"exit\" to leave the shell.\n");
        fflush(stdout);
        clearerr(stdin); // Clear EOF condition so readline can continue
        return strdup(""); // Return empty string to continue shell loop
    }
    
    // Debug: Track what commands are being entered in interactive mode

    
    // STABILITY FIX: Remove Enter-key syntax highlighting
    // Real-time word-boundary highlighting is now implemented instead
    // if (line && *line && config.enhanced_display_mode && syntax_highlighting_enabled) {
    //     lusush_show_command_syntax_preview(line);
    // }
    
    // Call post-input hook
    if (post_input_hook) {
        post_input_hook(line);
    }
    
    // Add to history if non-empty and in interactive mode
    // Note: Only use GNU readline history API when using GNU readline
    // LLE will implement its own history system in Spec 09
    if (line && *line && is_interactive_shell() && !config.use_lle) {
        lusush_history_add(line);
    }
    
    // Enhanced Performance Monitoring: Record readline operation timing
    gettimeofday(&end_time, NULL);
    uint64_t operation_time_ns = ((uint64_t)(end_time.tv_sec - start_time.tv_sec)) * 1000000000ULL +
                                 ((uint64_t)(end_time.tv_usec - start_time.tv_usec)) * 1000ULL;
    display_integration_record_display_timing(operation_time_ns);
    
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

MAYBE_UNUSED
static bool is_duplicate_in_cache(const char *line) {
    if (!history_cache || !line) return false;
    
    for (int i = 0; i < history_cache_size; i++) {
        if (history_cache[i] && strcmp(history_cache[i], line) == 0) {
            return true;
        }
    }
    return false;
}

MAYBE_UNUSED
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
    
    /* When LLE is enabled, it manages its own history - do not use GNU Readline history */
    extern config_values_t config;
    if (config.use_lle) {
        return;
    }
    
    // Skip whitespace-only lines
    const char *trimmed = line;
    while (*trimmed && isspace(*trimmed)) trimmed++;
    if (!*trimmed) return;
    
    // Check nolog option: prevent function definitions from entering history
    bool skip_history = false;
    if (shell_opts.nolog) {
        // Simple detection for function definitions: look for pattern "name()" or "name ()"
        const char *paren_pos = strstr(trimmed, "()");
        if (paren_pos) {
            // Check if there's a valid function name before the parentheses
            const char *name_start = trimmed;
            const char *name_end = paren_pos;
            
            // Skip backward past any whitespace before ()
            while (name_end > name_start && isspace(*(name_end - 1))) {
                name_end--;
            }
            
            // Check if we have a valid identifier before the ()
            if (name_end > name_start) {
                const char *p = name_end - 1;
                // Check if the character before () is part of a valid identifier
                if (isalnum(*p) || *p == '_') {
                    skip_history = true; // Skip adding function definitions to history
                }
            }
        }
    }
    
    // Check for duplicates if hist_no_dups is enabled (skip for function definitions when nolog is enabled)
    if (config.history_no_dups && !skip_history) {
        // Check recent history (last 50 entries) and remove old duplicates
        for (int i = 1; i <= 50 && i <= history_length; i++) {
            HIST_ENTRY *entry = history_get(history_length - i + 1);
            if (entry && entry->line && strcmp(entry->line, line) == 0) {
                // Found duplicate - remove the old entry to keep the new one
                HIST_ENTRY *removed = remove_history(history_length - i);
                if (removed) {
                    free(removed->line);
                    free(removed->data);
                    free(removed);
                }
                break; // Only remove the first (most recent) duplicate found
            }
        }
    }
    
    // Add to readline history (skip for function definitions when nolog is enabled)
    if (!skip_history) {
        add_history(line);
    }
    
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
    if (!history_file_path) return;
    
    // Check if we have any history to save
    HIST_ENTRY **hist_list = history_list();
    if (!hist_list || history_length == 0) {
        return;
    }
    
    // Create backup file path for safety
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "%s.tmp", history_file_path);
    
    // Try manual writing first (more reliable)
    FILE *file = fopen(backup_path, "w");
    if (!file) {
        // Try creating directory if it doesn't exist
        char *dir_end = strrchr(history_file_path, '/');
        if (dir_end) {
            *dir_end = '\0';
            mkdir(history_file_path, 0755);
            *dir_end = '/';
        }
        file = fopen(backup_path, "w");
        if (!file) return;
    }
    
    // Write all history entries
    int entries_written = 0;
    for (int i = 0; hist_list[i]; i++) {
        if (hist_list[i]->line && strlen(hist_list[i]->line) > 0) {
            fprintf(file, "%s\n", hist_list[i]->line);
            entries_written++;
        }
    }
    
    // Force flush and close
    fflush(file);
    fsync(fileno(file));
    fclose(file);
    
    // Atomically move backup to actual file
    if (entries_written > 0) {
        rename(backup_path, history_file_path);
    } else {
        unlink(backup_path);
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
        // History loaded successfully
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
    
    // Determine if we're completing a file argument (simple check)
    bool is_file_context = false;
    if (start == 0 || (start == (int)(cmd_start - line_buffer))) {
        // First word - command completion (not file)
        is_file_context = false;
    } else if (text && text[0] != '$' && text[0] != '-') {
        // Not a variable or option - likely a file argument
        is_file_context = true;
    }
    
    // Context-aware completion for specific commands takes priority
    if (cmd_len == 3 && memcmp(cmd_start, "git", 3) == 0 && start >= 4) {
        matches = lusush_git_subcommand_completion(text);
        if (matches) return matches;
    }
    
    if (cmd_len == 2 && memcmp(cmd_start, "cd", 2) == 0 && start >= 3) {
        matches = lusush_directory_only_completion(text);
        if (matches) return matches;
    }
    
    if (cmd_len == 6 && memcmp(cmd_start, "config", 6) == 0 && start >= 7) {
        matches = lusush_config_subcommand_completion(text);
        if (matches) return matches;
    }
    
    // display command - supports nested subcommands (display lle, display performance)
    if (cmd_len == 7 && memcmp(cmd_start, "display", 7) == 0 && start >= 8) {
        matches = lusush_display_subcommand_completion(text, line_buffer);
        if (matches) return matches;
    }
    
    // history command subcommands
    if (cmd_len == 7 && memcmp(cmd_start, "history", 7) == 0 && start >= 8) {
        matches = lusush_history_subcommand_completion(text);
        if (matches) return matches;
    }
    
    // unalias command - complete with defined alias names
    if (cmd_len == 7 && memcmp(cmd_start, "unalias", 7) == 0 && start >= 8) {
        matches = lusush_alias_completion(text);
        if (matches) return matches;
    }
    
    // source/. command - complete with files only (not directories)
    if ((cmd_len == 6 && memcmp(cmd_start, "source", 6) == 0 && start >= 7) ||
        (cmd_len == 1 && *cmd_start == '.' && start >= 2)) {
        matches = lusush_path_completion(text);
        if (matches) return matches;
    }
    
    // Final simple attempt: enable path completion for all file contexts
    if (is_file_context && text && strlen(text) > 0) {
        matches = lusush_path_completion(text);
        if (matches) return matches;
    }

    // NOTE: Rich completion system was removed in v1.3.0 cleanup.
    // LLE has its own completion system in src/lle/completion/.
    
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
    
    // Fallback: if no specific completion was handled, use readline's default file completion
    if (matches == NULL) {
        matches = rl_completion_matches(text, rl_filename_completion_function);
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
 * @brief Config subcommand completion for config command
 */
static char **lusush_config_subcommand_completion(const char *text) {
    static const char *config_subcommands[] = {
        "show", "set", "get", "reload", "save", NULL
    };
    
    char **matches = NULL;
    int count = 0;
    size_t text_len = strlen(text);
    
    // Count matching subcommands
    for (int i = 0; config_subcommands[i]; i++) {
        if (strncmp(text, config_subcommands[i], text_len) == 0) {
            count++;
        }
    }
    
    if (count == 0) return NULL;
    
    // Allocate matches array
    matches = malloc((count + 2) * sizeof(char*));
    if (!matches) return NULL;
    
    // Find common prefix for substitution
    if (count == 1) {
        // Single match - find the matching command
        for (int i = 0; config_subcommands[i]; i++) {
            if (strncmp(text, config_subcommands[i], text_len) == 0) {
                matches[0] = strdup(config_subcommands[i]);
                matches[1] = strdup(config_subcommands[i]);
                matches[2] = NULL;
                break;
            }
        }
    } else {
        // Multiple matches - set substitution text to input text
        matches[0] = strdup(text);
        
        // Add all matching commands
        int match_idx = 1;
        for (int i = 0; config_subcommands[i] && match_idx <= count; i++) {
            if (strncmp(text, config_subcommands[i], text_len) == 0) {
                matches[match_idx] = strdup(config_subcommands[i]);
                match_idx++;
            }
        }
        matches[match_idx] = NULL;
    }
    
    return matches;
}

/**
 * @brief Display subcommand completion for display command
 * Supports nested subcommands like "display lle keybindings"
 */
static char **lusush_display_subcommand_completion(const char *text, const char *line_buffer) {
    static const char *display_subcommands[] = {
        "status", "config", "stats", "diagnostics", "lle", 
        "performance", "test", "help", NULL
    };
    
    static const char *display_lle_subcommands[] = {
        "enable", "disable", "status", "keybindings", "diagnostics",
        "autosuggestions", "syntax", "multiline", "history-import", NULL
    };
    
    static const char *display_performance_subcommands[] = {
        "init", "report", "layers", "memory", "baseline", 
        "reset", "targets", "monitoring", "debug", NULL
    };
    
    const char **subcommands = display_subcommands;
    
    // Check for nested subcommand context
    if (line_buffer) {
        // Check for "display lle " context
        if (strstr(line_buffer, "display lle ") == line_buffer ||
            strstr(line_buffer, "display  lle ")) {
            subcommands = display_lle_subcommands;
        }
        // Check for "display performance " context  
        else if (strstr(line_buffer, "display performance ") == line_buffer ||
                 strstr(line_buffer, "display  performance ")) {
            subcommands = display_performance_subcommands;
        }
    }
    
    char **matches = NULL;
    int count = 0;
    size_t text_len = strlen(text);
    
    // Count matching subcommands
    for (int i = 0; subcommands[i]; i++) {
        if (strncmp(text, subcommands[i], text_len) == 0) {
            count++;
        }
    }
    
    if (count == 0) return NULL;
    
    // Allocate matches array
    matches = malloc((count + 2) * sizeof(char*));
    if (!matches) return NULL;
    
    if (count == 1) {
        // Single match
        for (int i = 0; subcommands[i]; i++) {
            if (strncmp(text, subcommands[i], text_len) == 0) {
                matches[0] = strdup(subcommands[i]);
                matches[1] = strdup(subcommands[i]);
                matches[2] = NULL;
                break;
            }
        }
    } else {
        // Multiple matches
        matches[0] = strdup(text);
        int match_idx = 1;
        for (int i = 0; subcommands[i] && match_idx <= count; i++) {
            if (strncmp(text, subcommands[i], text_len) == 0) {
                matches[match_idx] = strdup(subcommands[i]);
                match_idx++;
            }
        }
        matches[match_idx] = NULL;
    }
    
    return matches;
}

/**
 * @brief History subcommand completion for history command
 */
static char **lusush_history_subcommand_completion(const char *text) {
    static const char *history_subcommands[] = {
        "show", "clear", "delete", "search", NULL
    };
    
    char **matches = NULL;
    int count = 0;
    size_t text_len = strlen(text);
    
    // Count matching subcommands
    for (int i = 0; history_subcommands[i]; i++) {
        if (strncmp(text, history_subcommands[i], text_len) == 0) {
            count++;
        }
    }
    
    if (count == 0) return NULL;
    
    // Allocate matches array
    matches = malloc((count + 2) * sizeof(char*));
    if (!matches) return NULL;
    
    if (count == 1) {
        for (int i = 0; history_subcommands[i]; i++) {
            if (strncmp(text, history_subcommands[i], text_len) == 0) {
                matches[0] = strdup(history_subcommands[i]);
                matches[1] = strdup(history_subcommands[i]);
                matches[2] = NULL;
                break;
            }
        }
    } else {
        matches[0] = strdup(text);
        int match_idx = 1;
        for (int i = 0; history_subcommands[i] && match_idx <= count; i++) {
            if (strncmp(text, history_subcommands[i], text_len) == 0) {
                matches[match_idx] = strdup(history_subcommands[i]);
                match_idx++;
            }
        }
        matches[match_idx] = NULL;
    }
    
    return matches;
}

/**
 * @brief Alias name completion for alias/unalias commands
 */
static char **lusush_alias_completion(const char *text) {
    extern ht_strstr_t *aliases;
    
    if (!aliases) return NULL;
    
    char **matches = NULL;
    int count = 0;
    size_t text_len = strlen(text);
    
    // First pass: count matching aliases
    ht_enum_t *e = ht_strstr_enum_create(aliases);
    if (!e) return NULL;
    
    const char *key;
    const char *value;
    
    while (ht_strstr_enum_next(e, &key, &value)) {
        if (strncmp(text, key, text_len) == 0) {
            count++;
        }
    }
    ht_strstr_enum_destroy(e);
    
    if (count == 0) return NULL;
    
    // Allocate matches array
    matches = malloc((count + 2) * sizeof(char*));
    if (!matches) return NULL;
    
    // Second pass: collect matching aliases
    e = ht_strstr_enum_create(aliases);
    if (!e) {
        free(matches);
        return NULL;
    }
    
    if (count == 1) {
        while (ht_strstr_enum_next(e, &key, &value)) {
            if (strncmp(text, key, text_len) == 0) {
                matches[0] = strdup(key);
                matches[1] = strdup(key);
                matches[2] = NULL;
                break;
            }
        }
    } else {
        matches[0] = strdup(text);
        int match_idx = 1;
        while (ht_strstr_enum_next(e, &key, &value) && match_idx <= count) {
            if (strncmp(text, key, text_len) == 0) {
                matches[match_idx] = strdup(key);
                match_idx++;
            }
        }
        matches[match_idx] = NULL;
    }
    ht_strstr_enum_destroy(e);
    
    return matches;
}

/**
 * @brief Simplified path completion using readline default behavior
 */
static char **lusush_path_completion(const char *text) {
    // Use readline's default file completion without complex filtering
    return rl_completion_matches(text, rl_filename_completion_function);
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
    static bool in_prompt_generation = false;
    
    // Prevent infinite recursion
    if (in_prompt_generation) {
        return strdup("$ ");
    }
    
    in_prompt_generation = true;
    
    // STABILITY FIX: Disable enhanced prompt in enhanced display mode to prevent corruption
    // Phase 1: Enable layered display integration for prompt generation
    // Use enhanced prompt when layered display is available
    if (display_integration_is_layered_active()) {
        char *enhanced_prompt = NULL;
        if (display_integration_get_enhanced_prompt(&enhanced_prompt)) {
            if (enhanced_prompt) {
                in_prompt_generation = false;
                return enhanced_prompt;
            }
        }
    }
    
    // Fallback to standard prompt generation
    if (prompt_callback) {
        char *result = prompt_callback();
        in_prompt_generation = false;
        return result;
    }
    
    // Use basic Lusush prompt building system
    build_prompt();
    
    // Get the generated prompt from symbol table
    const char *ps1 = symtable_get_global_default("PS1", "$ ");
    
    // Make a copy for readline
    current_prompt = strdup(ps1 ? ps1 : "$ ");
    in_prompt_generation = false;
    return current_prompt;
}

void lusush_prompt_update(void) {
    // STABILITY FIX: Use standard prompt updates to prevent display corruption
    // Enhanced display mode should not interfere with prompt generation
    // Phase 2: Enable layered display integration for prompt updates
    // Use layered display for prompt updates
    if (display_integration_is_layered_active()) {
        display_integration_prompt_update();
        return;
    }
    
    // Use stable fallback for all modes
    rebuild_prompt();
    lusush_generate_prompt();
}

void lusush_prompt_set_callback(lusush_prompt_callback_t callback) {
    prompt_callback = callback;
}



// ============================================================================
// KEY BINDINGS AND SHORTCUTS
// ============================================================================

// Custom function to abort current line (Ctrl+G)
static int lusush_abort_line(int count, int key) {
    (void)count; (void)key;
    
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
    
    return 0;
}

    // Ctrl-R uses standard readline reverse search

// Custom function to clear screen (Ctrl+L)
static int lusush_clear_screen_and_redisplay(int count, int key) {
    (void)count; (void)key;
    
    // Enhanced Performance Monitoring: Start timing for clear screen operation
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    // Clear screen with ANSI codes like clear command
    printf("\033[2J\033[H");  // Clear screen and home cursor
    fflush(stdout);
    
    // Tell readline to redraw prompt and current line
    rl_forced_update_display();
    
    // Enhanced Performance Monitoring: Record clear screen timing
    gettimeofday(&end_time, NULL);
    uint64_t operation_time_ns = ((uint64_t)(end_time.tv_sec - start_time.tv_sec)) * 1000000000ULL +
                                 ((uint64_t)(end_time.tv_usec - start_time.tv_usec)) * 1000ULL;
    display_integration_record_display_timing(operation_time_ns);
    
    return 0;
}

// Custom history navigation with artifact prevention
static int lusush_previous_history(int count, int key) {
    (void)count; (void)key;
    
    // Clear layered autosuggestions before history navigation
    display_integration_clear_autosuggestions();
    
    // Move to previous history entry - let readline handle the display
    return rl_get_previous_history(count, key);
}

static int lusush_next_history(int count, int key) {
    (void)count; (void)key;
    
    // Check if we have content BEFORE calling rl_get_next_history (which might clear it)
    bool has_content = rl_line_buffer && strlen(rl_line_buffer) > 0;
    
    // Try to check if we can move forward in history
    HIST_ENTRY *next = next_history();
    if (next) {
        // We can move forward, put it back and use normal navigation
        previous_history();

        // Clear layered autosuggestions before history navigation
        display_integration_clear_autosuggestions();
        
        return rl_get_next_history(count, key);
    } else {
        // We're at the end of history
        if (has_content) {
            // Clear layered autosuggestions before clearing line
            display_integration_clear_autosuggestions();
            
            // Let readline handle line clearing properly to avoid display corruption
            rl_replace_line("", 0);
            rl_point = 0;
            rl_end = 0;
            rl_redisplay();
        }
        return 0;
    }
}

static void setup_key_bindings(void) {
    // Ctrl key bindings
    rl_bind_key(1, rl_beg_of_line);   // Ctrl-A: beginning of line
    rl_bind_key(5, rl_end_of_line);   // Ctrl-E: end of line
    rl_bind_key(7, lusush_abort_line); // Ctrl-G: abort/cancel line
    rl_bind_key(12, lusush_clear_screen_and_redisplay); // Ctrl-L: clear screen
    rl_bind_key(21, rl_unix_line_discard); // Ctrl-U: kill line
    rl_bind_key(11, rl_kill_line);    // Ctrl-K: kill to end
    rl_bind_key(23, rl_unix_word_rubout); // Ctrl-W: kill word
    // Ctrl-R uses standard readline reverse search (rl_reverse_search_history)
    
    // History navigation with clean up/down arrow handling
    rl_bind_keyseq("\\e[A", lusush_previous_history); // Up arrow key
    rl_bind_keyseq("\\e[B", lusush_next_history); // Down arrow key
    
    // Standard key bindings for cursor movement
    rl_bind_key(CTRL('F'), rl_forward_char);     // Ctrl+F: forward character
    rl_bind_keyseq("\\e[C", rl_forward_char);    // Right arrow: forward character
    rl_bind_keyseq("\\e[1;5C", rl_forward_word); // Ctrl+Right arrow: forward word
    
    // Set initial editing mode
    update_readline_editing_mode();
}

void lusush_keybindings_setup(void) {
    setup_key_bindings();
}

// Update readline editing mode based on vi/emacs options
static void update_readline_editing_mode(void) {
    if (is_vi_mode_enabled()) {
        rl_editing_mode = 0; // vi mode
    } else {
        rl_editing_mode = 1; // emacs mode (default)
    }
}

// Public function to update editing mode when option changes
void lusush_update_editing_mode(void) {
    if (initialized) {
        update_readline_editing_mode();
    }
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
    
    // Use standard readline redisplay function for v1.3.0 stability
    // Direct assignment to rl_redisplay ensures proper line wrapping
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
    // Use layered display for clear screen
    if (display_integration_is_layered_active()) {
        display_integration_clear_screen();
        return;
    }
    
    // Desperate fallback only
    rl_clear_screen(0, 0);
}

void lusush_refresh_line(void) {
    // Use layered display for line refresh
    if (display_integration_is_layered_active()) {
        if (debug_enabled) {
            fprintf(stderr, "[READLINE_DEBUG] lusush_refresh_line: calling display_integration_redisplay()\n");
        }
        display_integration_redisplay();
        return;
    }
    
    // Desperate fallback only
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

/**
 * Enhanced readline with layered display integration
 * Provides graceful fallback to standard readline on any failure
 */
MAYBE_UNUSED
static char *lusush_readline_with_layered_display(const char *prompt) {
    if (!prompt) {
        return NULL;  // Safety check - fallback to standard readline
    }
    
    // Safety: Ensure display integration is actually available
    if (!display_integration_is_layered_active()) {
        if (debug_enabled) {
            fprintf(stderr, "[READLINE_DEBUG] Layered display not active during readline call\n");
        }
        return NULL;  // Fallback to standard readline
    }
    
    // NEW APPROACH: Use layered display system to generate the prompt content,
    // then let readline handle ALL display and cursor management
    // This respects readline's architecture instead of fighting it
    
    static char layered_prompt_buffer[4096];
    layered_prompt_buffer[0] = '\0';
    
    // Generate themed prompt using the existing lusush prompt system
    // This will create the multiline prompt content properly
    char *generated_prompt = lusush_generate_prompt();
    
    if (generated_prompt) {
        // Use the generated prompt instead of the basic one
        strncpy(layered_prompt_buffer, generated_prompt, sizeof(layered_prompt_buffer) - 1);
        layered_prompt_buffer[sizeof(layered_prompt_buffer) - 1] = '\0';
        
        if (debug_enabled) {
            fprintf(stderr, "[READLINE_DEBUG] Using generated layered prompt (length: %zu)\n", 
                    strlen(layered_prompt_buffer));
        }
        
        // Let readline handle the entire prompt display and cursor positioning
        // This ensures multiline prompts work correctly with readline's systems
        char *line = readline(layered_prompt_buffer);
        
        return line;
    } else {
        if (debug_enabled) {
            fprintf(stderr, "[READLINE_DEBUG] Failed to generate layered prompt, using fallback\n");
        }
        return NULL;  // Fallback to standard readline
    }
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

    // Handle EOF properly - let readline handle EINTR internally
    // Fighting readline's signal handling causes more problems than it solves
    if (c == EOF) {
        if (feof(stream)) {
            return EOF;
        } else if (ferror(stream)) {
            clearerr(stream);
            return EOF;
        }
    }

    return c;
}
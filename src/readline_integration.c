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
#include <readline/readline.h>
#include <readline/history.h>

#include "../include/readline_integration.h"
#include "../include/display_integration.h"
#include "../include/libhashtable/ht.h"
#include "../include/lusush.h"
#include "../include/prompt.h"
#include "../include/themes.h"
#include "../include/completion.h"
#include "../include/config.h"
#include "../include/symtable.h"
#include "../include/lle/lle_readline.h"
#include "../include/posix_history.h"
#include "../include/autosuggestions.h"
#include "../include/rich_completion.h"
#include "../include/lusush.h"
#include "../include/init.h"
#include "../include/input.h"

// Forward declarations
bool is_emacs_mode_enabled(void);
bool is_vi_mode_enabled(void);
static void update_readline_editing_mode(void);

// Global state
static bool initialized = false;
static char *current_prompt = NULL;
static char *history_file_path = NULL;
static bool syntax_highlighting_enabled = false;
static bool debug_enabled = false;
static bool is_multiline_theme = false;

// Syntax highlighting colors (without readline escape sequences for direct output)
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

// Autosuggestion support
static lusush_autosuggestion_t *current_suggestion = NULL;
static int history_cache_size = 0;

// Forward declarations for new layered display integration functions
static char *lusush_readline_with_layered_display(const char *prompt);
static void try_layered_display_prompt(const char *prompt);
static int history_cache_capacity = 0;

// Track continuation prompt transitions to clear autosuggestions
static bool was_in_continuation_prompt = false;

// Phase 3: Performance Optimization - Change Detection System
typedef struct {
    char *cached_line;
    size_t cached_length;
    size_t last_cursor_pos;
    uint32_t line_hash;
    bool cache_valid;
    clock_t last_update_time;
} change_detector_t;

static change_detector_t change_cache = {NULL, 0, 0, 0, false, 0};

// Phase 3: Region-based highlighting for incremental updates
typedef struct {
    size_t start_pos;
    size_t end_pos;
    bool full_line;
} highlight_region_t;

// Phase 3: Performance monitoring
typedef struct {
    uint64_t total_updates;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t incremental_updates;
    uint64_t full_updates;
    double total_update_time;
    double peak_update_time;
    double avg_update_time;
} perf_stats_t;

static perf_stats_t perf_stats = {0, 0, 0, 0, 0, 0.0, 0.0, 0.0};

// Forward declarations
static void setup_readline_config(void);
static void setup_key_bindings(void);
static char **lusush_tab_completion(const char *text, int start, int end);
static char *lusush_completion_entry_function(const char *text, int state);
static void apply_syntax_highlighting(void);
static void lusush_simple_syntax_display(void);
static char **lusush_git_subcommand_completion(const char *text);
static char **lusush_config_subcommand_completion(const char *text);
static char **lusush_display_subcommand_completion(const char *text, const char *line_buffer);
static char **lusush_history_subcommand_completion(const char *text);
static char **lusush_alias_completion(const char *text);
static char **lusush_path_completion(const char *text);
static char **lusush_directory_only_completion(const char *text);
static bool lusush_is_shell_keyword(const char *word, size_t length);
static bool lusush_is_shell_builtin(const char *word, size_t length);
static bool lusush_is_word_separator(char c);
static void lusush_custom_redisplay(void);
static void lusush_redisplay_with_suggestions(void);
// New function for wrapped output
static void lusush_output_colored_line_wrapped(const char *line, int available_width) {
    if (!line) return;
    
    // Color definitions
    const char *cmd_color = "\033[1;32m";    // Bright green for commands
    const char *str_color = "\033[1;33m";    // Yellow for strings
    const char *var_color = "\033[1;35m";    // Magenta for variables
    const char *keyword_color = "\033[1;34m"; // Blue for keywords
    const char *reset = "\033[0m";
    
    size_t len = strlen(line);
    size_t pos = 0;
    int current_col = 0;
    bool in_string = false;
    char string_delimiter = 0;
    bool first_word = true;
    
    while (pos < len) {
        char c = line[pos];
        
        // Handle line wrapping
        if (current_col >= available_width && available_width > 0) {
            printf("\n");
            current_col = 0;
        }
        
        // Handle strings
        if (!in_string && (c == '"' || c == '\'' || c == '`')) {
            in_string = true;
            string_delimiter = c;
            printf("%s%c", str_color, c);
            current_col++;
            pos++;
            continue;
        }
        
        if (in_string) {
            if (c == string_delimiter) {
                printf("%c%s", c, reset);
                in_string = false;
            } else {
                printf("%c", c);
            }
            current_col++;
            pos++;
            continue;
        }
        
        // Handle variables
        if (c == '$') {
            printf("%s$", var_color);
            current_col++;
            pos++;
            
            // Highlight variable name
            while (pos < len && (isalnum(line[pos]) || line[pos] == '_')) {
                printf("%c", line[pos]);
                current_col++;
                if (current_col >= available_width && available_width > 0) {
                    printf("\n");
                    current_col = 0;
                }
                pos++;
            }
            printf("%s", reset);
            continue;
        }
        
        // Handle words (potential commands/keywords)
        if (isalpha(c) && (pos == 0 || isspace(line[pos-1]) || line[pos-1] == '|' || line[pos-1] == '&' || line[pos-1] == ';')) {
            size_t word_start = pos;
            size_t word_end = pos;
            
            // Find end of word
            while (word_end < len && (isalnum(line[word_end]) || line[word_end] == '_' || line[word_end] == '-')) {
                word_end++;
            }
            
            // Extract word
            char word[64];
            size_t word_len = word_end - word_start;
            if (word_len < sizeof(word)) {
                strncpy(word, line + word_start, word_len);
                word[word_len] = '\0';
                
                // Check if it's a keyword or command
                const char *color = NULL;
                if (first_word || (pos > 0 && (line[pos-1] == '|' || line[pos-1] == '&' || line[pos-1] == ';'))) {
                    // First word or after pipe/semicolon is likely a command
                    if (strcmp(word, "if") == 0 || strcmp(word, "then") == 0 || strcmp(word, "else") == 0 || 
                        strcmp(word, "elif") == 0 || strcmp(word, "fi") == 0 || strcmp(word, "for") == 0 || 
                        strcmp(word, "while") == 0 || strcmp(word, "do") == 0 || strcmp(word, "done") == 0 ||
                        strcmp(word, "case") == 0 || strcmp(word, "esac") == 0) {
                        color = keyword_color;
                    } else {
                        color = cmd_color;
                    }
                }
                
                if (color) {
                    printf("%s", color);
                }
                
                // Print the word with wrapping
                for (size_t i = 0; i < word_len; i++) {
                    if (current_col >= available_width && available_width > 0) {
                        printf("\n");
                        current_col = 0;
                    }
                    printf("%c", word[word_start + i]);
                    current_col++;
                }
                
                if (color) {
                    printf("%s", reset);
                }
                
                pos = word_end;
                first_word = false;
                continue;
            }
        }
        
        // Handle regular characters
        if (!isspace(c)) {
            first_word = false;
        }
        
        printf("%c", c);
        current_col++;
        pos++;
    }
}

// Original function for compatibility  
static void lusush_output_colored_line(const char *line, int cursor_pos);
static int lusush_getc(FILE *stream);
static int lusush_getc_with_autosuggestions(FILE *stream);
static int lusush_abort_line(int count, int key);
static int lusush_clear_screen_and_redisplay(int count, int key);
static int lusush_previous_history(int count, int key);
static int lusush_next_history(int count, int key);
static int lusush_accept_suggestion_key(int count, int key);
static int lusush_accept_suggestion_word_key(int count, int key);

// Phase 3: Optimization function declarations
static bool init_change_detector(void);
static void cleanup_change_detector(void);
static bool needs_highlight_update(void);
static void update_change_cache(void);
static uint32_t calculate_line_hash(const char *line, size_t length);
static highlight_region_t calculate_update_region(void);
static size_t expand_to_word_boundary_left(size_t pos);
static size_t expand_to_word_boundary_right(size_t pos);
static double get_current_time_ms(void);
static void lusush_apply_optimized_highlighting(void);
static void lusush_get_highlight_performance_stats(uint64_t *cache_hits, uint64_t *cache_misses, uint64_t *total_updates, double *avg_time);
static void lusush_reset_highlight_performance_stats(void);
static void lusush_report_performance(void);

// Public interface
void lusush_show_highlight_performance(void);
void lusush_set_debug_enabled(bool enabled);


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
    
    // v1.3.0: Autosuggestions disabled for stability
    // lusush_autosuggestions_init() call removed
    
    // Initialize rich completion system
    if (!lusush_rich_completion_init()) {
        fprintf(stderr, "Warning: Failed to initialize rich completions\n");
    }
    
    // v1.3.0: Syntax highlighting disabled for stability
    // lusush_syntax_highlighting_set_enabled() call removed
    
    // Phase 3: Initialize change detection and performance monitoring
    init_change_detector();
    lusush_reset_highlight_performance_stats();
    
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
    
    // Cleanup autosuggestions safely
    if (current_suggestion) {
        lusush_free_autosuggestion(current_suggestion);
        current_suggestion = NULL;
    }
    // v1.3.0: Autosuggestions disabled for stability
    // lusush_autosuggestions_cleanup() call removed
    
    // Cleanup rich completion system
    lusush_rich_completion_cleanup();
    

    
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
    
    // Phase 3: Cleanup optimization systems
    cleanup_change_detector();
    
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
    
    // Determine completion context
    completion_context_t context = CONTEXT_MIXED;
    if (start == 0 || (start == (cmd_start - line_buffer))) {
        context = CONTEXT_COMMAND;  // First word - command completion
    } else if (start > 3) {
        // Argument completion - could be files, options, etc.
        if (text && text[0] == '$') {
            context = CONTEXT_VARIABLE;
        } else if (text && text[0] == '-') {
            context = CONTEXT_OPTION;
        } else {
            context = CONTEXT_FILE;
        }
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
    if (context == CONTEXT_FILE && text && strlen(text) > 0) {
        matches = lusush_path_completion(text);
        if (matches) return matches;
    }

    // Try rich completion system for other cases
    if (lusush_are_rich_completions_enabled()) {
        rich_completion_list_t *rich_completions = lusush_get_rich_completions(text, context);
        if (rich_completions && rich_completions->count > 0) {
            // Convert rich completions to readline format
            matches = malloc((rich_completions->count + 2) * sizeof(char*));
            if (matches) {
                // First element is the substitution text (common prefix)
                if (rich_completions->count == 1) {
                    matches[0] = strdup(rich_completions->items[0]->completion);
                } else {
                    // Find common prefix
                    size_t prefix_len = strlen(rich_completions->items[0]->completion);
                    for (size_t i = 1; i < rich_completions->count; i++) {
                        size_t j = 0;
                        while (j < prefix_len && 
                               j < strlen(rich_completions->items[i]->completion) &&
                               rich_completions->items[0]->completion[j] == 
                               rich_completions->items[i]->completion[j]) {
                            j++;
                        }
                        prefix_len = j;
                    }
                    
                    if (prefix_len > strlen(text)) {
                        matches[0] = malloc(prefix_len + 1);
                        if (matches[0]) {
                            strncpy(matches[0], rich_completions->items[0]->completion, prefix_len);
                            matches[0][prefix_len] = '\0';
                        }
                    } else {
                        matches[0] = strdup(text);
                    }
                }
                
                // Copy completions
                for (size_t i = 0; i < rich_completions->count; i++) {
                    matches[i + 1] = strdup(rich_completions->items[i]->completion);
                }
                matches[rich_completions->count + 1] = NULL;
                
                // Display rich completions if multiple matches and descriptions available
                // Temporarily disabled to prevent display corruption
                /*
                if (rich_completions->count > 1) {
                    printf("\n");
                    lusush_display_rich_completions(rich_completions);
                }
                */
            }
            
            lusush_free_rich_completions(rich_completions);
            if (matches) return matches;
        }
        if (rich_completions) {
            lusush_free_rich_completions(rich_completions);
        }
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
// SYNTAX HIGHLIGHTING API
// ============================================================================

// Configuration structure for smart triggering
typedef struct {
    int trigger_frequency;     // Every Nth character during typing (default: 3)
    int max_fps;              // Maximum updates per second (default: 30)
    bool debug_logging;       // Enable debug output (default: false)
} smart_trigger_config_t;

static smart_trigger_config_t trigger_config = {3, 30, false};

// Global state for intelligent triggering
static struct {
    int consecutive_alphas;
    int last_char;
    bool in_word;
    clock_t last_trigger_time;
} typing_state = {0, 0, false, 0};

// Forward declarations for smart character detection
static bool should_trigger_highlighting(int c);
static int lusush_syntax_update_hook(void);
static void reset_typing_state(void);
static bool is_word_boundary_char(int c);
static bool is_syntax_significant_char(int c);
static bool is_safe_for_highlighting(void);
static bool detect_multiline_theme(void);

static void lusush_safe_redisplay(void);

// Buffer management for safe highlighting
typedef struct {
    char *colored_buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    bool needs_realloc;
} highlight_buffer_t;

static highlight_buffer_t highlight_buf = {NULL, 0, 0, false};

// Initialize highlighting buffer
static bool init_highlight_buffer(size_t initial_size) {
    if (highlight_buf.colored_buffer) {
        free(highlight_buf.colored_buffer);
    }
    
    highlight_buf.buffer_capacity = initial_size * 10; // Extra space for color codes
    highlight_buf.colored_buffer = malloc(highlight_buf.buffer_capacity);
    
    if (!highlight_buf.colored_buffer) {
        highlight_buf.buffer_capacity = 0;
        return false;
    }
    
    highlight_buf.buffer_size = 0;
    highlight_buf.needs_realloc = false;
    return true;
}

// Clean up highlighting buffer
static void cleanup_highlight_buffer(void) {
    if (highlight_buf.colored_buffer) {
        free(highlight_buf.colored_buffer);
        highlight_buf.colored_buffer = NULL;
        highlight_buf.buffer_size = 0;
        highlight_buf.buffer_capacity = 0;
    }
}

// Ensure buffer has enough space
static bool ensure_buffer_capacity(size_t needed_size) {
    if (needed_size <= highlight_buf.buffer_capacity) {
        return true;
    }
    
    size_t new_capacity = needed_size * 2;
    char *new_buffer = realloc(highlight_buf.colored_buffer, new_capacity);
    
    if (!new_buffer) {
        return false;
    }
    
    highlight_buf.colored_buffer = new_buffer;
    highlight_buf.buffer_capacity = new_capacity;
    return true;
}

// Public function to configure smart triggering
void lusush_configure_smart_triggering(int frequency, int max_fps, bool debug) {
    trigger_config.trigger_frequency = frequency;
    trigger_config.max_fps = max_fps;
    trigger_config.debug_logging = debug;
    

}

// Public function to get current triggering statistics
void lusush_get_trigger_stats(int *total_triggers, int *chars_processed) {
    static int total_chars = 0;
    static int total_highlights = 0;
    
    // These would be updated in the actual triggering functions
    *total_triggers = total_highlights;
    *chars_processed = total_chars;
}

// Simple function to enable debug mode for testing
void lusush_enable_trigger_debug(void) {
    trigger_config.debug_logging = true;
}

void lusush_syntax_highlighting_set_enabled(bool enabled) {
    if (enabled) {
        // Enable real-time word-boundary syntax highlighting
        syntax_highlighting_enabled = true;
        
        // Use custom getc function to detect word boundaries for highlighting
        // Set custom getc for character-by-character processing with autosuggestions
        rl_getc_function = lusush_getc_with_autosuggestions;
        
        reset_typing_state();
        
        // Initialize highlighting buffer
        if (!init_highlight_buffer(1024)) {
            fprintf(stderr, "[ERROR] Failed to initialize highlighting buffer\n");
            syntax_highlighting_enabled = false;
            rl_getc_function = rl_getc;
            return;
        }
        
        // Use standard readline redisplay function for v1.3.0 stability
        rl_redisplay_function = rl_redisplay;

    } else {
        // Disable: clean up and use standard functions
        syntax_highlighting_enabled = false;
        cleanup_highlight_buffer();
        rl_redisplay_function = rl_redisplay;
        rl_getc_function = lusush_getc_with_autosuggestions;
        rl_pre_input_hook = NULL;
    }
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
    
    // Use enhanced syntax highlighting display when enabled
    // STABILITY FIX: Always use safe redisplay regardless of mode
    // Enhanced display mode syntax highlighting disabled to prevent constant redraws
    rl_redisplay();
    
    /* ORIGINAL CODE DISABLED FOR STABILITY
    if (config.enhanced_display_mode) {
        lusush_simple_syntax_display();
    } else {
        rl_redisplay();
    }
    */
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
        "typeset", "fc", "theme", "config", "debug",
        "display", "network", "clear", "termcap",
        "true", "false", "break", "continue", "return_value",
        "times", "getopts", "hash", ":"
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

// Robust syntax highlighting with proper line wrapping support
static void lusush_simple_syntax_display(void) {
    if (!rl_line_buffer || !config.enhanced_display_mode) {
        rl_redisplay();
        return;
    }
    
    // Get terminal dimensions for proper wrapping calculations
    int terminal_width = 80; // Safe default
    int terminal_height = 24;
    rl_get_screen_size(&terminal_height, &terminal_width);
    if (terminal_width <= 0) terminal_width = 80;
    
    // Calculate prompt length (without ANSI escape sequences)
    int prompt_len = rl_prompt ? rl_expand_prompt(rl_prompt) : 0;
    if (prompt_len < 0) prompt_len = 0;
    
    // Calculate total visual length
    size_t command_len = strlen(rl_line_buffer);
    int total_visual_len = prompt_len + command_len;
    
    // Calculate cursor position accounting for wrapping
    int cursor_visual_pos = prompt_len + rl_point;
    int cursor_line = cursor_visual_pos / terminal_width;
    int cursor_col = cursor_visual_pos % terminal_width;
    
    // Save current cursor position
    printf("\033[s");
    
    // Move to beginning of current line group
    if (cursor_line > 0) {
        printf("\033[%dA", cursor_line); // Move up cursor_line lines
    }
    printf("\r"); // Move to beginning of line
    
    // Clear all lines that might be affected
    int total_lines = (total_visual_len + terminal_width - 1) / terminal_width;
    for (int i = 0; i < total_lines; i++) {
        printf("\033[K"); // Clear current line
        if (i < total_lines - 1) {
            printf("\n"); // Move to next line
        }
    }
    
    // Move back to start
    if (total_lines > 1) {
        printf("\033[%dA", total_lines - 1);
    }
    printf("\r");
    
    // Print prompt
    if (rl_prompt) {
        printf("%s", rl_prompt);
    }
    
    // Apply syntax highlighting with proper wrapping
    lusush_output_colored_line_wrapped(rl_line_buffer, terminal_width - prompt_len);
    
    // Position cursor correctly
    int final_line = cursor_visual_pos / terminal_width;
    int final_col = cursor_visual_pos % terminal_width;
    
    // Move to correct line
    if (final_line > 0) {
        printf("\033[%dB", final_line);
    }
    
    // Move to correct column
    printf("\r");
    if (final_col > 0) {
        printf("\033[%dC", final_col);
    }
    
    fflush(stdout);
}

// Original function for compatibility
static void lusush_output_colored_line(const char *line, int cursor_pos __attribute__((unused))) {
    // Simple implementation without wrapping for compatibility
    if (!line) return;
    
    const char *cmd_color = "\033[1;32m";    // Bright green for commands
    const char *reset = "\033[0m";
    
    size_t len = strlen(line);
    bool first_word = true;
    
    for (size_t i = 0; i < len; i++) {
        char c = line[i];
        
        // Simple highlighting - first word as command
        if (first_word && isalpha(c)) {
            printf("%s", cmd_color);
            while (i < len && !isspace(line[i])) {
                printf("%c", line[i]);
                i++;
            }
            printf("%s", reset);
            first_word = false;
            i--; // Adjust for loop increment
        } else {
            printf("%c", c);
            if (!isspace(c)) first_word = false;
        }
    }
}

// Show syntax highlighted version of command when user presses Enter
void lusush_show_command_syntax_preview(const char *command) {
    // STABILITY FIX: Enable Enter-key syntax highlighting for enhanced mode
    if (!command || !*command || !config.enhanced_display_mode) {
        return;
    }
    
    // Show a brief syntax-highlighted version of what was just entered
    printf("   → ");
    lusush_output_colored_line(command, strlen(command));
    printf("\n");
    fflush(stdout); // Ensure output is visible immediately
    (void)command; // Suppress unused parameter warning
    return;
}

// ============================================================================
// PHASE 3: PERFORMANCE OPTIMIZATION FUNCTIONS
// ============================================================================

// Fast hash function for line content
static uint32_t calculate_line_hash(const char *line, size_t length) {
    if (!line || length == 0) return 0;
    
    uint32_t hash = 5381;
    for (size_t i = 0; i < length; i++) {
        hash = ((hash << 5) + hash) + (unsigned char)line[i];
    }
    return hash;
}

// Initialize change detection system
static bool init_change_detector(void) {
    if (change_cache.cached_line) {
        free(change_cache.cached_line);
    }
    
    change_cache.cached_line = NULL;
    change_cache.cached_length = 0;
    change_cache.last_cursor_pos = 0;
    change_cache.line_hash = 0;
    change_cache.cache_valid = false;
    change_cache.last_update_time = 0;
    
    return true;
}

// Clean up change detection system
static void cleanup_change_detector(void) {
    if (change_cache.cached_line) {
        free(change_cache.cached_line);
        change_cache.cached_line = NULL;
    }
    change_cache.cached_length = 0;
    change_cache.cache_valid = false;
}

// Check if highlighting update is needed
static bool needs_highlight_update(void) {
    if (!rl_line_buffer) {
        return change_cache.cache_valid; // Clear if we had content before
    }
    
    size_t current_length = strlen(rl_line_buffer);
    uint32_t current_hash = calculate_line_hash(rl_line_buffer, current_length);
    
    // Check for changes
    bool content_changed = (current_hash != change_cache.line_hash) ||
                          (current_length != change_cache.cached_length) ||
                          !change_cache.cache_valid;
    
    bool cursor_moved = (rl_point != (int)change_cache.last_cursor_pos);
    
    // Throttle updates to maximum refresh rate
    clock_t current_time = clock();
    bool throttle_ok = (current_time - change_cache.last_update_time) > 
                       (CLOCKS_PER_SEC / 60); // Max 60 FPS
    
    bool needs_update = (content_changed || cursor_moved) && throttle_ok;
    
    // Debug output for cache behavior
    if (debug_enabled && rl_line_buffer && strlen(rl_line_buffer) > 0) {

    }
    
    return needs_update;
}

// Update the change cache
static void update_change_cache(void) {
    if (!rl_line_buffer) {
        change_cache.cache_valid = false;
        return;
    }
    
    size_t current_length = strlen(rl_line_buffer);
    
    // Reallocate cache buffer if needed
    if (current_length + 1 > change_cache.cached_length) {
        char *new_cache = realloc(change_cache.cached_line, current_length + 1);
        if (!new_cache) {
            change_cache.cache_valid = false;
            return;
        }
        change_cache.cached_line = new_cache;
        change_cache.cached_length = current_length + 1;
    }
    
    // Update cache content
    strcpy(change_cache.cached_line, rl_line_buffer);
    change_cache.line_hash = calculate_line_hash(rl_line_buffer, current_length);
    change_cache.last_cursor_pos = rl_point;
    change_cache.cache_valid = true;
    change_cache.last_update_time = clock();
}

// Expand region to word boundary (left)
static size_t expand_to_word_boundary_left(size_t pos) {
    if (!rl_line_buffer || pos == 0) return 0;
    
    // Move left to find word/token boundary
    while (pos > 0 && !lusush_is_word_separator(rl_line_buffer[pos - 1]) &&
           rl_line_buffer[pos - 1] != '|' && rl_line_buffer[pos - 1] != '&' &&
           rl_line_buffer[pos - 1] != ';' && rl_line_buffer[pos - 1] != '<' &&
           rl_line_buffer[pos - 1] != '>') {
        pos--;
    }
    
    return pos;
}

// Expand region to word boundary (right)
static size_t expand_to_word_boundary_right(size_t pos) {
    if (!rl_line_buffer) return 0;
    
    size_t len = strlen(rl_line_buffer);
    if (pos >= len) return len;
    
    // Move right to find word/token boundary
    while (pos < len && !lusush_is_word_separator(rl_line_buffer[pos]) &&
           rl_line_buffer[pos] != '|' && rl_line_buffer[pos] != '&' &&
           rl_line_buffer[pos] != ';' && rl_line_buffer[pos] != '<' &&
           rl_line_buffer[pos] != '>') {
        pos++;
    }
    
    return pos;
}

// Calculate the minimal region that needs re-highlighting
static highlight_region_t calculate_update_region(void) {
    highlight_region_t region = {0, 0, true};
    
    if (!rl_line_buffer || !change_cache.cached_line || !change_cache.cache_valid) {
        // Full update needed
        region.full_line = true;
        region.start_pos = 0;
        region.end_pos = rl_line_buffer ? strlen(rl_line_buffer) : 0;
        return region;
    }
    
    size_t current_len = strlen(rl_line_buffer);
    size_t cached_len = strlen(change_cache.cached_line);
    
    // Find first difference
    size_t start_diff = 0;
    size_t min_len = (current_len < cached_len) ? current_len : cached_len;
    
    while (start_diff < min_len && 
           rl_line_buffer[start_diff] == change_cache.cached_line[start_diff]) {
        start_diff++;
    }
    
    // Find last difference
    size_t end_diff_current = current_len;
    size_t end_diff_cached = cached_len;
    
    while (end_diff_current > start_diff && end_diff_cached > start_diff &&
           rl_line_buffer[end_diff_current - 1] == change_cache.cached_line[end_diff_cached - 1]) {
        end_diff_current--;
        end_diff_cached--;
    }
    
    // Expand region to word boundaries for better syntax highlighting
    size_t region_start = expand_to_word_boundary_left(start_diff);
    size_t region_end = expand_to_word_boundary_right(end_diff_current);
    
    // Check if region is significant enough for partial update
    if (region_end - region_start > current_len * 0.7) {
        // Large change, do full update
        region.full_line = true;
        region.start_pos = 0;
        region.end_pos = current_len;
    } else {
        // Small change, partial update
        region.full_line = false;
        region.start_pos = region_start;
        region.end_pos = region_end;
    }
    
    return region;
}

// Get current time in milliseconds for performance monitoring
static double get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Optimized highlighting with performance monitoring
static void lusush_apply_optimized_highlighting(void) {
    double start_time = get_current_time_ms();
    
    // Check if update is needed
    if (!needs_highlight_update()) {
        perf_stats.cache_hits++;

        return;
    }
    
    perf_stats.cache_misses++;
    perf_stats.total_updates++;
    

    
    // Calculate update region
    highlight_region_t region = calculate_update_region();
    
    if (region.full_line) {
        perf_stats.full_updates++;
        
        // Full line update - use existing function
        printf("\r\033[K");  // Return to start and clear line
        
        // Print current prompt
        const char *prompt = rl_prompt ? rl_prompt : "$ ";
        printf("%s", prompt);
        
        // Print line with syntax highlighting
        lusush_output_colored_line(rl_line_buffer, rl_point);
        
        // Move cursor to correct position
        printf("\r%s", prompt);
        for (int i = 0; i < rl_point && i < rl_end; i++) {
            printf("\033[C");  // Move cursor right
        }
    } else {
        perf_stats.incremental_updates++;
        
        // For now, fall back to full update for safety
        // TODO: Implement true incremental updates in future optimization
        printf("\r\033[K");
        
        const char *prompt = rl_prompt ? rl_prompt : "$ ";
        printf("%s", prompt);
        lusush_output_colored_line(rl_line_buffer, rl_point);
        
        printf("\r%s", prompt);
        for (int i = 0; i < rl_point && i < rl_end; i++) {
            printf("\033[C");
        }
    }
    
    // Update cache
    update_change_cache();
    
    // Update performance statistics
    double end_time = get_current_time_ms();
    double update_time = end_time - start_time;
    perf_stats.total_update_time += update_time;
    if (update_time > perf_stats.peak_update_time) {
        perf_stats.peak_update_time = update_time;
    }
    perf_stats.avg_update_time = perf_stats.total_update_time / perf_stats.total_updates;
    
    fflush(stdout);
}

// Performance monitoring and reporting functions
static void lusush_get_highlight_performance_stats(uint64_t *cache_hits, uint64_t *cache_misses, 
                                                  uint64_t *total_updates, double *avg_time) {
    if (cache_hits) *cache_hits = perf_stats.cache_hits;
    if (cache_misses) *cache_misses = perf_stats.cache_misses;
    if (total_updates) *total_updates = perf_stats.total_updates;
    if (avg_time) *avg_time = perf_stats.avg_update_time;
}

static void lusush_reset_highlight_performance_stats(void) {
    perf_stats.total_updates = 0;
    perf_stats.cache_hits = 0;
    perf_stats.cache_misses = 0;
    perf_stats.incremental_updates = 0;
    perf_stats.full_updates = 0;
    perf_stats.total_update_time = 0.0;
    perf_stats.peak_update_time = 0.0;
    perf_stats.avg_update_time = 0.0;
}

static void lusush_report_performance(void) {
    if (perf_stats.total_updates == 0) {
        printf("No highlighting updates recorded.\n");
        return;
    }
    
    double cache_hit_rate = (double)perf_stats.cache_hits / 
                           (perf_stats.cache_hits + perf_stats.cache_misses) * 100.0;
    
    printf("Syntax Highlighting Performance Report:\n");
    printf("  Total updates: %lu\n", perf_stats.total_updates);
    printf("  Cache hits: %lu (%.1f%%)\n", perf_stats.cache_hits, cache_hit_rate);
    printf("  Cache misses: %lu\n", perf_stats.cache_misses);
    printf("  Full updates: %lu\n", perf_stats.full_updates);
    printf("  Incremental updates: %lu\n", perf_stats.incremental_updates);
    printf("  Average update time: %.3f ms\n", perf_stats.avg_update_time);
    printf("  Peak update time: %.3f ms\n", perf_stats.peak_update_time);
}

// Public interface for performance stats
void lusush_show_highlight_performance(void) {
    lusush_report_performance();
}

// Enable debug output for Phase 3 optimization
void lusush_set_debug_enabled(bool enabled) {
    debug_enabled = enabled;

}

// Custom redisplay function - disabled for safety
static void lusush_custom_redisplay(void) {
    // Always use standard redisplay to prevent display corruption
    rl_redisplay();
}

// Hybrid redisplay function - supports both syntax highlighting and autosuggestions
void lusush_redisplay_with_suggestions(void) {
    // Check if we're transitioning to a continuation prompt
    extern const char *lusush_get_current_continuation_prompt(void);
    const char *current_prompt_type = lusush_get_current_continuation_prompt();
    bool is_continuation = (current_prompt_type && 
                           (strstr(current_prompt_type, "loop>") || 
                            strstr(current_prompt_type, "if>") ||
                            strstr(current_prompt_type, "quote>") ||
                            strstr(current_prompt_type, "function>") ||
                            strcmp(current_prompt_type, "> ") == 0));
    
    // Clear suggestions when transitioning TO continuation prompt
    if (is_continuation && !was_in_continuation_prompt && current_suggestion) {
        // Clean transition: just clear the current suggestion state
        lusush_free_autosuggestion(current_suggestion);
        current_suggestion = NULL;
    }
    was_in_continuation_prompt = is_continuation;
    
    // Enhanced Performance Monitoring: Start timing for redisplay with suggestions
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    // Always do standard redisplay first for stability
    rl_redisplay();
    
    // DEBUG: Show when function is called (disabled by default)
    if (debug_enabled) {
        fprintf(stderr, "[DEBUG] redisplay_with_suggestions called, line='%s', point=%d, end=%d\n", 
                rl_line_buffer ? rl_line_buffer : "(null)", rl_point, rl_end);
    }
    
    // SAFETY: Skip autosuggestions in special readline states
    if (RL_ISSTATE(RL_STATE_OVERWRITE | RL_STATE_ISEARCH | 
                   RL_STATE_MOREINPUT | RL_STATE_CALLBACK | 
                   RL_STATE_VICMDONCE | RL_STATE_VIMOTION)) {
        if (debug_enabled) fprintf(stderr, "[DEBUG] Skipping - in special readline state\n");
        return;
    }
    
    // SAFETY: Only show suggestions if we have input and cursor is at end
    if (!rl_line_buffer || !*rl_line_buffer || rl_point != rl_end || rl_end < 2) {
        // Clear any existing suggestion display using layered system
        display_integration_clear_autosuggestions();
        if (current_suggestion) {
            // Clear the visual suggestion from screen
            printf("\033[K");  // Clear to end of line
            fflush(stdout);
            lusush_free_autosuggestion(current_suggestion);
            current_suggestion = NULL;
        }
        if (debug_enabled) fprintf(stderr, "[DEBUG] Skipping - no input or cursor not at end\n");
        return;
    }
    
    // SAFETY: Skip if line is too long to prevent display issues
    if (rl_end > 80) {
        // Clear any existing suggestion display
        display_integration_clear_autosuggestions();
        if (current_suggestion) {
            // Clear the visual suggestion from screen
            printf("\033[K");  // Clear to end of line
            fflush(stdout);
            lusush_free_autosuggestion(current_suggestion);
            current_suggestion = NULL;
        }
        if (debug_enabled) fprintf(stderr, "[DEBUG] Skipping - line too long (%d chars)\n", rl_end);
        return;
    }
    
    if (debug_enabled) fprintf(stderr, "[DEBUG] Getting suggestion for: '%s'\n", rl_line_buffer);
    
    // Use layered display system for autosuggestions instead of direct terminal control
    if (display_integration_update_autosuggestions(rl_line_buffer, rl_point, rl_end)) {
        if (debug_enabled) fprintf(stderr, "[DEBUG] Autosuggestions updated via layered display\n");
    } else {
        if (debug_enabled) fprintf(stderr, "[DEBUG] Layered autosuggestions failed, fallback to legacy\n");
        
        // Fallback to legacy system only if layered system fails
        lusush_autosuggestion_t *suggestion = lusush_get_suggestion(rl_line_buffer, rl_point);
        if (debug_enabled) {
            if (suggestion) {
                fprintf(stderr, "[DEBUG] Got suggestion from lusush_get_suggestion: '%s'\n", 
                       suggestion->display_text ? suggestion->display_text : "NULL");
            } else {
                fprintf(stderr, "[DEBUG] lusush_get_suggestion returned NULL\n");
            }
        }
        
        if (suggestion && suggestion->display_text && *suggestion->display_text) {
            size_t sugg_len = strlen(suggestion->display_text);
            if (debug_enabled) fprintf(stderr, "[DEBUG] Suggestion length: %zu\n", sugg_len);
            
            if (sugg_len > 0 && sugg_len < 50 && !strchr(suggestion->display_text, '\n')) {
                // SAFETY: Check terminal width to prevent line wrap corruption
                int terminal_width = 80; // Safe default
                int terminal_height = 24;
                rl_get_screen_size(&terminal_height, &terminal_width);
                if (terminal_width <= 0) terminal_width = 80;
                
                int current_line_length = rl_end + sugg_len;
                
                // Only display if suggestion won't cause line wrapping (5 char safety margin)
                if (current_line_length < terminal_width - 5) {
                    if (debug_enabled) fprintf(stderr, "[DEBUG] Displaying suggestion (safe width)\n");

                    // Save terminal state
                    printf("\033[s");

                    // Display suggestion in visible gray after cursor (Fish-like style)
                    printf("\033[90m%s\033[0m", suggestion->display_text);

                    // Restore terminal state
                    printf("\033[u");
                    fflush(stdout);
                } else {
                    if (debug_enabled) fprintf(stderr, "[DEBUG] Skipping suggestion - would cause line wrap (%d + %zu >= %d)\n", 
                                             rl_end, sugg_len, terminal_width);
                    // Don't display but still store for key acceptance
                }

                // Store for keypress handling
                if (current_suggestion && current_suggestion != suggestion) {
                    lusush_free_autosuggestion(current_suggestion);
                }
                current_suggestion = suggestion;
            } else {
                if (debug_enabled) fprintf(stderr, "[DEBUG] Suggestion rejected: len=%zu, has_newline=%s\n", 
                                         sugg_len, strchr(suggestion->display_text, '\n') ? "yes" : "no");
                lusush_free_autosuggestion(suggestion);
            }
        } else {
            if (debug_enabled) fprintf(stderr, "[DEBUG] No valid suggestion found\n");
            if (current_suggestion) {
                // Clear old suggestion display and free memory
                printf("\033[K");  // Clear to end of line
                fflush(stdout);
                lusush_free_autosuggestion(current_suggestion);
                current_suggestion = NULL;
            }
        }
    }
    
    // Enhanced Performance Monitoring: Record redisplay timing
    gettimeofday(&end_time, NULL);
    uint64_t operation_time_ns = ((uint64_t)(end_time.tv_sec - start_time.tv_sec)) * 1000000000ULL +
                                 ((uint64_t)(end_time.tv_usec - start_time.tv_usec)) * 1000ULL;
    display_integration_record_display_timing(operation_time_ns);
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
    
    // Clear any autosuggestion before history navigation
    if (current_suggestion) {
        lusush_dismiss_suggestion_with_display();
        current_suggestion = NULL;
    }
    
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

        // Clear any autosuggestion before history navigation
        if (current_suggestion) {
            lusush_dismiss_suggestion_with_display();
            current_suggestion = NULL;
        }
        
        return rl_get_next_history(count, key);
    } else {
        // We're at the end of history
        if (has_content) {
            // Clear any autosuggestion before clearing line
            if (current_suggestion) {
                lusush_dismiss_suggestion_with_display();
                current_suggestion = NULL;
            }
            
            // Let readline handle line clearing properly to avoid display corruption
            rl_replace_line("", 0);
            rl_point = 0;
            rl_end = 0;
            rl_redisplay();
        }
        return 0;
    }
}

// Autosuggestion key handlers
static int lusush_accept_suggestion_key(int count, int key) {
    (void)count; (void)key;
    
    if (current_suggestion && current_suggestion->is_valid && current_suggestion->display_text) {
        // Insert suggestion text at cursor
        rl_insert_text(current_suggestion->display_text);
        
        // Accept the suggestion (updates statistics)
        lusush_accept_suggestion(current_suggestion);
        
        // Clear current suggestion
        if (current_suggestion) {
            lusush_free_autosuggestion(current_suggestion);
            current_suggestion = NULL;
        }
        
        return 0;
    }
    
    // If no suggestion, handle as normal key
    if (key == CTRL('F')) {
        // Ctrl+F with no suggestion - move forward one character
        return rl_forward_char(count, key);
    } else {
        // Right arrow with no suggestion - move forward one character  
        return rl_forward_char(count, key);
    }
}

static int lusush_accept_suggestion_word_key(int count, int key) {
    (void)count; (void)key;
    
    if (current_suggestion && current_suggestion->is_valid && current_suggestion->display_text) {
        // Find next word boundary in suggestion
        const char *text = current_suggestion->display_text;
        size_t word_len = 0;
        
        // Skip leading whitespace
        while (text[word_len] && isspace(text[word_len])) {
            word_len++;
        }
        
        // Find end of word
        while (text[word_len] && !isspace(text[word_len])) {
            word_len++;
        }
        
        if (word_len > 0) {
            // Insert just the first word
            char *word = strndup(text, word_len);
            if (word) {
                rl_insert_text(word);
                free(word);
            }
            
            // Update suggestion to remove the accepted part
            char *remaining = strdup(text + word_len);
            if (remaining && *remaining) {
                free(current_suggestion->display_text);
                current_suggestion->display_text = remaining;
            } else {
                // No more text, clear suggestion
                free(remaining);
                if (current_suggestion) {
                    lusush_free_autosuggestion(current_suggestion);
                    current_suggestion = NULL;
                }
            }
        }
        
        return 0;
    }
    
    // No suggestion - move forward one word
    return rl_forward_word(count, key);
}

// Custom newline handler to clear autosuggestions when commands are executed
static int lusush_newline_with_clear(int count, int key) {
    // Clear any autosuggestion before executing command
    if (current_suggestion) {
        printf("\033[K");  // Clear to end of line
        fflush(stdout);
        lusush_free_autosuggestion(current_suggestion);
        current_suggestion = NULL;
    }
    
    // Execute normal newline behavior
    return rl_newline(count, key);
}

static void setup_key_bindings(void) {
    // DISABLE tab completion to prevent interference with arrow keys
    rl_bind_key('\t', rl_insert);  // TAB just inserts tab character
    // rl_bind_key(27, rl_complete); // Disable Alt+Tab completion
    
    rl_bind_key('\n', lusush_newline_with_clear);   // Enter
    rl_bind_key('\r', lusush_newline_with_clear);   // Return
    
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
    
    // Autosuggestion key bindings
    rl_bind_key(CTRL('F'), lusush_accept_suggestion_key);     // Ctrl+F: accept full suggestion
    rl_bind_keyseq("\\e[C", lusush_accept_suggestion_key);    // Right arrow: accept full suggestion
    rl_bind_keyseq("\\e[1;5C", lusush_accept_suggestion_word_key); // Ctrl+Right arrow: accept word
    
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

/**
 * Legacy function - simplified to avoid interfering with readline
 * New approach generates prompt content and lets readline handle display
 */
static void try_layered_display_prompt(const char *prompt) {
    // No longer manually displays prompts - this was causing cursor positioning issues
    // The new approach generates prompt content and lets readline display it properly
    (void)prompt;  // Suppress unused parameter warning
    
    if (debug_enabled) {
        fprintf(stderr, "[READLINE_DEBUG] try_layered_display_prompt: using new prompt generation approach\n");
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
// SMART CHARACTER TRIGGERING
// ============================================================================

static bool should_trigger_highlighting(int c) {
    // Use existing comprehensive word separator function plus quotes for strings
    return lusush_is_word_separator(c) || c == '"' || c == '\'';
}
 
// Enhanced highlighting with variables, operators, strings, and numbers
static void lusush_highlight_previous_word(void) {
    if (!rl_line_buffer || rl_point < 1) {
        return;
    }

#ifdef __linux__
    // Linux-specific: Apply safety checks for potential wrapping issues
    if (rl_point > 70) {  // Conservative check for potential wrapping
        return;  // Skip highlighting for edge cases
    }
#endif
     
    char separator = rl_line_buffer[rl_point - 1];
     
    // Handle string literals (quotes)
    if (separator == '"' || separator == '\'') {
        // Find matching quote to highlight entire string
        int quote_start = rl_point - 1;
        for (int i = quote_start - 1; i >= 0; i--) {
            if (rl_line_buffer[i] == separator) {
                // Found opening quote, check if safe to highlight
                int string_len = quote_start - i + 1;
#ifdef __linux__
                // Linux-specific: Safety checks for string length
                if (string_len > 50) return; // Safety check for very long strings
                
                // Conservative check: if string might wrap, just skip highlighting
                if (string_len > 20) {
                    return;  // Skip highlighting for potentially long strings
                }
#endif
                 
                printf("\033[s");
                printf("\033[%dD", rl_point - i);
                printf("%s", string_color);
                for (int j = 0; j < string_len; j++) {
                    printf("%c", rl_line_buffer[i + j]);
                }
                printf("%s", reset_color);
                printf("\033[u");
                fflush(stdout);
                return;
            }
        }
        return; // No matching opening quote found
    }
     
    // Handle operators immediately
    if (separator == '|' || separator == '&' || separator == ';' || separator == '<' || separator == '>') {
        printf("\033[s");
        printf("\033[1D");
        printf("%s%c%s", operator_color, separator, reset_color);
        printf("\033[u");
        fflush(stdout);
        return;
    }
     
    // Handle variables ($VAR)
    if (rl_point >= 2 && rl_line_buffer[rl_point - 2] == '$') {
        int var_start = rl_point - 2;
        int var_end = rl_point - 1;
         
        // Extend to capture full variable name
        while (var_end < (int)strlen(rl_line_buffer) && 
               (isalnum(rl_line_buffer[var_end]) || rl_line_buffer[var_end] == '_')) {
            var_end++;
        }
        var_end--; // Back to last char of variable
         
        int var_len = var_end - var_start + 1;
#ifdef __linux__
        // Linux-specific: Safety checks for variable length
        if (var_len > 30) return; // Safety check
        
        // Conservative check: if variable might wrap, just skip highlighting
        if (var_len > 15) {
            return;  // Skip highlighting for potentially long variables
        }
#endif
         
        printf("\033[s");
        printf("\033[%dD", rl_point - var_start);
        printf("%s", variable_color);
        for (int i = 0; i < var_len; i++) {
            printf("%c", rl_line_buffer[var_start + i]);
        }
        printf("%s", reset_color);
        printf("\033[u");
        fflush(stdout);
        return;
    }
    
    // Find the word that was just completed (before the separator we just typed)
    int word_end = rl_point - 1;
    while (word_end >= 0 && lusush_is_word_separator(rl_line_buffer[word_end])) {
        word_end--;
    }
    if (word_end < 0) {
        return;
    }
    
    int word_start = word_end;
    while (word_start > 0 && !lusush_is_word_separator(rl_line_buffer[word_start - 1])) {
        word_start--;
    }
    
    int word_len = word_end - word_start + 1;
    if (word_len <= 0) {
        return;
    }
    // REMOVED: word_len > 32 restriction - now handles all word lengths
    
    char word[256];  // Increased buffer size to handle longer words safely
    if (word_len >= sizeof(word)) word_len = sizeof(word) - 1;  // Safety clamp
    strncpy(word, &rl_line_buffer[word_start], word_len);
    word[word_len] = '\0';
    
    bool should_highlight = false;
    const char *color = NULL;
    
    // Check for keywords
    if (lusush_is_shell_keyword(word, word_len)) {
        should_highlight = true;
        color = keyword_color;
    }
    // Check for built-ins
    else if (lusush_is_shell_builtin(word, word_len)) {
        should_highlight = true;
        color = command_color;
    }
    // Check for numbers
    else if (word_len > 0 && isdigit(word[0])) {
        bool is_number = true;
        for (int j = 0; j < word_len; j++) {
            if (!isdigit(word[j]) && word[j] != '.') {
                is_number = false;
                break;
            }
        }
        if (is_number) {
            should_highlight = true;
            color = number_color;
        }
    }
     
    if (should_highlight) {
#ifdef __linux__
        // Linux-specific: Safety checks for word length
        if (word_len > 40) return; // Safety check for very long words
        
        // Conservative check: if word might wrap, just skip highlighting
        if (word_len > 12) {
            return;  // Skip highlighting for potentially long words
        }
#endif
         
        printf("\033[s");
        printf("\033[%dD", rl_point - word_start);
        printf("%s%s%s", color, word, reset_color);
        printf("\033[u");
        fflush(stdout);
    }
}

static bool is_word_boundary_char(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_syntax_significant_char(int c) {
    return strchr("|&;<>()[]{}$\"'#", c) != NULL;
}

static int lusush_syntax_update_hook(void) {
    // Apply syntax highlighting actively but safely
    if (syntax_highlighting_enabled && rl_line_buffer && *rl_line_buffer) {
        // Only apply if we're in a safe state
        if (!(rl_readline_state & (RL_STATE_ISEARCH | RL_STATE_NSEARCH | 
                                  RL_STATE_SEARCH | RL_STATE_COMPLETING |
                                  RL_STATE_VICMDONCE | RL_STATE_VIMOTION))) {
            apply_syntax_highlighting();
        }
    }
    
    // Remove the hook after use
    rl_pre_input_hook = NULL;
    return 0;
}

static void reset_typing_state(void) {
    typing_state.consecutive_alphas = 0;
    typing_state.last_char = 0;
    typing_state.in_word = false;
    typing_state.last_trigger_time = 0;
}

static bool is_safe_for_highlighting(void) {
    // Enhanced safety checks for Phase 2
    
    // 1. Basic readline state validation
    if (rl_readline_state & (RL_STATE_ISEARCH | RL_STATE_NSEARCH | 
                            RL_STATE_SEARCH | RL_STATE_COMPLETING |
                            RL_STATE_VICMDONCE | RL_STATE_VIMOTION |
                            RL_STATE_MOREINPUT | RL_STATE_MULTIKEY)) {
        return false;
    }
    
    // 2. Buffer validity - must have content
    if (!rl_line_buffer || rl_end <= 0 || strlen(rl_line_buffer) == 0) {
        return false;
    }
    
    // 3. Terminal state
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }
    
    // 4. Recursion protection
    static bool in_highlighting_check = false;
    if (in_highlighting_check) {
        return false;
    }
    
    // 5. Check if we're in the middle of input processing
    if (rl_point < 0 || rl_point > rl_end) {
        return false;
    }
    
    // 6. Skip highlighting for very short or empty input
    if (strlen(rl_line_buffer) < 2) {
        return false;
    }
    
    return true;
}













// Custom redisplay function for real-time syntax highlighting
static void lusush_safe_redisplay(void) {
    // This function should rarely be called now
    // Most redisplay operations should go through display_integration_redisplay
    
    // Prevent recursive calls
    static bool in_redisplay = false;
    if (in_redisplay) {
        rl_redisplay();
        return;
    }
    
    in_redisplay = true;
    
    // Try enhanced display first, even in legacy function
    if (display_integration_is_layered_active()) {
        if (debug_enabled) {
            fprintf(stderr, "[READLINE_DEBUG] lusush_safe_redisplay: calling display_integration_redisplay()\n");
        }
        display_integration_redisplay();
        
        // Use layered display system for autosuggestions instead of direct terminal control
        if (rl_line_buffer && *rl_line_buffer && rl_point == rl_end) {
            display_integration_update_autosuggestions(rl_line_buffer, rl_point, rl_end);
        } else {
            display_integration_clear_autosuggestions();
        }
        
        in_redisplay = false;
        return;
    }
    
    // Desperate fallback - original implementation
    // Check if we should apply highlighting (only for single-line themes)
           
    if (syntax_highlighting_enabled && is_safe_for_highlighting()) {
        // Runtime check for multi-line theme
        if (detect_multiline_theme()) {
            // Skip highlighting for multi-line themes
            in_redisplay = false;
            rl_redisplay();
            return;
        }
        // Use new robust syntax highlighting with line wrapping support
        lusush_simple_syntax_display();
        in_redisplay = false;
        return;
    }
    
    in_redisplay = false;
    // Final fallback to standard redisplay
    rl_redisplay();
}

// Detect if current theme uses multi-line prompts
static bool detect_multiline_theme(void) {
    const char *prompt = rl_prompt ? rl_prompt : "$ ";
    
    // Check for multi-line indicators in the prompt
    // Dark theme uses ┌─ and └─ box drawing characters
    if (strstr(prompt, "┌─") || strstr(prompt, "└─")) {
        return true;
    }
    
    // Check for explicit newlines in prompt
    if (strchr(prompt, '\n')) {
        return true;
    }
    
    // Single-line themes: minimal ($), light/classic (user@host:path$), colorful (● user@host)
    return false;
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

    // Update typing state
    typing_state.last_char = c;

    return c;
}

// Enhanced getc function with autosuggestion triggering
static int lusush_getc_with_autosuggestions(FILE *stream) {
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

    // Update typing state
    typing_state.last_char = c;

    return c;
}
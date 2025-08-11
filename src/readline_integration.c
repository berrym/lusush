/*
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
    rl_attempted_completion_function = lusush_tab_completion;
    rl_completion_entry_function = lusush_completion_entry_function;
    
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
    
    // Apply syntax highlighting hook if enabled
    if (syntax_highlighting_enabled) {
        rl_redisplay_function = apply_syntax_highlighting;
    }
    
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

static char **lusush_tab_completion(const char *text, int start, int end) {
    char **matches = NULL;
    
    // Use Lusush completion system
    lusush_completions_t completions = {0, NULL};
    
    // Get the full line buffer for context
    const char *line_buffer = rl_line_buffer;
    
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
    
    if (enabled) {
        rl_redisplay_function = apply_syntax_highlighting;
    } else {
        rl_redisplay_function = rl_redisplay;
    }
}

bool lusush_syntax_highlighting_is_enabled(void) {
    return syntax_highlighting_enabled;
}

static void apply_syntax_highlighting(void) {
    // Apply basic syntax highlighting
    // This is a simplified implementation - can be enhanced
    rl_redisplay();
}

void lusush_syntax_highlight_line(void) {
    if (syntax_highlighting_enabled) {
        apply_syntax_highlighting();
    }
}

void lusush_syntax_highlighting_configure(const char *commands_color,
                                         const char *strings_color,
                                         const char *comments_color,
                                         const char *keywords_color) {
    (void)commands_color; (void)strings_color; (void)comments_color; (void)keywords_color; // Suppress unused parameter warnings
    // Store colors for syntax highlighting
    // Implementation can be added as needed
}

// ============================================================================
// KEY BINDINGS AND SHORTCUTS
// ============================================================================

// Custom function to abort current line (Ctrl+G)
static int lusush_abort_line(int count, int key) {
    (void)count; (void)key;
    
    // Clear the current line
    rl_replace_line("", 0);
    rl_point = 0;
    rl_end = 0;
    
    // Force redisplay
    rl_forced_update_display();
    
    // Ring bell to indicate abort
    rl_ding();
    return 0;
}

// Custom function to clear screen (Ctrl+L)
static int lusush_clear_screen_and_redisplay(int count, int key) {
    (void)count; (void)key;
    
    // Clear screen using readline's function
    rl_clear_screen(0, 0);
    
    // Force complete redisplay to prevent artifacts
    rl_forced_update_display();
    rl_redisplay();
    return 0;
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
    // Tab completion - use menu-complete for cycling
    rl_bind_key('\t', rl_menu_complete);
    rl_bind_key(27, rl_complete); // Alt+Tab for normal completion
    
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
    
    // History navigation with artifact prevention
    rl_bind_key(16, lusush_previous_history); // Ctrl-P
    rl_bind_key(14, lusush_next_history);     // Ctrl-N
    
    // Arrow keys for history (if available)
    rl_bind_keyseq("\\e[A", lusush_previous_history); // Up arrow
    rl_bind_keyseq("\\e[B", lusush_next_history);     // Down arrow
    
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
    // Basic configuration
    rl_completion_append_character = ' '; // Add space after completion
    rl_attempted_completion_over = 0;
    rl_filename_completion_desired = 1;
    rl_filename_quoting_desired = 1;
    
    // History configuration
    // rl_history_search_delimiter_chars = " \t\n;&()|<>"; // Not available in all readline versions
    
    // Completion configuration
    rl_completion_query_items = 50;   // Ask before showing many completions
    rl_completion_suppress_quote = 0;
    rl_completion_quote_character = '"';
    rl_completion_suppress_append = 0; // Allow appending for normal behavior
    
    // Input configuration - minimal signal handling
    rl_catch_signals = 0; // Let shell handle signals for child processes like git
    rl_catch_sigwinch = 1; // Handle window resize only
    
    // Redisplay configuration for better history navigation
    rl_redisplay_function = rl_redisplay;

    
    // Variables for better behavior
    rl_variable_bind("completion-ignore-case", "on");
    rl_variable_bind("show-all-if-unmodified", "on");
    rl_variable_bind("visible-stats", "on");
    rl_variable_bind("mark-directories", "on");
    rl_variable_bind("colored-stats", "on");
    rl_variable_bind("page-completions", "off");
    rl_variable_bind("skip-completed-text", "on");
    
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
    
    return c;
}
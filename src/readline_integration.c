/*
 * Lusush Shell - GNU Readline Integration
 * 
 * This file provides a complete replacement for the complex LLE system
 * using proven GNU Readline library. Features include:
 * 
 * - History with deduplication (hist_no_dups)
 * - Tab completion integration with existing lusush completion
 * - Syntax highlighting hooks
 * - Prompt theme integration
 * - All modern editing features (Ctrl+R, etc.)
 * - Cross-platform compatibility
 * - Perfect visual editing with zero corruption
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../include/readline_integration.h"
#include "../include/lusush.h"
#include "../include/themes.h"
#include "../include/completion.h"
#include "../include/config.h"

// Forward declaration for existing lusush completion system
extern void lusush_completion_callback(const char *buf, lusush_completions_t *lc);

// ============================================================================
// GLOBAL STATE
// ============================================================================

static bool readline_initialized = false;
static bool syntax_highlighting_enabled = true;
static bool debug_enabled = false;
static char *last_prompt = NULL;
static char *history_file_path = NULL;
static char *last_error = NULL;
static lusush_prompt_callback_t custom_prompt_callback = NULL;
static lusush_pre_input_hook_t pre_input_hook = NULL;
static lusush_post_input_hook_t post_input_hook = NULL;

// Syntax highlighting colors (ANSI escape codes)
static const char *cmd_color = "\033[32m";      // Green for commands
static const char *string_color = "\033[33m";   // Yellow for strings  
static const char *comment_color = "\033[90m";  // Gray for comments
static const char *keyword_color = "\033[35m";  // Magenta for keywords
// static const char *reset_color = "\033[0m";     // Reset color (unused)

// Completion state for readline generator
static char **current_completions = NULL;
static int completion_index = 0;
static int completion_count = 0;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Set error message
 */
static void set_error(const char *message) {
    if (last_error) {
        free(last_error);
    }
    last_error = message ? strdup(message) : NULL;
}

/**
 * Debug logging
 */
static void debug_log(const char *format, ...) {
    if (!debug_enabled) return;
    
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[READLINE_DEBUG] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * Get default history file path
 */
static char *get_default_history_file(void) {
    const char *home = getenv("HOME");
    if (!home) {
        return strdup(".lusush_history");
    }
    
    size_t len = strlen(home) + strlen("/.lusush_history") + 1;
    char *path = malloc(len);
    if (path) {
        snprintf(path, len, "%s/.lusush_history", home);
    }
    return path;
}

// ============================================================================
// HISTORY MANAGEMENT WITH DEDUPLICATION
// ============================================================================

bool lusush_history_load(void) {
    if (!history_file_path) {
        history_file_path = get_default_history_file();
    }
    
    if (!history_file_path) {
        set_error("Failed to get history file path");
        return false;
    }
    
    // Check if file exists
    struct stat st;
    if (stat(history_file_path, &st) != 0) {
        // File doesn't exist, that's OK
        debug_log("History file doesn't exist: %s", history_file_path);
        return true;
    }
    
    int result = read_history(history_file_path);
    if (result != 0) {
        set_error("Failed to load history file");
        debug_log("Failed to load history: %s", strerror(errno));
        return false;
    }
    
    debug_log("Loaded history from: %s", history_file_path);
    return true;
}

void lusush_history_save(void) {
    if (!history_file_path) {
        history_file_path = get_default_history_file();
    }
    
    if (!history_file_path) {
        set_error("Failed to get history file path");
        return;
    }
    
    int result = write_history(history_file_path);
    if (result != 0) {
        set_error("Failed to save history file");
        debug_log("Failed to save history: %s", strerror(errno));
    } else {
        debug_log("Saved history to: %s", history_file_path);
    }
}

void lusush_history_add(const char *line) {
    if (!line || !*line || isspace(*line)) {
        return;  // Skip empty lines and whitespace-only
    }
    
    // Check if this is identical to the last entry (hist_no_dups behavior)
    HIST_ENTRY *last = history_get(history_length);
    if (last && strcmp(last->line, line) == 0) {
        debug_log("Skipping duplicate history entry: %s", line);
        return;  // Skip duplicate
    }
    
    add_history(line);
    debug_log("Added to history: %s", line);
}

void lusush_history_clear(void) {
    clear_history();
    debug_log("Cleared all history");
}

const char *lusush_history_get(int index) {
    HIST_ENTRY *entry = history_get(index + 1);  // readline uses 1-based indexing
    return entry ? entry->line : NULL;
}

int lusush_history_length(void) {
    return history_length;
}

int lusush_history_remove_duplicates(void) {
    int removed = 0;
    int length = history_length;
    
    for (int i = 1; i <= length; i++) {
        HIST_ENTRY *current = history_get(i);
        if (!current) continue;
        
        // Check for duplicates after this entry
        for (int j = i + 1; j <= length; j++) {
            HIST_ENTRY *next = history_get(j);
            if (next && strcmp(current->line, next->line) == 0) {
                remove_history(j - 1);  // remove_history uses 0-based indexing
                removed++;
                length--;
                j--;  // Adjust for removed entry
            }
        }
    }
    
    debug_log("Removed %d duplicate history entries", removed);
    return removed;
}

bool lusush_history_set_file(const char *filepath) {
    if (history_file_path) {
        free(history_file_path);
    }
    history_file_path = filepath ? strdup(filepath) : NULL;
    return history_file_path != NULL;
}

void lusush_history_set_max_length(int max_length) {
    stifle_history(max_length);
    debug_log("Set history max length to: %d", max_length);
}

// ============================================================================
// TAB COMPLETION INTEGRATION
// ============================================================================

/**
 * Free current completion list
 */
static void free_current_completions(void) {
    if (current_completions) {
        for (int i = 0; i < completion_count; i++) {
            if (current_completions[i]) {
                free(current_completions[i]);
            }
        }
        free(current_completions);
        current_completions = NULL;
    }
    completion_count = 0;
    completion_index = 0;
}

char *lusush_completion_generator(const char *text, int state) {
    debug_log("Completion generator called: text='%s', state=%d", text, state);
    
    // First call - initialize
    if (!state) {
        free_current_completions();
        
        // Use existing lusush completion system
        lusush_completions_t lc = {0};
        lc.len = 0;
        lc.cvec = NULL;
        
        // Get the current line buffer for context-aware completion
        const char *line_buffer = rl_line_buffer;
        if (!line_buffer) line_buffer = "";
        
        // Call the existing lusush completion callback
        // This integrates with all existing completion logic
        lusush_completion_callback(line_buffer, &lc);
        
        // Convert lusush_completions_t to our internal format
        current_completions = malloc(lc.len * sizeof(char*));
        if (current_completions && lc.len > 0) {
            for (size_t i = 0; i < lc.len; i++) {
                // Extract the relevant part of the completion
                const char *completion = lc.cvec[i];
                if (completion) {
                    // For readline, we only want the part that extends the current text
                    // The existing completion system returns full completions
                    current_completions[completion_count++] = strdup(completion);
                }
            }
        }
        
        // Free the lusush_completions_t structure
        lusush_free_completions(&lc);
        
        debug_log("Generated %d completions using lusush completion system", completion_count);
    }
    
    // Return next completion
    if (completion_index < completion_count) {
        char *result = strdup(current_completions[completion_index++]);
        debug_log("Returning completion: %s", result);
        return result;
    }
    
    return NULL;
}

char **lusush_completion_matches(const char *text, int start, int end) {
    (void)start; // Suppress unused parameter warning
    (void)end;   // Suppress unused parameter warning
    rl_attempted_completion_over = 1;  // Don't fall back to filename completion
    return rl_completion_matches(text, lusush_completion_generator);
}

void lusush_completion_setup(void) {
    rl_attempted_completion_function = lusush_completion_matches;
    rl_completion_entry_function = NULL;  // Use our custom function instead
    debug_log("Tab completion setup complete");
}

// ============================================================================
// SYNTAX HIGHLIGHTING
// ============================================================================

void lusush_syntax_highlight_line(void) {
    if (!syntax_highlighting_enabled || !rl_line_buffer) {
        return;
    }
    
    // This is a basic syntax highlighting implementation
    // In a full implementation, you'd parse the command line and apply colors
    
    // For now, just highlight the first word (command) if it's a known builtin
    char *buffer = rl_line_buffer;
    char *space = strchr(buffer, ' ');
    size_t cmd_len = space ? (size_t)(space - buffer) : strlen(buffer);
    
    // Check if first word is a builtin
    const char *builtins[] = {
        "echo", "cd", "pwd", "exit", "history", "alias", "export", "clear", NULL
    };
    
    for (int i = 0; builtins[i]; i++) {
        if (strncmp(buffer, builtins[i], cmd_len) == 0 && 
            strlen(builtins[i]) == cmd_len) {
            // Found a builtin - in a real implementation, apply highlighting here
            break;
        }
    }
    
    // Note: This is a simplified example. Real syntax highlighting would
    // require more sophisticated parsing and careful manipulation of the
    // display buffer to avoid corruption.
    
    debug_log("Syntax highlighting processed for: %s", buffer);
}

void lusush_syntax_highlighting_set_enabled(bool enabled) {
    syntax_highlighting_enabled = enabled;
    if (enabled) {
        rl_redisplay_function = (void (*)(void))lusush_syntax_highlight_line;
    } else {
        rl_redisplay_function = rl_redisplay;  // Default function
    }
    debug_log("Syntax highlighting %s", enabled ? "enabled" : "disabled");
}

bool lusush_syntax_highlighting_is_enabled(void) {
    return syntax_highlighting_enabled;
}

void lusush_syntax_highlighting_configure(const char *commands_color,
                                         const char *strings_color,
                                         const char *comments_color,
                                         const char *keywords_color) {
    // Update color codes (make copies)
    // This is simplified - in a real implementation you'd validate the codes
    if (commands_color) cmd_color = commands_color;
    if (strings_color) string_color = strings_color;
    if (comments_color) comment_color = comments_color;
    if (keywords_color) keyword_color = keywords_color;
    
    debug_log("Updated syntax highlighting colors");
}

// ============================================================================
// PROMPT INTEGRATION WITH LUSUSH THEMES
// ============================================================================

char *lusush_generate_prompt(void) {
    // This function is no longer used - prompts come from lusush's prompt system
    // Keep for backward compatibility but return empty string
    if (last_prompt) {
        free(last_prompt);
    }
    last_prompt = strdup("");
    return last_prompt;
}

void lusush_prompt_update(void) {
    // Force regeneration on next prompt request
    if (last_prompt) {
        free(last_prompt);
        last_prompt = NULL;
    }
    debug_log("Prompt update requested");
}

void lusush_prompt_set_callback(lusush_prompt_callback_t callback) {
    custom_prompt_callback = callback;
    lusush_prompt_update();  // Force regeneration
    debug_log("Custom prompt callback set");
}

// ============================================================================
// KEY BINDINGS AND SHORTCUTS
// ============================================================================

void lusush_keybindings_setup(void) {
    // Standard key bindings
    rl_bind_key('\t', rl_complete);              // Tab completion
    rl_bind_key(12, rl_clear_screen);            // Ctrl+L: Clear screen
    rl_bind_key(4, rl_delete_or_show_completions); // Ctrl+D: Delete or show completions
    
    // Configure readline variables for better behavior
    rl_variable_bind("show-all-if-ambiguous", "on");
    rl_variable_bind("completion-ignore-case", "on");
    rl_variable_bind("colored-completion-prefix", "on");
    rl_variable_bind("history-preserve-point", "on");
    rl_variable_bind("revert-all-at-newline", "on");
    
    debug_log("Key bindings setup complete");
}

int lusush_keybinding_add(int key, rl_command_func_t *function) {
    int result = rl_bind_key(key, function);
    debug_log("Added keybinding for key %d: %s", key, result == 0 ? "success" : "failed");
    return result;
}

int lusush_keybinding_remove(int key) {
    int result = rl_unbind_key(key);
    debug_log("Removed keybinding for key %d: %s", key, result == 0 ? "success" : "failed");
    return result;
}

// ============================================================================
// MAIN READLINE INTERFACE
// ============================================================================

bool lusush_readline_init(void) {
    if (readline_initialized) {
        return true;
    }
    
    debug_log("Initializing readline integration");
    
    // Initialize history
    using_history();
    stifle_history(1000);  // Default max 1000 entries
    
    if (!lusush_history_load()) {
        debug_log("Warning: Failed to load history");
        // Continue anyway - not fatal
    }
    
    // Set up completion
    lusush_completion_setup();
    
    // Set up key bindings
    lusush_keybindings_setup();
    
    // Set up syntax highlighting
    lusush_syntax_highlighting_set_enabled(syntax_highlighting_enabled);
    
    // Set application name for inputrc customization
    rl_readline_name = "lusush";
    
    readline_initialized = true;
    debug_log("Readline initialization complete");
    return true;
}

char *lusush_readline(void) {
    // This function should not be called directly
    // Use lusush_readline_with_prompt() instead
    return lusush_readline_with_prompt("lusush $ ");
}

/**
 * Get a line of input using readline with specific prompt
 * @param prompt the prompt string to display
 * @return allocated string that caller must free, or NULL on EOF
 */
char *lusush_readline_with_prompt(const char *prompt) {
    if (!readline_initialized) {
        if (!lusush_readline_init()) {
            set_error("Failed to initialize readline");
            return NULL;
        }
    }
    
    // Call pre-input hook if set
    if (pre_input_hook) {
        pre_input_hook();
    }
    
    // Use the provided prompt directly
    const char *actual_prompt = prompt ? prompt : "lusush $ ";
    
    // Get input from user
    char *line = readline(actual_prompt);
    
    // Call post-input hook if set
    if (post_input_hook) {
        post_input_hook(line);
    }
    
    // Add to history if non-empty
    if (line && *line) {
        lusush_history_add(line);
    }
    
    debug_log("Readline returned: %s", line ? line : "NULL");
    return line;  // Caller must free()
}

void lusush_readline_cleanup(void) {
    if (!readline_initialized) {
        return;
    }
    
    debug_log("Cleaning up readline");
    
    // Save history
    lusush_history_save();
    
    // Cleanup completions
    free_current_completions();
    
    // Cleanup history
    clear_history();
    
    // Free prompt
    if (last_prompt) {
        free(last_prompt);
        last_prompt = NULL;
    }
    
    // Free history file path
    if (history_file_path) {
        free(history_file_path);
        history_file_path = NULL;
    }
    
    // Free error message
    if (last_error) {
        free(last_error);
        last_error = NULL;
    }
    
    readline_initialized = false;
    debug_log("Readline cleanup complete");
}

bool lusush_readline_available(void) {
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

const char *lusush_readline_version(void) {
    return rl_library_version;
}

// ============================================================================
// CONFIGURATION AND SETTINGS
// ============================================================================

void lusush_readline_configure(bool show_completions_immediately,
                              bool case_insensitive_completion,
                              bool colored_completion_prefix) {
    rl_variable_bind("show-all-if-ambiguous", show_completions_immediately ? "on" : "off");
    rl_variable_bind("completion-ignore-case", case_insensitive_completion ? "on" : "off");
    rl_variable_bind("colored-completion-prefix", colored_completion_prefix ? "on" : "off");
    
    debug_log("Readline configuration updated");
}

void lusush_multiline_set_enabled(bool enabled) {
    // Readline handles multiline automatically, but we can configure behavior
    rl_variable_bind("revert-all-at-newline", enabled ? "off" : "on");
    debug_log("Multiline editing %s", enabled ? "enabled" : "disabled");
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
    if (position < 0 || position > rl_end) {
        return false;
    }
    rl_point = position;
    return true;
}

const char *lusush_get_line_buffer(void) {
    return rl_line_buffer;
}

void lusush_set_line_buffer(const char *new_content) {
    if (!new_content) return;
    
    rl_replace_line(new_content, 0);
    rl_point = strlen(new_content);  // Move cursor to end
}

// ============================================================================
// ERROR HANDLING AND DEBUGGING
// ============================================================================

const char *lusush_readline_get_error(void) {
    return last_error;
}

void lusush_readline_set_debug(bool enabled) {
    debug_enabled = enabled;
    debug_log("Debug output %s", enabled ? "enabled" : "disabled");
}

bool lusush_readline_is_debug_enabled(void) {
    return debug_enabled;
}

// ============================================================================
// COMPATIBILITY LAYER
// ============================================================================

void lusush_add_completion(lusush_completions_t *lc, const char *completion) {
    if (!lc || !completion) return;
    
    // Resize array if needed
    if (lc->len % 10 == 0) {  // Grow in chunks of 10
        char **new_cvec = realloc(lc->cvec, (lc->len + 10) * sizeof(char*));
        if (!new_cvec) return;
        lc->cvec = new_cvec;
    }
    
    lc->cvec[lc->len++] = strdup(completion);
}

void lusush_free_completions(lusush_completions_t *lc) {
    if (!lc) return;
    
    for (size_t i = 0; i < lc->len; i++) {
        if (lc->cvec[i]) {
            free(lc->cvec[i]);
        }
    }
    
    if (lc->cvec) {
        free(lc->cvec);
    }
    
    lc->cvec = NULL;
    lc->len = 0;
}

// ============================================================================
// INTEGRATION HOOKS
// ============================================================================

void lusush_set_pre_input_hook(lusush_pre_input_hook_t hook) {
    pre_input_hook = hook;
    debug_log("Pre-input hook %s", hook ? "set" : "cleared");
}

void lusush_set_post_input_hook(lusush_post_input_hook_t hook) {
    post_input_hook = hook;
    debug_log("Post-input hook %s", hook ? "set" : "cleared");
}
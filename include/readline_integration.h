#ifndef READLINE_INTEGRATION_H
#define READLINE_INTEGRATION_H

#include <stdbool.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MAIN READLINE INTERFACE
// ============================================================================

/**
 * Initialize the readline system for lusush
 * Sets up completion, history, key bindings, and syntax highlighting
 * @return true on success, false on failure
 */
bool lusush_readline_init(void);

/**
 * Get a line of input using readline with lusush theming
 * @return allocated string that caller must free, or NULL on EOF
 */
char *lusush_readline(void);

/**
 * Get a line of input using readline with specific prompt
 * @param prompt the prompt string to display
 * @return allocated string that caller must free, or NULL on EOF
 */
char *lusush_readline_with_prompt(const char *prompt);

/**
 * Cleanup readline resources and save history
 */
void lusush_readline_cleanup(void);

/**
 * Check if readline is available and working
 * @return true if readline can be used, false otherwise
 */
bool lusush_readline_available(void);

/**
 * Get readline version information
 * @return version string
 */
const char *lusush_readline_version(void);

// ============================================================================
// HISTORY MANAGEMENT WITH DEDUPLICATION
// ============================================================================

/**
 * Add command to history with automatic deduplication
 * Implements hist_no_dups behavior
 * @param line command to add to history
 */
void lusush_history_add(const char *line);

/**
 * Save history to file
 */
void lusush_history_save(void);

/**
 * Load history from file
 * @return true on success, false on failure
 */
bool lusush_history_load(void);

/**
 * Clear all history entries
 */
void lusush_history_clear(void);

/**
 * Get history entry by index
 * @param index history index (0-based)
 * @return history entry or NULL if not found
 */
const char *lusush_history_get(int index);

/**
 * Get total number of history entries
 * @return number of entries
 */
int lusush_history_length(void);

/**
 * Remove duplicate entries from history
 * @return number of duplicates removed
 */
int lusush_history_remove_duplicates(void);

// ============================================================================
// TAB COMPLETION INTEGRATION
// ============================================================================

/**
 * Setup tab completion integration with existing lusush completion system
 */
void lusush_completion_setup(void);

/**
 * Main completion generator for readline
 * @param text text to complete
 * @param state completion state (0 for first call, >0 for subsequent)
 * @return next completion string or NULL when done
 */
char *lusush_completion_generator(const char *text, int state);

/**
 * Custom completion function for readline
 * @param text text being completed
 * @param start start position in line buffer
 * @param end end position in line buffer
 * @return array of completion matches
 */
char **lusush_completion_matches(const char *text, int start, int end);

// ============================================================================
// SYNTAX HIGHLIGHTING
// ============================================================================

/**
 * Enable or disable syntax highlighting
 * @param enabled true to enable, false to disable
 */
void lusush_syntax_highlighting_set_enabled(bool enabled);

/**
 * Check if syntax highlighting is enabled
 * @return true if enabled, false otherwise
 */
bool lusush_syntax_highlighting_is_enabled(void);

/**
 * Apply syntax highlighting to current line buffer
 * Called automatically by readline during editing
 */
void lusush_syntax_highlight_line(void);

/**
 * Configure syntax highlighting colors and styles
 * @param commands_color ANSI color code for commands
 * @param strings_color ANSI color code for strings
 * @param comments_color ANSI color code for comments
 * @param keywords_color ANSI color code for keywords
 */
void lusush_syntax_highlighting_configure(const char *commands_color,
                                         const char *strings_color,
                                         const char *comments_color,
                                         const char *keywords_color);

// ============================================================================
// PROMPT INTEGRATION WITH LUSUSH THEMES
// ============================================================================

/**
 * Generate themed prompt for current readline session
 * Integrates with lusush theme system
 * @return allocated prompt string (managed internally)
 */
char *lusush_generate_prompt(void);

/**
 * Update prompt with new theme or context
 * Call this when theme changes or directory changes
 */
void lusush_prompt_update(void);

/**
 * Set custom prompt callback
 * @param callback function to generate custom prompts
 */
typedef char *(*lusush_prompt_callback_t)(void);
void lusush_prompt_set_callback(lusush_prompt_callback_t callback);

// ============================================================================
// KEY BINDINGS AND SHORTCUTS
// ============================================================================

/**
 * Setup custom key bindings for lusush
 */
void lusush_keybindings_setup(void);

/**
 * Bind a key to a readline function
 * @param key key code to bind
 * @param function readline function to execute
 * @return 0 on success, -1 on error
 */
int lusush_keybinding_add(int key, rl_command_func_t *function);

/**
 * Remove a key binding
 * @param key key code to unbind
 * @return 0 on success, -1 on error
 */
int lusush_keybinding_remove(int key);

// ============================================================================
// CONFIGURATION AND SETTINGS
// ============================================================================

/**
 * Configure readline behavior
 * @param show_completions_immediately show completions without double-tab
 * @param case_insensitive_completion enable case-insensitive completion
 * @param colored_completion_prefix highlight matching prefix in completions
 */
void lusush_readline_configure(bool show_completions_immediately,
                              bool case_insensitive_completion,
                              bool colored_completion_prefix);

/**
 * Set maximum history length
 * @param max_length maximum number of history entries
 */
void lusush_history_set_max_length(int max_length);

/**
 * Enable or disable multiline editing
 * @param enabled true to enable multiline, false for single line
 */
void lusush_multiline_set_enabled(bool enabled);

/**
 * Set custom history file path
 * @param filepath path to history file
 * @return true on success, false on error
 */
bool lusush_history_set_file(const char *filepath);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Clear the terminal screen
 */
void lusush_clear_screen(void);

/**
 * Force refresh of the current line display
 */
void lusush_refresh_line(void);

/**
 * Get current cursor position in line
 * @return cursor position (0-based)
 */
int lusush_get_cursor_position(void);

/**
 * Set cursor position in line
 * @param position new cursor position (0-based)
 * @return true on success, false on error
 */
bool lusush_set_cursor_position(int position);

/**
 * Get current line buffer content
 * @return current line content (do not free)
 */
const char *lusush_get_line_buffer(void);

/**
 * Replace current line buffer content
 * @param new_content new line content
 */
void lusush_set_line_buffer(const char *new_content);

// ============================================================================
// ERROR HANDLING AND DEBUGGING
// ============================================================================

/**
 * Get last readline error message
 * @return error message or NULL if no error
 */
const char *lusush_readline_get_error(void);

/**
 * Enable or disable readline debug output
 * @param enabled true to enable debug output
 */
void lusush_readline_set_debug(bool enabled);

/**
 * Check if readline debug is enabled
 * @return true if debug is enabled
 */
bool lusush_readline_is_debug_enabled(void);

// ============================================================================
// COMPATIBILITY LAYER
// ============================================================================

/**
 * Compatibility structure for existing completion code
 * Maps to readline's completion system
 */
typedef struct {
    size_t len;
    char **cvec;
} lusush_completions_t;

/**
 * Add completion to compatibility structure
 * @param lc completion structure
 * @param completion completion string to add
 */
void lusush_add_completion(lusush_completions_t *lc, const char *completion);

/**
 * Free completion structure
 * @param lc completion structure to free
 */
void lusush_free_completions(lusush_completions_t *lc);

// ============================================================================
// INTEGRATION HOOKS
// ============================================================================

/**
 * Hook called before readline processing
 * Allows lusush to perform setup before each line
 */
typedef void (*lusush_pre_input_hook_t)(void);

/**
 * Hook called after readline processing
 * Allows lusush to perform cleanup after each line
 */
typedef void (*lusush_post_input_hook_t)(const char *line);

/**
 * Set pre-input hook
 * @param hook function to call before input
 */
void lusush_set_pre_input_hook(lusush_pre_input_hook_t hook);

/**
 * Set post-input hook
 * @param hook function to call after input
 */
void lusush_set_post_input_hook(lusush_post_input_hook_t hook);

#ifdef __cplusplus
}
#endif

#endif /* READLINE_INTEGRATION_H */
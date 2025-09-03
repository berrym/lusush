#ifndef MENU_COMPLETION_H
#define MENU_COMPLETION_H

#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// MENU COMPLETION SYSTEM
// ============================================================================

/**
 * Menu completion state structure
 * Tracks the current state of menu completion including available completions,
 * selected index, and display configuration
 */
typedef struct {
    char **completions;             // Array of completion strings
    char **descriptions;            // Array of completion descriptions
    int count;                      // Number of completions
    int selected_index;             // Currently selected completion index
    int display_columns;            // Number of display columns
    int display_rows;               // Number of display rows
    bool active;                    // Menu completion is currently active
    char *original_text;            // Original text being completed
    int start_pos;                  // Start position of completion in line
    int end_pos;                    // End position of completion in line
    char *common_prefix;            // Common prefix of all completions
} menu_completion_state_t;

/**
 * Layout information for multi-column display
 */
typedef struct {
    int columns;                    // Number of columns
    int column_width;              // Width of each column
    int rows;                      // Number of rows
    int terminal_width;            // Available terminal width
    bool has_descriptions;         // Whether descriptions are shown
    int max_completion_width;      // Width of longest completion
    int max_description_width;     // Width of longest description
} menu_layout_info_t;

/**
 * Menu completion error codes
 */
typedef enum {
    MENU_COMPLETION_SUCCESS = 0,
    MENU_COMPLETION_ERROR_INIT,
    MENU_COMPLETION_ERROR_DISPLAY,
    MENU_COMPLETION_ERROR_MEMORY,
    MENU_COMPLETION_ERROR_TERMINAL,
    MENU_COMPLETION_ERROR_CONFIG
} menu_completion_error_t;

// ============================================================================
// CORE FUNCTIONS
// ============================================================================

/**
 * Initialize the menu completion system
 * Sets up readline integration and initializes internal state
 *
 * @return true on success, false on failure
 */
bool lusush_menu_completion_init(void);

/**
 * Cleanup the menu completion system
 * Frees all allocated resources and resets state
 */
void lusush_menu_completion_cleanup(void);

/**
 * Handle TAB key for forward menu completion cycling
 * Called by readline when TAB is pressed in menu completion mode
 *
 * @param count Number of times key was pressed
 * @param key The key that was pressed (should be TAB)
 * @return 0 on success, -1 on error
 */
int lusush_menu_complete_handler(int count, int key);

/**
 * Handle Shift-TAB for backward menu completion cycling
 * Called by readline when Shift-TAB is pressed in menu completion mode
 *
 * @param count Number of times key was pressed  
 * @param key The key sequence that was pressed
 * @return 0 on success, -1 on error
 */
int lusush_menu_complete_backward_handler(int count, int key);

/**
 * Custom completion display hook for readline
 * Called by readline to display completions in menu format
 *
 * @param matches Array of completion matches
 * @param len Number of matches
 * @param max Maximum length of a match
 */
void lusush_display_completion_menu(char **matches, int len, int max);

/**
 * Reset menu completion state
 * Clears current completion session and prepares for new one
 */
void lusush_menu_completion_reset(void);

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

/**
 * Calculate optimal layout for menu display
 * Determines columns, rows, and formatting based on terminal size
 *
 * @param completions Array of completion strings
 * @param descriptions Array of descriptions (can be NULL)
 * @param count Number of completions
 * @return Layout information structure
 */
menu_layout_info_t lusush_calculate_menu_layout(char **completions, 
                                                char **descriptions, 
                                                int count);

/**
 * Render the completion menu with current selection
 * Displays completions in organized columns with highlighting
 *
 * @param state Current menu completion state
 */
void lusush_render_completion_menu(menu_completion_state_t *state);

/**
 * Update the display when selection changes
 * Efficiently updates only the changed parts of the display
 *
 * @param state Current menu completion state
 * @param old_selection Previous selection index
 * @param new_selection New selection index
 */
void lusush_update_menu_selection(menu_completion_state_t *state,
                                 int old_selection,
                                 int new_selection);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Validate menu completion state
 * Checks that the state structure is valid and consistent
 *
 * @param state Menu completion state to validate
 * @return true if valid, false if invalid
 */
bool lusush_validate_menu_state(menu_completion_state_t *state);

/**
 * Get terminal dimensions for layout calculation
 * Safely retrieves current terminal width and height
 *
 * @param width Pointer to store terminal width
 * @param height Pointer to store terminal height
 * @return true on success, false on failure
 */
bool lusush_get_terminal_size(int *width, int *height);

/**
 * Format completion with description for display
 * Creates formatted string combining completion and description
 *
 * @param completion The completion string
 * @param description The description string (can be NULL)
 * @param max_width Maximum width for formatting
 * @return Formatted string (caller must free)
 */
char *lusush_format_completion_entry(const char *completion,
                                    const char *description,
                                    int max_width);

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

/**
 * Setup menu completion based on current configuration
 * Configures readline bindings and behavior based on user settings
 *
 * @return true on success, false on failure
 */
bool lusush_setup_menu_completion(void);

/**
 * Update menu completion configuration at runtime
 * Applies new configuration settings without restart
 *
 * @return true on success, false on failure
 */
bool lusush_update_menu_completion_config(void);

/**
 * Get current menu completion configuration status
 * Returns whether menu completion is currently enabled and configured
 *
 * @return true if enabled and working, false otherwise
 */
bool lusush_is_menu_completion_active(void);

// ============================================================================
// PERFORMANCE AND DEBUGGING
// ============================================================================

/**
 * Performance statistics structure
 */
typedef struct {
    uint64_t menu_displays;         // Number of times menu was displayed
    uint64_t cache_hits;            // Number of completion cache hits
    uint64_t cache_misses;          // Number of completion cache misses
    double avg_display_time_ms;     // Average time to display menu
    double avg_cycling_time_ms;     // Average time to cycle selections
    uint64_t total_completions;     // Total completions processed
} menu_performance_stats_t;

/**
 * Get performance statistics
 * Returns current performance metrics for monitoring
 *
 * @return Performance statistics structure
 */
menu_performance_stats_t lusush_get_menu_performance_stats(void);

/**
 * Reset performance statistics
 * Clears all performance counters
 */
void lusush_reset_menu_performance_stats(void);

/**
 * Log debug information about menu completion state
 * Outputs detailed state information for debugging
 *
 * @param state Menu completion state to debug
 */
void lusush_debug_menu_state(const menu_completion_state_t *state);

// ============================================================================
// GLOBAL STATE ACCESS
// ============================================================================

/**
 * Get current menu completion state
 * Returns pointer to current state (read-only access)
 *
 * @return Current menu completion state or NULL if not initialized
 */
const menu_completion_state_t *lusush_get_menu_completion_state(void);

/**
 * Check if menu completion is currently in progress
 * Quick check for active completion session
 *
 * @return true if menu completion is active, false otherwise
 */
bool lusush_is_menu_completion_in_progress(void);

#endif /* MENU_COMPLETION_H */
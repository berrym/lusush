#ifndef LLE_DISPLAY_H
#define LLE_DISPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include "text_buffer.h"
#include "prompt.h"
#include "terminal_manager.h"
#include "cursor_math.h"
#include "syntax.h"
#include "theme_integration.h"

// Forward declaration to avoid circular dependency
struct lle_line_editor;

/**
 * @file display.h
 * @brief Lusush Line Editor - Display Management
 *
 * This module manages the display of prompts and input text, handling
 * multiline scenarios, cursor positioning, and efficient screen updates.
 * It coordinates between the prompt system, text buffer, and terminal
 * output to provide a complete line editing display system.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

/**
 * @brief Display flags for controlling display behavior
 */
typedef enum {
    LLE_DISPLAY_FLAG_NONE = 0,              /**< No special display flags */
    LLE_DISPLAY_FLAG_FORCE_REFRESH = 1 << 0, /**< Force complete display refresh */
    LLE_DISPLAY_FLAG_CURSOR_ONLY = 1 << 1,   /**< Update cursor position only */
    LLE_DISPLAY_FLAG_CLEAR_FIRST = 1 << 2,   /**< Clear display before rendering */
    LLE_DISPLAY_FLAG_NO_CURSOR = 1 << 3      /**< Don't show cursor after update */
} lle_display_flags_t;

/**
 * @brief Display cache for performance optimization (Phase 2C)
 *
 * Caches rendered content to avoid redundant calculations and terminal writes.
 * Tracks cache validity and provides hit/miss statistics for performance monitoring.
 */
typedef struct {
    char *cached_content;               /**< Cached rendered text content */
    size_t cache_size;                  /**< Size of cached content buffer */
    size_t cached_length;               /**< Length of valid cached content */
    bool cache_valid;                   /**< Whether cache is valid for current state */
    
    // Cache metadata for optimization
    size_t cached_text_length;         /**< Length of text when cache was created */
    size_t cached_cursor_position;     /**< Cursor position when cache was created */
    uint32_t cached_display_flags;     /**< Display flags when cache was created */
    
    // Performance metrics
    size_t cache_hits;                  /**< Number of successful cache hits */
    size_t cache_misses;                /**< Number of cache misses */
    size_t cache_updates;               /**< Number of cache updates */
} lle_display_cache_t;

/**
 * @brief Batched terminal operations for performance optimization (Phase 2C)
 *
 * Batches multiple terminal operations into single writes to reduce system call overhead.
 * Provides significant performance improvement for complex display updates.
 */
typedef struct {
    char *batch_buffer;                 /**< Buffer for batched operations */
    size_t buffer_size;                 /**< Size of batch buffer */
    size_t buffer_used;                 /**< Bytes used in batch buffer */ 
    bool batch_active;                  /**< Whether batching is currently active */
    
    // Performance tracking
    size_t operations_batched;          /**< Number of operations batched */
    size_t total_writes;                /**< Total number of write operations */
    size_t bytes_written;               /**< Total bytes written */
} lle_terminal_batch_t;

/**
 * @brief Performance metrics for display operations (Phase 2C)
 *
 * Tracks timing and efficiency metrics for display operations to validate
 * performance improvements and identify bottlenecks.
 */
typedef struct {
    // Timing metrics (in microseconds)
    uint64_t total_render_time;         /**< Total time spent in display renders */
    uint64_t total_incremental_time;    /**< Total time spent in incremental updates */
    uint64_t total_cache_time;          /**< Total time spent in cache operations */
    
    // Operation counters
    size_t render_calls;                /**< Number of full render calls */
    size_t incremental_calls;           /**< Number of incremental update calls */
    size_t cache_operations;            /**< Number of cache operations */
    
    // Performance targets (in microseconds)
    uint64_t target_char_insert_time;   /**< Target time for character insertion (1000µs) */
    uint64_t target_cursor_move_time;   /**< Target time for cursor movement (1000µs) */
    uint64_t target_display_update_time; /**< Target time for display update (5000µs) */
    
    // Efficiency metrics
    double cache_hit_rate;              /**< Cache hit rate percentage */
    double batch_efficiency;            /**< Batching efficiency percentage */
} lle_display_performance_t;

/**
 * @brief Visual footprint calculation for text content
 *
 * Represents the exact visual space occupied by text content on the terminal,
 * including line wrapping behavior and total display dimensions.
 */
typedef struct {
    size_t rows_used;                   /**< Number of terminal rows occupied by content */
    size_t end_column;                  /**< Column position on the last row */
    bool wraps_lines;                   /**< Whether content wraps across multiple lines */
    size_t total_visual_width;          /**< Total visual width including wrapping */
} lle_visual_footprint_t;

/**
 * @brief Display state for managing complete line editor display
 *
 * This structure maintains all state needed for displaying the complete
 * line editor interface including prompt, input text, and cursor position.
 * It provides efficient updates and handles complex multiline scenarios.
 */
typedef struct {
    lle_prompt_t *prompt;               /**< Prompt structure for display */
    lle_text_buffer_t *buffer;          /**< Text buffer containing input */
    lle_terminal_manager_t *terminal;   /**< Terminal manager for output */
    lle_cursor_position_t cursor_pos;   /**< Current cursor position */
    lle_terminal_geometry_t geometry;   /**< Cached terminal geometry */
    size_t display_start_offset;       /**< Start offset for horizontal scrolling */
    size_t display_start_line;         /**< Start line for vertical scrolling */
    size_t last_rendered_length;       /**< Length of last rendered content */
    size_t last_rendered_lines;        /**< Number of lines in last render */
    bool needs_refresh;                 /**< True if display needs complete refresh */
    bool cursor_visible;                /**< True if cursor should be visible */
    bool initialized;                   /**< True if display state is initialized */
    uint32_t display_flags;             /**< Display behavior flags */
    
    // Multi-line absolute position tracking (Phase 1A: Architecture Rewrite)
    size_t prompt_start_row;            /**< Absolute terminal row where prompt starts (0-based) */
    size_t prompt_start_col;            /**< Absolute terminal column where prompt starts (0-based) */
    size_t prompt_end_row;              /**< Absolute terminal row where prompt ends (0-based) */
    size_t prompt_end_col;              /**< Absolute terminal column where prompt ends (0-based) */
    size_t content_start_row;           /**< Absolute terminal row where text content starts (0-based) */
    size_t content_start_col;           /**< Absolute terminal column where text content starts (0-based) */
    size_t content_end_row;             /**< Absolute terminal row where text content ends (0-based) */
    size_t content_end_col;             /**< Absolute terminal column where text content ends (0-based) */
    bool position_tracking_valid;       /**< True if position tracking data is current */
    
    // Syntax highlighting integration
    lle_syntax_highlighter_t *syntax_highlighter;  /**< Syntax highlighter for coloring */
    lle_theme_integration_t *theme_integration;    /**< Theme integration for colors */
    bool syntax_highlighting_enabled;              /**< Enable/disable syntax highlighting */
    char last_applied_color[32];                   /**< Last applied color code for optimization */
    
    // Phase 2C: Performance optimization components
    lle_display_cache_t display_cache;             /**< Display cache for performance optimization */
    lle_terminal_batch_t terminal_batch;           /**< Batched terminal operations */
    lle_display_performance_t performance_metrics; /**< Performance metrics and timing */
    bool performance_optimization_enabled;         /**< Enable/disable performance optimizations */
    
    // Display state tracking for true incremental updates (Character Duplication Fix)
    char last_displayed_content[512];              /**< What was actually written to screen */
    size_t last_displayed_length;                  /**< Length of what was displayed */
    bool display_state_valid;                      /**< Whether tracking is valid */
    
    // Enhanced visual footprint tracking for backspace refinement
    size_t last_visual_rows;                       /**< Number of terminal rows used in last render */
    size_t last_visual_end_col;                    /**< Column position on last row after render */
    size_t last_total_chars;                       /**< Total characters rendered in last update */
    bool last_had_wrapping;                        /**< Whether content wrapped lines in last render */
    
    // Consistency tracking for unified rendering behavior
    uint32_t last_content_hash;                    /**< Hash of last rendered content for change detection */
    bool syntax_highlighting_applied;              /**< Track whether syntax highlighting was applied */
    
    // Clearing state for intelligent region clearing
    size_t clear_start_row;                        /**< Row where clearing should begin */
    size_t clear_start_col;                        /**< Column where clearing should begin */
    size_t clear_end_row;                          /**< Row where clearing should end */
    size_t clear_end_col;                          /**< Column where clearing should end */
    bool clear_region_valid;                       /**< Whether clearing region coordinates are valid */
} lle_display_state_t;

/**
 * @brief Initialize display state structure
 *
 * Initializes all fields of the display state to safe defaults.
 * Must be called before using any other display functions.
 *
 * @param state Display state structure to initialize
 * @return true on success, false if state is NULL
 */
bool lle_display_init(lle_display_state_t *state);

/**
 * @brief Create a new display state with associated components
 *
 * Creates a new display state and associates it with the provided
 * prompt, text buffer, and terminal manager.
 *
 * @param prompt Prompt structure to associate
 * @param buffer Text buffer to associate
 * @param terminal Terminal manager to associate
 * @return Pointer to new display state, NULL on failure
 */
lle_display_state_t *lle_display_create(
    lle_prompt_t *prompt,
    lle_text_buffer_t *buffer,
    lle_terminal_manager_t *terminal
);

/**
 * @brief Clean up display state
 *
 * Cleans up display state but does not free the associated
 * prompt, buffer, or terminal manager structures.
 *
 * @param state Display state to clean up
 * @return true on success, false if state is NULL
 */
bool lle_display_cleanup(lle_display_state_t *state);

/**
 * @brief Destroy display state and free memory
 *
 * Destroys the display state structure and frees memory.
 * Does not affect associated prompt, buffer, or terminal structures.
 *
 * @param state Display state to destroy (may be NULL)
 */
void lle_display_destroy(lle_display_state_t *state);

/**
 * @brief Validate display state structure
 *
 * Checks that all fields of the display state are in a valid state.
 * Useful for debugging and ensuring structure consistency.
 *
 * @param state Display state to validate
 * @return true if structure is valid, false otherwise
 */
bool lle_display_validate(const lle_display_state_t *state);

/**
 * @brief Render complete display (prompt + input text)
 *
 * Renders the complete line editor display including prompt and input text.
 * Handles multiline scenarios and updates cursor position.
 *
 * @param state Display state containing all components
 * @return true on success, false on error
 */
bool lle_display_render(lle_display_state_t *state);

/**
 * @brief Update cursor position only
 *
 * Updates only the cursor position without re-rendering text.
 * Used for efficient cursor movement operations.
 *
 * @param state Display state with current cursor position
 * @return true on success, false on error
 */
bool lle_display_update_cursor(lle_display_state_t *state);

/**
 * @brief Clear the display
 *
 * Clears the current display including prompt and input text.
 * Leaves cursor at the beginning of the cleared area.
 *
 * @param state Display state to clear
 * @return true on success, false on error
 */
bool lle_display_clear(lle_display_state_t *state);

/**
 * @brief Refresh display with current buffer content
 *
 * Forces a complete refresh of the display with current buffer content.
 * More efficient than clear + render for updates.
 *
 * @param state Display state to refresh
 * @return true on success, false on error
 */
bool lle_display_refresh(lle_display_state_t *state);

/**
 * @brief Update display incrementally without redrawing prompt
 *
 * Efficiently updates only the text content without redrawing the prompt.
 * This prevents the visual chaos caused by constant prompt redraws during
 * character-by-character input. Falls back to cursor-only updates for
 * complex scenarios like line wrapping.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
bool lle_display_update_incremental(lle_display_state_t *state);

/**
 * @brief Update display after text insertion
 *
 * Efficiently updates display after text has been inserted into the buffer.
 * Handles cursor repositioning and minimizes screen updates.
 *
 * @param state Display state to update
 * @param insert_offset Offset where text was inserted
 * @param insert_length Length of inserted text
 * @return true on success, false on error
 */
bool lle_display_update_after_insert(
    lle_display_state_t *state,
    size_t insert_offset,
    size_t insert_length
);

/**
 * @brief Update display after text deletion
 *
 * Efficiently updates display after text has been deleted from the buffer.
 * Handles cursor repositioning and screen cleanup.
 *
 * @param state Display state to update
 * @param delete_offset Offset where text was deleted
 * @param delete_length Length of deleted text
 * @return true on success, false on error
 */
bool lle_display_update_after_delete(
    lle_display_state_t *state,
    size_t delete_offset,
    size_t delete_length
);

/**
 * @brief Set cursor position and update display
 *
 * Sets the cursor to a specific position in the text buffer
 * and updates the display accordingly.
 *
 * @param state Display state to update
 * @param offset Byte offset in text buffer for cursor
 * @return true on success, false on error
 */
bool lle_display_set_cursor_offset(lle_display_state_t *state, size_t offset);

/**
 * @brief Calculate cursor position for current buffer offset
 *
 * Calculates the terminal cursor position for the current
 * text buffer cursor offset, accounting for prompt and wrapping.
 *
 * @param state Display state with current configuration
 * @param cursor_pos Output cursor position structure
 * @return true on success, false on error
 */
bool lle_display_calculate_cursor_position(
    const lle_display_state_t *state,
    lle_cursor_position_t *cursor_pos
);

/**
 * @brief Set display flags for controlling behavior
 *
 * Sets display flags that control various aspects of display behavior
 * such as refresh strategy and cursor visibility.
 *
 * @param state Display state to modify
 * @param flags Display flags to set
 * @return true on success, false if state is NULL
 */
bool lle_display_set_flags(lle_display_state_t *state, uint32_t flags);

/**
 * @brief Get current display flags
 *
 * Returns the current display flags for the given display state.
 *
 * @param state Display state to query
 * @return Current display flags, 0 if state is NULL
 */
uint32_t lle_display_get_flags(const lle_display_state_t *state);

/**
 * @brief Check if display needs refresh
 *
 * Returns true if the display needs a complete refresh due to
 * changes in terminal size or other factors.
 *
 * @param state Display state to check
 * @return true if refresh needed, false otherwise
 */
bool lle_display_needs_refresh(const lle_display_state_t *state);

/**
 * @brief Update terminal geometry cache
 *
 * Updates the cached terminal geometry information and determines
 * if display refresh is needed due to size changes.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
bool lle_display_update_geometry(lle_display_state_t *state);

/**
 * @brief Get display statistics
 *
 * Returns information about the current display state including
 * number of lines rendered and cursor position.
 *
 * @param state Display state to query
 * @param lines_rendered Output for number of lines rendered
 * @param chars_rendered Output for number of characters rendered
 * @param cursor_line Output for current cursor line
 * @param cursor_col Output for current cursor column
 * @return true on success, false if any parameter is NULL
 */
bool lle_display_get_statistics(
    const lle_display_state_t *state,
    size_t *lines_rendered,
    size_t *chars_rendered,
    size_t *cursor_line,
    size_t *cursor_col
);

// ============================================================================
// Phase 2C: Performance Optimization Functions
// ============================================================================

/**
 * @brief Initialize display cache for performance optimization
 *
 * Initializes the display cache system with the specified buffer size.
 * Must be called before using any cache-related functions.
 *
 * @param cache Display cache structure to initialize
 * @param buffer_size Size of cache buffer to allocate
 * @return true on success, false on error
 */
bool lle_display_cache_init(lle_display_cache_t *cache, size_t buffer_size);

/**
 * @brief Clean up display cache resources
 *
 * Frees all resources associated with the display cache.
 * Cache becomes invalid after this call.
 *
 * @param cache Display cache to clean up
 * @return true on success, false if cache is NULL
 */
bool lle_display_cache_cleanup(lle_display_cache_t *cache);

/**
 * @brief Check if cached content is valid for current state
 *
 * Validates that cached content matches current display state.
 * Returns true if cache can be used, false if update is needed.
 *
 * @param state Display state to check against cache
 * @return true if cache is valid, false if cache miss
 */
bool lle_display_cache_is_valid(const lle_display_state_t *state);

/**
 * @brief Update display cache with current rendered content
 *
 * Stores the current rendered content in the cache for future use.
 * Invalidates old cache and updates metadata.
 *
 * @param state Display state with current content
 * @param content Rendered content to cache
 * @param length Length of content to cache
 * @return true on success, false on error
 */
bool lle_display_cache_update(lle_display_state_t *state, const char *content, size_t length);

/**
 * @brief Initialize terminal batching system
 *
 * Initializes the terminal batch system for combining multiple
 * terminal operations into single writes.
 *
 * @param batch Terminal batch structure to initialize
 * @param buffer_size Size of batch buffer to allocate
 * @return true on success, false on error
 */
bool lle_terminal_batch_init(lle_terminal_batch_t *batch, size_t buffer_size);

/**
 * @brief Clean up terminal batching resources
 *
 * Flushes any pending operations and frees batch resources.
 * Batch becomes invalid after this call.
 *
 * @param batch Terminal batch to clean up
 * @return true on success, false if batch is NULL
 */
bool lle_terminal_batch_cleanup(lle_terminal_batch_t *batch);

/**
 * @brief Start terminal operation batching
 *
 * Begins batching terminal operations for later execution.
 * All subsequent terminal writes will be batched until flush.
 *
 * @param batch Terminal batch to start
 * @return true on success, false on error
 */
bool lle_terminal_batch_start(lle_terminal_batch_t *batch);

/**
 * @brief Add operation to terminal batch
 *
 * Adds a terminal operation to the current batch.
 * Operation will be executed when batch is flushed.
 *
 * @param batch Terminal batch to add to
 * @param data Data to add to batch
 * @param length Length of data to add
 * @return true on success, false if batch is full
 */
bool lle_terminal_batch_add(lle_terminal_batch_t *batch, const char *data, size_t length);

/**
 * @brief Flush all batched terminal operations
 *
 * Executes all batched terminal operations in a single write.
 * Clears the batch for new operations after execution.
 *
 * @param state Display state with terminal for output
 * @return true on success, false on error
 */
bool lle_terminal_batch_flush(lle_display_state_t *state);

/**
 * @brief Initialize performance metrics tracking
 *
 * Initializes performance metrics with target values.
 * Must be called before performance monitoring begins.
 *
 * @param metrics Performance metrics structure to initialize
 * @return true on success, false if metrics is NULL
 */
bool lle_display_performance_init(lle_display_performance_t *metrics);

/**
 * @brief Start timing a display operation
 *
 * Begins timing measurement for a display operation.
 * Returns timestamp for use with lle_display_performance_end_timing.
 *
 * @return Timestamp in microseconds, 0 on error
 */
uint64_t lle_display_performance_start_timing(void);

/**
 * @brief End timing and record performance metric
 *
 * Ends timing measurement and records the elapsed time.
 * Updates appropriate performance counters and averages.
 *
 * @param metrics Performance metrics to update
 * @param start_time Start timestamp from lle_display_performance_start_timing
 * @param operation_type Type of operation being timed
 * @return Elapsed time in microseconds
 */
uint64_t lle_display_performance_end_timing(lle_display_performance_t *metrics, 
                                           uint64_t start_time, 
                                           const char *operation_type);

/**
 * @brief Get current performance statistics
 *
 * Returns current performance metrics including timing averages,
 * cache hit rates, and efficiency measurements.
 *
 * @param state Display state with performance metrics
 * @param avg_render_time Output for average render time (microseconds)
 * @param avg_incremental_time Output for average incremental time (microseconds)
 * @param cache_hit_rate Output for cache hit rate percentage
 * @param batch_efficiency Output for batch efficiency percentage
 * @return true on success, false if any parameter is NULL
 */
bool lle_display_get_performance_stats(const lle_display_state_t *state,
                                      uint64_t *avg_render_time,
                                      uint64_t *avg_incremental_time,
                                      double *cache_hit_rate,
                                      double *batch_efficiency);

/**
 * @brief Enable or disable performance optimizations
 *
 * Controls whether performance optimizations (caching, batching) are active.
 * Can be used to disable optimizations for debugging or compatibility.
 *
 * @param state Display state to configure
 * @param enabled true to enable optimizations, false to disable
 * @return true on success, false if state is NULL
 */
bool lle_display_set_performance_optimization(lle_display_state_t *state, bool enabled);

// ============================================================================
// Syntax Highlighting Integration Functions
// ============================================================================

/**
 * @brief Set syntax highlighter for display
 *
 * Associates a syntax highlighter with the display state for syntax coloring.
 * The display state does not take ownership of the highlighter.
 *
 * @param state Display state to modify
 * @param highlighter Syntax highlighter to associate (can be NULL to disable)
 * @return true on success, false if state is NULL
 */
bool lle_display_set_syntax_highlighter(lle_display_state_t *state,
                                        lle_syntax_highlighter_t *highlighter);

/**
 * @brief Set theme integration for display
 *
 * Associates a theme integration system with the display state for color mapping.
 * The display state does not take ownership of the theme integration.
 *
 * @param state Display state to modify
 * @param theme_integration Theme integration to associate (can be NULL to disable)
 * @return true on success, false if state is NULL
 */
bool lle_display_set_theme_integration(lle_display_state_t *state,
                                       lle_theme_integration_t *theme_integration);

/**
 * @brief Enable or disable syntax highlighting in display
 *
 * Controls whether syntax highlighting is applied during text rendering.
 * Requires both syntax highlighter and theme integration to be set.
 *
 * @param state Display state to modify
 * @param enable true to enable syntax highlighting, false to disable
 * @return true on success, false if state is NULL
 */
bool lle_display_enable_syntax_highlighting(lle_display_state_t *state, bool enable);

/**
 * @brief Check if syntax highlighting is enabled
 *
 * Returns whether syntax highlighting is currently enabled and functional.
 *
 * @param state Display state to check
 * @return true if syntax highlighting is enabled and functional, false otherwise
 */
bool lle_display_is_syntax_highlighting_enabled(const lle_display_state_t *state);

/**
 * @brief Update syntax highlighting for current buffer content
 *
 * Triggers syntax highlighting update for the current buffer content.
 * Should be called when buffer content changes.
 *
 * @param state Display state with syntax highlighter
 * @return true on success, false on error
 */
bool lle_display_update_syntax_highlighting(lle_display_state_t *state);

/**
 * @brief Render text with syntax highlighting
 *
 * Enhanced text rendering function that applies syntax highlighting colors
 * based on syntax regions and theme integration.
 *
 * @param state Display state with syntax highlighting configured
 * @param text Text to render
 * @param length Length of text
 * @param start_col Starting column position
 * @return true on success, false on error
 */
bool lle_display_render_with_syntax_highlighting(lle_display_state_t *state,
                                                 const char *text,
                                                 size_t length,
                                                 size_t start_col);

// ============================================================================
// Cursor Movement Convenience APIs for Keybinding Integration
// ============================================================================

/**
 * @brief Move cursor to beginning of line with proper display update
 *
 * Combines buffer cursor movement with display system cursor positioning
 * for keybinding implementations. Ensures display state consistency.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
bool lle_display_move_cursor_home(lle_display_state_t *state);

/**
 * @brief Move cursor to end of line with proper display update
 *
 * Combines buffer cursor movement with display system cursor positioning
 * for keybinding implementations. Ensures display state consistency.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
bool lle_display_move_cursor_end(lle_display_state_t *state);

/**
 * @brief Clear entire line with proper display update
 *
 * Clears the text buffer and updates display without breaking state
 * consistency. Used for Ctrl+U and Ctrl+G keybinding implementations.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
bool lle_display_clear_line(lle_display_state_t *state);

/**
 * @brief Enter reverse search mode with proper display management
 *
 * Transitions display system into reverse search mode without manual
 * terminal operations. Manages state consistency throughout search.
 *
 * @param state Display state to update
 * @return true on success, false on error
 */
bool lle_display_enter_search_mode(lle_display_state_t *state);

/**
 * @brief Exit reverse search mode with proper display restoration
 *
 * Restores normal display mode from reverse search without manual
 * terminal operations. Ensures proper state cleanup and cursor positioning.
 *
 * @param state Display state to restore
 * @return true on success, false on error
 */
bool lle_display_exit_search_mode(lle_display_state_t *state);

/**
 * @brief Update search prompt and results with proper display management
 *
 * Updates the search prompt and matched text without manual terminal
 * operations. Maintains display state consistency during search operations.
 *
 * @param state Display state to update
 * @param search_term Current search term
 * @param search_length Length of search term
 * @param match_text Matched history text (can be NULL)
 * @param match_length Length of matched text
 * @return true on success, false on error
 */
bool lle_display_update_search_prompt(lle_display_state_t *state,
                                      const char *search_term,
                                      size_t search_length,
                                      const char *match_text,
                                      size_t match_length);

/**
 * @brief Calculate the exact visual footprint of text content
 *
 * Calculates how much visual space text content will occupy on the terminal,
 * including line wrapping behavior and total display dimensions.
 *
 * @param text Text content to analyze
 * @param length Length of text content
 * @param prompt_width Width of prompt on first line
 * @param terminal_width Width of terminal for wrapping calculations
 * @param footprint Output structure for calculated footprint
 * @return true on success, false on error
 */
bool lle_calculate_visual_footprint(const char *text, size_t length,
                                   size_t prompt_width, size_t terminal_width,
                                   lle_visual_footprint_t *footprint);

/**
 * @brief Clear exact visual region used by previous content
 *
 * Intelligently clears the visual region occupied by previous content,
 * handling line wrapping and multi-line scenarios correctly.
 *
 * @param tm Terminal manager for clearing operations
 * @param old_footprint Visual footprint of content to clear
 * @param new_footprint Visual footprint of new content (for optimization)
 * @return true on success, false on error
 */
bool lle_clear_visual_region(lle_terminal_manager_t *tm,
                            const lle_visual_footprint_t *old_footprint,
                            const lle_visual_footprint_t *new_footprint);

/**
 * @brief Robust fallback clearing when cursor position queries fail
 *
 * Provides a robust fallback clearing mechanism for situations where
 * cursor position tracking is unavailable or unreliable.
 *
 * @param tm Terminal manager for clearing operations
 * @param footprint Visual footprint of content to clear
 * @return true on success, false on error
 */
bool lle_clear_multi_line_fallback(lle_terminal_manager_t *tm,
                                   const lle_visual_footprint_t *footprint);

/**
 * @brief Ensure consistent rendering regardless of path taken
 *
 * Provides unified rendering behavior to prevent inconsistencies between
 * incremental updates and fallback rewrites.
 *
 * @param display Display state for rendering
 * @param force_full_render Whether to force complete rerender
 * @return true on success, false on error
 */
bool lle_display_update_unified(lle_display_state_t *display,
                               bool force_full_render);

/**
 * @brief Apply consistent highlighting policy
 *
 * Ensures syntax highlighting is applied consistently regardless of
 * whether content arrived via incremental updates or fallback rewrites.
 *
 * @param display Display state for highlighting
 * @param old_footprint Previous content footprint
 * @param new_footprint New content footprint
 * @return true on success, false on error
 */
bool lle_render_with_consistent_highlighting(lle_display_state_t *display,
                                            const lle_visual_footprint_t *old_footprint,
                                            const lle_visual_footprint_t *new_footprint);



#endif // LLE_DISPLAY_H
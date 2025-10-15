// src/lle/foundation/display/display_buffer.h
//
// LLE Display-Buffer Integration (Week 9)
//
// This module bridges the gap buffer system and the display system,
// handling the rendering of buffer content to the screen with proper
// scroll region management and cursor synchronization.
//
// Key responsibilities:
// - Render buffer content to display cells
// - Manage viewport (visible region of buffer)
// - Synchronize buffer cursor with screen cursor
// - Handle horizontal/vertical scrolling
// - Line wrapping (basic support, full multiline in Month 3)

#ifndef LLE_FOUNDATION_DISPLAY_BUFFER_H
#define LLE_FOUNDATION_DISPLAY_BUFFER_H

#include "../buffer/buffer.h"
#include "../buffer/buffer_manager.h"
#include "display.h"
#include <stdint.h>
#include <stdbool.h>

// Viewport state (which part of buffer is visible)
typedef struct {
    size_t top_line;        // First visible line (0-indexed)
    size_t left_column;     // First visible column (horizontal scroll)
    size_t visible_lines;   // Number of lines visible on screen
    size_t visible_cols;    // Number of columns visible on screen
} lle_viewport_t;

// Display-buffer renderer context
typedef struct {
    // References (not owned)
    lle_buffer_manager_t *buffer_manager;
    lle_display_t *display;
    
    // Viewport state
    lle_viewport_t viewport;
    
    // Cursor tracking
    size_t buffer_cursor_line;     // Cursor line in buffer
    size_t buffer_cursor_col;      // Cursor column in buffer
    uint16_t screen_cursor_row;    // Cursor row on screen
    uint16_t screen_cursor_col;    // Cursor column on screen
    
    // Configuration
    bool auto_scroll;              // Auto-scroll to keep cursor visible
    bool show_line_numbers;        // Display line numbers (Phase 2)
    bool wrap_long_lines;          // Wrap lines longer than screen width
    size_t tab_width;              // Tab display width
    
    // Rendering options
    bool syntax_highlighting;      // Enable syntax highlighting (Week 11)
    uint8_t default_fg_color;      // Default foreground color
    uint8_t default_bg_color;      // Default background color
    
    // Performance tracking
    uint64_t render_count;
    uint64_t scroll_count;
    uint64_t cursor_sync_count;
    
    // State
    bool initialized;
} lle_display_buffer_renderer_t;

// Error codes
typedef enum {
    LLE_DISPLAY_BUFFER_OK = 0,
    LLE_DISPLAY_BUFFER_ERR_NULL_PTR = -1,
    LLE_DISPLAY_BUFFER_ERR_NOT_INIT = -2,
    LLE_DISPLAY_BUFFER_ERR_NO_BUFFER = -3,
    LLE_DISPLAY_BUFFER_ERR_DISPLAY_FAILED = -4,
    LLE_DISPLAY_BUFFER_ERR_INVALID_VIEWPORT = -5
} lle_display_buffer_error_t;

// Initialize display-buffer renderer
int lle_display_buffer_init(lle_display_buffer_renderer_t *renderer,
                            lle_buffer_manager_t *buffer_manager,
                            lle_display_t *display);

// Configure renderer options
void lle_display_buffer_set_auto_scroll(lle_display_buffer_renderer_t *renderer, bool enable);
void lle_display_buffer_set_wrap_lines(lle_display_buffer_renderer_t *renderer, bool enable);
void lle_display_buffer_set_tab_width(lle_display_buffer_renderer_t *renderer, size_t width);
void lle_display_buffer_set_colors(lle_display_buffer_renderer_t *renderer,
                                   uint8_t fg_color,
                                   uint8_t bg_color);

// Render current buffer to display
// This is the main rendering function - updates display cells from buffer content
int lle_display_buffer_render(lle_display_buffer_renderer_t *renderer);

// Render specific line range
int lle_display_buffer_render_lines(lle_display_buffer_renderer_t *renderer,
                                    size_t start_line,
                                    size_t end_line);

// Viewport management
int lle_display_buffer_set_viewport(lle_display_buffer_renderer_t *renderer,
                                    size_t top_line,
                                    size_t left_column);

int lle_display_buffer_scroll_up(lle_display_buffer_renderer_t *renderer, size_t lines);
int lle_display_buffer_scroll_down(lle_display_buffer_renderer_t *renderer, size_t lines);
int lle_display_buffer_scroll_left(lle_display_buffer_renderer_t *renderer, size_t cols);
int lle_display_buffer_scroll_right(lle_display_buffer_renderer_t *renderer, size_t cols);

// Auto-scroll to ensure cursor is visible
int lle_display_buffer_ensure_cursor_visible(lle_display_buffer_renderer_t *renderer);

// Get viewport info
const lle_viewport_t* lle_display_buffer_get_viewport(const lle_display_buffer_renderer_t *renderer);

// Cursor synchronization
// Updates buffer cursor position from screen cursor (after user input)
int lle_display_buffer_sync_cursor_to_buffer(lle_display_buffer_renderer_t *renderer,
                                             uint16_t screen_row,
                                             uint16_t screen_col);

// Updates screen cursor position from buffer cursor (after buffer operations)
int lle_display_buffer_sync_cursor_to_screen(lle_display_buffer_renderer_t *renderer);

// Get cursor positions
void lle_display_buffer_get_buffer_cursor(const lle_display_buffer_renderer_t *renderer,
                                         size_t *line,
                                         size_t *column);

void lle_display_buffer_get_screen_cursor(const lle_display_buffer_renderer_t *renderer,
                                         uint16_t *row,
                                         uint16_t *col);

// Buffer-to-screen coordinate conversion
int lle_display_buffer_buffer_to_screen(const lle_display_buffer_renderer_t *renderer,
                                       size_t buffer_line,
                                       size_t buffer_col,
                                       uint16_t *screen_row,
                                       uint16_t *screen_col);

// Screen-to-buffer coordinate conversion
int lle_display_buffer_screen_to_buffer(const lle_display_buffer_renderer_t *renderer,
                                       uint16_t screen_row,
                                       uint16_t screen_col,
                                       size_t *buffer_line,
                                       size_t *buffer_col);

// Handle terminal resize
int lle_display_buffer_handle_resize(lle_display_buffer_renderer_t *renderer,
                                     uint16_t new_rows,
                                     uint16_t new_cols);

// Clear display and reset viewport
int lle_display_buffer_clear(lle_display_buffer_renderer_t *renderer);

// Performance metrics
void lle_display_buffer_get_metrics(const lle_display_buffer_renderer_t *renderer,
                                    uint64_t *render_count,
                                    uint64_t *scroll_count,
                                    uint64_t *cursor_sync_count);

// Cleanup
void lle_display_buffer_cleanup(lle_display_buffer_renderer_t *renderer);

// Utility: Convert error code to string
const char* lle_display_buffer_error_string(int error_code);

// TODO Phase 1 Month 2: UTF-8 support for character positioning
// - Handle multi-byte character display width
// - Proper cursor positioning in UTF-8 text
// - Column calculations accounting for wide characters

// TODO Phase 1 Month 3: Advanced multiline support
// - Soft wrapping with proper reflow
// - Long line display optimization
// - Virtual line tracking

// TODO Phase 2: Advanced rendering features
// - Line number display in gutter
// - Incremental rendering (only changed lines)
// - Double-buffering optimization

#endif // LLE_FOUNDATION_DISPLAY_BUFFER_H

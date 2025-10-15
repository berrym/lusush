// src/lle/foundation/display/display_buffer.c
//
// LLE Display-Buffer Integration Implementation

#include "display_buffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Helper: Count lines in buffer
static size_t count_buffer_lines(const lle_buffer_t *buffer) {
    size_t count = lle_buffer_line_count(buffer);
    // Empty buffer has 1 line (empty line)
    return (count == 0) ? 1 : count;
}

// Helper: Get line content from buffer
// Returns line length, fills dest with line content (without newline)
static size_t get_buffer_line(const lle_buffer_t *buffer,
                             size_t line_num,
                             char *dest,
                             size_t dest_size) {
    if (!buffer || !dest || dest_size == 0) {
        return 0;
    }
    
    // Find line start
    size_t pos = 0;
    for (size_t i = 0; i < line_num; i++) {
        size_t line_start = lle_buffer_line_start(buffer, pos);
        if (line_start == pos && i > 0) {
            // No more lines
            dest[0] = '\0';
            return 0;
        }
        
        // Find next newline
        char ch;
        while (pos < lle_buffer_size(buffer)) {
            if (lle_buffer_get_char(buffer, pos, &ch) == 0 && ch == '\n') {
                pos++; // Skip newline
                break;
            }
            pos++;
        }
    }
    
    // Now pos is at start of target line
    size_t line_len = 0;
    char ch;
    while (line_len < dest_size - 1 && pos < lle_buffer_size(buffer)) {
        if (lle_buffer_get_char(buffer, pos, &ch) != 0) {
            break;
        }
        if (ch == '\n') {
            break; // End of line
        }
        dest[line_len++] = ch;
        pos++;
    }
    
    dest[line_len] = '\0';
    return line_len;
}

// Helper: Calculate cursor line and column from buffer position
static void calculate_cursor_position(const lle_buffer_t *buffer,
                                     size_t cursor_pos,
                                     size_t *out_line,
                                     size_t *out_col) {
    size_t line = 0;
    size_t col = 0;
    
    // Count newlines before cursor
    for (size_t i = 0; i < cursor_pos && i < lle_buffer_size(buffer); i++) {
        char ch;
        if (lle_buffer_get_char(buffer, i, &ch) == 0 && ch == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    
    *out_line = line;
    *out_col = col;
}

// Helper: Expand tabs in line
static size_t expand_tabs(const char *src, size_t src_len,
                         char *dest, size_t dest_size,
                         size_t tab_width) {
    size_t dest_pos = 0;
    size_t col = 0;
    
    for (size_t i = 0; i < src_len && dest_pos < dest_size - 1; i++) {
        if (src[i] == '\t') {
            // Expand tab to spaces
            size_t spaces = tab_width - (col % tab_width);
            for (size_t j = 0; j < spaces && dest_pos < dest_size - 1; j++) {
                dest[dest_pos++] = ' ';
                col++;
            }
        } else {
            dest[dest_pos++] = src[i];
            col++;
        }
    }
    
    dest[dest_pos] = '\0';
    return dest_pos;
}

// Initialize display-buffer renderer
int lle_display_buffer_init(lle_display_buffer_renderer_t *renderer,
                            lle_buffer_manager_t *buffer_manager,
                            lle_display_t *display) {
    if (!renderer || !buffer_manager || !display) {
        return LLE_DISPLAY_BUFFER_ERR_NULL_PTR;
    }
    
    memset(renderer, 0, sizeof(*renderer));
    
    renderer->buffer_manager = buffer_manager;
    renderer->display = display;
    
    // Get display dimensions for viewport
    renderer->viewport.visible_lines = renderer->display->buffer.rows;
    renderer->viewport.visible_cols = renderer->display->buffer.cols;
    renderer->viewport.top_line = 0;
    renderer->viewport.left_column = 0;
    
    // Default configuration
    renderer->auto_scroll = true;
    renderer->wrap_long_lines = true;
    renderer->tab_width = 4;
    renderer->show_line_numbers = false;
    renderer->syntax_highlighting = false;
    
    // Default colors (0 = default terminal colors)
    renderer->default_fg_color = 0;
    renderer->default_bg_color = 0;
    
    renderer->initialized = true;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Configure renderer options
void lle_display_buffer_set_auto_scroll(lle_display_buffer_renderer_t *renderer, bool enable) {
    if (renderer) {
        renderer->auto_scroll = enable;
    }
}

void lle_display_buffer_set_wrap_lines(lle_display_buffer_renderer_t *renderer, bool enable) {
    if (renderer) {
        renderer->wrap_long_lines = enable;
    }
}

void lle_display_buffer_set_tab_width(lle_display_buffer_renderer_t *renderer, size_t width) {
    if (renderer && width > 0 && width <= 16) {
        renderer->tab_width = width;
    }
}

void lle_display_buffer_set_colors(lle_display_buffer_renderer_t *renderer,
                                   uint8_t fg_color,
                                   uint8_t bg_color) {
    if (renderer) {
        renderer->default_fg_color = fg_color;
        renderer->default_bg_color = bg_color;
    }
}

// Render current buffer to display
int lle_display_buffer_render(lle_display_buffer_renderer_t *renderer) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    lle_managed_buffer_t *managed = lle_buffer_manager_get_current(renderer->buffer_manager);
    if (!managed) {
        return LLE_DISPLAY_BUFFER_ERR_NO_BUFFER;
    }
    
    lle_buffer_t *buffer = &managed->buffer;
    
    // Calculate cursor position in buffer
    size_t cursor_pos = buffer->gap_start;  // gap_start is the cursor position
    calculate_cursor_position(buffer, cursor_pos,
                             &renderer->buffer_cursor_line,
                             &renderer->buffer_cursor_col);
    
    // Auto-scroll to keep cursor visible if enabled
    if (renderer->auto_scroll) {
        lle_display_buffer_ensure_cursor_visible(renderer);
    }
    
    // Clear display
    lle_display_clear(renderer->display);
    
    // Render visible lines
    size_t total_lines = count_buffer_lines(buffer);
    size_t visible_lines = renderer->viewport.visible_lines;
    size_t visible_cols = renderer->viewport.visible_cols;
    
    char line_buffer[4096]; // Line buffer (TODO Phase 1 Month 2: dynamic sizing)
    char expanded_buffer[4096]; // For tab expansion
    
    for (size_t i = 0; i < visible_lines; i++) {
        size_t buffer_line = renderer->viewport.top_line + i;
        
        if (buffer_line >= total_lines) {
            // Past end of buffer - render empty line or tilde
            break;
        }
        
        // Get line from buffer
        size_t line_len = get_buffer_line(buffer, buffer_line,
                                         line_buffer, sizeof(line_buffer));
        
        // Expand tabs
        size_t expanded_len = expand_tabs(line_buffer, line_len,
                                         expanded_buffer, sizeof(expanded_buffer),
                                         renderer->tab_width);
        
        // Apply horizontal scroll
        const char *render_text = expanded_buffer;
        size_t render_len = expanded_len;
        
        if (renderer->viewport.left_column > 0) {
            if (renderer->viewport.left_column < expanded_len) {
                render_text += renderer->viewport.left_column;
                render_len -= renderer->viewport.left_column;
            } else {
                render_len = 0;
            }
        }
        
        // Truncate or wrap to visible width
        if (!renderer->wrap_long_lines && render_len > visible_cols) {
            render_len = visible_cols;
        }
        
        // Render line to display
        if (render_len > 0) {
            int result = lle_display_render_line(renderer->display,
                                                (uint16_t)i,
                                                render_text,
                                                render_len);
            if (result != LLE_DISPLAY_OK) {
                return LLE_DISPLAY_BUFFER_ERR_DISPLAY_FAILED;
            }
        }
    }
    
    // Synchronize cursor
    lle_display_buffer_sync_cursor_to_screen(renderer);
    
    renderer->render_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Render specific line range
int lle_display_buffer_render_lines(lle_display_buffer_renderer_t *renderer,
                                    size_t start_line,
                                    size_t end_line) {
    // TODO Phase 2: Implement incremental rendering
    // For now, just do full render
    return lle_display_buffer_render(renderer);
}

// Viewport management
int lle_display_buffer_set_viewport(lle_display_buffer_renderer_t *renderer,
                                    size_t top_line,
                                    size_t left_column) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    renderer->viewport.top_line = top_line;
    renderer->viewport.left_column = left_column;
    
    return LLE_DISPLAY_BUFFER_OK;
}

int lle_display_buffer_scroll_up(lle_display_buffer_renderer_t *renderer, size_t lines) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    if (renderer->viewport.top_line >= lines) {
        renderer->viewport.top_line -= lines;
    } else {
        renderer->viewport.top_line = 0;
    }
    
    renderer->scroll_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

int lle_display_buffer_scroll_down(lle_display_buffer_renderer_t *renderer, size_t lines) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    lle_managed_buffer_t *managed = lle_buffer_manager_get_current(renderer->buffer_manager);
    if (!managed) {
        return LLE_DISPLAY_BUFFER_ERR_NO_BUFFER;
    }
    
    size_t total_lines = count_buffer_lines(&managed->buffer);
    size_t max_top_line = (total_lines > renderer->viewport.visible_lines) ?
                         (total_lines - renderer->viewport.visible_lines) : 0;
    
    renderer->viewport.top_line += lines;
    if (renderer->viewport.top_line > max_top_line) {
        renderer->viewport.top_line = max_top_line;
    }
    
    renderer->scroll_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

int lle_display_buffer_scroll_left(lle_display_buffer_renderer_t *renderer, size_t cols) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    if (renderer->viewport.left_column >= cols) {
        renderer->viewport.left_column -= cols;
    } else {
        renderer->viewport.left_column = 0;
    }
    
    renderer->scroll_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

int lle_display_buffer_scroll_right(lle_display_buffer_renderer_t *renderer, size_t cols) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    renderer->viewport.left_column += cols;
    renderer->scroll_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Auto-scroll to ensure cursor is visible
int lle_display_buffer_ensure_cursor_visible(lle_display_buffer_renderer_t *renderer) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    size_t cursor_line = renderer->buffer_cursor_line;
    size_t cursor_col = renderer->buffer_cursor_col;
    
    // Vertical scroll
    if (cursor_line < renderer->viewport.top_line) {
        // Cursor above viewport - scroll up
        renderer->viewport.top_line = cursor_line;
    } else if (cursor_line >= renderer->viewport.top_line + renderer->viewport.visible_lines) {
        // Cursor below viewport - scroll down
        renderer->viewport.top_line = cursor_line - renderer->viewport.visible_lines + 1;
    }
    
    // Horizontal scroll
    if (cursor_col < renderer->viewport.left_column) {
        // Cursor left of viewport - scroll left
        renderer->viewport.left_column = cursor_col;
    } else if (cursor_col >= renderer->viewport.left_column + renderer->viewport.visible_cols) {
        // Cursor right of viewport - scroll right
        renderer->viewport.left_column = cursor_col - renderer->viewport.visible_cols + 1;
    }
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Get viewport info
const lle_viewport_t* lle_display_buffer_get_viewport(const lle_display_buffer_renderer_t *renderer) {
    if (!renderer || !renderer->initialized) {
        return NULL;
    }
    return &renderer->viewport;
}

// Synchronize screen cursor to buffer cursor
int lle_display_buffer_sync_cursor_to_screen(lle_display_buffer_renderer_t *renderer) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    // Convert buffer cursor to screen coordinates
    uint16_t screen_row, screen_col;
    int result = lle_display_buffer_buffer_to_screen(renderer,
                                                     renderer->buffer_cursor_line,
                                                     renderer->buffer_cursor_col,
                                                     &screen_row,
                                                     &screen_col);
    if (result != LLE_DISPLAY_BUFFER_OK) {
        return result;
    }
    
    renderer->screen_cursor_row = screen_row;
    renderer->screen_cursor_col = screen_col;
    
    // Update display cursor
    lle_display_set_cursor(renderer->display, screen_row, screen_col);
    lle_display_show_cursor(renderer->display, true);
    
    renderer->cursor_sync_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Synchronize buffer cursor to screen cursor
int lle_display_buffer_sync_cursor_to_buffer(lle_display_buffer_renderer_t *renderer,
                                             uint16_t screen_row,
                                             uint16_t screen_col) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    size_t buffer_line, buffer_col;
    int result = lle_display_buffer_screen_to_buffer(renderer,
                                                     screen_row,
                                                     screen_col,
                                                     &buffer_line,
                                                     &buffer_col);
    if (result != LLE_DISPLAY_BUFFER_OK) {
        return result;
    }
    
    renderer->buffer_cursor_line = buffer_line;
    renderer->buffer_cursor_col = buffer_col;
    renderer->screen_cursor_row = screen_row;
    renderer->screen_cursor_col = screen_col;
    
    // Update buffer cursor position (TODO: needs buffer position calculation)
    // For now, just track the line/column separately
    
    renderer->cursor_sync_count++;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Get cursor positions
void lle_display_buffer_get_buffer_cursor(const lle_display_buffer_renderer_t *renderer,
                                         size_t *line,
                                         size_t *column) {
    if (renderer && line && column) {
        *line = renderer->buffer_cursor_line;
        *column = renderer->buffer_cursor_col;
    }
}

void lle_display_buffer_get_screen_cursor(const lle_display_buffer_renderer_t *renderer,
                                         uint16_t *row,
                                         uint16_t *col) {
    if (renderer && row && col) {
        *row = renderer->screen_cursor_row;
        *col = renderer->screen_cursor_col;
    }
}

// Buffer-to-screen coordinate conversion
int lle_display_buffer_buffer_to_screen(const lle_display_buffer_renderer_t *renderer,
                                       size_t buffer_line,
                                       size_t buffer_col,
                                       uint16_t *screen_row,
                                       uint16_t *screen_col) {
    if (!renderer || !renderer->initialized || !screen_row || !screen_col) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    // Check if position is in viewport
    if (buffer_line < renderer->viewport.top_line ||
        buffer_line >= renderer->viewport.top_line + renderer->viewport.visible_lines) {
        // Line not visible
        return LLE_DISPLAY_BUFFER_ERR_INVALID_VIEWPORT;
    }
    
    if (buffer_col < renderer->viewport.left_column ||
        buffer_col >= renderer->viewport.left_column + renderer->viewport.visible_cols) {
        // Column not visible
        return LLE_DISPLAY_BUFFER_ERR_INVALID_VIEWPORT;
    }
    
    *screen_row = (uint16_t)(buffer_line - renderer->viewport.top_line);
    *screen_col = (uint16_t)(buffer_col - renderer->viewport.left_column);
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Screen-to-buffer coordinate conversion
int lle_display_buffer_screen_to_buffer(const lle_display_buffer_renderer_t *renderer,
                                       uint16_t screen_row,
                                       uint16_t screen_col,
                                       size_t *buffer_line,
                                       size_t *buffer_col) {
    if (!renderer || !renderer->initialized || !buffer_line || !buffer_col) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    *buffer_line = renderer->viewport.top_line + screen_row;
    *buffer_col = renderer->viewport.left_column + screen_col;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Handle terminal resize
int lle_display_buffer_handle_resize(lle_display_buffer_renderer_t *renderer,
                                     uint16_t new_rows,
                                     uint16_t new_cols) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    // Update viewport dimensions
    renderer->viewport.visible_lines = new_rows;
    renderer->viewport.visible_cols = new_cols;
    
    // Resize display
    int result = lle_display_resize(renderer->display, new_rows, new_cols);
    if (result != LLE_DISPLAY_OK) {
        return LLE_DISPLAY_BUFFER_ERR_DISPLAY_FAILED;
    }
    
    // Ensure cursor still visible after resize
    if (renderer->auto_scroll) {
        lle_display_buffer_ensure_cursor_visible(renderer);
    }
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Clear display and reset viewport
int lle_display_buffer_clear(lle_display_buffer_renderer_t *renderer) {
    if (!renderer || !renderer->initialized) {
        return LLE_DISPLAY_BUFFER_ERR_NOT_INIT;
    }
    
    lle_display_clear(renderer->display);
    renderer->viewport.top_line = 0;
    renderer->viewport.left_column = 0;
    
    return LLE_DISPLAY_BUFFER_OK;
}

// Performance metrics
void lle_display_buffer_get_metrics(const lle_display_buffer_renderer_t *renderer,
                                    uint64_t *render_count,
                                    uint64_t *scroll_count,
                                    uint64_t *cursor_sync_count) {
    if (renderer) {
        if (render_count) *render_count = renderer->render_count;
        if (scroll_count) *scroll_count = renderer->scroll_count;
        if (cursor_sync_count) *cursor_sync_count = renderer->cursor_sync_count;
    }
}

// Cleanup
void lle_display_buffer_cleanup(lle_display_buffer_renderer_t *renderer) {
    if (renderer) {
        // We don't own buffer_manager or display, so just clear our state
        memset(renderer, 0, sizeof(*renderer));
    }
}

// Utility: Convert error code to string
const char* lle_display_buffer_error_string(int error_code) {
    switch (error_code) {
        case LLE_DISPLAY_BUFFER_OK:
            return "Success";
        case LLE_DISPLAY_BUFFER_ERR_NULL_PTR:
            return "Null pointer";
        case LLE_DISPLAY_BUFFER_ERR_NOT_INIT:
            return "Not initialized";
        case LLE_DISPLAY_BUFFER_ERR_NO_BUFFER:
            return "No buffer available";
        case LLE_DISPLAY_BUFFER_ERR_DISPLAY_FAILED:
            return "Display operation failed";
        case LLE_DISPLAY_BUFFER_ERR_INVALID_VIEWPORT:
            return "Invalid viewport position";
        default:
            return "Unknown error";
    }
}

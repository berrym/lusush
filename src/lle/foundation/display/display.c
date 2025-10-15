// src/lle/foundation/display/display.c
//
// LLE Display System Implementation

#include "display.h"
#include "../terminal/terminal.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

// Get nanosecond timestamp
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Allocate display buffer
static int allocate_buffer(lle_display_buffer_t *buffer,
                          uint16_t rows, uint16_t cols) {
    size_t cell_count = (size_t)rows * cols;
    
    buffer->cells = calloc(cell_count, sizeof(lle_display_cell_t));
    if (!buffer->cells) {
        return LLE_DISPLAY_ERR_ALLOC_FAILED;
    }
    
    buffer->previous = calloc(cell_count, sizeof(lle_display_cell_t));
    if (!buffer->previous) {
        free(buffer->cells);
        return LLE_DISPLAY_ERR_ALLOC_FAILED;
    }
    
    buffer->dirty_lines = calloc(rows, sizeof(bool));
    if (!buffer->dirty_lines) {
        free(buffer->cells);
        free(buffer->previous);
        return LLE_DISPLAY_ERR_ALLOC_FAILED;
    }
    
    buffer->rows = rows;
    buffer->cols = cols;
    buffer->needs_full_redraw = true;
    
    return LLE_DISPLAY_OK;
}

// Free display buffer
static void free_buffer(lle_display_buffer_t *buffer) {
    if (buffer->cells) {
        free(buffer->cells);
        buffer->cells = NULL;
    }
    if (buffer->previous) {
        free(buffer->previous);
        buffer->previous = NULL;
    }
    if (buffer->dirty_lines) {
        free(buffer->dirty_lines);
        buffer->dirty_lines = NULL;
    }
}

int lle_display_init(lle_display_t *display,
                     lle_terminal_abstraction_t *term,
                     uint16_t rows,
                     uint16_t cols) {
    if (!display) {
        return LLE_DISPLAY_ERR_NULL_PTR;
    }
    
    if (rows == 0 || cols == 0) {
        return LLE_DISPLAY_ERR_INVALID_DIMS;
    }
    
    // Zero-initialize
    memset(display, 0, sizeof(*display));
    
    // Allocate buffer
    int result = allocate_buffer(&display->buffer, rows, cols);
    if (result != LLE_DISPLAY_OK) {
        return result;
    }
    
    // Set terminal reference
    display->term = term;
    
    // Initialize cursor
    display->cursor_row = 0;
    display->cursor_col = 0;
    display->cursor_visible = true;
    
    // Enable optimizations
    display->dirty_tracking_enabled = true;
    display->double_buffering_enabled = true;
    
    // Display controller (Phase 1: will connect to actual display system)
    display->display_controller = NULL;
    
    display->initialized = true;
    
    return LLE_DISPLAY_OK;
}

int lle_display_resize(lle_display_t *display,
                       uint16_t new_rows,
                       uint16_t new_cols) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (new_rows == 0 || new_cols == 0) {
        return LLE_DISPLAY_ERR_INVALID_DIMS;
    }
    
    // Free old buffer
    free_buffer(&display->buffer);
    
    // Allocate new buffer
    int result = allocate_buffer(&display->buffer, new_rows, new_cols);
    if (result != LLE_DISPLAY_OK) {
        display->initialized = false;
        return result;
    }
    
    // Adjust cursor if out of bounds
    if (display->cursor_row >= new_rows) {
        display->cursor_row = new_rows - 1;
    }
    if (display->cursor_col >= new_cols) {
        display->cursor_col = new_cols - 1;
    }
    
    return LLE_DISPLAY_OK;
}

int lle_display_clear(lle_display_t *display) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    size_t cell_count = (size_t)display->buffer.rows * display->buffer.cols;
    memset(display->buffer.cells, 0, cell_count * sizeof(lle_display_cell_t));
    
    // Mark all lines as dirty
    memset(display->buffer.dirty_lines, 1, display->buffer.rows);
    display->buffer.needs_full_redraw = true;
    
    return LLE_DISPLAY_OK;
}

int lle_display_clear_region(lle_display_t *display,
                             const lle_display_region_t *region) {
    if (!display || !display->initialized || !region) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    // Validate region
    if (region->start_row >= display->buffer.rows ||
        region->end_row >= display->buffer.rows ||
        region->start_col >= display->buffer.cols ||
        region->end_col >= display->buffer.cols ||
        region->start_row > region->end_row ||
        region->start_col > region->end_col) {
        return LLE_DISPLAY_ERR_INVALID_REGION;
    }
    
    // Clear region
    for (uint16_t row = region->start_row; row <= region->end_row; row++) {
        for (uint16_t col = region->start_col; col <= region->end_col; col++) {
            size_t idx = (size_t)row * display->buffer.cols + col;
            display->buffer.cells[idx].codepoint = 0;
            display->buffer.cells[idx].fg_color = 7;
            display->buffer.cells[idx].bg_color = 0;
            display->buffer.cells[idx].attrs = 0;
        }
        display->buffer.dirty_lines[row] = true;
    }
    
    return LLE_DISPLAY_OK;
}

int lle_display_render_line(lle_display_t *display,
                            uint16_t row,
                            const char *text,
                            size_t text_len) {
    if (!display || !display->initialized || !text) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (row >= display->buffer.rows) {
        return LLE_DISPLAY_ERR_INVALID_REGION;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Render text to display buffer
    size_t col = 0;
    for (size_t i = 0; i < text_len && col < display->buffer.cols; i++) {
        size_t idx = (size_t)row * display->buffer.cols + col;
        display->buffer.cells[idx].codepoint = (uint32_t)(unsigned char)text[i];
        display->buffer.cells[idx].fg_color = 7;  // Default white
        display->buffer.cells[idx].bg_color = 0;  // Default black
        display->buffer.cells[idx].attrs = 0;
        col++;
    }
    
    // Clear rest of line
    for (; col < display->buffer.cols; col++) {
        size_t idx = (size_t)row * display->buffer.cols + col;
        display->buffer.cells[idx].codepoint = 0;
    }
    
    // Mark line as dirty
    display->buffer.dirty_lines[row] = true;
    
    // Track performance
    uint64_t end = get_timestamp_ns();
    uint64_t elapsed = end - start;
    
    display->last_render_time_ns = elapsed;
    display->total_render_time_ns += elapsed;
    display->render_count++;
    
    return LLE_DISPLAY_OK;
}

int lle_display_render_multiline(lle_display_t *display,
                                 uint16_t start_row,
                                 const char *text,
                                 size_t text_len,
                                 size_t cursor_offset) {
    if (!display || !display->initialized || !text) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Render with wrapping
    uint16_t row = start_row;
    uint16_t col = 0;
    
    for (size_t i = 0; i < text_len && row < display->buffer.rows; i++) {
        // Wrap to next line if needed
        if (col >= display->buffer.cols) {
            row++;
            col = 0;
            if (row >= display->buffer.rows) {
                break;
            }
        }
        
        size_t idx = (size_t)row * display->buffer.cols + col;
        display->buffer.cells[idx].codepoint = (uint32_t)(unsigned char)text[i];
        display->buffer.cells[idx].fg_color = 7;
        display->buffer.cells[idx].bg_color = 0;
        display->buffer.cells[idx].attrs = 0;
        
        // Update cursor position if at cursor offset
        if (i == cursor_offset) {
            display->cursor_row = row;
            display->cursor_col = col;
        }
        
        col++;
    }
    
    // Mark affected lines as dirty
    for (uint16_t r = start_row; r <= row && r < display->buffer.rows; r++) {
        display->buffer.dirty_lines[r] = true;
    }
    
    // Track performance
    uint64_t end = get_timestamp_ns();
    uint64_t elapsed = end - start;
    
    display->last_render_time_ns = elapsed;
    display->total_render_time_ns += elapsed;
    display->render_count++;
    
    return LLE_DISPLAY_OK;
}

int lle_display_render_highlighted(lle_display_t *display,
                                   uint16_t row,
                                   const char *text,
                                   const uint8_t *attrs,
                                   size_t len) {
    if (!display || !display->initialized || !text || !attrs) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (row >= display->buffer.rows) {
        return LLE_DISPLAY_ERR_INVALID_REGION;
    }
    
    uint64_t start = get_timestamp_ns();
    
    // Render with attributes
    size_t col = 0;
    for (size_t i = 0; i < len && col < display->buffer.cols; i++) {
        size_t idx = (size_t)row * display->buffer.cols + col;
        display->buffer.cells[idx].codepoint = (uint32_t)(unsigned char)text[i];
        display->buffer.cells[idx].fg_color = 7;
        display->buffer.cells[idx].bg_color = 0;
        display->buffer.cells[idx].attrs = attrs[i];
        col++;
    }
    
    // Mark line as dirty
    display->buffer.dirty_lines[row] = true;
    
    // Track performance
    uint64_t end = get_timestamp_ns();
    uint64_t elapsed = end - start;
    
    display->last_render_time_ns = elapsed;
    display->total_render_time_ns += elapsed;
    display->render_count++;
    
    return LLE_DISPLAY_OK;
}

int lle_display_set_cell(lle_display_t *display,
                        uint16_t row,
                        uint16_t col,
                        uint32_t codepoint,
                        uint8_t fg_color,
                        uint8_t bg_color,
                        uint8_t attrs) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (row >= display->buffer.rows || col >= display->buffer.cols) {
        return LLE_DISPLAY_ERR_INVALID_REGION;
    }
    
    size_t idx = (size_t)row * display->buffer.cols + col;
    display->buffer.cells[idx].codepoint = codepoint;
    display->buffer.cells[idx].fg_color = fg_color;
    display->buffer.cells[idx].bg_color = bg_color;
    display->buffer.cells[idx].attrs = attrs;
    
    display->buffer.dirty_lines[row] = true;
    
    return LLE_DISPLAY_OK;
}

const lle_display_cell_t* lle_display_get_cell(const lle_display_t *display,
                                               uint16_t row,
                                               uint16_t col) {
    if (!display || !display->initialized) {
        return NULL;
    }
    
    if (row >= display->buffer.rows || col >= display->buffer.cols) {
        return NULL;
    }
    
    size_t idx = (size_t)row * display->buffer.cols + col;
    return &display->buffer.cells[idx];
}

int lle_display_set_cursor(lle_display_t *display,
                           uint16_t row,
                           uint16_t col) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (row >= display->buffer.rows || col >= display->buffer.cols) {
        return LLE_DISPLAY_ERR_INVALID_REGION;
    }
    
    display->cursor_row = row;
    display->cursor_col = col;
    
    return LLE_DISPLAY_OK;
}

int lle_display_show_cursor(lle_display_t *display, bool visible) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    display->cursor_visible = visible;
    
    return LLE_DISPLAY_OK;
}

void lle_display_get_cursor(const lle_display_t *display,
                           uint16_t *row,
                           uint16_t *col,
                           bool *visible) {
    if (!display || !display->initialized) {
        return;
    }
    
    if (row) *row = display->cursor_row;
    if (col) *col = display->cursor_col;
    if (visible) *visible = display->cursor_visible;
}

// Scroll region management
int lle_display_set_scroll_region(lle_display_t *display,
                                  uint16_t top_row,
                                  uint16_t bottom_row) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    // Validate region bounds
    if (top_row >= display->buffer.rows ||
        bottom_row >= display->buffer.rows ||
        top_row > bottom_row) {
        return LLE_DISPLAY_ERR_INVALID_REGION;
    }
    
    display->scroll_region.top_row = top_row;
    display->scroll_region.bottom_row = bottom_row;
    display->scroll_region.scroll_offset = 0;
    display->scroll_region.enabled = true;
    
    return LLE_DISPLAY_OK;
}

int lle_display_clear_scroll_region(lle_display_t *display) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    display->scroll_region.enabled = false;
    display->scroll_region.top_row = 0;
    display->scroll_region.bottom_row = 0;
    display->scroll_region.scroll_offset = 0;
    
    return LLE_DISPLAY_OK;
}

int lle_display_scroll_up(lle_display_t *display, uint16_t lines) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (!display->scroll_region.enabled) {
        return LLE_DISPLAY_OK;  // No-op if scroll region not enabled
    }
    
    if (lines == 0) {
        return LLE_DISPLAY_OK;
    }
    
    uint16_t top = display->scroll_region.top_row;
    uint16_t bottom = display->scroll_region.bottom_row;
    uint16_t region_height = bottom - top + 1;
    
    // Limit scroll to region height
    if (lines > region_height) {
        lines = region_height;
    }
    
    // Shift content up within scroll region
    for (uint16_t row = top; row <= bottom - lines; row++) {
        size_t dest_idx = (size_t)row * display->buffer.cols;
        size_t src_idx = (size_t)(row + lines) * display->buffer.cols;
        
        memcpy(&display->buffer.cells[dest_idx],
               &display->buffer.cells[src_idx],
               display->buffer.cols * sizeof(lle_display_cell_t));
        
        display->buffer.dirty_lines[row] = true;
    }
    
    // Clear the newly exposed lines at the bottom
    for (uint16_t row = bottom - lines + 1; row <= bottom; row++) {
        for (uint16_t col = 0; col < display->buffer.cols; col++) {
            size_t idx = (size_t)row * display->buffer.cols + col;
            display->buffer.cells[idx].codepoint = 0;
            display->buffer.cells[idx].fg_color = 7;
            display->buffer.cells[idx].bg_color = 0;
            display->buffer.cells[idx].attrs = 0;
        }
        display->buffer.dirty_lines[row] = true;
    }
    
    // Update scroll offset
    display->scroll_region.scroll_offset += lines;
    
    return LLE_DISPLAY_OK;
}

int lle_display_scroll_down(lle_display_t *display, uint16_t lines) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (!display->scroll_region.enabled) {
        return LLE_DISPLAY_OK;  // No-op if scroll region not enabled
    }
    
    if (lines == 0) {
        return LLE_DISPLAY_OK;
    }
    
    uint16_t top = display->scroll_region.top_row;
    uint16_t bottom = display->scroll_region.bottom_row;
    uint16_t region_height = bottom - top + 1;
    
    // Limit scroll to region height
    if (lines > region_height) {
        lines = region_height;
    }
    
    // Shift content down within scroll region (work backwards to avoid overwriting)
    for (uint16_t row = bottom; row >= top + lines; row--) {
        size_t dest_idx = (size_t)row * display->buffer.cols;
        size_t src_idx = (size_t)(row - lines) * display->buffer.cols;
        
        memcpy(&display->buffer.cells[dest_idx],
               &display->buffer.cells[src_idx],
               display->buffer.cols * sizeof(lle_display_cell_t));
        
        display->buffer.dirty_lines[row] = true;
        
        if (row == top + lines) break;  // Avoid underflow
    }
    
    // Clear the newly exposed lines at the top
    for (uint16_t row = top; row < top + lines; row++) {
        for (uint16_t col = 0; col < display->buffer.cols; col++) {
            size_t idx = (size_t)row * display->buffer.cols + col;
            display->buffer.cells[idx].codepoint = 0;
            display->buffer.cells[idx].fg_color = 7;
            display->buffer.cells[idx].bg_color = 0;
            display->buffer.cells[idx].attrs = 0;
        }
        display->buffer.dirty_lines[row] = true;
    }
    
    // Update scroll offset (decrease when scrolling down)
    if (display->scroll_region.scroll_offset >= lines) {
        display->scroll_region.scroll_offset -= lines;
    } else {
        display->scroll_region.scroll_offset = 0;
    }
    
    return LLE_DISPLAY_OK;
}

const lle_scroll_region_t* lle_display_get_scroll_region(const lle_display_t *display) {
    if (!display || !display->initialized) {
        return NULL;
    }
    
    return &display->scroll_region;
}

int lle_display_flush(lle_display_t *display) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    if (!display->term) {
        return LLE_DISPLAY_ERR_NULL_PTR;
    }
    
    // Phase 1: This is where we write to terminal through display system
    // For now, we'll write directly (Phase 2 will integrate with display controller)
    
    char buf[4096];
    int fd = display->term->unix_interface->output_fd;
    
    // Move cursor to home
    const char *home = "\x1b[H";
    write(fd, home, strlen(home));
    
    // Write each line
    for (uint16_t row = 0; row < display->buffer.rows; row++) {
        size_t offset = 0;
        
        for (uint16_t col = 0; col < display->buffer.cols; col++) {
            size_t idx = (size_t)row * display->buffer.cols + col;
            lle_display_cell_t *cell = &display->buffer.cells[idx];
            
            if (cell->codepoint > 0 && cell->codepoint < 128) {
                if (offset < sizeof(buf) - 1) {
                    buf[offset++] = (char)cell->codepoint;
                }
            }
        }
        
        if (offset > 0) {
            write(fd, buf, offset);
        }
        
        // Newline if not last row
        if (row < display->buffer.rows - 1) {
            write(fd, "\r\n", 2);
        }
    }
    
    // Position cursor
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
             display->cursor_row + 1, display->cursor_col + 1);
    write(fd, buf, strlen(buf));
    
    // Copy current to previous (for dirty detection)
    size_t cell_count = (size_t)display->buffer.rows * display->buffer.cols;
    memcpy(display->buffer.previous, display->buffer.cells,
           cell_count * sizeof(lle_display_cell_t));
    
    // Clear dirty flags
    memset(display->buffer.dirty_lines, 0, display->buffer.rows);
    display->buffer.needs_full_redraw = false;
    
    return LLE_DISPLAY_OK;
}

int lle_display_flush_dirty(lle_display_t *display) {
    if (!display || !display->initialized) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    // If full redraw needed, use full flush
    if (display->buffer.needs_full_redraw) {
        return lle_display_flush(display);
    }
    
    // Otherwise, only flush dirty lines (optimization for Phase 2)
    return lle_display_flush(display);
}

int lle_display_mark_dirty(lle_display_t *display,
                          const lle_display_region_t *region) {
    if (!display || !display->initialized || !region) {
        return LLE_DISPLAY_ERR_NOT_INIT;
    }
    
    for (uint16_t row = region->start_row;
         row <= region->end_row && row < display->buffer.rows;
         row++) {
        display->buffer.dirty_lines[row] = true;
    }
    
    return LLE_DISPLAY_OK;
}

void lle_display_mark_all_dirty(lle_display_t *display) {
    if (!display || !display->initialized) {
        return;
    }
    
    memset(display->buffer.dirty_lines, 1, display->buffer.rows);
    display->buffer.needs_full_redraw = true;
}

void lle_display_get_metrics(const lle_display_t *display,
                            uint64_t *render_count,
                            double *avg_time_ms) {
    if (!display || !display->initialized) {
        return;
    }
    
    if (render_count) {
        *render_count = display->render_count;
    }
    
    if (avg_time_ms && display->render_count > 0) {
        double avg_ns = (double)display->total_render_time_ns / display->render_count;
        *avg_time_ms = avg_ns / 1000000.0;
    }
}

void lle_display_cleanup(lle_display_t *display) {
    if (!display || !display->initialized) {
        return;
    }
    
    free_buffer(&display->buffer);
    display->initialized = false;
}

const char* lle_display_error_string(int error_code) {
    switch (error_code) {
        case LLE_DISPLAY_OK:
            return "Success";
        case LLE_DISPLAY_ERR_NULL_PTR:
            return "Null pointer argument";
        case LLE_DISPLAY_ERR_INVALID_DIMS:
            return "Invalid dimensions";
        case LLE_DISPLAY_ERR_ALLOC_FAILED:
            return "Memory allocation failed";
        case LLE_DISPLAY_ERR_INVALID_REGION:
            return "Invalid region";
        case LLE_DISPLAY_ERR_NOT_INIT:
            return "Not initialized";
        default:
            return "Unknown error";
    }
}

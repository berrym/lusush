// src/lle/foundation/display/display.h
//
// LLE Display System Integration (Production)
//
// This module implements LLE as a pure client of the Lusush display system.
// Key architectural principles validated in Phase 0:
// - Zero direct terminal control (no escape sequences from LLE)
// - All rendering through display buffer abstraction
// - Atomic display updates coordinated with display system
// - Multi-line rendering with proper wrapping
// - Syntax highlighting support

#ifndef LLE_FOUNDATION_DISPLAY_H
#define LLE_FOUNDATION_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../terminal/terminal.h"

// Display cell representing a single character position
typedef struct {
    uint32_t codepoint;     // Unicode codepoint
    uint8_t fg_color;       // Foreground color (0-255)
    uint8_t bg_color;       // Background color (0-255)
    uint8_t attrs;          // Text attributes (bitfield)
} lle_display_cell_t;

// Text attribute flags
typedef enum {
    LLE_DISPLAY_ATTR_NONE       = 0x00,
    LLE_DISPLAY_ATTR_BOLD       = 0x01,
    LLE_DISPLAY_ATTR_UNDERLINE  = 0x02,
    LLE_DISPLAY_ATTR_REVERSE    = 0x04,
    LLE_DISPLAY_ATTR_BLINK      = 0x08,
    LLE_DISPLAY_ATTR_DIM        = 0x10,
    LLE_DISPLAY_ATTR_ITALIC     = 0x20
} lle_display_attr_t;

// Display buffer (screen representation)
typedef struct {
    lle_display_cell_t *cells;  // Cell array (rows * cols)
    lle_display_cell_t *previous; // Previous frame (for dirty detection)
    uint16_t rows;               // Buffer height
    uint16_t cols;               // Buffer width
    bool *dirty_lines;           // Per-line dirty flags
    bool needs_full_redraw;      // Full redraw required
} lle_display_buffer_t;

// Display region (for partial updates)
typedef struct {
    uint16_t start_row;
    uint16_t start_col;
    uint16_t end_row;
    uint16_t end_col;
} lle_display_region_t;

// Error codes
typedef enum {
    LLE_DISPLAY_OK = 0,
    LLE_DISPLAY_ERR_NULL_PTR = -1,
    LLE_DISPLAY_ERR_INVALID_DIMS = -2,
    LLE_DISPLAY_ERR_ALLOC_FAILED = -3,
    LLE_DISPLAY_ERR_INVALID_REGION = -4,
    LLE_DISPLAY_ERR_NOT_INIT = -5
} lle_display_error_t;

// Display context
typedef struct {
    // Display buffer
    lle_display_buffer_t buffer;
    
    // Cursor position (logical)
    uint16_t cursor_row;
    uint16_t cursor_col;
    bool cursor_visible;
    
    // Terminal reference (for output)
    lle_term_t *term;
    
    // Display system integration (Phase 1: will connect to actual display controller)
    void *display_controller;    // Opaque display system reference
    
    // Performance tracking
    uint64_t render_count;
    uint64_t total_render_time_ns;
    uint64_t last_render_time_ns;
    
    // Optimization
    bool dirty_tracking_enabled;
    bool double_buffering_enabled;
    
    // Initialization state
    bool initialized;
} lle_display_t;

// Initialize display system
int lle_display_init(lle_display_t *display,
                     lle_term_t *term,
                     uint16_t rows,
                     uint16_t cols);

// Resize display (called on terminal resize)
int lle_display_resize(lle_display_t *display,
                       uint16_t new_rows,
                       uint16_t new_cols);

// Clear display buffer
int lle_display_clear(lle_display_t *display);

// Clear specific region
int lle_display_clear_region(lle_display_t *display,
                             const lle_display_region_t *region);

// Render single-line content
int lle_display_render_line(lle_display_t *display,
                            uint16_t row,
                            const char *text,
                            size_t text_len);

// Render multi-line content with wrapping
int lle_display_render_multiline(lle_display_t *display,
                                 uint16_t start_row,
                                 const char *text,
                                 size_t text_len,
                                 size_t cursor_offset);

// Render with syntax highlighting
int lle_display_render_highlighted(lle_display_t *display,
                                   uint16_t row,
                                   const char *text,
                                   const uint8_t *attrs,
                                   size_t len);

// Set cell directly (for custom rendering)
int lle_display_set_cell(lle_display_t *display,
                        uint16_t row,
                        uint16_t col,
                        uint32_t codepoint,
                        uint8_t fg_color,
                        uint8_t bg_color,
                        uint8_t attrs);

// Get cell (read access)
const lle_display_cell_t* lle_display_get_cell(const lle_display_t *display,
                                               uint16_t row,
                                               uint16_t col);

// Cursor management
int lle_display_set_cursor(lle_display_t *display,
                           uint16_t row,
                           uint16_t col);

int lle_display_show_cursor(lle_display_t *display, bool visible);

void lle_display_get_cursor(const lle_display_t *display,
                           uint16_t *row,
                           uint16_t *col,
                           bool *visible);

// Flush display buffer to terminal
// This is where we actually write to the terminal
int lle_display_flush(lle_display_t *display);

// Flush only dirty regions (optimized)
int lle_display_flush_dirty(lle_display_t *display);

// Mark region as dirty
int lle_display_mark_dirty(lle_display_t *display,
                          const lle_display_region_t *region);

// Mark entire display as dirty
void lle_display_mark_all_dirty(lle_display_t *display);

// Performance metrics
void lle_display_get_metrics(const lle_display_t *display,
                            uint64_t *render_count,
                            double *avg_time_ms);

// Cleanup
void lle_display_cleanup(lle_display_t *display);

// Utility: Convert error code to string
const char* lle_display_error_string(int error_code);

#endif // LLE_FOUNDATION_DISPLAY_H

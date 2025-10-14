/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Display Client Interface - Week 2 Day 6
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LLE_VALIDATION_DISPLAY_CLIENT_H
#define LLE_VALIDATION_DISPLAY_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * DISPLAY CLIENT INTERFACE
 * 
 * This interface validates the architectural principle that LLE operates as
 * a pure client of the Lusush display system. During Phase 0 validation,
 * this is a simplified interface. In Phase 1, it will integrate with the
 * actual display controller API.
 * 
 * Key Validation Goals:
 * - LLE never uses direct terminal control (escape sequences)
 * - All rendering goes through display system interface
 * - Display updates are atomic and coordinated
 * - Performance targets: <10ms for complex multi-line edits
 * 
 * Success Criteria (Week 2):
 * - All 7 rendering scenarios work correctly
 * - Zero direct escape sequences in LLE code
 * - Update latency <10ms for complex edits
 * - No visual artifacts
 */

// Display cell (character + attributes)
typedef struct {
    uint32_t codepoint;     // Unicode codepoint
    uint8_t fg_color;       // Foreground color (0-255)
    uint8_t bg_color;       // Background color (0-255)
    uint8_t attrs;          // Attributes: bold, underline, etc.
} lle_display_cell_t;

// Attribute flags
#define LLE_ATTR_BOLD       0x01
#define LLE_ATTR_UNDERLINE  0x02
#define LLE_ATTR_REVERSE    0x04
#define LLE_ATTR_BLINK      0x08
#define LLE_ATTR_DIM        0x10

// Display buffer (screen representation)
typedef struct {
    lle_display_cell_t *cells;  // Cell array (rows * cols)
    uint16_t rows;               // Buffer height
    uint16_t cols;               // Buffer width
    bool dirty;                  // Needs redraw flag
} lle_display_buffer_t;

// Display client context
typedef struct {
    // Reference to Lusush display system (opaque for Phase 0)
    void *display_system;        // Display controller reference
    
    // Display buffer
    lle_display_buffer_t buffer;
    
    // Cursor position (maintained by LLE, reported to display system)
    uint16_t cursor_row;
    uint16_t cursor_col;
    
    // Performance tracking
    uint64_t render_count;           // Total render operations
    uint64_t total_render_time_ns;   // Total render time
    uint64_t last_render_time_ns;    // Last render operation time
    
    // Validation tracking
    uint64_t escape_sequence_violations;  // Count of direct terminal control attempts
    bool validation_mode;                 // Enable strict validation checks
} lle_display_client_t;

/**
 * Initialize display client.
 * 
 * Sets up the display client context with the specified screen dimensions.
 * In Phase 0 validation, this creates a minimal display buffer. In Phase 1,
 * this will connect to the actual Lusush display controller.
 * 
 * @param client Display client context to initialize
 * @param rows Screen height in rows
 * @param cols Screen width in columns
 * @return 0 on success, -1 on failure
 */
int lle_display_client_init(lle_display_client_t *client, 
                            uint16_t rows, 
                            uint16_t cols);

/**
 * Render line editor content through display system.
 * 
 * Renders the provided line content through the display system interface.
 * This function must NEVER emit direct terminal escape sequences - all
 * rendering goes through the display buffer which the display system
 * will coordinate.
 * 
 * Phase 0 Validation: Updates internal buffer and validates architectural
 * compliance (zero escape sequences).
 * 
 * Phase 1 Implementation: Will call display_controller_display() or
 * equivalent display system API.
 * 
 * @param client Display client context
 * @param line Line content to render (null-terminated string)
 * @return 0 on success, -1 on failure
 */
int lle_display_client_render(lle_display_client_t *client, 
                               const char *line);

/**
 * Render multi-line content through display system.
 * 
 * Renders multi-line editor content (for lines >80 chars that wrap).
 * Uses the same architectural principle: zero direct terminal control.
 * 
 * @param client Display client context
 * @param line Line content (may span multiple screen lines)
 * @param cursor_offset Cursor position within line (for cursor rendering)
 * @return 0 on success, -1 on failure
 */
int lle_display_client_render_multiline(lle_display_client_t *client,
                                         const char *line,
                                         size_t cursor_offset);

/**
 * Render with syntax highlighting.
 * 
 * Renders line content with syntax highlighting attributes applied.
 * The highlighting information is passed as an attribute array parallel
 * to the line content.
 * 
 * @param client Display client context
 * @param line Line content
 * @param attrs Attribute array (same length as line)
 * @param attr_count Number of attributes
 * @return 0 on success, -1 on failure
 */
int lle_display_client_render_highlighted(lle_display_client_t *client,
                                           const char *line,
                                           const uint8_t *attrs,
                                           size_t attr_count);

/**
 * Update cursor position.
 * 
 * Updates the cursor position in the display client. The display system
 * will be notified of the cursor position during the next render operation.
 * 
 * @param client Display client context
 * @param row Cursor row (0-indexed)
 * @param col Cursor column (0-indexed)
 * @return 0 on success, -1 on failure
 */
int lle_display_client_set_cursor(lle_display_client_t *client,
                                   uint16_t row,
                                   uint16_t col);

/**
 * Get performance metrics.
 * 
 * Retrieves performance metrics for display operations including render
 * count, timing, and validation compliance.
 * 
 * @param client Display client context
 * @param avg_time_ms Average render time in milliseconds (output)
 * @param violation_count Escape sequence violation count (output)
 * @return 0 on success, -1 on failure
 */
int lle_display_client_get_metrics(const lle_display_client_t *client,
                                    double *avg_time_ms,
                                    uint64_t *violation_count);

/**
 * Clear display buffer.
 * 
 * Clears the internal display buffer. Does not directly clear the terminal -
 * that operation goes through the display system.
 * 
 * @param client Display client context
 * @return 0 on success, -1 on failure
 */
int lle_display_client_clear(lle_display_client_t *client);

/**
 * Cleanup display client.
 * 
 * Releases all resources associated with the display client.
 * 
 * @param client Display client context
 */
void lle_display_client_cleanup(lle_display_client_t *client);

#endif // LLE_VALIDATION_DISPLAY_CLIENT_H

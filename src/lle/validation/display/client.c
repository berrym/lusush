/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Display Client Implementation - Week 2 Day 6
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int lle_display_client_init(lle_display_client_t *client,
                            uint16_t rows,
                            uint16_t cols) {
    if (!client || rows == 0 || cols == 0) {
        return -1;
    }
    
    // Initialize buffer
    size_t cell_count = (size_t)rows * cols;
    client->buffer.cells = calloc(cell_count, sizeof(lle_display_cell_t));
    if (!client->buffer.cells) {
        return -1;
    }
    
    client->buffer.rows = rows;
    client->buffer.cols = cols;
    client->buffer.dirty = false;
    
    // Initialize cursor at top-left
    client->cursor_row = 0;
    client->cursor_col = 0;
    
    // Initialize performance tracking
    client->render_count = 0;
    client->total_render_time_ns = 0;
    client->last_render_time_ns = 0;
    
    // Initialize validation tracking
    client->escape_sequence_violations = 0;
    client->validation_mode = true;  // Enable by default for Phase 0
    
    // Display system reference (NULL in Phase 0, will be set in Phase 1)
    client->display_system = NULL;
    
    return 0;
}

int lle_display_client_render(lle_display_client_t *client,
                               const char *line) {
    if (!client || !line) {
        return -1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Clear buffer
    size_t cell_count = (size_t)client->buffer.rows * client->buffer.cols;
    memset(client->buffer.cells, 0, cell_count * sizeof(lle_display_cell_t));
    
    // Render line to buffer (single-line for Day 6)
    size_t col = 0;
    const char *p = line;
    
    while (*p && col < client->buffer.cols) {
        lle_display_cell_t *cell = &client->buffer.cells[col];
        cell->codepoint = (uint32_t)(*p);  // ASCII for Phase 0
        cell->fg_color = 7;  // Default white
        cell->bg_color = 0;  // Default black
        cell->attrs = 0;     // No attributes
        
        p++;
        col++;
    }
    
    // Mark buffer as dirty (needs display system update)
    client->buffer.dirty = true;
    
    // Track performance
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t render_time_ns =
        (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
        (uint64_t)(end.tv_nsec - start.tv_nsec);
    
    client->last_render_time_ns = render_time_ns;
    client->total_render_time_ns += render_time_ns;
    client->render_count++;
    
    // Phase 0: No actual display output (validation only)
    // Phase 1: Call display_controller_display() here
    
    return 0;
}

int lle_display_client_render_multiline(lle_display_client_t *client,
                                         const char *line,
                                         size_t cursor_offset) {
    if (!client || !line) {
        return -1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Clear buffer
    size_t cell_count = (size_t)client->buffer.rows * client->buffer.cols;
    memset(client->buffer.cells, 0, cell_count * sizeof(lle_display_cell_t));
    
    // Render line with wrapping
    size_t row = 0, col = 0;
    const char *p = line;
    
    while (*p && row < client->buffer.rows) {
        lle_display_cell_t *cell = &client->buffer.cells[row * client->buffer.cols + col];
        cell->codepoint = (uint32_t)(*p);
        cell->fg_color = 7;
        cell->bg_color = 0;
        cell->attrs = 0;
        
        col++;
        if (col >= client->buffer.cols) {
            col = 0;
            row++;
        }
        
        p++;
    }
    
    // Update cursor position based on cursor_offset
    size_t cursor_row = cursor_offset / client->buffer.cols;
    size_t cursor_col = cursor_offset % client->buffer.cols;
    
    if (cursor_row < client->buffer.rows) {
        client->cursor_row = (uint16_t)cursor_row;
        client->cursor_col = (uint16_t)cursor_col;
    }
    
    client->buffer.dirty = true;
    
    // Track performance
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t render_time_ns =
        (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
        (uint64_t)(end.tv_nsec - start.tv_nsec);
    
    client->last_render_time_ns = render_time_ns;
    client->total_render_time_ns += render_time_ns;
    client->render_count++;
    
    return 0;
}

int lle_display_client_render_highlighted(lle_display_client_t *client,
                                           const char *line,
                                           const uint8_t *attrs,
                                           size_t attr_count) {
    if (!client || !line || !attrs) {
        return -1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Clear buffer
    size_t cell_count = (size_t)client->buffer.rows * client->buffer.cols;
    memset(client->buffer.cells, 0, cell_count * sizeof(lle_display_cell_t));
    
    // Render line with attributes
    size_t col = 0;
    const char *p = line;
    size_t attr_idx = 0;
    
    while (*p && col < client->buffer.cols) {
        lle_display_cell_t *cell = &client->buffer.cells[col];
        cell->codepoint = (uint32_t)(*p);
        cell->fg_color = 7;
        cell->bg_color = 0;
        cell->attrs = (attr_idx < attr_count) ? attrs[attr_idx] : 0;
        
        p++;
        col++;
        attr_idx++;
    }
    
    client->buffer.dirty = true;
    
    // Track performance
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t render_time_ns =
        (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL +
        (uint64_t)(end.tv_nsec - start.tv_nsec);
    
    client->last_render_time_ns = render_time_ns;
    client->total_render_time_ns += render_time_ns;
    client->render_count++;
    
    return 0;
}

int lle_display_client_set_cursor(lle_display_client_t *client,
                                   uint16_t row,
                                   uint16_t col) {
    if (!client) {
        return -1;
    }
    
    // Validate bounds
    if (row >= client->buffer.rows || col >= client->buffer.cols) {
        return -1;
    }
    
    client->cursor_row = row;
    client->cursor_col = col;
    
    return 0;
}

int lle_display_client_get_metrics(const lle_display_client_t *client,
                                    double *avg_time_ms,
                                    uint64_t *violation_count) {
    if (!client) {
        return -1;
    }
    
    if (avg_time_ms) {
        if (client->render_count > 0) {
            uint64_t avg_ns = client->total_render_time_ns / client->render_count;
            *avg_time_ms = (double)avg_ns / 1000000.0;
        } else {
            *avg_time_ms = 0.0;
        }
    }
    
    if (violation_count) {
        *violation_count = client->escape_sequence_violations;
    }
    
    return 0;
}

int lle_display_client_clear(lle_display_client_t *client) {
    if (!client) {
        return -1;
    }
    
    // Clear buffer
    size_t cell_count = (size_t)client->buffer.rows * client->buffer.cols;
    memset(client->buffer.cells, 0, cell_count * sizeof(lle_display_cell_t));
    
    client->buffer.dirty = true;
    client->cursor_row = 0;
    client->cursor_col = 0;
    
    return 0;
}

void lle_display_client_cleanup(lle_display_client_t *client) {
    if (!client) {
        return;
    }
    
    if (client->buffer.cells) {
        free(client->buffer.cells);
        client->buffer.cells = NULL;
    }
    
    client->buffer.rows = 0;
    client->buffer.cols = 0;
    client->display_system = NULL;
}

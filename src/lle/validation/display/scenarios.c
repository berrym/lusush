/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Rendering Scenarios Implementation - Week 2 Day 7
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#include "scenarios.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int lle_scenario_render_single_line(lle_display_client_t *client,
                                     const scenario_single_line_t *scenario) {
    if (!client || !scenario || !scenario->input) {
        return -1;
    }
    
    // Render single line through display client
    int ret = lle_display_client_render(client, scenario->input);
    if (ret != 0) {
        return -1;
    }
    
    // Set cursor position
    size_t input_len = strlen(scenario->input);
    if (scenario->cursor_pos <= input_len) {
        lle_display_client_set_cursor(client, 0, (uint16_t)scenario->cursor_pos);
    }
    
    return 0;
}

int lle_scenario_render_multiline(lle_display_client_t *client,
                                   const scenario_multiline_t *scenario) {
    if (!client || !scenario || !scenario->input) {
        return -1;
    }
    
    // Render multi-line through display client
    int ret = lle_display_client_render_multiline(client, 
                                                   scenario->input,
                                                   scenario->cursor_pos);
    if (ret != 0) {
        return -1;
    }
    
    return 0;
}

int lle_scenario_render_prompt(lle_display_client_t *client,
                                const scenario_prompt_t *scenario) {
    if (!client || !scenario || !scenario->prompt || !scenario->input) {
        return -1;
    }
    
    // Combine prompt + input
    char combined[512];
    size_t prompt_len = strlen(scenario->prompt);
    size_t input_len = strlen(scenario->input);
    
    if (prompt_len + input_len >= sizeof(combined)) {
        return -1;  // Buffer too small
    }
    
    snprintf(combined, sizeof(combined), "%s%s", scenario->prompt, scenario->input);
    
    // Render combined string
    int ret = lle_display_client_render(client, combined);
    if (ret != 0) {
        return -1;
    }
    
    // Set cursor position (prompt_len + cursor_pos)
    size_t total_cursor_pos = prompt_len + scenario->cursor_pos;
    if (total_cursor_pos < client->buffer.cols) {
        lle_display_client_set_cursor(client, 0, (uint16_t)total_cursor_pos);
    }
    
    return 0;
}

int lle_scenario_render_highlighting(lle_display_client_t *client,
                                      const scenario_highlighting_t *scenario) {
    if (!client || !scenario || !scenario->input || !scenario->highlight_attrs) {
        return -1;
    }
    
    // Render with highlighting through display client
    int ret = lle_display_client_render_highlighted(client,
                                                     scenario->input,
                                                     scenario->highlight_attrs,
                                                     scenario->attr_count);
    if (ret != 0) {
        return -1;
    }
    
    // Set cursor position
    size_t input_len = strlen(scenario->input);
    if (scenario->cursor_pos <= input_len && scenario->cursor_pos < client->buffer.cols) {
        lle_display_client_set_cursor(client, 0, (uint16_t)scenario->cursor_pos);
    }
    
    return 0;
}

int lle_scenario_render_completion(lle_display_client_t *client,
                                    const scenario_completion_t *scenario) {
    if (!client || !scenario || !scenario->input || !scenario->completion) {
        return -1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Clear buffer
    lle_display_client_clear(client);
    
    // Render input normally
    size_t input_len = strlen(scenario->input);
    const char *p = scenario->input;
    size_t col = 0;
    
    while (*p && col < client->buffer.cols) {
        lle_display_cell_t *cell = &client->buffer.cells[col];
        cell->codepoint = (uint32_t)(*p);
        cell->fg_color = 7;  // Normal white
        cell->bg_color = 0;
        cell->attrs = 0;
        p++;
        col++;
    }
    
    // Render completion preview dimmed (after input)
    p = scenario->completion;
    while (*p && col < client->buffer.cols) {
        lle_display_cell_t *cell = &client->buffer.cells[col];
        cell->codepoint = (uint32_t)(*p);
        cell->fg_color = 8;  // Dimmed gray
        cell->bg_color = 0;
        cell->attrs = LLE_ATTR_DIM;
        p++;
        col++;
    }
    
    client->buffer.dirty = true;
    
    // Set cursor at end of actual input
    if (scenario->cursor_pos <= input_len && scenario->cursor_pos < client->buffer.cols) {
        lle_display_client_set_cursor(client, 0, (uint16_t)scenario->cursor_pos);
    }
    
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

int lle_scenario_render_scroll_region(lle_display_client_t *client,
                                       const scenario_scroll_region_t *scenario) {
    if (!client || !scenario || !scenario->lines) {
        return -1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Clear buffer
    lle_display_client_clear(client);
    
    // Render visible lines
    size_t row = 0;
    for (size_t i = 0; i < scenario->visible_count && row < client->buffer.rows; i++) {
        size_t line_idx = scenario->visible_start + i;
        if (line_idx >= scenario->line_count) {
            break;
        }
        
        const char *line = scenario->lines[line_idx];
        const char *p = line;
        size_t col = 0;
        
        while (*p && col < client->buffer.cols) {
            lle_display_cell_t *cell = &client->buffer.cells[row * client->buffer.cols + col];
            cell->codepoint = (uint32_t)(*p);
            cell->fg_color = 7;
            cell->bg_color = 0;
            cell->attrs = 0;
            p++;
            col++;
        }
        
        row++;
    }
    
    client->buffer.dirty = true;
    
    // Set cursor at cursor_line
    if (scenario->cursor_line < client->buffer.rows) {
        lle_display_client_set_cursor(client, (uint16_t)scenario->cursor_line, 0);
    }
    
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

int lle_scenario_render_atomic_update(lle_display_client_t *client,
                                       const scenario_atomic_update_t *scenario) {
    if (!client || !scenario || !scenario->new_content) {
        return -1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // For Phase 0, we simply render the new content
    // In Phase 1, this will use display system diff algorithms
    
    if (scenario->should_diff && scenario->old_content) {
        // Phase 0: Full update (Phase 1 will implement diff algorithm)
        lle_display_client_render(client, scenario->new_content);
    } else {
        // Simple atomic update
        lle_display_client_render(client, scenario->new_content);
    }
    
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

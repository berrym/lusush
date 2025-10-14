/*
 * Lusush Line Editor (LLE) - Phase 0 Validation
 * Rendering Scenarios Interface - Week 2 Day 7
 * 
 * Copyright (C) 2021-2025  Michael Berry
 */

#ifndef LLE_VALIDATION_DISPLAY_SCENARIOS_H
#define LLE_VALIDATION_DISPLAY_SCENARIOS_H

#include "client.h"

/*
 * RENDERING SCENARIOS
 * 
 * This module implements the 7 critical rendering scenarios that validate
 * LLE's ability to function as a pure display system client. All scenarios
 * must work without any direct terminal escape sequences.
 * 
 * Success Criteria (Week 2):
 * - All 7 scenarios render correctly
 * - Zero escape sequences in implementation
 * - Update latency <10ms for complex scenarios
 * - No visual artifacts
 */

// Scenario 1: Single-line rendering
typedef struct {
    const char *input;          // Input line
    size_t cursor_pos;          // Cursor position in input
} scenario_single_line_t;

// Scenario 2: Multi-line rendering (wrapping)
typedef struct {
    const char *input;          // Input line (may be >80 chars)
    size_t cursor_pos;          // Cursor position in input
    size_t expected_rows;       // Expected number of rows used
} scenario_multiline_t;

// Scenario 3: Prompt rendering
typedef struct {
    const char *prompt;         // Prompt string
    const char *input;          // Input line
    size_t cursor_pos;          // Cursor position in input (not including prompt)
} scenario_prompt_t;

// Scenario 4: Syntax highlighting
typedef struct {
    const char *input;          // Input line
    const uint8_t *highlight_attrs; // Highlighting attributes array
    size_t attr_count;          // Number of attributes
    size_t cursor_pos;          // Cursor position
} scenario_highlighting_t;

// Scenario 5: Completion preview
typedef struct {
    const char *input;          // Current input
    const char *completion;     // Completion suggestion (shown dimmed)
    size_t cursor_pos;          // Cursor position
} scenario_completion_t;

// Scenario 6: Scroll region management
typedef struct {
    const char **lines;         // Array of lines (for history scrollback)
    size_t line_count;          // Number of lines
    size_t visible_start;       // First visible line index
    size_t visible_count;       // Number of visible lines
    size_t cursor_line;         // Cursor line (relative to visible_start)
} scenario_scroll_region_t;

// Scenario 7: Atomic updates
typedef struct {
    const char *old_content;    // Previous content
    const char *new_content;    // New content
    bool should_diff;           // Whether to use diff algorithm
} scenario_atomic_update_t;

/**
 * Render Scenario 1: Single-line input.
 * Simple single-line rendering with cursor positioning.
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_single_line(lle_display_client_t *client,
                                     const scenario_single_line_t *scenario);

/**
 * Render Scenario 2: Multi-line input with wrapping.
 * Handles long input lines that wrap across multiple screen lines.
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_multiline(lle_display_client_t *client,
                                   const scenario_multiline_t *scenario);

/**
 * Render Scenario 3: Prompt + input.
 * Renders prompt followed by input, with cursor position relative to input start.
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_prompt(lle_display_client_t *client,
                                const scenario_prompt_t *scenario);

/**
 * Render Scenario 4: Syntax highlighting.
 * Renders input with syntax highlighting attributes applied.
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_highlighting(lle_display_client_t *client,
                                      const scenario_highlighting_t *scenario);

/**
 * Render Scenario 5: Completion preview.
 * Renders input with dimmed completion suggestion after cursor.
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_completion(lle_display_client_t *client,
                                    const scenario_completion_t *scenario);

/**
 * Render Scenario 6: Scroll region.
 * Manages scrollable content with visible region (e.g., history scrollback).
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_scroll_region(lle_display_client_t *client,
                                       const scenario_scroll_region_t *scenario);

/**
 * Render Scenario 7: Atomic updates.
 * Performs atomic display updates, optionally using diff algorithm.
 * 
 * @param client Display client
 * @param scenario Scenario data
 * @return 0 on success, -1 on failure
 */
int lle_scenario_render_atomic_update(lle_display_client_t *client,
                                       const scenario_atomic_update_t *scenario);

#endif // LLE_VALIDATION_DISPLAY_SCENARIOS_H

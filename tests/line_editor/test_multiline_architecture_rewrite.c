/**
 * @file test_multiline_architecture_rewrite.c
 * @brief Test suite for Multi-Line Architecture Rewrite (Phase 1A)
 *
 * Tests the fundamental architectural changes needed to support proper
 * multi-line cursor positioning, including coordinate conversion functions,
 * position tracking, and multi-line terminal operations.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/cursor_math.h"
#include "../src/line_editor/terminal_manager.h"
#include "../src/line_editor/display.h"
#include "test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test framework macros
#define LLE_TEST(name) \
    static void test_##name(void); \
    static void test_##name(void)

#define LLE_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s at %s:%d\n", \
                    #condition, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "ASSERTION FAILED: %s (%zu) != %s (%zu) at %s:%d\n", \
                    #actual, (size_t)(actual), #expected, (size_t)(expected), \
                    __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

#define LLE_ASSERT_TRUE(condition) LLE_ASSERT(condition)
#define LLE_ASSERT_FALSE(condition) LLE_ASSERT(!(condition))

// Phase 1A: Coordinate Conversion Tests

LLE_TEST(coordinate_conversion_basic) {
    printf("Testing basic coordinate conversion... ");
    
    // Create a simple relative cursor position
    lle_cursor_position_t relative_pos = {
        .absolute_row = 1,
        .absolute_col = 5,
        .relative_row = 1,
        .relative_col = 5,
        .at_boundary = false,
        .valid = true
    };
    
    // Convert to terminal coordinates with prompt at (2, 10)
    lle_terminal_coordinates_t terminal_coords = lle_convert_to_terminal_coordinates(
        &relative_pos, 2, 10);
    
    LLE_ASSERT_TRUE(terminal_coords.valid);
    LLE_ASSERT_EQ(terminal_coords.terminal_row, 3);  // 2 + 1
    LLE_ASSERT_EQ(terminal_coords.terminal_col, 15); // 10 + 5
    
    printf("PASSED\n");
}

LLE_TEST(coordinate_conversion_prompt_origin) {
    printf("Testing coordinate conversion with prompt at origin... ");
    
    // Cursor position relative to prompt
    lle_cursor_position_t relative_pos = {
        .absolute_row = 0,
        .absolute_col = 0,
        .relative_row = 0,
        .relative_col = 0,
        .at_boundary = false,
        .valid = true
    };
    
    // Convert with prompt at terminal origin
    lle_terminal_coordinates_t terminal_coords = lle_convert_to_terminal_coordinates(
        &relative_pos, 0, 0);
    
    LLE_ASSERT_TRUE(terminal_coords.valid);
    LLE_ASSERT_EQ(terminal_coords.terminal_row, 0);
    LLE_ASSERT_EQ(terminal_coords.terminal_col, 0);
    
    printf("PASSED\n");
}

LLE_TEST(coordinate_conversion_multiline_wrapped) {
    printf("Testing coordinate conversion for wrapped content... ");
    
    // Cursor position on second wrapped line
    lle_cursor_position_t relative_pos = {
        .absolute_row = 2,
        .absolute_col = 10,
        .relative_row = 2,
        .relative_col = 10,
        .at_boundary = false,
        .valid = true
    };
    
    // Convert with prompt starting at (1, 5)
    lle_terminal_coordinates_t terminal_coords = lle_convert_to_terminal_coordinates(
        &relative_pos, 1, 5);
    
    LLE_ASSERT_TRUE(terminal_coords.valid);
    LLE_ASSERT_EQ(terminal_coords.terminal_row, 3);  // 1 + 2
    LLE_ASSERT_EQ(terminal_coords.terminal_col, 15); // 5 + 10
    
    printf("PASSED\n");
}

LLE_TEST(coordinate_conversion_invalid_input) {
    printf("Testing coordinate conversion with invalid input... ");
    
    // Test with NULL relative position
    lle_terminal_coordinates_t result1 = lle_convert_to_terminal_coordinates(
        NULL, 0, 0);
    LLE_ASSERT_FALSE(result1.valid);
    
    // Test with invalid relative position
    lle_cursor_position_t invalid_pos = {
        .absolute_row = 0,
        .absolute_col = 0,
        .relative_row = 0,
        .relative_col = 0,
        .at_boundary = false,
        .valid = false
    };
    
    lle_terminal_coordinates_t result2 = lle_convert_to_terminal_coordinates(
        &invalid_pos, 0, 0);
    LLE_ASSERT_FALSE(result2.valid);
    
    printf("PASSED\n");
}

LLE_TEST(reverse_coordinate_conversion) {
    printf("Testing reverse coordinate conversion... ");
    
    // Terminal geometry for bounds checking
    lle_terminal_geometry_t geometry = {
        .width = 80,
        .height = 24,
        .prompt_width = 10,
        .prompt_height = 1
    };
    
    // Terminal coordinates to convert back
    lle_terminal_coordinates_t terminal_coords = {
        .terminal_row = 5,
        .terminal_col = 15,
        .valid = true
    };
    
    // Convert back to relative position (prompt at 2, 5)
    lle_cursor_position_t relative_pos = lle_convert_from_terminal_coordinates(
        &terminal_coords, 2, 5, &geometry);
    
    LLE_ASSERT_TRUE(relative_pos.valid);
    LLE_ASSERT_EQ(relative_pos.absolute_row, 3);  // 5 - 2
    LLE_ASSERT_EQ(relative_pos.absolute_col, 10); // 15 - 5
    
    printf("PASSED\n");
}

LLE_TEST(content_start_coordinates_single_line) {
    printf("Testing content start coordinates for single-line prompt... ");
    
    // Single-line prompt geometry
    lle_prompt_geometry_t prompt_geom = {
        .width = 15,
        .height = 1,
        .last_line_width = 15
    };
    
    // Calculate content start (prompt at 2, 5)
    lle_terminal_coordinates_t content_start = lle_calculate_content_start_coordinates(
        2, 5, &prompt_geom);
    
    LLE_ASSERT_TRUE(content_start.valid);
    LLE_ASSERT_EQ(content_start.terminal_row, 2);  // Same row as prompt
    LLE_ASSERT_EQ(content_start.terminal_col, 20); // 5 + 15
    
    printf("PASSED\n");
}

LLE_TEST(content_start_coordinates_multiline) {
    printf("Testing content start coordinates for multi-line prompt... ");
    
    // Multi-line prompt geometry
    lle_prompt_geometry_t prompt_geom = {
        .width = 25,
        .height = 3,
        .last_line_width = 12
    };
    
    // Calculate content start (prompt at 1, 0)
    lle_terminal_coordinates_t content_start = lle_calculate_content_start_coordinates(
        1, 0, &prompt_geom);
    
    LLE_ASSERT_TRUE(content_start.valid);
    LLE_ASSERT_EQ(content_start.terminal_row, 3);  // 1 + 3 - 1
    LLE_ASSERT_EQ(content_start.terminal_col, 12); // 0 + 12
    
    printf("PASSED\n");
}

LLE_TEST(terminal_coordinates_validation) {
    printf("Testing terminal coordinates validation... ");
    
    lle_terminal_geometry_t geometry = {
        .width = 80,
        .height = 24,
        .prompt_width = 10,
        .prompt_height = 1
    };
    
    // Valid coordinates
    lle_terminal_coordinates_t valid_coords = {
        .terminal_row = 10,
        .terminal_col = 30,
        .valid = true
    };
    LLE_ASSERT_TRUE(lle_validate_terminal_coordinates(&valid_coords, &geometry));
    
    // Invalid coordinates (out of bounds)
    lle_terminal_coordinates_t invalid_coords1 = {
        .terminal_row = 25,  // >= height
        .terminal_col = 30,
        .valid = true
    };
    LLE_ASSERT_FALSE(lle_validate_terminal_coordinates(&invalid_coords1, &geometry));
    
    lle_terminal_coordinates_t invalid_coords2 = {
        .terminal_row = 10,
        .terminal_col = 80,  // >= width
        .valid = true
    };
    LLE_ASSERT_FALSE(lle_validate_terminal_coordinates(&invalid_coords2, &geometry));
    
    // Invalid coordinates (marked as invalid)
    lle_terminal_coordinates_t invalid_coords3 = {
        .terminal_row = 10,
        .terminal_col = 30,
        .valid = false
    };
    LLE_ASSERT_FALSE(lle_validate_terminal_coordinates(&invalid_coords3, &geometry));
    
    printf("PASSED\n");
}

// Phase 1A: Display State Position Tracking Tests

LLE_TEST(display_state_position_tracking_init) {
    printf("Testing display state position tracking initialization... ");
    
    lle_display_state_t state;
    memset(&state, 0, sizeof(state));
    
    LLE_ASSERT_TRUE(lle_display_init(&state));
    
    // Check that position tracking fields are initialized
    LLE_ASSERT_EQ(state.prompt_start_row, 0);
    LLE_ASSERT_EQ(state.prompt_start_col, 0);
    LLE_ASSERT_EQ(state.prompt_end_row, 0);
    LLE_ASSERT_EQ(state.prompt_end_col, 0);
    LLE_ASSERT_EQ(state.content_start_row, 0);
    LLE_ASSERT_EQ(state.content_start_col, 0);
    LLE_ASSERT_EQ(state.content_end_row, 0);
    LLE_ASSERT_EQ(state.content_end_col, 0);
    LLE_ASSERT_FALSE(state.position_tracking_valid);
    
    printf("PASSED\n");
}

// Phase 1A: Multi-Line Terminal Operations Tests
// Note: These tests require mock terminal manager for complete testing
// For now, we test the parameter validation and bounds checking

LLE_TEST(terminal_clear_region_parameter_validation) {
    printf("Testing terminal clear region parameter validation... ");
    
    // Test with NULL terminal manager
    bool result1 = lle_terminal_clear_region(NULL, 0, 0, 1, 10);
    LLE_ASSERT_FALSE(result1);
    
    // Test with invalid region (start > end)
    lle_terminal_manager_t tm = {0};
    tm.termcap_initialized = true;
    tm.geometry_valid = true;
    tm.geometry.width = 80;
    tm.geometry.height = 24;
    
    bool result2 = lle_terminal_clear_region(&tm, 5, 0, 3, 10);  // start_row > end_row
    LLE_ASSERT_FALSE(result2);
    
    bool result3 = lle_terminal_clear_region(&tm, 3, 15, 3, 10); // start_col > end_col
    LLE_ASSERT_FALSE(result3);
    
    printf("PASSED\n");
}

LLE_TEST(terminal_clear_lines_parameter_validation) {
    printf("Testing terminal clear lines parameter validation... ");
    
    // Test with NULL terminal manager
    bool result1 = lle_terminal_clear_lines(NULL, 0, 5);
    LLE_ASSERT_FALSE(result1);
    
    // Test with zero lines
    lle_terminal_manager_t tm = {0};
    tm.termcap_initialized = true;
    
    bool result2 = lle_terminal_clear_lines(&tm, 0, 0);
    LLE_ASSERT_FALSE(result2);
    
    printf("PASSED\n");
}

LLE_TEST(terminal_clear_from_position_parameter_validation) {
    printf("Testing terminal clear from position parameter validation... ");
    
    // Test with NULL terminal manager
    bool result1 = lle_terminal_clear_from_position_to_eol(NULL, 0, 0);
    LLE_ASSERT_FALSE(result1);
    
    bool result2 = lle_terminal_clear_from_position_to_eos(NULL, 0, 0);
    LLE_ASSERT_FALSE(result2);
    
    printf("PASSED\n");
}

LLE_TEST(terminal_cursor_position_parameter_validation) {
    printf("Testing terminal cursor position query parameter validation... ");
    
    size_t row, col;
    
    // Test with NULL terminal manager
    bool result1 = lle_terminal_query_cursor_position(NULL, &row, &col);
    LLE_ASSERT_FALSE(result1);
    
    // Test with NULL output parameters
    lle_terminal_manager_t tm = {0};
    tm.termcap_initialized = true;
    
    bool result2 = lle_terminal_query_cursor_position(&tm, NULL, &col);
    LLE_ASSERT_FALSE(result2);
    
    bool result3 = lle_terminal_query_cursor_position(&tm, &row, NULL);
    LLE_ASSERT_FALSE(result3);
    
    printf("PASSED\n");
}

// Integration Tests

LLE_TEST(coordinate_conversion_round_trip) {
    printf("Testing coordinate conversion round trip... ");
    
    lle_terminal_geometry_t geometry = {
        .width = 80,
        .height = 24,
        .prompt_width = 12,
        .prompt_height = 1
    };
    
    // Original relative position
    lle_cursor_position_t original = {
        .absolute_row = 2,
        .absolute_col = 25,
        .relative_row = 2,
        .relative_col = 25,
        .at_boundary = false,
        .valid = true
    };
    
    size_t prompt_start_row = 3;
    size_t prompt_start_col = 7;
    
    // Convert to terminal coordinates
    lle_terminal_coordinates_t terminal_coords = lle_convert_to_terminal_coordinates(
        &original, prompt_start_row, prompt_start_col);
    LLE_ASSERT_TRUE(terminal_coords.valid);
    
    // Convert back to relative position
    lle_cursor_position_t converted_back = lle_convert_from_terminal_coordinates(
        &terminal_coords, prompt_start_row, prompt_start_col, &geometry);
    LLE_ASSERT_TRUE(converted_back.valid);
    
    // Should match original (excluding at_boundary which needs more context)
    LLE_ASSERT_EQ(converted_back.absolute_row, original.absolute_row);
    LLE_ASSERT_EQ(converted_back.absolute_col, original.absolute_col);
    
    printf("PASSED\n");
}

LLE_TEST(multiline_scenario_comprehensive) {
    printf("Testing comprehensive multi-line scenario... ");
    
    // Terminal setup: 80x24
    lle_terminal_geometry_t geometry = {
        .width = 80,
        .height = 24,
        .prompt_width = 15,
        .prompt_height = 2
    };
    
    // Multi-line prompt starting at row 5, col 0
    lle_prompt_geometry_t prompt_geom = {
        .width = 20,
        .height = 2,
        .last_line_width = 15
    };
    
    size_t prompt_start_row = 5;
    size_t prompt_start_col = 0;
    
    // Calculate where content should start
    lle_terminal_coordinates_t content_start = lle_calculate_content_start_coordinates(
        prompt_start_row, prompt_start_col, &prompt_geom);
    
    LLE_ASSERT_TRUE(content_start.valid);
    LLE_ASSERT_EQ(content_start.terminal_row, 6);  // 5 + 2 - 1
    LLE_ASSERT_EQ(content_start.terminal_col, 15); // 0 + 15
    
    // Cursor position in wrapped text (3rd line of content)
    lle_cursor_position_t text_cursor = {
        .absolute_row = 2,  // 2 lines after content start
        .absolute_col = 30,
        .relative_row = 2,
        .relative_col = 30,
        .at_boundary = false,
        .valid = true
    };
    
    // Convert to terminal coordinates
    lle_terminal_coordinates_t terminal_cursor = lle_convert_to_terminal_coordinates(
        &text_cursor, content_start.terminal_row, content_start.terminal_col);
    
    LLE_ASSERT_TRUE(terminal_cursor.valid);
    LLE_ASSERT_EQ(terminal_cursor.terminal_row, 8);  // 6 + 2
    LLE_ASSERT_EQ(terminal_cursor.terminal_col, 45); // 15 + 30
    
    // Validate the final coordinates
    LLE_ASSERT_TRUE(lle_validate_terminal_coordinates(&terminal_cursor, &geometry));
    
    printf("PASSED\n");
}

// Main test runner
int main(void) {
    printf("=== Multi-Line Architecture Rewrite Tests (Phase 1A) ===\n\n");
    
    // Coordinate conversion tests
    printf("--- Coordinate Conversion Tests ---\n");
    test_coordinate_conversion_basic();
    test_coordinate_conversion_prompt_origin();
    test_coordinate_conversion_multiline_wrapped();
    test_coordinate_conversion_invalid_input();
    test_reverse_coordinate_conversion();
    test_content_start_coordinates_single_line();
    test_content_start_coordinates_multiline();
    test_terminal_coordinates_validation();
    
    // Display state tests
    printf("\n--- Display State Position Tracking Tests ---\n");
    test_display_state_position_tracking_init();
    
    // Multi-line terminal operations tests
    printf("\n--- Multi-Line Terminal Operations Tests ---\n");
    test_terminal_clear_region_parameter_validation();
    test_terminal_clear_lines_parameter_validation();
    test_terminal_clear_from_position_parameter_validation();
    test_terminal_cursor_position_parameter_validation();
    
    // Integration tests
    printf("\n--- Integration Tests ---\n");
    test_coordinate_conversion_round_trip();
    test_multiline_scenario_comprehensive();
    
    printf("\n=== All Multi-Line Architecture Rewrite Tests Passed! ===\n");
    printf("Phase 1A Infrastructure: COMPLETE\n");
    printf("Ready for Phase 2A: Core Display System Rewrite\n");
    
    return 0;
}
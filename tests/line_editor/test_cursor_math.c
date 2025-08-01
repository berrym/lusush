/**
 * @file test_cursor_math.c
 * @brief Test suite for LLE cursor mathematics structures
 *
 * Tests for cursor position structures and validation functions.
 * Ensures mathematical correctness and proper structure validation.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/cursor_math.h"
#include "../src/line_editor/text_buffer.h"
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

#define LLE_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "ASSERTION FAILED: %s is NULL at %s:%d\n", \
                    #ptr, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test) \
    do { \
        printf("Running test_%s...", #test); \
        fflush(stdout); \
        test_##test(); \
        tests_run++; \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

// =====================================
// LLE-006: Cursor Position Structure Tests
// =====================================

// Test: Cursor position structure initialization
LLE_TEST(cursor_position_structure) {
    lle_cursor_position_t pos;
    
    // Initialize structure
    pos.absolute_row = 5;
    pos.absolute_col = 10;
    pos.relative_row = 2;
    pos.relative_col = 8;
    pos.at_boundary = false;
    pos.valid = true;
    
    // Verify fields are accessible and hold correct values
    LLE_ASSERT_EQ(pos.absolute_row, 5);
    LLE_ASSERT_EQ(pos.absolute_col, 10);
    LLE_ASSERT_EQ(pos.relative_row, 2);
    LLE_ASSERT_EQ(pos.relative_col, 8);
    LLE_ASSERT(pos.at_boundary == false);
    LLE_ASSERT(pos.valid == true);
}

// Test: Terminal geometry structure initialization
LLE_TEST(terminal_geometry_structure) {
    lle_terminal_geometry_t geometry;
    
    // Initialize structure
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 1;
    
    // Verify fields are accessible and hold correct values
    LLE_ASSERT_EQ(geometry.width, 80);
    LLE_ASSERT_EQ(geometry.height, 24);
    LLE_ASSERT_EQ(geometry.prompt_width, 12);
    LLE_ASSERT_EQ(geometry.prompt_height, 1);
}

// Test: Terminal geometry validation with valid values
LLE_TEST(terminal_geometry_validation_valid) {
    lle_terminal_geometry_t geometry;
    
    // Test valid standard terminal
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 1;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
    
    // Test valid wide terminal
    geometry.width = 120;
    geometry.height = 30;
    geometry.prompt_width = 20;
    geometry.prompt_height = 2;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
    
    // Test minimum valid terminal
    geometry.width = LLE_MIN_TERMINAL_WIDTH;
    geometry.height = LLE_MIN_TERMINAL_HEIGHT;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
    
    // Test maximum valid terminal
    geometry.width = LLE_MAX_TERMINAL_WIDTH;
    geometry.height = LLE_MAX_TERMINAL_HEIGHT;
    geometry.prompt_width = LLE_MAX_TERMINAL_WIDTH;
    geometry.prompt_height = LLE_MAX_TERMINAL_HEIGHT;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
}

// Test: Terminal geometry validation with invalid values
LLE_TEST(terminal_geometry_validation_invalid) {
    lle_terminal_geometry_t geometry;
    
    // Test NULL pointer
    LLE_ASSERT(!lle_validate_terminal_geometry(NULL));
    
    // Test width too small
    geometry.width = LLE_MIN_TERMINAL_WIDTH - 1;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test width too large
    geometry.width = LLE_MAX_TERMINAL_WIDTH + 1;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test height too small
    geometry.width = 80;
    geometry.height = LLE_MIN_TERMINAL_HEIGHT - 1;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test height too large
    geometry.width = 80;
    geometry.height = LLE_MAX_TERMINAL_HEIGHT + 1;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test prompt width exceeds terminal width
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 81;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test prompt height exceeds terminal height
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 25;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
}

// Test: Terminal geometry initialization
LLE_TEST(terminal_geometry_initialization) {
    lle_terminal_geometry_t geometry;
    
    // Initialize with defaults
    lle_init_terminal_geometry(&geometry);
    
    // Verify default values
    LLE_ASSERT_EQ(geometry.width, LLE_DEFAULT_TERMINAL_WIDTH);
    LLE_ASSERT_EQ(geometry.height, LLE_DEFAULT_TERMINAL_HEIGHT);
    LLE_ASSERT_EQ(geometry.prompt_width, 0);
    LLE_ASSERT_EQ(geometry.prompt_height, 1);
    
    // Verify initialized geometry is valid
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
}

// Test: Terminal geometry initialization with NULL pointer
LLE_TEST(terminal_geometry_initialization_null) {
    // Should not crash
    lle_init_terminal_geometry(NULL);
}

// Test: Constants are properly defined
LLE_TEST(constants_defined) {
    // Check that all constants are defined and reasonable
    LLE_ASSERT(LLE_MAX_TERMINAL_WIDTH > 0);
    LLE_ASSERT(LLE_MAX_TERMINAL_HEIGHT > 0);
    LLE_ASSERT(LLE_MIN_TERMINAL_WIDTH > 0);
    LLE_ASSERT(LLE_MIN_TERMINAL_HEIGHT > 0);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_WIDTH > 0);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_HEIGHT > 0);
    
    // Check relationships between constants
    LLE_ASSERT(LLE_MIN_TERMINAL_WIDTH <= LLE_DEFAULT_TERMINAL_WIDTH);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_WIDTH <= LLE_MAX_TERMINAL_WIDTH);
    LLE_ASSERT(LLE_MIN_TERMINAL_HEIGHT <= LLE_DEFAULT_TERMINAL_HEIGHT);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_HEIGHT <= LLE_MAX_TERMINAL_HEIGHT);
    
    // Check reasonable values
    LLE_ASSERT(LLE_MIN_TERMINAL_WIDTH >= 10);
    LLE_ASSERT(LLE_MIN_TERMINAL_HEIGHT >= 3);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_WIDTH >= 80);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_HEIGHT >= 24);
}

// Test: Structure size and alignment
LLE_TEST(structure_properties) {
    // Check that structures have reasonable sizes
    LLE_ASSERT(sizeof(lle_cursor_position_t) > 0);
    LLE_ASSERT(sizeof(lle_terminal_geometry_t) > 0);
    
    // Structures should not be unreasonably large
    LLE_ASSERT(sizeof(lle_cursor_position_t) < 1024);
    LLE_ASSERT(sizeof(lle_terminal_geometry_t) < 1024);
    
    // Check that bool fields work correctly
    lle_cursor_position_t pos;
    pos.at_boundary = true;
    pos.valid = false;
    LLE_ASSERT(pos.at_boundary == true);
    LLE_ASSERT(pos.valid == false);
}

// Test: Mathematical invariants documentation
LLE_TEST(mathematical_invariants) {
    lle_cursor_position_t pos;
    lle_terminal_geometry_t geometry;
    
    // Test documented invariant: absolute_row >= relative_row
    pos.absolute_row = 10;
    pos.relative_row = 5;
    LLE_ASSERT(pos.absolute_row >= pos.relative_row);
    
    // Test boundary conditions
    pos.absolute_row = 0;
    pos.relative_row = 0;
    LLE_ASSERT(pos.absolute_row >= pos.relative_row);
    
    // Test geometry constraints
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 2;
    
    // Documented invariants
    LLE_ASSERT(geometry.prompt_width <= geometry.width);
    LLE_ASSERT(geometry.prompt_height <= geometry.height);
}

// =====================================
// LLE-007: Cursor Position Calculation Tests
// =====================================

// Test: Basic cursor position calculation
LLE_TEST(calculate_cursor_position_basic) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup terminal geometry
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Insert some text: "hello"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    buffer->cursor_pos = 3; // Position at 'l'
    
    // Calculate cursor position
    lle_cursor_position_t pos = lle_calculate_cursor_position(buffer, &geometry, 12);
    
    // Verify result
    LLE_ASSERT(pos.valid);
    LLE_ASSERT_EQ(pos.relative_row, 0);
    LLE_ASSERT_EQ(pos.relative_col, 15); // 12 (prompt) + 3 (text position)
    LLE_ASSERT_EQ(pos.absolute_row, 0);
    LLE_ASSERT_EQ(pos.absolute_col, 15);
    LLE_ASSERT(!pos.at_boundary);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Cursor position with line wrapping
LLE_TEST(calculate_cursor_position_wrapping) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup narrow terminal  
    geometry.width = 25;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Insert text that will wrap: "hello world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    buffer->cursor_pos = 11; // End of text
    
    // Calculate cursor position with prompt "$ " (2 chars)
    lle_cursor_position_t pos = lle_calculate_cursor_position(buffer, &geometry, 2);
    
    // Verify result: 2 + 11 = 13 chars total
    // Line 0: 25 chars, all on one line
    LLE_ASSERT(pos.valid);
    LLE_ASSERT_EQ(pos.relative_row, 0);
    LLE_ASSERT_EQ(pos.relative_col, 13);
    LLE_ASSERT_EQ(pos.absolute_row, 0);
    LLE_ASSERT_EQ(pos.absolute_col, 13);
    LLE_ASSERT(!pos.at_boundary);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Cursor position at line boundary
LLE_TEST(calculate_cursor_position_boundary) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup terminal
    geometry.width = 20;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Insert text that exactly fills first line after prompt
    LLE_ASSERT(lle_text_insert_string(buffer, "123456789012345678")); // 18 chars
    buffer->cursor_pos = 18;
    
    // Calculate cursor position with prompt "$ " (2 chars) = 20 total
    lle_cursor_position_t pos = lle_calculate_cursor_position(buffer, &geometry, 2);
    
    // Should be at start of second line due to wrapping
    LLE_ASSERT(pos.valid);
    LLE_ASSERT_EQ(pos.relative_row, 1);
    LLE_ASSERT_EQ(pos.relative_col, 0);
    LLE_ASSERT(pos.at_boundary);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Empty buffer cursor position
LLE_TEST(calculate_cursor_position_empty) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup terminal
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Empty buffer - cursor at position 0
    LLE_ASSERT_EQ(buffer->cursor_pos, 0);
    LLE_ASSERT_EQ(buffer->length, 0);
    
    // Calculate cursor position
    lle_cursor_position_t pos = lle_calculate_cursor_position(buffer, &geometry, 5);
    
    // Should be at prompt position
    LLE_ASSERT(pos.valid);
    LLE_ASSERT_EQ(pos.relative_row, 0);
    LLE_ASSERT_EQ(pos.relative_col, 5); // Just after prompt
    LLE_ASSERT_EQ(pos.absolute_row, 0);
    LLE_ASSERT_EQ(pos.absolute_col, 5);
    LLE_ASSERT(!pos.at_boundary);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Invalid input validation
LLE_TEST(calculate_cursor_position_invalid_input) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup geometry
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Test NULL buffer
    lle_cursor_position_t pos = lle_calculate_cursor_position(NULL, &geometry, 5);
    LLE_ASSERT(!pos.valid);
    
    // Test NULL geometry
    pos = lle_calculate_cursor_position(buffer, NULL, 5);
    LLE_ASSERT(!pos.valid);
    
    // Test prompt wider than terminal
    pos = lle_calculate_cursor_position(buffer, &geometry, 100);
    LLE_ASSERT(!pos.valid);
    
    // Test zero width terminal
    geometry.width = 0;
    pos = lle_calculate_cursor_position(buffer, &geometry, 5);
    LLE_ASSERT(!pos.valid);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Cursor position validation
LLE_TEST(validate_cursor_position_basic) {
    lle_cursor_position_t pos;
    
    // Test valid position
    pos.absolute_row = 5;
    pos.absolute_col = 10;
    pos.relative_row = 3;
    pos.relative_col = 8;
    pos.at_boundary = false;
    pos.valid = true;
    LLE_ASSERT(lle_validate_cursor_position(&pos));
    
    // Test NULL pointer
    LLE_ASSERT(!lle_validate_cursor_position(NULL));
    
    // Test invalid flag
    pos.valid = false;
    LLE_ASSERT(!lle_validate_cursor_position(&pos));
}

// Test: Cursor position validation with invalid coordinates
LLE_TEST(validate_cursor_position_invalid) {
    lle_cursor_position_t pos;
    
    // Setup base valid position
    pos.absolute_row = 5;
    pos.absolute_col = 10;
    pos.relative_row = 3;
    pos.relative_col = 8;
    pos.at_boundary = false;
    pos.valid = true;
    
    // Test coordinates too large
    pos.absolute_row = 20000;
    LLE_ASSERT(!lle_validate_cursor_position(&pos));
    
    pos.absolute_row = 5;
    pos.absolute_col = 20000;
    LLE_ASSERT(!lle_validate_cursor_position(&pos));
    
    pos.absolute_col = 10;
    pos.relative_row = 20000;
    LLE_ASSERT(!lle_validate_cursor_position(&pos));
    
    pos.relative_row = 3;
    pos.relative_col = 20000;
    LLE_ASSERT(!lle_validate_cursor_position(&pos));
    
    // Test invariant violation: absolute < relative
    pos.relative_col = 8;
    pos.absolute_row = 2; // Less than relative_row (3)
    LLE_ASSERT(!lle_validate_cursor_position(&pos));
}

// Test: Cursor position at specific offset
LLE_TEST(calculate_cursor_position_at_offset) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup terminal
    geometry.width = 20;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Insert text: "hello world"
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Test position at offset 5 (space character)
    lle_cursor_position_t pos = lle_calculate_cursor_position_at_offset(buffer, &geometry, 2, 5);
    
    // 2 (prompt) + 5 (text) = 7 chars, should be on first line
    LLE_ASSERT(pos.valid);
    LLE_ASSERT_EQ(pos.relative_row, 0);
    LLE_ASSERT_EQ(pos.relative_col, 7);
    
    // Test position at offset 9 (should still be on first line)
    pos = lle_calculate_cursor_position_at_offset(buffer, &geometry, 2, 9);
    
    // 2 (prompt) + 9 (text) = 11 chars, still on first line
    LLE_ASSERT(pos.valid);
    LLE_ASSERT_EQ(pos.relative_row, 0);
    LLE_ASSERT_EQ(pos.relative_col, 11);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Calculate text lines
LLE_TEST(calculate_text_lines) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup terminal
    geometry.width = 20;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    
    // Empty buffer should use 1 line
    size_t lines = lle_calculate_text_lines(buffer, &geometry, 2);
    LLE_ASSERT_EQ(lines, 1);
    
    // Add text that fits in one line: "hello" + prompt = 7 chars
    LLE_ASSERT(lle_text_insert_string(buffer, "hello"));
    lines = lle_calculate_text_lines(buffer, &geometry, 2);
    LLE_ASSERT_EQ(lines, 1);
    
    // Add more text to force second line: "hello world plus extra text" + prompt = 32 chars
    lle_text_buffer_clear(buffer);
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world plus extra text"));
    lines = lle_calculate_text_lines(buffer, &geometry, 2);
    LLE_ASSERT_EQ(lines, 2);
    
    lle_text_buffer_destroy(buffer);
}

// Test: Check line wrapping requirement
LLE_TEST(cursor_position_requires_wrap) {
    lle_cursor_position_t pos;
    lle_terminal_geometry_t geometry;
    
    geometry.width = 20;
    geometry.height = 24;
    
    // Initialize pos structure completely
    memset(&pos, 0, sizeof(pos));
    
    // Position at rightmost column - should require wrap
    pos.relative_row = 0;
    pos.relative_col = 19; // width - 1
    pos.valid = true;
    pos.at_boundary = false;
    LLE_ASSERT(lle_cursor_position_requires_wrap(&pos, &geometry));
    
    // Position not at rightmost column - should not require wrap
    pos.relative_col = 5;
    pos.at_boundary = false; // Explicitly reset boundary flag
    LLE_ASSERT(!lle_cursor_position_requires_wrap(&pos, &geometry));
    
    // Invalid position - should not require wrap
    pos.valid = false;
    LLE_ASSERT(!lle_cursor_position_requires_wrap(&pos, &geometry));
}


// =====================================
// LLE-008: Prompt Geometry Calculation Tests
// =====================================

// Test: Simple single-line prompt geometry
LLE_TEST(prompt_geometry_single_line) {
    lle_terminal_geometry_t terminal;
    terminal.width = 80;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test simple prompt "$ "
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry("$ ", &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 2);
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 2);
    
    // Test longer single-line prompt
    geom = lle_calculate_prompt_geometry("user@host:~/path$ ", &terminal);
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 18);
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 18);
}

// Test: Empty prompt geometry
LLE_TEST(prompt_geometry_empty) {
    lle_terminal_geometry_t terminal;
    terminal.width = 80;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test empty prompt
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry("", &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 0);
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 0);
}

// Test: Multiline prompt geometry
LLE_TEST(prompt_geometry_multiline) {
    lle_terminal_geometry_t terminal;
    terminal.width = 80;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test multiline prompt
    const char *multiline_prompt = "Line 1\nLine 2 is longer\nShort";
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry(multiline_prompt, &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 16); // "Line 2 is longer" is widest
    LLE_ASSERT_EQ(geom.height, 3); // Three lines
    LLE_ASSERT_EQ(geom.last_line_width, 5); // "Short" is 5 chars
}

// Test: Prompt with line wrapping
LLE_TEST(prompt_geometry_wrapping) {
    lle_terminal_geometry_t terminal;
    terminal.width = 20; // Narrow terminal
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test prompt that wraps
    const char *long_prompt = "This is a very long prompt";
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry(long_prompt, &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 26); // Full width before wrapping
    LLE_ASSERT_EQ(geom.height, 2); // Wraps to 2 lines (26 chars / 20 = 1.3 -> 2)
    LLE_ASSERT_EQ(geom.last_line_width, 6); // 26 % 20 = 6
}

// Test: Prompt with ANSI escape sequences
LLE_TEST(prompt_geometry_ansi) {
    lle_terminal_geometry_t terminal;
    terminal.width = 80;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test prompt with ANSI colors
    const char *ansi_prompt = "\033[32mgreen\033[0m$ ";
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry(ansi_prompt, &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 7); // "green$ " without ANSI codes
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 7);
}

// Test: ANSI display width calculation
LLE_TEST(ansi_display_width) {
    // Test simple text
    LLE_ASSERT_EQ(lle_calculate_display_width_ansi("hello", 5), 5);
    
    // Test text with ANSI color codes
    const char *ansi_text = "\033[32mgreen\033[0m";
    LLE_ASSERT_EQ(lle_calculate_display_width_ansi(ansi_text, strlen(ansi_text)), 5);
    
    // Test mixed ANSI and text
    const char *mixed_text = "before\033[31mred\033[0mafter";
    LLE_ASSERT_EQ(lle_calculate_display_width_ansi(mixed_text, strlen(mixed_text)), 14);
    
    // Test empty string
    LLE_ASSERT_EQ(lle_calculate_display_width_ansi("", 0), 0);
    
    // Test NULL pointer
    LLE_ASSERT_EQ(lle_calculate_display_width_ansi(NULL, 5), 0);
}

// Test: Prompt geometry validation
LLE_TEST(prompt_geometry_validation) {
    lle_prompt_geometry_t geom;
    
    // Test valid geometry
    geom.width = 10;
    geom.height = 2;
    geom.last_line_width = 5;
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    
    // Test NULL pointer
    LLE_ASSERT(!lle_validate_prompt_geometry(NULL));
    
    // Test zero height (invalid)
    geom.height = 0;
    LLE_ASSERT(!lle_validate_prompt_geometry(&geom));
    
    // Test last_line_width > width (invalid)
    geom.height = 1;
    geom.width = 5;
    geom.last_line_width = 10;
    LLE_ASSERT(!lle_validate_prompt_geometry(&geom));
    
    // Test excessive dimensions
    geom.width = 20000;
    geom.height = 1;
    geom.last_line_width = 5;
    LLE_ASSERT(!lle_validate_prompt_geometry(&geom));
}

// Test: Invalid input handling
LLE_TEST(prompt_geometry_invalid_input) {
    lle_terminal_geometry_t terminal;
    terminal.width = 80;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test NULL prompt
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry(NULL, &terminal);
    LLE_ASSERT_EQ(geom.width, 0);
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 0);
    
    // Test NULL terminal
    geom = lle_calculate_prompt_geometry("$ ", NULL);
    LLE_ASSERT_EQ(geom.width, 0);
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 0);
    
    // Test invalid terminal geometry
    terminal.width = 5; // Too small (below LLE_MIN_TERMINAL_WIDTH)
    geom = lle_calculate_prompt_geometry("$ ", &terminal);
    LLE_ASSERT_EQ(geom.width, 0);
    LLE_ASSERT_EQ(geom.height, 1);
    LLE_ASSERT_EQ(geom.last_line_width, 0);
}

// Test: Complex multiline prompt with wrapping
LLE_TEST(prompt_geometry_complex) {
    lle_terminal_geometry_t terminal;
    terminal.width = 25;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Complex multiline prompt with different line lengths
    const char *complex_prompt = "Short\nThis is a much longer line that will wrap\nMed\n$";
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry(complex_prompt, &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 41); // Second line is longest
    LLE_ASSERT_EQ(geom.last_line_width, 1); // "$" is last line
    // Height: 1 (Short) + 2 (wrapped long line) + 1 (Med) + 1 ($) = 5
    LLE_ASSERT_EQ(geom.height, 5);
}

// Test: Edge case with newlines
LLE_TEST(prompt_geometry_newlines) {
    lle_terminal_geometry_t terminal;
    terminal.width = 80;
    terminal.height = 24;
    terminal.prompt_width = 0;
    terminal.prompt_height = 1;
    
    // Test prompt with empty lines
    const char *newline_prompt = "Line1\n\nLine3\n";
    lle_prompt_geometry_t geom = lle_calculate_prompt_geometry(newline_prompt, &terminal);
    
    LLE_ASSERT(lle_validate_prompt_geometry(&geom));
    LLE_ASSERT_EQ(geom.width, 5); // "Line1" and "Line3" are both 5 chars
    LLE_ASSERT_EQ(geom.height, 4); // Line1, empty line, Line3, empty line after \n
    LLE_ASSERT_EQ(geom.last_line_width, 0); // Last line is empty
}

// Test: Calculate offset for position (reverse calculation)
LLE_TEST(calculate_offset_for_position) {
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    lle_terminal_geometry_t geometry;
    lle_cursor_position_t target_pos;
    
    LLE_ASSERT_NOT_NULL(buffer);
    
    // Setup terminal and buffer
    geometry.width = 20;
    geometry.height = 24;
    LLE_ASSERT(lle_text_insert_string(buffer, "hello world"));
    
    // Test position on first line
    target_pos.relative_row = 0;
    target_pos.relative_col = 7; // 2 (prompt) + 5 (text) = position at offset 5
    target_pos.valid = true;
    
    size_t offset = lle_calculate_offset_for_position(buffer, &geometry, 2, &target_pos);
    LLE_ASSERT_EQ(offset, 5);
    
    // Test position on second line
    target_pos.relative_row = 1;
    target_pos.relative_col = 1; // Should correspond to offset 19 (20 + 1 - 2)
    
    offset = lle_calculate_offset_for_position(buffer, &geometry, 2, &target_pos);
    LLE_ASSERT_EQ(offset, 11); // min(calculated, buffer->length)
    
    // Test position within prompt area
    target_pos.relative_row = 0;
    target_pos.relative_col = 1; // Within prompt area
    
    offset = lle_calculate_offset_for_position(buffer, &geometry, 2, &target_pos);
    LLE_ASSERT_EQ(offset, 0);
    
    lle_text_buffer_destroy(buffer);
}

// Main test runner
int main(void) {
    printf("Running LLE Cursor Math Tests\n");
    printf("=============================\n\n");
    
    RUN_TEST(cursor_position_structure);
    RUN_TEST(terminal_geometry_structure);
    RUN_TEST(terminal_geometry_validation_valid);
    RUN_TEST(terminal_geometry_validation_invalid);
    RUN_TEST(terminal_geometry_initialization);
    RUN_TEST(terminal_geometry_initialization_null);
    RUN_TEST(constants_defined);
    RUN_TEST(structure_properties);
    RUN_TEST(mathematical_invariants);
    
    // LLE-007: Cursor position calculation tests
    RUN_TEST(calculate_cursor_position_basic);
    RUN_TEST(calculate_cursor_position_wrapping);
    RUN_TEST(calculate_cursor_position_boundary);
    RUN_TEST(calculate_cursor_position_empty);
    RUN_TEST(calculate_cursor_position_invalid_input);
    RUN_TEST(validate_cursor_position_basic);
    RUN_TEST(validate_cursor_position_invalid);
    RUN_TEST(calculate_cursor_position_at_offset);
    RUN_TEST(calculate_text_lines);
    RUN_TEST(cursor_position_requires_wrap);
    RUN_TEST(calculate_offset_for_position);
    
    // LLE-008: Prompt geometry calculation tests
    RUN_TEST(prompt_geometry_single_line);
    RUN_TEST(prompt_geometry_empty);
    RUN_TEST(prompt_geometry_multiline);
    RUN_TEST(prompt_geometry_wrapping);
    RUN_TEST(prompt_geometry_ansi);
    RUN_TEST(ansi_display_width);
    RUN_TEST(prompt_geometry_validation);
    RUN_TEST(prompt_geometry_invalid_input);
    RUN_TEST(prompt_geometry_complex);
    RUN_TEST(prompt_geometry_newlines);
    
    printf("\n=============================\n");
    printf("Tests completed: %d/%d passed\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
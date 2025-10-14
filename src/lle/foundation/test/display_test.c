// src/lle/foundation/test/display_test.c
//
// Comprehensive tests for display system

#define _POSIX_C_SOURCE 200809L
#include "../display/display.h"
#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        tests_run++; \
        printf("Running test: %s...", #name); \
        fflush(stdout); \
        test_##name(); \
        tests_passed++; \
        printf(" PASS\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("\n  FAIL: %s\n", message); \
            tests_failed++; \
            return; \
        } \
    } while (0)

// Test 1: Basic initialization and cleanup
TEST(init_cleanup) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    memset(&display, 0, sizeof(display));
    
    // Initialize terminal first
    int result = lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    ASSERT(result == LLE_TERM_OK, "Terminal init should succeed");
    
    // Initialize display
    result = lle_display_init(&display, &term, 24, 80);
    ASSERT(result == LLE_DISPLAY_OK, "Display init should succeed");
    ASSERT(display.initialized, "Display should be initialized");
    ASSERT(display.buffer.rows == 24, "Rows should be 24");
    ASSERT(display.buffer.cols == 80, "Cols should be 80");
    ASSERT(display.buffer.cells != NULL, "Cells should be allocated");
    
    // Cleanup
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
    
    ASSERT(!display.initialized, "Display should not be initialized after cleanup");
}

// Test 2: Null pointer handling
TEST(null_pointer_handling) {
    int result = lle_display_init(NULL, NULL, 24, 80);
    ASSERT(result == LLE_DISPLAY_ERR_NULL_PTR, "Should reject null display");
    
    lle_display_t display;
    memset(&display, 0, sizeof(display));
    
    result = lle_display_render_line(&display, 0, "test", 4);
    ASSERT(result == LLE_DISPLAY_ERR_NOT_INIT, "Should reject uninitialized display");
}

// Test 3: Invalid dimensions
TEST(invalid_dimensions) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    int result = lle_display_init(&display, &term, 0, 80);
    ASSERT(result == LLE_DISPLAY_ERR_INVALID_DIMS, "Should reject 0 rows");
    
    result = lle_display_init(&display, &term, 24, 0);
    ASSERT(result == LLE_DISPLAY_ERR_INVALID_DIMS, "Should reject 0 cols");
    
    lle_term_cleanup(&term);
}

// Test 4: Single line rendering
TEST(single_line_rendering) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 80);
    
    const char *text = "Hello, World!";
    int result = lle_display_render_line(&display, 0, text, strlen(text));
    ASSERT(result == LLE_DISPLAY_OK, "Render should succeed");
    
    // Verify cells
    for (size_t i = 0; i < strlen(text); i++) {
        const lle_display_cell_t *cell = lle_display_get_cell(&display, 0, i);
        ASSERT(cell != NULL, "Cell should exist");
        ASSERT(cell->codepoint == (uint32_t)text[i], "Cell should match text");
    }
    
    // Verify dirty flag
    ASSERT(display.buffer.dirty_lines[0], "Line 0 should be dirty");
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

// Test 5: Multi-line rendering with wrapping
TEST(multiline_rendering) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 40);  // Smaller width for wrapping
    
    // Text longer than one line
    const char *text = "This is a long line that will wrap to multiple lines on the display";
    size_t cursor_offset = 10;
    
    int result = lle_display_render_multiline(&display, 0, text, strlen(text), cursor_offset);
    ASSERT(result == LLE_DISPLAY_OK, "Multiline render should succeed");
    
    // Verify first cell
    const lle_display_cell_t *cell = lle_display_get_cell(&display, 0, 0);
    ASSERT(cell != NULL, "First cell should exist");
    ASSERT(cell->codepoint == (uint32_t)text[0], "First cell should match text");
    
    // Verify wrapping occurred (should mark multiple lines dirty)
    ASSERT(display.buffer.dirty_lines[0], "Line 0 should be dirty");
    ASSERT(display.buffer.dirty_lines[1], "Line 1 should be dirty (wrapped)");
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

// Test 6: Syntax highlighting
TEST(syntax_highlighting) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 80);
    
    const char *text = "function test()";
    uint8_t attrs[16];
    memset(attrs, 0, sizeof(attrs));
    
    // Bold for "function" keyword
    for (int i = 0; i < 8; i++) {
        attrs[i] = LLE_DISPLAY_ATTR_BOLD;
    }
    
    int result = lle_display_render_highlighted(&display, 0, text, attrs, strlen(text));
    ASSERT(result == LLE_DISPLAY_OK, "Highlighted render should succeed");
    
    // Verify attributes
    const lle_display_cell_t *cell = lle_display_get_cell(&display, 0, 0);
    ASSERT(cell != NULL, "Cell should exist");
    ASSERT(cell->attrs & LLE_DISPLAY_ATTR_BOLD, "First char should be bold");
    
    cell = lle_display_get_cell(&display, 0, 9);
    ASSERT(cell != NULL, "Cell should exist");
    ASSERT(!(cell->attrs & LLE_DISPLAY_ATTR_BOLD), "Space should not be bold");
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

// Test 7: Cursor management
TEST(cursor_management) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 80);
    
    // Initial cursor
    uint16_t row, col;
    bool visible;
    lle_display_get_cursor(&display, &row, &col, &visible);
    ASSERT(row == 0 && col == 0, "Initial cursor should be at 0,0");
    ASSERT(visible, "Cursor should be visible initially");
    
    // Set cursor
    int result = lle_display_set_cursor(&display, 5, 10);
    ASSERT(result == LLE_DISPLAY_OK, "Set cursor should succeed");
    
    lle_display_get_cursor(&display, &row, &col, &visible);
    ASSERT(row == 5 && col == 10, "Cursor should be at 5,10");
    
    // Hide cursor
    result = lle_display_show_cursor(&display, false);
    ASSERT(result == LLE_DISPLAY_OK, "Hide cursor should succeed");
    
    lle_display_get_cursor(&display, &row, &col, &visible);
    ASSERT(!visible, "Cursor should be hidden");
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

// Test 8: Clear operations
TEST(clear_operations) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 80);
    
    // Render some content
    lle_display_render_line(&display, 0, "Line 1", 6);
    lle_display_render_line(&display, 1, "Line 2", 6);
    
    // Clear entire display
    int result = lle_display_clear(&display);
    ASSERT(result == LLE_DISPLAY_OK, "Clear should succeed");
    
    // Verify cleared
    const lle_display_cell_t *cell = lle_display_get_cell(&display, 0, 0);
    ASSERT(cell->codepoint == 0, "Cell should be cleared");
    
    // Clear region
    lle_display_render_line(&display, 0, "Test line", 9);
    
    lle_display_region_t region = {0, 0, 0, 4};
    result = lle_display_clear_region(&display, &region);
    ASSERT(result == LLE_DISPLAY_OK, "Clear region should succeed");
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

// Test 9: Resize handling
TEST(resize_handling) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 80);
    
    // Render content
    lle_display_render_line(&display, 0, "Test", 4);
    
    // Resize
    int result = lle_display_resize(&display, 30, 100);
    ASSERT(result == LLE_DISPLAY_OK, "Resize should succeed");
    ASSERT(display.buffer.rows == 30, "Rows should be 30");
    ASSERT(display.buffer.cols == 100, "Cols should be 100");
    
    // Cursor should be adjusted if out of bounds
    lle_display_set_cursor(&display, 29, 50);
    result = lle_display_resize(&display, 20, 60);
    ASSERT(result == LLE_DISPLAY_OK, "Resize should succeed");
    
    uint16_t row, col;
    lle_display_get_cursor(&display, &row, &col, NULL);
    ASSERT(row < 20, "Cursor row should be adjusted");
    ASSERT(col < 60, "Cursor col should be adjusted");
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

// Test 10: Performance metrics
TEST(performance_metrics) {
    lle_term_t term;
    lle_display_t display;
    
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, &term, 24, 80);
    
    // Perform multiple renders
    for (int i = 0; i < 100; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Line %d", i);
        lle_display_render_line(&display, i % 24, buf, strlen(buf));
    }
    
    // Get metrics
    uint64_t render_count;
    double avg_time_ms;
    lle_display_get_metrics(&display, &render_count, &avg_time_ms);
    
    ASSERT(render_count == 100, "Should have 100 renders");
    ASSERT(avg_time_ms >= 0.0, "Average time should be non-negative");
    
    printf("\n  Performance: %lu renders, avg %.3f ms", 
           (unsigned long)render_count, avg_time_ms);
    
    lle_display_cleanup(&display);
    lle_term_cleanup(&term);
}

int main(void) {
    printf("LLE Display System Tests\n");
    printf("========================\n\n");
    
    // Check if running in a terminal
    if (!isatty(STDIN_FILENO)) {
        printf("ERROR: Not running in a terminal (stdin not a tty)\n");
        printf("Please run this test in an interactive terminal.\n");
        return 1;
    }
    
    // Run all tests
    run_test_init_cleanup();
    run_test_null_pointer_handling();
    run_test_invalid_dimensions();
    run_test_single_line_rendering();
    run_test_multiline_rendering();
    run_test_syntax_highlighting();
    run_test_cursor_management();
    run_test_clear_operations();
    run_test_resize_handling();
    run_test_performance_metrics();
    
    // Summary
    printf("\n========================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\nFAILURE: %d test(s) failed\n", tests_failed);
        return 1;
    }
    
    printf("\nSUCCESS: All tests passed\n");
    return 0;
}

// src/lle/foundation/test/display_scroll_test.c
//
// Scroll Region Test Suite
//
// Tests scroll region management functionality in the display system.

#include "../display/display.h"
#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Test result codes
#define TEST_PASS 0
#define TEST_FAIL 1

// Test macros
#define ASSERT_TRUE(msg, cond) \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s\n", msg); \
        return TEST_FAIL; \
    }

#define ASSERT_EQ(msg, val, expected) \
    if ((val) != (expected)) { \
        fprintf(stderr, "FAIL: %s (expected %d, got %d)\n", msg, (int)(expected), (int)(val)); \
        return TEST_FAIL; \
    }

// Test set scroll region
static int test_set_scroll_region(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    int result = lle_display_init(&display, term, 24, 80);
    ASSERT_EQ("Display init", result, LLE_DISPLAY_OK);
    
    // Set scroll region
    result = lle_display_set_scroll_region(&display, 5, 20);
    ASSERT_EQ("Set scroll region", result, LLE_DISPLAY_OK);
    
    // Verify region was set
    const lle_scroll_region_t *region = lle_display_get_scroll_region(&display);
    ASSERT_TRUE("Region enabled", region->enabled);
    ASSERT_EQ("Top row", region->top_row, 5);
    ASSERT_EQ("Bottom row", region->bottom_row, 20);
    ASSERT_EQ("Scroll offset", region->scroll_offset, 0);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test invalid scroll region
static int test_invalid_scroll_region(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Top > bottom
    int result = lle_display_set_scroll_region(&display, 20, 5);
    ASSERT_EQ("Top > bottom rejected", result, LLE_DISPLAY_ERR_INVALID_REGION);
    
    // Out of bounds
    result = lle_display_set_scroll_region(&display, 0, 30);
    ASSERT_EQ("Out of bounds rejected", result, LLE_DISPLAY_ERR_INVALID_REGION);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test clear scroll region
static int test_clear_scroll_region(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Set then clear
    lle_display_set_scroll_region(&display, 5, 20);
    int result = lle_display_clear_scroll_region(&display);
    ASSERT_EQ("Clear scroll region", result, LLE_DISPLAY_OK);
    
    // Verify disabled
    const lle_scroll_region_t *region = lle_display_get_scroll_region(&display);
    ASSERT_TRUE("Region disabled", !region->enabled);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll up basic
static int test_scroll_up_basic(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Set scroll region (rows 5-10)
    lle_display_set_scroll_region(&display, 5, 10);
    
    // Fill region with test pattern
    for (uint16_t row = 5; row <= 10; row++) {
        char line[80];
        snprintf(line, sizeof(line), "Line %d", row);
        lle_display_render_line(&display, row, line, strlen(line));
    }
    
    // Get initial content of row 6
    const lle_display_cell_t *cell_before = lle_display_get_cell(&display, 6, 0);
    uint32_t char_row6_before = cell_before->codepoint;
    
    // Scroll up 1 line
    int result = lle_display_scroll_up(&display, 1);
    ASSERT_EQ("Scroll up", result, LLE_DISPLAY_OK);
    
    // Row 5 should now have content from row 6
    const lle_display_cell_t *cell_after = lle_display_get_cell(&display, 5, 0);
    ASSERT_EQ("Content shifted up", cell_after->codepoint, char_row6_before);
    
    // Row 10 (bottom) should be cleared
    const lle_display_cell_t *bottom_cell = lle_display_get_cell(&display, 10, 0);
    ASSERT_EQ("Bottom cleared", bottom_cell->codepoint, 0);
    
    // Verify scroll offset
    const lle_scroll_region_t *region = lle_display_get_scroll_region(&display);
    ASSERT_EQ("Scroll offset", region->scroll_offset, 1);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll down basic
static int test_scroll_down_basic(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Set scroll region (rows 5-10)
    lle_display_set_scroll_region(&display, 5, 10);
    
    // Fill region with test pattern
    for (uint16_t row = 5; row <= 10; row++) {
        char line[80];
        snprintf(line, sizeof(line), "Line %d", row);
        lle_display_render_line(&display, row, line, strlen(line));
    }
    
    // Get initial content of row 6
    const lle_display_cell_t *cell_before = lle_display_get_cell(&display, 6, 0);
    uint32_t char_row6_before = cell_before->codepoint;
    
    // Scroll down 1 line
    int result = lle_display_scroll_down(&display, 1);
    ASSERT_EQ("Scroll down", result, LLE_DISPLAY_OK);
    
    // Row 7 should now have content from row 6
    const lle_display_cell_t *cell_after = lle_display_get_cell(&display, 7, 0);
    ASSERT_EQ("Content shifted down", cell_after->codepoint, char_row6_before);
    
    // Row 5 (top) should be cleared
    const lle_display_cell_t *top_cell = lle_display_get_cell(&display, 5, 0);
    ASSERT_EQ("Top cleared", top_cell->codepoint, 0);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll up multiple lines
static int test_scroll_up_multiple(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Set scroll region
    lle_display_set_scroll_region(&display, 0, 9);
    
    // Fill with numbers
    for (uint16_t row = 0; row < 10; row++) {
        char line[80];
        snprintf(line, sizeof(line), "%d", row);
        lle_display_render_line(&display, row, line, strlen(line));
    }
    
    // Scroll up 3 lines
    lle_display_scroll_up(&display, 3);
    
    // Row 0 should have '3' (from row 3)
    const lle_display_cell_t *cell = lle_display_get_cell(&display, 0, 0);
    ASSERT_EQ("Row 0 has content from row 3", cell->codepoint, '3');
    
    // Row 6 should have '9' (from row 9)
    cell = lle_display_get_cell(&display, 6, 0);
    ASSERT_EQ("Row 6 has content from row 9", cell->codepoint, '9');
    
    // Rows 7-9 should be cleared
    cell = lle_display_get_cell(&display, 7, 0);
    ASSERT_EQ("Row 7 cleared", cell->codepoint, 0);
    cell = lle_display_get_cell(&display, 8, 0);
    ASSERT_EQ("Row 8 cleared", cell->codepoint, 0);
    cell = lle_display_get_cell(&display, 9, 0);
    ASSERT_EQ("Row 9 cleared", cell->codepoint, 0);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll down multiple lines
static int test_scroll_down_multiple(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Set scroll region
    lle_display_set_scroll_region(&display, 0, 9);
    
    // Fill with numbers
    for (uint16_t row = 0; row < 10; row++) {
        char line[80];
        snprintf(line, sizeof(line), "%d", row);
        lle_display_render_line(&display, row, line, strlen(line));
    }
    
    // Scroll down 3 lines
    lle_display_scroll_down(&display, 3);
    
    // Row 3 should have '0' (from row 0)
    const lle_display_cell_t *cell = lle_display_get_cell(&display, 3, 0);
    ASSERT_EQ("Row 3 has content from row 0", cell->codepoint, '0');
    
    // Row 9 should have '6' (from row 6)
    cell = lle_display_get_cell(&display, 9, 0);
    ASSERT_EQ("Row 9 has content from row 6", cell->codepoint, '6');
    
    // Rows 0-2 should be cleared
    cell = lle_display_get_cell(&display, 0, 0);
    ASSERT_EQ("Row 0 cleared", cell->codepoint, 0);
    cell = lle_display_get_cell(&display, 1, 0);
    ASSERT_EQ("Row 1 cleared", cell->codepoint, 0);
    cell = lle_display_get_cell(&display, 2, 0);
    ASSERT_EQ("Row 2 cleared", cell->codepoint, 0);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll without region enabled
static int test_scroll_no_region(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Render content
    lle_display_render_line(&display, 0, "Test", 4);
    
    // Get content before scroll
    const lle_display_cell_t *cell_before = lle_display_get_cell(&display, 0, 0);
    uint32_t char_before = cell_before->codepoint;
    
    // Scroll without setting region (should be no-op)
    int result = lle_display_scroll_up(&display, 1);
    ASSERT_EQ("Scroll without region", result, LLE_DISPLAY_OK);
    
    // Content should be unchanged
    const lle_display_cell_t *cell_after = lle_display_get_cell(&display, 0, 0);
    ASSERT_EQ("Content unchanged", cell_after->codepoint, char_before);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll region isolation
static int test_scroll_region_isolation(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    // Set scroll region (rows 5-10)
    lle_display_set_scroll_region(&display, 5, 10);
    
    // Fill entire display
    for (uint16_t row = 0; row < 24; row++) {
        char line[80];
        snprintf(line, sizeof(line), "Row %d", row);
        lle_display_render_line(&display, row, line, strlen(line));
    }
    
    // Get content outside scroll region
    const lle_display_cell_t *row4 = lle_display_get_cell(&display, 4, 0);
    const lle_display_cell_t *row11 = lle_display_get_cell(&display, 11, 0);
    uint32_t row4_char = row4->codepoint;
    uint32_t row11_char = row11->codepoint;
    
    // Scroll within region
    lle_display_scroll_up(&display, 2);
    
    // Content outside region should be unchanged
    const lle_display_cell_t *row4_after = lle_display_get_cell(&display, 4, 0);
    const lle_display_cell_t *row11_after = lle_display_get_cell(&display, 11, 0);
    ASSERT_EQ("Row 4 unchanged", row4_after->codepoint, row4_char);
    ASSERT_EQ("Row 11 unchanged", row11_after->codepoint, row11_char);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test scroll offset tracking
static int test_scroll_offset_tracking(void) {
    lle_terminal_abstraction_t *term = NULL;
    lle_display_t display;
    
    lle_terminal_abstraction_init(&term, NULL, STDIN_FILENO, STDOUT_FILENO);
    lle_display_init(&display, term, 24, 80);
    
    lle_display_set_scroll_region(&display, 0, 9);
    
    // Scroll up multiple times
    lle_display_scroll_up(&display, 3);
    const lle_scroll_region_t *region = lle_display_get_scroll_region(&display);
    ASSERT_EQ("Offset after scroll up 3", region->scroll_offset, 3);
    
    lle_display_scroll_up(&display, 2);
    region = lle_display_get_scroll_region(&display);
    ASSERT_EQ("Offset after scroll up 2 more", region->scroll_offset, 5);
    
    // Scroll down
    lle_display_scroll_down(&display, 2);
    region = lle_display_get_scroll_region(&display);
    ASSERT_EQ("Offset after scroll down 2", region->scroll_offset, 3);
    
    lle_display_cleanup(&display);
    lle_terminal_abstraction_cleanup(term);
    return TEST_PASS;
}

// Test structure
typedef struct {
    const char *name;
    int (*func)(void);
} test_case_t;

// Test suite
static test_case_t tests[] = {
    {"Set scroll region", test_set_scroll_region},
    {"Invalid scroll region", test_invalid_scroll_region},
    {"Clear scroll region", test_clear_scroll_region},
    {"Scroll up basic", test_scroll_up_basic},
    {"Scroll down basic", test_scroll_down_basic},
    {"Scroll up multiple lines", test_scroll_up_multiple},
    {"Scroll down multiple lines", test_scroll_down_multiple},
    {"Scroll without region enabled", test_scroll_no_region},
    {"Scroll region isolation", test_scroll_region_isolation},
    {"Scroll offset tracking", test_scroll_offset_tracking},
};

int main(void) {
    int total = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;
    
    printf("Running Display Scroll Region Test Suite (%d tests)\n", total);
    printf("================================================\n\n");
    
    for (int i = 0; i < total; i++) {
        printf("Test %d: %s... ", i + 1, tests[i].name);
        fflush(stdout);
        
        int result = tests[i].func();
        if (result == TEST_PASS) {
            printf("PASS\n");
            passed++;
        } else {
            printf("FAIL\n");
        }
    }
    
    printf("\n================================================\n");
    printf("Results: %d/%d tests passed\n", passed, total);
    
    return (passed == total) ? 0 : 1;
}

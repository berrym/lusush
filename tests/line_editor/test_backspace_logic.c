/**
 * @file test_backspace_logic.c
 * @brief Focused tests for backspace enhancement core logic
 *
 * Tests the core logic of the backspace enhancement without requiring
 * terminal operations. Focuses on visual footprint calculation and
 * boundary detection logic.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "../../src/line_editor/display.h"

/**
 * Test framework macros
 */
#define LLE_TEST(name) static bool test_##name(void)
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("❌ Test failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("❌ Test failed at %s:%d: expected %zu, got %zu\n", __FILE__, __LINE__, (size_t)(expected), (size_t)(actual)); \
            return false; \
        } \
    } while(0)

/**
 * Test visual footprint calculation for empty content
 */
LLE_TEST(visual_footprint_empty) {
    printf("🧪 Testing visual footprint calculation for empty content...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test empty content with different prompt widths
    TEST_ASSERT(lle_calculate_visual_footprint("", 0, 0, 80, &footprint));
    TEST_ASSERT_EQ(1, footprint.rows_used);
    TEST_ASSERT_EQ(0, footprint.end_column);
    TEST_ASSERT(!footprint.wraps_lines);
    TEST_ASSERT_EQ(0, footprint.total_visual_width);
    
    TEST_ASSERT(lle_calculate_visual_footprint("", 0, 10, 80, &footprint));
    TEST_ASSERT_EQ(1, footprint.rows_used);
    TEST_ASSERT_EQ(10, footprint.end_column);
    TEST_ASSERT(!footprint.wraps_lines);
    TEST_ASSERT_EQ(10, footprint.total_visual_width);
    
    printf("✅ Empty content footprint test PASSED\n");
    return true;
}

/**
 * Test visual footprint calculation for single line content
 */
LLE_TEST(visual_footprint_single_line) {
    printf("🧪 Testing visual footprint calculation for single line...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test short content that fits on one line
    TEST_ASSERT(lle_calculate_visual_footprint("hello", 5, 10, 80, &footprint));
    TEST_ASSERT_EQ(1, footprint.rows_used);
    TEST_ASSERT_EQ(15, footprint.end_column); // prompt_width + text_length
    TEST_ASSERT(!footprint.wraps_lines);
    TEST_ASSERT_EQ(15, footprint.total_visual_width);
    
    // Test content at exact terminal width boundary
    char boundary_text[71]; // 80 - 10 + 1 for exact fit
    memset(boundary_text, 'a', 70);
    boundary_text[70] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(boundary_text, 70, 10, 80, &footprint));
    TEST_ASSERT_EQ(1, footprint.rows_used);
    TEST_ASSERT_EQ(80, footprint.end_column);
    TEST_ASSERT(!footprint.wraps_lines);
    TEST_ASSERT_EQ(80, footprint.total_visual_width);
    
    printf("✅ Single line footprint test PASSED\n");
    return true;
}

/**
 * Test visual footprint calculation for multi-line wrapping
 */
LLE_TEST(visual_footprint_wrapping) {
    printf("🧪 Testing visual footprint calculation for wrapping...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test content that wraps by one character
    char wrap_text[72]; // Exceeds terminal width by 1 char
    memset(wrap_text, 'a', 71);
    wrap_text[71] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(wrap_text, 71, 10, 80, &footprint));
    TEST_ASSERT_EQ(2, footprint.rows_used); // Should wrap to 2 lines
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT_EQ(81, footprint.total_visual_width); // prompt + text
    TEST_ASSERT_EQ(1, footprint.end_column); // 1 char on second line
    
    // Test content that spans exactly 2 lines
    char two_line_text[151]; // Should fill first line + 70 chars on second
    memset(two_line_text, 'b', 150);
    two_line_text[150] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(two_line_text, 150, 10, 80, &footprint));
    TEST_ASSERT_EQ(2, footprint.rows_used);
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT_EQ(160, footprint.total_visual_width); // prompt + text
    TEST_ASSERT_EQ(80, footprint.end_column); // 80 chars on second line (exactly fills row)
    
    printf("✅ Wrapping footprint test PASSED\n");
    return true;
}

/**
 * Test boundary crossing detection logic
 */
LLE_TEST(boundary_crossing_detection) {
    printf("🧪 Testing boundary crossing detection...\n");
    
    lle_visual_footprint_t before, after;
    
    // Test case 1: No boundary crossing (both single line)
    TEST_ASSERT(lle_calculate_visual_footprint("abc", 3, 10, 80, &before));
    TEST_ASSERT(lle_calculate_visual_footprint("ab", 2, 10, 80, &after));
    
    bool crossing1 = (before.rows_used != after.rows_used) ||
                     (before.wraps_lines && !after.wraps_lines);
    TEST_ASSERT(!crossing1); // Should not be crossing boundary
    
    // Test case 2: Boundary crossing (from wrapped to single line)
    TEST_ASSERT(lle_calculate_visual_footprint("a", 1, 79, 80, &before));
    TEST_ASSERT(lle_calculate_visual_footprint("ab", 2, 79, 80, &after));
    
    bool crossing2 = (before.rows_used != after.rows_used) ||
                     (before.wraps_lines && !after.wraps_lines);
    TEST_ASSERT(crossing2); // Should be crossing boundary
    
    // Test case 3: Multiple line reduction
    char long_text[200];
    memset(long_text, 'x', 199);
    long_text[199] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(long_text, 199, 10, 80, &before));
    TEST_ASSERT(lle_calculate_visual_footprint("short", 5, 10, 80, &after));
    
    bool crossing3 = (before.rows_used != after.rows_used) ||
                     (before.wraps_lines && !after.wraps_lines);
    TEST_ASSERT(crossing3); // Should be crossing boundary
    
    printf("✅ Boundary crossing detection test PASSED\n");
    return true;
}

/**
 * Test edge cases and error conditions
 */
LLE_TEST(edge_cases) {
    printf("🧪 Testing edge cases and error conditions...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test with NULL parameters
    TEST_ASSERT(!lle_calculate_visual_footprint(NULL, 4, 10, 80, &footprint));
    TEST_ASSERT(!lle_calculate_visual_footprint("test", 4, 10, 80, NULL));
    
    // Test with zero terminal width
    TEST_ASSERT(!lle_calculate_visual_footprint("test", 4, 10, 0, &footprint));
    
    // Test with very large prompt width (100 chars, terminal 80)
    // Prompt: 100 chars = 2 rows (80 + 20), text: 4 chars fits on row 2
    // Row 1: 80 chars prompt, Row 2: 20 chars prompt + 4 chars text = 24 end column
    TEST_ASSERT(lle_calculate_visual_footprint("test", 4, 100, 80, &footprint));
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT_EQ(104, footprint.total_visual_width);

    TEST_ASSERT_EQ(2, footprint.rows_used);
    TEST_ASSERT_EQ(24, footprint.end_column);
    
    // Test with terminal width of 1
    TEST_ASSERT(lle_calculate_visual_footprint("a", 1, 0, 1, &footprint));
    TEST_ASSERT_EQ(1, footprint.rows_used);
    TEST_ASSERT_EQ(1, footprint.end_column);
    TEST_ASSERT(!footprint.wraps_lines);
    
    // Test with content longer than terminal width of 1
    TEST_ASSERT(lle_calculate_visual_footprint("ab", 2, 0, 1, &footprint));
    TEST_ASSERT_EQ(2, footprint.rows_used);
    TEST_ASSERT(footprint.wraps_lines);
    
    printf("✅ Edge cases test PASSED\n");
    return true;
}

/**
 * Test the mathematical accuracy of footprint calculations
 */
LLE_TEST(mathematical_accuracy) {
    printf("🧪 Testing mathematical accuracy of calculations...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test precise boundary conditions
    for (size_t prompt_width = 0; prompt_width < 20; prompt_width++) {
        for (size_t text_length = 0; text_length < 200; text_length++) {
            char text[201];
            memset(text, 'x', text_length);
            text[text_length] = '\0';
            
            TEST_ASSERT(lle_calculate_visual_footprint(text, text_length, prompt_width, 80, &footprint));
            
            // Verify mathematical consistency
            size_t expected_total_width = prompt_width + text_length;
            TEST_ASSERT_EQ(expected_total_width, footprint.total_visual_width);
            
            // Verify wrapping logic
            bool should_wrap = expected_total_width > 80;
            TEST_ASSERT_EQ(should_wrap, footprint.wraps_lines);
            
            // Verify row count
            if (!should_wrap) {
                TEST_ASSERT_EQ(1, footprint.rows_used);
                TEST_ASSERT_EQ(expected_total_width, footprint.end_column);
            } else {

                TEST_ASSERT(footprint.rows_used > 1);
                // Complex wrapping math verification would go here
            }
        }
    }
    
    printf("✅ Mathematical accuracy test PASSED\n");
    return true;
}

/**
 * Test performance with large content
 */
LLE_TEST(performance_large_content) {
    printf("🧪 Testing performance with large content...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test with very large content
    char large_text[5000];
    memset(large_text, 'a', 4999);
    large_text[4999] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(large_text, 4999, 10, 80, &footprint));
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT(footprint.rows_used > 1);
    TEST_ASSERT_EQ(5009, footprint.total_visual_width); // prompt + text
    
    // Verify it handles large content correctly
    size_t expected_rows = (5009 + 79) / 80; // Ceiling division
    TEST_ASSERT_EQ(expected_rows, footprint.rows_used);
    
    printf("✅ Performance large content test PASSED\n");
    return true;
}

/**
 * Test consistency across different terminal widths
 */
LLE_TEST(terminal_width_consistency) {
    printf("🧪 Testing consistency across different terminal widths...\n");
    
    lle_visual_footprint_t footprint;
    const char *test_text = "This is a test line of moderate length";
    size_t text_len = strlen(test_text);
    
    // Test with various terminal widths
    size_t terminal_widths[] = {20, 40, 60, 80, 100, 120};
    size_t num_widths = sizeof(terminal_widths) / sizeof(terminal_widths[0]);
    
    for (size_t i = 0; i < num_widths; i++) {
        size_t width = terminal_widths[i];
        TEST_ASSERT(lle_calculate_visual_footprint(test_text, text_len, 10, width, &footprint));
        
        // Verify total width is always consistent
        TEST_ASSERT_EQ(text_len + 10, footprint.total_visual_width);
        
        // Verify wrapping behavior is correct
        bool should_wrap = (text_len + 10) > width;
        TEST_ASSERT_EQ(should_wrap, footprint.wraps_lines);
        
        // Verify row calculation
        if (should_wrap) {
            TEST_ASSERT(footprint.rows_used > 1);
        } else {
            TEST_ASSERT_EQ(1, footprint.rows_used);
        }
    }
    
    printf("✅ Terminal width consistency test PASSED\n");
    return true;
}

/**
 * Main test function
 */
int main(void) {
    printf("🚀 Starting backspace logic core tests...\n\n");
    
    // Run all tests
    bool all_passed = true;
    
    all_passed &= test_visual_footprint_empty();
    all_passed &= test_visual_footprint_single_line();
    all_passed &= test_visual_footprint_wrapping();
    all_passed &= test_boundary_crossing_detection();
    all_passed &= test_edge_cases();
    all_passed &= test_mathematical_accuracy();
    all_passed &= test_performance_large_content();
    all_passed &= test_terminal_width_consistency();
    
    printf("\n");
    
    if (all_passed) {
        printf("🎉 All backspace logic core tests PASSED!\n");
        printf("✅ Enhanced visual footprint calculation is working correctly\n");
        printf("✅ Boundary crossing detection logic is accurate\n");
        printf("✅ Mathematical calculations are consistent\n");
        return 0;
    } else {
        printf("❌ Some backspace logic core tests FAILED!\n");
        return 1;
    }
}
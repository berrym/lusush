/**
 * @file test_backspace_enhancement.c
 * @brief Comprehensive tests for backspace enhancement functionality
 *
 * Tests the enhanced backspace refinement implementation including:
 * - Visual footprint calculation
 * - Intelligent clearing strategies
 * - Consistent rendering behavior
 * - Smart boundary detection
 * - Cross-line backspace handling
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
#include "../../src/line_editor/text_buffer.h"
#include "../../src/line_editor/terminal_manager.h"
#include "../../src/line_editor/prompt.h"

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

#define TEST_ASSERT_STR_EQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("❌ Test failed at %s:%d: expected '%s', got '%s'\n", __FILE__, __LINE__, (expected), (actual)); \
            return false; \
        } \
    } while(0)

/**
 * Test visual footprint calculation for single line content
 */
LLE_TEST(visual_footprint_single_line) {
    printf("🧪 Testing visual footprint calculation for single line...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test empty content
    TEST_ASSERT(lle_calculate_visual_footprint("", 0, 10, 80, &footprint));
    TEST_ASSERT_EQ(1, footprint.rows_used);
    TEST_ASSERT_EQ(10, footprint.end_column);
    TEST_ASSERT(!footprint.wraps_lines);
    TEST_ASSERT_EQ(10, footprint.total_visual_width);
    
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
 * Test visual footprint calculation for multi-line content
 */
LLE_TEST(visual_footprint_multi_line) {
    printf("🧪 Testing visual footprint calculation for multi-line...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test content that wraps to second line
    char wrap_text[81]; // Exceeds terminal width
    memset(wrap_text, 'a', 80);
    wrap_text[80] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(wrap_text, 80, 10, 80, &footprint));
    TEST_ASSERT_EQ(2, footprint.rows_used); // Should wrap to 2 lines
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT_EQ(90, footprint.total_visual_width); // prompt + text
    
    // Test content that wraps to multiple lines
    char long_text[161]; // Should span 3 lines with prompt
    memset(long_text, 'b', 160);
    long_text[160] = '\0';
    
    TEST_ASSERT(lle_calculate_visual_footprint(long_text, 160, 10, 80, &footprint));
    TEST_ASSERT_EQ(3, footprint.rows_used); // Should wrap to 3 lines
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT_EQ(170, footprint.total_visual_width); // prompt + text
    
    printf("✅ Multi-line footprint test PASSED\n");
    return true;
}

/**
 * Test visual footprint calculation edge cases
 */
LLE_TEST(visual_footprint_edge_cases) {
    printf("🧪 Testing visual footprint edge cases...\n");
    
    lle_visual_footprint_t footprint;
    
    // Test with zero terminal width (should not crash)
    TEST_ASSERT(!lle_calculate_visual_footprint("test", 4, 0, 0, &footprint));
    
    // Test with NULL parameters
    TEST_ASSERT(!lle_calculate_visual_footprint(NULL, 4, 10, 80, &footprint));
    TEST_ASSERT(!lle_calculate_visual_footprint("test", 4, 10, 80, NULL));
    
    // Test with very large prompt width
    TEST_ASSERT(lle_calculate_visual_footprint("test", 4, 100, 80, &footprint));
    TEST_ASSERT(footprint.wraps_lines);
    TEST_ASSERT_EQ(104, footprint.total_visual_width);
    
    printf("✅ Edge cases footprint test PASSED\n");
    return true;
}

/**
 * Test enhanced backspace logic in incremental update
 */
LLE_TEST(enhanced_backspace_logic) {
    printf("🧪 Testing enhanced backspace logic...\n");
    
    // Create test components
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    TEST_ASSERT(buffer != NULL);
    
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    TEST_ASSERT(terminal != NULL);
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    
    // Initialize terminal with basic settings
    terminal->geometry.width = 80;
    terminal->geometry.height = 24;
    terminal->geometry_valid = true;
    terminal->termcap_initialized = true;
    
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    TEST_ASSERT(lle_prompt_parse(&prompt, "[test]$ "));
    
    lle_display_state_t *display = lle_display_create(&prompt, buffer, terminal);
    TEST_ASSERT(display != NULL);
    
    // Test single character backspace (simple case)
    TEST_ASSERT(lle_text_insert_at(buffer, 0, "hello"));
    display->last_displayed_length = 5;
    memcpy(display->last_displayed_content, "hello", 5);
    display->last_displayed_content[5] = '\0';
    display->display_state_valid = true;
    
    // Simulate backspace by removing last character
    TEST_ASSERT(lle_text_delete_range(buffer, 4, 5));
    
    // The enhanced logic should handle this correctly
    TEST_ASSERT(lle_display_update_incremental(display));
    
    // Cleanup
    lle_display_destroy(display);
    lle_prompt_clear(&prompt);
    free(terminal);
    lle_text_buffer_destroy(buffer);
    
    printf("✅ Enhanced backspace logic test PASSED\n");
    return true;
}

/**
 * Test consistent rendering behavior
 */
LLE_TEST(consistent_rendering) {
    printf("🧪 Testing consistent rendering behavior...\n");
    
    // Create test components
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    TEST_ASSERT(buffer != NULL);
    
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    TEST_ASSERT(terminal != NULL);
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    
    terminal->geometry.width = 80;
    terminal->geometry.height = 24;
    terminal->geometry_valid = true;
    terminal->termcap_initialized = true;
    
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    TEST_ASSERT(lle_prompt_parse(&prompt, "[test]$ "));
    
    lle_display_state_t *display = lle_display_create(&prompt, buffer, terminal);
    TEST_ASSERT(display != NULL);
    
    // Test unified rendering
    TEST_ASSERT(lle_text_insert_at(buffer, 0, "test content"));
    TEST_ASSERT(lle_display_update_unified(display, false));
    
    // Test forced rendering
    TEST_ASSERT(lle_display_update_unified(display, true));
    
    // Cleanup
    lle_display_destroy(display);
    lle_prompt_clear(&prompt);
    free(terminal);
    lle_text_buffer_destroy(buffer);
    
    printf("✅ Consistent rendering test PASSED\n");
    return true;
}

/**
 * Test intelligent clearing strategies
 */
LLE_TEST(intelligent_clearing) {
    printf("🧪 Testing intelligent clearing strategies...\n");
    
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    TEST_ASSERT(terminal != NULL);
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    
    terminal->geometry.width = 80;
    terminal->geometry.height = 24;
    terminal->geometry_valid = true;
    terminal->termcap_initialized = true;
    
    // Test single-line clearing
    lle_visual_footprint_t old_footprint = {
        .rows_used = 1,
        .end_column = 20,
        .wraps_lines = false,
        .total_visual_width = 20
    };
    
    lle_visual_footprint_t new_footprint = {
        .rows_used = 1,
        .end_column = 15,
        .wraps_lines = false,
        .total_visual_width = 15
    };
    
    // This should not crash and should handle clearing
    TEST_ASSERT(lle_clear_visual_region(terminal, &old_footprint, &new_footprint));
    
    // Test multi-line clearing fallback
    old_footprint.rows_used = 3;
    old_footprint.wraps_lines = true;
    TEST_ASSERT(lle_clear_multi_line_fallback(terminal, &old_footprint));
    
    free(terminal);
    
    printf("✅ Intelligent clearing test PASSED\n");
    return true;
}

/**
 * Test cross-line boundary detection
 */
LLE_TEST(boundary_detection) {
    printf("🧪 Testing cross-line boundary detection...\n");
    
    lle_visual_footprint_t before, after;
    
    // Test wrapping boundary crossing
    TEST_ASSERT(lle_calculate_visual_footprint("a", 1, 79, 80, &before));
    TEST_ASSERT_EQ(1, before.rows_used);
    TEST_ASSERT(!before.wraps_lines);
    
    TEST_ASSERT(lle_calculate_visual_footprint("ab", 2, 79, 80, &after));
    TEST_ASSERT_EQ(2, after.rows_used);
    TEST_ASSERT(after.wraps_lines);
    
    // Verify boundary crossing detection logic
    bool crossing_boundary = (before.rows_used != after.rows_used) ||
                           (before.wraps_lines && !after.wraps_lines);
    TEST_ASSERT(crossing_boundary);
    
    printf("✅ Boundary detection test PASSED\n");
    return true;
}

/**
 * Test enhanced display state tracking
 */
LLE_TEST(display_state_tracking) {
    printf("🧪 Testing enhanced display state tracking...\n");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    TEST_ASSERT(buffer != NULL);
    
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    TEST_ASSERT(terminal != NULL);
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    
    terminal->geometry.width = 80;
    terminal->geometry.height = 24;
    terminal->geometry_valid = true;
    terminal->termcap_initialized = true;
    
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    TEST_ASSERT(lle_prompt_parse(&prompt, "[test]$ "));
    
    lle_display_state_t *display = lle_display_create(&prompt, buffer, terminal);
    TEST_ASSERT(display != NULL);
    
    // Verify initial state
    TEST_ASSERT_EQ(1, display->last_visual_rows);
    TEST_ASSERT_EQ(0, display->last_visual_end_col);
    TEST_ASSERT_EQ(0, display->last_total_chars);
    TEST_ASSERT(!display->last_had_wrapping);
    TEST_ASSERT_EQ(0, display->last_content_hash);
    TEST_ASSERT(!display->syntax_highlighting_applied);
    TEST_ASSERT(!display->clear_region_valid);
    
    // Test state updates during rendering
    TEST_ASSERT(lle_text_insert_at(buffer, 0, "test"));
    TEST_ASSERT(lle_display_update_unified(display, true));
    
    // State should be updated after rendering
    TEST_ASSERT(display->display_state_valid);
    
    // Cleanup
    lle_display_destroy(display);
    lle_prompt_clear(&prompt);
    free(terminal);
    lle_text_buffer_destroy(buffer);
    
    printf("✅ Display state tracking test PASSED\n");
    return true;
}

/**
 * Test performance under various scenarios
 */
LLE_TEST(performance_scenarios) {
    printf("🧪 Testing performance scenarios...\n");
    
    lle_text_buffer_t *buffer = lle_text_buffer_create(256);
    TEST_ASSERT(buffer != NULL);
    
    lle_terminal_manager_t *terminal = malloc(sizeof(lle_terminal_manager_t));
    TEST_ASSERT(terminal != NULL);
    memset(terminal, 0, sizeof(lle_terminal_manager_t));
    
    terminal->geometry.width = 80;
    terminal->geometry.height = 24;
    terminal->geometry_valid = true;
    terminal->termcap_initialized = true;
    
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    TEST_ASSERT(lle_prompt_parse(&prompt, "[test]$ "));
    
    lle_display_state_t *display = lle_display_create(&prompt, buffer, terminal);
    TEST_ASSERT(display != NULL);
    
    // Test large content handling
    char large_content[200];
    memset(large_content, 'x', 199);
    large_content[199] = '\0';
    
    TEST_ASSERT(lle_text_insert_at(buffer, 0, large_content));
    
    lle_visual_footprint_t footprint;
    TEST_ASSERT(lle_calculate_visual_footprint(large_content, 199, 10, 80, &footprint));
    TEST_ASSERT(footprint.rows_used > 1);
    TEST_ASSERT(footprint.wraps_lines);
    
    // Test multiple rapid updates
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT(lle_display_update_unified(display, false));
    }
    
    // Cleanup
    lle_display_destroy(display);
    lle_prompt_clear(&prompt);
    free(terminal);
    lle_text_buffer_destroy(buffer);
    
    printf("✅ Performance scenarios test PASSED\n");
    return true;
}

/**
 * Main test function
 */
int main(void) {
    printf("🚀 Starting backspace enhancement tests...\n\n");
    
    // Run all tests
    bool all_passed = true;
    
    all_passed &= test_visual_footprint_single_line();
    all_passed &= test_visual_footprint_multi_line();
    all_passed &= test_visual_footprint_edge_cases();
    all_passed &= test_enhanced_backspace_logic();
    all_passed &= test_consistent_rendering();
    all_passed &= test_intelligent_clearing();
    all_passed &= test_boundary_detection();
    all_passed &= test_display_state_tracking();
    all_passed &= test_performance_scenarios();
    
    printf("\n");
    
    if (all_passed) {
        printf("🎉 All backspace enhancement tests PASSED!\n");
        return 0;
    } else {
        printf("❌ Some backspace enhancement tests FAILED!\n");
        return 1;
    }
}
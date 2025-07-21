/**
 * test_termcap_integration.c - Basic tests for LLE termcap integration
 *
 * This file tests the integrated termcap system in LLE to ensure:
 * - Proper namespacing of all functions
 * - Basic initialization and cleanup
 * - iTerm2 detection functionality (key requirement)
 * - Core terminal capabilities
 */

#include "../src/line_editor/termcap/lle_termcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
            fprintf(stderr, "ASSERTION FAILED: %s (%d) != %s (%d) at %s:%d\n", \
                    #actual, (int)(actual), #expected, (int)(expected), \
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

/* Test basic termcap initialization */
LLE_TEST(termcap_basic_init) {
    // Test initialization
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    if (result == LLE_TERMCAP_OK) {
        // Test getting terminal info
        const lle_terminal_info_t *info = lle_termcap_get_info();
        LLE_ASSERT_NOT_NULL(info);
        
        // Basic sanity checks on terminal info
        LLE_ASSERT(info->rows > 0);
        LLE_ASSERT(info->cols > 0);
    }
    
    // Cleanup
    lle_termcap_cleanup();
}

/* Test that iTerm2 detection function exists and doesn't crash */
LLE_TEST(termcap_iterm2_detection) {
    // Initialize first
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Call iTerm2 detection - should not crash regardless of environment
    bool is_iterm2 = lle_termcap_is_iterm2();
    // We can't assert the value since it depends on environment,
    // but function should execute without error
    (void)is_iterm2; // Suppress unused variable warning
    
    // Cleanup
    lle_termcap_cleanup();
}

/* Test color functionality */
LLE_TEST(termcap_color_functions) {
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test basic color setting (should not crash)
    result = lle_termcap_set_color(LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test color reset
    result = lle_termcap_reset_colors();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    lle_termcap_cleanup();
}

/* Test capability detection */
LLE_TEST(termcap_capability_detection) {
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test capability detection (should not crash)
    result = lle_termcap_detect_capabilities();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test individual capability checks (should not crash)
    bool colors = lle_termcap_supports_colors();
    bool colors_256 = lle_termcap_supports_256_colors();
    bool truecolor = lle_termcap_supports_truecolor();
    
    // These calls should execute without error
    (void)colors;
    (void)colors_256;
    (void)truecolor;
    
    lle_termcap_cleanup();
}

/* Test cursor operations */
LLE_TEST(termcap_cursor_operations) {
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test cursor visibility (should not crash)
    result = lle_termcap_hide_cursor();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    result = lle_termcap_show_cursor();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test cursor movement (should not crash)
    result = lle_termcap_move_cursor(1, 1);
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    lle_termcap_cleanup();
}

/* Test screen management */
LLE_TEST(termcap_screen_management) {
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test screen clearing operations (should not crash)
    result = lle_termcap_clear_to_eol();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    lle_termcap_cleanup();
}

/* Test platform detection functions */
LLE_TEST(termcap_platform_detection) {
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test platform detection functions (should not crash)
    bool is_iterm2 = lle_termcap_is_iterm2();
    bool is_tmux = lle_termcap_is_tmux();
    bool is_screen = lle_termcap_is_screen();
    
    // These should execute without error
    (void)is_iterm2;
    (void)is_tmux;
    (void)is_screen;
    
    lle_termcap_cleanup();
}

/* Test double initialization/cleanup */
LLE_TEST(termcap_double_init_cleanup) {
    // First init/cleanup cycle
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    lle_termcap_cleanup();
    
    // Second init/cleanup cycle (should work fine)
    result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    lle_termcap_cleanup();
    
    // Test multiple cleanups (should be safe)
    lle_termcap_cleanup();
}

/* Test constants are properly namespaced */
LLE_TEST(termcap_constants_namespaced) {
    // Test that color constants are properly namespaced
    LLE_ASSERT(LLE_TERMCAP_COLOR_RED == 1);
    LLE_ASSERT(LLE_TERMCAP_COLOR_GREEN == 2);
    LLE_ASSERT(LLE_TERMCAP_COLOR_BLUE == 4);
    
    // Test return code constants
    LLE_ASSERT(LLE_TERMCAP_OK == 0);
    LLE_ASSERT(LLE_TERMCAP_ERROR == -1);
    
    // Test size constants exist
    LLE_ASSERT(LLE_TERMCAP_DEFAULT_ROWS > 0);
    LLE_ASSERT(LLE_TERMCAP_DEFAULT_COLS > 0);
}

/* Test that integration doesn't break when called without a terminal */
LLE_TEST(termcap_non_terminal_safe) {
    // Test init works (might return NOT_TERMINAL but shouldn't crash)
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    // Test that functions handle non-terminal gracefully
    if (result == LLE_TERMCAP_OK) {
        lle_termcap_set_color(LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
        lle_termcap_cleanup();
    }
}

/* Test output buffer functionality */
LLE_TEST(termcap_output_buffer) {
    int result = lle_termcap_init();
    LLE_ASSERT(result == LLE_TERMCAP_OK || result == LLE_TERMCAP_NOT_TERMINAL);
    
    if (result == LLE_TERMCAP_OK) {
        lle_termcap_output_buffer_t buffer;
        
        // Test buffer initialization
        result = lle_termcap_output_buffer_init(&buffer, 1024);
        LLE_ASSERT(result == LLE_TERMCAP_OK);
        
        // Test adding data to buffer
        const char *test_data = "test";
        result = lle_termcap_output_buffer_add(&buffer, test_data, strlen(test_data));
        LLE_ASSERT(result == LLE_TERMCAP_OK);
        
        // Test cleanup
        lle_termcap_output_buffer_cleanup(&buffer);
    }
    
    lle_termcap_cleanup();
}

int main(void) {
    printf("Running LLE Termcap Integration Tests...\n");
    
    // Run all tests
    test_termcap_basic_init();
    printf("✓ termcap_basic_init\n");
    
    test_termcap_iterm2_detection();
    printf("✓ termcap_iterm2_detection\n");
    
    test_termcap_color_functions();
    printf("✓ termcap_color_functions\n");
    
    test_termcap_capability_detection();
    printf("✓ termcap_capability_detection\n");
    
    test_termcap_cursor_operations();
    printf("✓ termcap_cursor_operations\n");
    
    test_termcap_screen_management();
    printf("✓ termcap_screen_management\n");
    
    test_termcap_platform_detection();
    printf("✓ termcap_platform_detection\n");
    
    test_termcap_double_init_cleanup();
    printf("✓ termcap_double_init_cleanup\n");
    
    test_termcap_constants_namespaced();
    printf("✓ termcap_constants_namespaced\n");
    
    test_termcap_non_terminal_safe();
    printf("✓ termcap_non_terminal_safe\n");
    
    test_termcap_output_buffer();
    printf("✓ termcap_output_buffer\n");
    
    printf("\nAll LLE Termcap Integration Tests Passed!\n");
    return 0;
}
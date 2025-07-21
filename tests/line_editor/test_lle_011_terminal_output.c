/**
 * test_lle_011_terminal_output.c - LLE-011 Terminal Output Integration Tests
 *
 * This file tests the LLE-011 task completion: terminal output integration
 * using the integrated termcap system. Tests verify:
 * - Terminal write operations with error handling
 * - Cursor movement using termcap functions
 * - Line and screen clearing operations
 * - Color setting and resetting functionality
 * - Cursor visibility control
 * - Error handling for all output operations
 * - Integration with terminal manager capabilities
 */

#include "../src/line_editor/terminal_manager.h"
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

/* Test LLE-011: Terminal write function */
LLE_TEST(terminal_write_basic) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test basic write operation
        const char *test_data = "Hello, Terminal!";
        bool write_result = lle_terminal_write(&tm, test_data, strlen(test_data));
        LLE_ASSERT(write_result);
        
        // Test empty string write
        write_result = lle_terminal_write(&tm, "", 0);
        LLE_ASSERT(!write_result); // Should fail with zero length
        
        // Test single character write
        write_result = lle_terminal_write(&tm, "X", 1);
        LLE_ASSERT(write_result);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Terminal write error handling */
LLE_TEST(terminal_write_error_handling) {
    lle_terminal_manager_t tm;
    
    // Test with NULL pointer
    bool result = lle_terminal_write(NULL, "test", 4);
    LLE_ASSERT(!result);
    
    // Test with NULL data
    memset(&tm, 0, sizeof(tm));
    tm.termcap_initialized = true;
    result = lle_terminal_write(&tm, NULL, 4);
    LLE_ASSERT(!result);
    
    // Test with uninitialized termcap
    memset(&tm, 0, sizeof(tm));
    tm.stdout_fd = STDOUT_FILENO;
    result = lle_terminal_write(&tm, "test", 4);
    LLE_ASSERT(!result);
}

/* Test LLE-011: Cursor movement operations */
LLE_TEST(terminal_cursor_movement) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test basic cursor movement
        bool move_result = lle_terminal_move_cursor(&tm, 0, 0);
        LLE_ASSERT(move_result);
        
        // Test movement within terminal bounds
        if (tm.geometry_valid) {
            move_result = lle_terminal_move_cursor(&tm, 1, 1);
            LLE_ASSERT(move_result);
            
            // Test movement to last valid position
            if (tm.geometry.height > 1 && tm.geometry.width > 1) {
                move_result = lle_terminal_move_cursor(&tm, tm.geometry.height - 1, tm.geometry.width - 1);
                LLE_ASSERT(move_result);
            }
            
            // Test movement beyond bounds (should fail)
            move_result = lle_terminal_move_cursor(&tm, tm.geometry.height, tm.geometry.width);
            LLE_ASSERT(!move_result);
        }
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Cursor movement error handling */
LLE_TEST(terminal_cursor_movement_error_handling) {
    lle_terminal_manager_t tm;
    
    // Test with NULL pointer
    bool result = lle_terminal_move_cursor(NULL, 0, 0);
    LLE_ASSERT(!result);
    
    // Test with uninitialized termcap
    memset(&tm, 0, sizeof(tm));
    result = lle_terminal_move_cursor(&tm, 0, 0);
    LLE_ASSERT(!result);
}

/* Test LLE-011: Screen and line clearing operations */
LLE_TEST(terminal_clear_operations) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test clear line
        bool clear_result = lle_terminal_clear_line(&tm);
        LLE_ASSERT(clear_result);
        
        // Test clear to end of line
        clear_result = lle_terminal_clear_to_eol(&tm);
        LLE_ASSERT(clear_result);
        
        // Test clear screen
        clear_result = lle_terminal_clear_screen(&tm);
        LLE_ASSERT(clear_result);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Clear operations error handling */
LLE_TEST(terminal_clear_operations_error_handling) {
    lle_terminal_manager_t tm;
    
    // Test clear line with NULL pointer
    bool result = lle_terminal_clear_line(NULL);
    LLE_ASSERT(!result);
    
    // Test clear to EOL with uninitialized termcap
    memset(&tm, 0, sizeof(tm));
    result = lle_terminal_clear_to_eol(&tm);
    LLE_ASSERT(!result);
    
    // Test clear screen with uninitialized termcap
    result = lle_terminal_clear_screen(&tm);
    LLE_ASSERT(!result);
}

/* Test LLE-011: Color setting operations */
LLE_TEST(terminal_color_operations) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test color setting (only if terminal supports colors)
        if (lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS)) {
            bool color_result = lle_terminal_set_color(&tm, LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
            LLE_ASSERT(color_result);
            
            color_result = lle_terminal_set_color(&tm, LLE_TERMCAP_COLOR_GREEN, LLE_TERMCAP_COLOR_WHITE);
            LLE_ASSERT(color_result);
        }
        
        // Test color reset (should work regardless of color support)
        bool reset_result = lle_terminal_reset_colors(&tm);
        LLE_ASSERT(reset_result);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Color operations with non-color terminal */
LLE_TEST(terminal_color_operations_no_support) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Force disable color capability for testing
        uint32_t original_caps = tm.capabilities;
        tm.capabilities &= ~LLE_TERM_CAP_COLORS;
        
        // Color setting should fail without color support
        bool color_result = lle_terminal_set_color(&tm, LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
        LLE_ASSERT(!color_result);
        
        // Restore original capabilities
        tm.capabilities = original_caps;
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Color operations error handling */
LLE_TEST(terminal_color_operations_error_handling) {
    lle_terminal_manager_t tm;
    
    // Test with NULL pointer
    bool result = lle_terminal_set_color(NULL, LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
    LLE_ASSERT(!result);
    
    // Test reset colors with NULL pointer
    result = lle_terminal_reset_colors(NULL);
    LLE_ASSERT(!result);
    
    // Test with uninitialized termcap
    memset(&tm, 0, sizeof(tm));
    result = lle_terminal_set_color(&tm, LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
    LLE_ASSERT(!result);
    
    result = lle_terminal_reset_colors(&tm);
    LLE_ASSERT(!result);
}

/* Test LLE-011: Cursor visibility operations */
LLE_TEST(terminal_cursor_visibility) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test hide cursor
        bool hide_result = lle_terminal_hide_cursor(&tm);
        LLE_ASSERT(hide_result);
        
        // Test show cursor
        bool show_result = lle_terminal_show_cursor(&tm);
        LLE_ASSERT(show_result);
        
        // Test hide/show cycle
        hide_result = lle_terminal_hide_cursor(&tm);
        LLE_ASSERT(hide_result);
        
        show_result = lle_terminal_show_cursor(&tm);
        LLE_ASSERT(show_result);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Cursor visibility error handling */
LLE_TEST(terminal_cursor_visibility_error_handling) {
    lle_terminal_manager_t tm;
    
    // Test with NULL pointer
    bool result = lle_terminal_hide_cursor(NULL);
    LLE_ASSERT(!result);
    
    result = lle_terminal_show_cursor(NULL);
    LLE_ASSERT(!result);
    
    // Test with uninitialized termcap
    memset(&tm, 0, sizeof(tm));
    result = lle_terminal_hide_cursor(&tm);
    LLE_ASSERT(!result);
    
    result = lle_terminal_show_cursor(&tm);
    LLE_ASSERT(!result);
}

/* Test LLE-011: Combined operations sequence */
LLE_TEST(terminal_combined_operations) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test a sequence of operations that would be typical in line editing
        
        // 1. Clear screen and move to home
        bool op_result = lle_terminal_clear_screen(&tm);
        LLE_ASSERT(op_result);
        
        op_result = lle_terminal_move_cursor(&tm, 0, 0);
        LLE_ASSERT(op_result);
        
        // 2. Write some text
        op_result = lle_terminal_write(&tm, "LLE Test: ", 10);
        LLE_ASSERT(op_result);
        
        // 3. Set color if supported and write colored text
        if (lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS)) {
            op_result = lle_terminal_set_color(&tm, LLE_TERMCAP_COLOR_GREEN, LLE_TERMCAP_COLOR_BLACK);
            LLE_ASSERT(op_result);
            
            op_result = lle_terminal_write(&tm, "SUCCESS", 7);
            LLE_ASSERT(op_result);
            
            op_result = lle_terminal_reset_colors(&tm);
            LLE_ASSERT(op_result);
        } else {
            op_result = lle_terminal_write(&tm, "SUCCESS", 7);
            LLE_ASSERT(op_result);
        }
        
        // 4. Move to next line and clear to end
        if (tm.geometry_valid && tm.geometry.height > 1) {
            op_result = lle_terminal_move_cursor(&tm, 1, 0);
            LLE_ASSERT(op_result);
        }
        
        op_result = lle_terminal_clear_to_eol(&tm);
        LLE_ASSERT(op_result);
        
        // 5. Hide cursor during updates, then show
        op_result = lle_terminal_hide_cursor(&tm);
        LLE_ASSERT(op_result);
        
        op_result = lle_terminal_write(&tm, "Hidden cursor update", 20);
        LLE_ASSERT(op_result);
        
        op_result = lle_terminal_show_cursor(&tm);
        LLE_ASSERT(op_result);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Integration with terminal capabilities */
LLE_TEST(terminal_output_capability_integration) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Verify that output functions respect terminal capabilities
        
        // Test cursor movement (may not be supported in all environments)
        bool has_cursor_movement = lle_terminal_has_capability(&tm, LLE_TERM_CAP_CURSOR_MOVEMENT);
        bool move_result = lle_terminal_move_cursor(&tm, 0, 0);
        // Move should succeed if capability is supported, or work in non-terminal mode
        if (has_cursor_movement) {
            LLE_ASSERT(move_result);
        }
        
        // Test screen clearing (may not be supported in all environments)
        bool has_clear_screen = lle_terminal_has_capability(&tm, LLE_TERM_CAP_CLEAR_SCREEN);
        bool clear_result = lle_terminal_clear_to_eol(&tm);
        // Clear should succeed if capability is supported, or work in non-terminal mode
        if (has_clear_screen) {
            LLE_ASSERT(clear_result);
        }
        
        // Color operations should only succeed if supported
        bool has_colors = lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS);
        bool color_result = lle_terminal_set_color(&tm, LLE_TERMCAP_COLOR_RED, LLE_TERMCAP_COLOR_BLACK);
        // Color operation success should match capability detection
        if (has_colors) {
            LLE_ASSERT(color_result);
        }
        
        // Reset should always work
        bool reset_result = lle_terminal_reset_colors(&tm);
        LLE_ASSERT(reset_result);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-011: Performance and bounds checking */
LLE_TEST(terminal_output_bounds_and_performance) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS && tm.geometry_valid) {
        // Test bounds checking for cursor movement
        bool move_result;
        
        // Valid positions should work
        move_result = lle_terminal_move_cursor(&tm, 0, 0);
        LLE_ASSERT(move_result);
        
        if (tm.geometry.height > 1) {
            move_result = lle_terminal_move_cursor(&tm, tm.geometry.height - 1, 0);
            LLE_ASSERT(move_result);
        }
        
        if (tm.geometry.width > 1) {
            move_result = lle_terminal_move_cursor(&tm, 0, tm.geometry.width - 1);
            LLE_ASSERT(move_result);
        }
        
        // Invalid positions should fail
        move_result = lle_terminal_move_cursor(&tm, tm.geometry.height, 0);
        LLE_ASSERT(!move_result);
        
        move_result = lle_terminal_move_cursor(&tm, 0, tm.geometry.width);
        LLE_ASSERT(!move_result);
        
        // Test multiple operations for performance (should not hang)
        for (int i = 0; i < 10; i++) {
            lle_terminal_write(&tm, ".", 1);
            lle_terminal_move_cursor(&tm, 0, 0);
            lle_terminal_clear_to_eol(&tm);
        }
        
        lle_terminal_cleanup(&tm);
    }
}

int main(void) {
    printf("Running LLE-011 Terminal Output Integration Tests...\n");
    
    // Run all tests
    test_terminal_write_basic();
    printf("✓ terminal_write_basic\n");
    
    test_terminal_write_error_handling();
    printf("✓ terminal_write_error_handling\n");
    
    test_terminal_cursor_movement();
    printf("✓ terminal_cursor_movement\n");
    
    test_terminal_cursor_movement_error_handling();
    printf("✓ terminal_cursor_movement_error_handling\n");
    
    test_terminal_clear_operations();
    printf("✓ terminal_clear_operations\n");
    
    test_terminal_clear_operations_error_handling();
    printf("✓ terminal_clear_operations_error_handling\n");
    
    test_terminal_color_operations();
    printf("✓ terminal_color_operations\n");
    
    test_terminal_color_operations_no_support();
    printf("✓ terminal_color_operations_no_support\n");
    
    test_terminal_color_operations_error_handling();
    printf("✓ terminal_color_operations_error_handling\n");
    
    test_terminal_cursor_visibility();
    printf("✓ terminal_cursor_visibility\n");
    
    test_terminal_cursor_visibility_error_handling();
    printf("✓ terminal_cursor_visibility_error_handling\n");
    
    test_terminal_combined_operations();
    printf("✓ terminal_combined_operations\n");
    
    test_terminal_output_capability_integration();
    printf("✓ terminal_output_capability_integration\n");
    
    test_terminal_output_bounds_and_performance();
    printf("✓ terminal_output_bounds_and_performance\n");
    
    printf("\nAll LLE-011 Terminal Output Integration Tests Passed!\n");
    printf("Terminal output functions successfully integrated with termcap system.\n");
    
    return 0;
}
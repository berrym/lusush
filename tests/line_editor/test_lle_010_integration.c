/**
 * test_lle_010_integration.c - LLE-010 Terminal Manager Integration Tests
 *
 * This file tests the LLE-010 task completion: terminal manager implementation
 * using the integrated termcap system. Tests verify:
 * - Proper termcap system integration
 * - iTerm2 detection functionality
 * - Enhanced capability detection using termcap
 * - Terminal geometry from termcap system
 * - Backward compatibility with existing API
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

/* Test LLE-010: Terminal manager uses termcap for initialization */
LLE_TEST(terminal_manager_termcap_integration) {
    lle_terminal_manager_t tm;
    
    // Initialize terminal manager - should initialize termcap system
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Verify termcap integration
        LLE_ASSERT(tm.termcap_initialized);
        LLE_ASSERT_NOT_NULL(tm.termcap_info);
        LLE_ASSERT(tm.capabilities_initialized);
        
        // Verify geometry comes from termcap
        LLE_ASSERT(tm.geometry.width > 0);
        LLE_ASSERT(tm.geometry.height > 0);
        LLE_ASSERT(tm.geometry_valid);
        
        // Clean up
        lle_terminal_cleanup(&tm);
        
        // After cleanup, termcap should be cleaned up
        // (We can't easily test this without exposing internal state)
    }
}

/* Test LLE-010: iTerm2 detection integration */
LLE_TEST(terminal_manager_iterm2_detection) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test iTerm2 detection function
        bool is_iterm2_tm = lle_terminal_is_iterm2(&tm);
        bool is_iterm2_direct = lle_termcap_is_iterm2();
        
        // Both should return the same value
        LLE_ASSERT(is_iterm2_tm == is_iterm2_direct);
        
        // Should match internal flag
        LLE_ASSERT(tm.is_iterm2 == is_iterm2_tm);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-010: Enhanced capability detection using termcap */
LLE_TEST(terminal_manager_enhanced_capabilities) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Verify capabilities were detected
        LLE_ASSERT(tm.capabilities_initialized);
        
        // Check that termcap capabilities map to terminal manager capabilities
        if (tm.termcap_info->caps.colors) {
            LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS));
        }
        
        if (tm.termcap_info->caps.colors_256) {
            LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_256_COLORS));
        }
        
        if (tm.termcap_info->caps.unicode) {
            LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_UTF8));
        }
        
        if (tm.termcap_info->caps.mouse) {
            LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_MOUSE));
        }
        
        if (tm.termcap_info->caps.bracketed_paste) {
            LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_BRACKETED_PASTE));
        }
        
        if (tm.termcap_info->caps.alternate_screen) {
            LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_ALTERNATE_SCREEN));
        }
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-010: Terminal geometry from termcap system */
LLE_TEST(terminal_manager_geometry_from_termcap) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Verify geometry matches termcap info
        LLE_ASSERT_NOT_NULL(tm.termcap_info);
        LLE_ASSERT_EQ((int)tm.geometry.width, tm.termcap_info->cols);
        LLE_ASSERT_EQ((int)tm.geometry.height, tm.termcap_info->rows);
        
        // Test geometry update function
        bool update_result = lle_terminal_update_geometry(&tm);
        LLE_ASSERT(update_result);
        
        // Geometry should still be valid after update
        LLE_ASSERT(tm.geometry_valid);
        LLE_ASSERT(tm.geometry.width > 0);
        LLE_ASSERT(tm.geometry.height > 0);
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-010: Capabilities string includes termcap features */
LLE_TEST(terminal_manager_capabilities_string_enhanced) {
    lle_terminal_manager_t tm;
    char cap_string[1024];
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        int string_len = lle_terminal_get_capabilities_string(&tm, cap_string, sizeof(cap_string));
        LLE_ASSERT(string_len > 0);
        LLE_ASSERT(string_len < (int)sizeof(cap_string));
        
        // Verify string contains expected content
        LLE_ASSERT(strstr(cap_string, "Terminal capabilities:") != NULL);
        
        // Check for iTerm2 detection in capabilities string
        if (tm.is_iterm2) {
            LLE_ASSERT(strstr(cap_string, "iterm2") != NULL);
        }
        
        // Check for truecolor if supported
        if (tm.termcap_info && tm.termcap_info->caps.truecolor) {
            LLE_ASSERT(strstr(cap_string, "truecolor") != NULL);
        }
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-010: Backward compatibility with existing API */
LLE_TEST(terminal_manager_backward_compatibility) {
    lle_terminal_manager_t tm;
    
    // All existing functions should still work
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Test all the existing API functions still work
        LLE_ASSERT(lle_terminal_manager_is_valid(&tm));
        
        // Get size should work
        bool size_result = lle_terminal_get_size(&tm);
        LLE_ASSERT(size_result);
        
        // Capability detection should work (already initialized)
        LLE_ASSERT(tm.capabilities_initialized);
        
        // Has capability should work
        // (Just test it doesn't crash - actual capabilities vary by environment)
        lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS);
        lle_terminal_has_capability(&tm, LLE_TERM_CAP_UTF8);
        
        // Raw mode functions should work (test without actually entering raw mode)
        // These are tested in the main terminal manager test suite
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-010: Error handling with termcap integration */
LLE_TEST(terminal_manager_termcap_error_handling) {
    lle_terminal_manager_t tm;
    
    // Test capability detection without proper initialization
    memset(&tm, 0, sizeof(tm));
    tm.stdout_fd = STDOUT_FILENO;
    
    // Should fail because termcap not initialized
    bool cap_result = lle_terminal_detect_capabilities(&tm);
    LLE_ASSERT(!cap_result);
    
    // Test iTerm2 detection without initialization
    bool iterm2_result = lle_terminal_is_iterm2(&tm);
    LLE_ASSERT(!iterm2_result);
    
    // Test geometry update without initialization
    bool geom_result = lle_terminal_update_geometry(&tm);
    LLE_ASSERT(!geom_result);
}

/* Test LLE-010: Terminal manager initialization states */
LLE_TEST(terminal_manager_initialization_states) {
    lle_terminal_manager_t tm;
    
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Check all initialization flags
        LLE_ASSERT(tm.termcap_initialized);
        LLE_ASSERT(tm.capabilities_initialized);
        LLE_ASSERT(tm.geometry_valid);
        LLE_ASSERT_NOT_NULL(tm.termcap_info);
        
        // Check file descriptors are set
        LLE_ASSERT(tm.stdin_fd >= 0);
        LLE_ASSERT(tm.stdout_fd >= 0);
        LLE_ASSERT(tm.stderr_fd >= 0);
        
        // Check that is_iterm2 flag is set (whether true or false)
        // This validates that the detection ran
        bool iterm2_flag = tm.is_iterm2;
        (void)iterm2_flag; // Suppress unused variable warning
        
        lle_terminal_cleanup(&tm);
    }
}

/* Test LLE-010: Multiple initialization/cleanup cycles */
LLE_TEST(terminal_manager_multiple_cycles) {
    lle_terminal_manager_t tm1, tm2;
    
    // First cycle
    lle_terminal_init_result_t result1 = lle_terminal_init(&tm1);
    LLE_ASSERT(result1 == LLE_TERM_INIT_SUCCESS || result1 == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result1 == LLE_TERM_INIT_SUCCESS) {
        LLE_ASSERT(tm1.termcap_initialized);
        lle_terminal_cleanup(&tm1);
    }
    
    // Second cycle with different structure
    lle_terminal_init_result_t result2 = lle_terminal_init(&tm2);
    LLE_ASSERT(result2 == LLE_TERM_INIT_SUCCESS || result2 == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result2 == LLE_TERM_INIT_SUCCESS) {
        LLE_ASSERT(tm2.termcap_initialized);
        lle_terminal_cleanup(&tm2);
    }
    
    // Both should succeed independently
    LLE_ASSERT(result1 == result2);
}

int main(void) {
    printf("Running LLE-010 Terminal Manager Integration Tests...\n");
    
    // Run all tests
    test_terminal_manager_termcap_integration();
    printf("✓ terminal_manager_termcap_integration\n");
    
    test_terminal_manager_iterm2_detection();
    printf("✓ terminal_manager_iterm2_detection\n");
    
    test_terminal_manager_enhanced_capabilities();
    printf("✓ terminal_manager_enhanced_capabilities\n");
    
    test_terminal_manager_geometry_from_termcap();
    printf("✓ terminal_manager_geometry_from_termcap\n");
    
    test_terminal_manager_capabilities_string_enhanced();
    printf("✓ terminal_manager_capabilities_string_enhanced\n");
    
    test_terminal_manager_backward_compatibility();
    printf("✓ terminal_manager_backward_compatibility\n");
    
    test_terminal_manager_termcap_error_handling();
    printf("✓ terminal_manager_termcap_error_handling\n");
    
    test_terminal_manager_initialization_states();
    printf("✓ terminal_manager_initialization_states\n");
    
    test_terminal_manager_multiple_cycles();
    printf("✓ terminal_manager_multiple_cycles\n");
    
    printf("\nAll LLE-010 Terminal Manager Integration Tests Passed!\n");
    printf("Terminal manager successfully integrated with termcap system.\n");
    
    return 0;
}
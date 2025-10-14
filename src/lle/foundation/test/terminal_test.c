// src/lle/foundation/test/terminal_test.c
//
// Comprehensive tests for terminal abstraction layer

#define _POSIX_C_SOURCE 200809L
#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

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
    memset(&term, 0, sizeof(term));
    
    int result = lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    ASSERT(result == LLE_TERM_OK, "Initialization should succeed");
    ASSERT(term.initialized, "Terminal should be marked as initialized");
    ASSERT(term.state.rows > 0, "Rows should be positive");
    ASSERT(term.state.cols > 0, "Columns should be positive");
    
    lle_term_cleanup(&term);
    ASSERT(!term.initialized, "Terminal should be marked as uninitialized");
}

// Test 2: Null pointer handling
TEST(null_pointer_handling) {
    int result = lle_term_init(NULL, STDIN_FILENO, STDOUT_FILENO);
    ASSERT(result == LLE_TERM_ERR_NULL_PTR, "Should reject null pointer");
    
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    const lle_term_state_t *state = lle_term_get_state(NULL);
    ASSERT(state == NULL, "Should return NULL for null term");
    
    lle_term_cleanup(&term);
}

// Test 3: Double initialization prevention
TEST(double_init_prevention) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    
    int result1 = lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    ASSERT(result1 == LLE_TERM_OK, "First initialization should succeed");
    
    int result2 = lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    ASSERT(result2 == LLE_TERM_ERR_ALREADY_INIT, 
           "Second initialization should fail");
    
    lle_term_cleanup(&term);
}

// Test 4: Cursor update (no terminal query)
TEST(cursor_update) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    // Update cursor position
    lle_term_update_cursor(&term, 5, 10);
    
    const lle_term_state_t *state = lle_term_get_state(&term);
    ASSERT(state != NULL, "Should get state");
    ASSERT(state->cursor_row == 5, "Cursor row should be 5");
    ASSERT(state->cursor_col == 10, "Cursor column should be 10");
    ASSERT(state->update_count == 1, "Update count should be 1");
    
    // Update again
    lle_term_update_cursor(&term, 7, 15);
    ASSERT(state->cursor_row == 7, "Cursor row should be 7");
    ASSERT(state->cursor_col == 15, "Cursor column should be 15");
    ASSERT(state->update_count == 2, "Update count should be 2");
    
    lle_term_cleanup(&term);
}

// Test 5: Scroll region management
TEST(scroll_region) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    const lle_term_state_t *state = lle_term_get_state(&term);
    ASSERT(state != NULL, "Should get state");
    
    // Initially, scroll region should be full screen
    ASSERT(state->scroll_top == 0, "Scroll top should be 0");
    ASSERT(state->scroll_bottom == state->rows - 1, 
           "Scroll bottom should be rows-1");
    
    // Update scroll region
    lle_term_update_scroll_region(&term, 2, state->rows - 3);
    ASSERT(state->scroll_top == 2, "Scroll top should be 2");
    ASSERT(state->scroll_bottom == state->rows - 3, 
           "Scroll bottom should be rows-3");
    
    lle_term_cleanup(&term);
}

// Test 6: Raw mode enter/exit
TEST(raw_mode) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    ASSERT(!term.raw_mode_active, "Should not be in raw mode initially");
    
    int result = lle_term_enter_raw_mode(&term);
    ASSERT(result == LLE_TERM_OK, "Should enter raw mode successfully");
    ASSERT(term.raw_mode_active, "Should be in raw mode");
    
    // Double enter should be harmless
    result = lle_term_enter_raw_mode(&term);
    ASSERT(result == LLE_TERM_OK, "Should handle double enter");
    
    result = lle_term_exit_raw_mode(&term);
    ASSERT(result == LLE_TERM_OK, "Should exit raw mode successfully");
    ASSERT(!term.raw_mode_active, "Should not be in raw mode");
    
    lle_term_cleanup(&term);
}

// Test 7: Capability detection
TEST(capability_detection) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    const lle_term_capabilities_t *caps = lle_term_get_capabilities(&term);
    ASSERT(caps != NULL, "Should get capabilities");
    ASSERT(caps->type != 0, "Should detect terminal type");
    ASSERT(caps->detection_time_ms < 100, 
           "Detection should complete within 100ms");
    
    // Print detected capabilities for manual verification
    printf("\n  Detected terminal type: ");
    switch (caps->type) {
        case LLE_TERM_TYPE_XTERM: printf("XTERM"); break;
        case LLE_TERM_TYPE_XTERM_256: printf("XTERM_256"); break;
        case LLE_TERM_TYPE_KONSOLE: printf("KONSOLE"); break;
        case LLE_TERM_TYPE_GNOME: printf("GNOME"); break;
        case LLE_TERM_TYPE_ALACRITTY: printf("ALACRITTY"); break;
        case LLE_TERM_TYPE_KITTY: printf("KITTY"); break;
        default: printf("UNKNOWN"); break;
    }
    printf("\n  Color: %s, 256-color: %s, Truecolor: %s\n",
           caps->has_color ? "YES" : "NO",
           caps->has_256_color ? "YES" : "NO",
           caps->has_true_color ? "YES" : "NO");
    printf("  Unicode: %s, Mouse: %s, Bracketed paste: %s\n",
           caps->has_unicode ? "YES" : "NO",
           caps->has_mouse ? "YES" : "NO",
           caps->has_bracketed_paste ? "YES" : "NO");
    
    lle_term_cleanup(&term);
}

// Test 8: Error string conversion
TEST(error_strings) {
    const char *str = lle_term_error_string(LLE_TERM_OK);
    ASSERT(str != NULL && strlen(str) > 0, "Should have error string for OK");
    
    str = lle_term_error_string(LLE_TERM_ERR_NULL_PTR);
    ASSERT(str != NULL && strlen(str) > 0, 
           "Should have error string for NULL_PTR");
    
    str = lle_term_error_string(LLE_TERM_ERR_NOT_TTY);
    ASSERT(str != NULL && strlen(str) > 0, 
           "Should have error string for NOT_TTY");
    
    str = lle_term_error_string(-9999);
    ASSERT(str != NULL, "Should have fallback for unknown error");
}

// Test 9: Performance - state updates
TEST(performance_state_updates) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    // Perform 10,000 state updates
    const int iterations = 10000;
    for (int i = 0; i < iterations; i++) {
        lle_term_update_cursor(&term, i % 24, i % 80);
    }
    
    // Check performance
    uint64_t avg_time_ns = term.total_update_time_ns / term.total_updates;
    ASSERT(avg_time_ns < 100000, "Average update time should be <100μs");
    
    printf("\n  Performance: %lu updates, avg %lu ns/update", 
           (unsigned long)term.total_updates,
           (unsigned long)avg_time_ns);
    
    lle_term_cleanup(&term);
}

// Test 10: Window size handling
TEST(window_size) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    lle_term_init(&term, STDIN_FILENO, STDOUT_FILENO);
    
    const lle_term_state_t *state = lle_term_get_state(&term);
    ASSERT(state != NULL, "Should get state");
    
    uint16_t old_rows = state->rows;
    uint16_t old_cols = state->cols;
    
    printf("\n  Current size: %dx%d", old_cols, old_rows);
    
    // Note: We can't actually resize the terminal in this test,
    // but we verify the size was detected
    ASSERT(old_rows >= 24 && old_rows <= 200, 
           "Rows should be reasonable (24-200)");
    ASSERT(old_cols >= 80 && old_cols <= 500, 
           "Cols should be reasonable (80-500)");
    
    lle_term_cleanup(&term);
}

int main(void) {
    printf("LLE Terminal Abstraction Layer Tests\n");
    printf("=====================================\n\n");
    
    // Check if running in a terminal
    if (!isatty(STDIN_FILENO)) {
        printf("ERROR: Not running in a terminal (stdin not a tty)\n");
        printf("Please run this test in an interactive terminal.\n");
        return 1;
    }
    
    // Run all tests
    run_test_init_cleanup();
    run_test_null_pointer_handling();
    run_test_double_init_prevention();
    run_test_cursor_update();
    run_test_scroll_region();
    run_test_raw_mode();
    run_test_capability_detection();
    run_test_error_strings();
    run_test_performance_state_updates();
    run_test_window_size();
    
    // Summary
    printf("\n=====================================\n");
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

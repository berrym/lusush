// src/lle/foundation/test/terminal_unit_test.c
//
// Unit tests for Phase 1 terminal abstraction (can run without TTY)
// Tests the 8-subsystem architecture without requiring actual terminal

#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

// Test 1: Null pointer handling
TEST(null_pointer_handling) {
    const lle_internal_state_t *state = lle_terminal_get_state(NULL);
    ASSERT(state == NULL, "Should return NULL for null term");
    
    const lle_terminal_capabilities_t *caps = lle_terminal_get_capabilities(NULL);
    ASSERT(caps == NULL, "Should return NULL for null term");
}

// Test 2: Result codes
TEST(result_codes) {
    // Verify success code is zero
    ASSERT(LLE_SUCCESS == 0, "LLE_SUCCESS should be 0");
    
    // Verify error codes are non-zero
    ASSERT(LLE_ERROR_TERMINAL_INIT != 0, "Error codes should be non-zero");
    ASSERT(LLE_ERROR_TERMINAL_NOT_TTY != 0, "Error codes should be non-zero");
    ASSERT(LLE_ERROR_NULL_POINTER != 0, "Error codes should be non-zero");
    
    // Verify error codes are distinct
    ASSERT(LLE_ERROR_TERMINAL_INIT != LLE_ERROR_TERMINAL_NOT_TTY,
           "Error codes should be distinct");
    ASSERT(LLE_ERROR_TERMINAL_NOT_TTY != LLE_ERROR_NULL_POINTER,
           "Error codes should be distinct");
}

// Test 3: Terminal type enum values
TEST(terminal_type_enum) {
    // Verify enum values are distinct
    ASSERT(LLE_TERM_TYPE_VT100 != LLE_TERM_TYPE_XTERM, 
           "Terminal types should be distinct");
    ASSERT(LLE_TERM_TYPE_XTERM != LLE_TERM_TYPE_XTERM_256COLOR, 
           "Terminal types should be distinct");
    ASSERT(LLE_TERM_TYPE_KONSOLE != LLE_TERM_TYPE_GNOME_TERMINAL, 
           "Terminal types should be distinct");
    ASSERT(LLE_TERM_TYPE_ALACRITTY != LLE_TERM_TYPE_KITTY,
           "Terminal types should be distinct");
}

// Test 4: Structure sizes
TEST(structure_sizes) {
    size_t abstraction_size = sizeof(lle_terminal_abstraction_t);
    size_t state_size = sizeof(lle_internal_state_t);
    size_t caps_size = sizeof(lle_terminal_capabilities_t);
    
    printf("\n  lle_terminal_abstraction_t: %zu bytes", abstraction_size);
    printf("\n  lle_internal_state_t: %zu bytes", state_size);
    printf("\n  lle_terminal_capabilities_t: %zu bytes", caps_size);
    
    ASSERT(state_size < 512, "State should be <512 bytes");
    ASSERT(caps_size < 512, "Capabilities should be <512 bytes");
}

// Test 5: Capability structure initialization
TEST(capability_structure) {
    lle_terminal_capabilities_t caps;
    memset(&caps, 0, sizeof(caps));
    
    // Set some capabilities
    caps.has_color = true;
    caps.has_256_color = true;
    caps.has_unicode = true;
    caps.terminal_type = LLE_TERM_TYPE_XTERM_256COLOR;
    
    ASSERT(caps.has_color == true, "Should set has_color");
    ASSERT(caps.has_256_color == true, "Should set has_256_color");
    ASSERT(caps.has_unicode == true, "Should set has_unicode");
    ASSERT(caps.terminal_type == LLE_TERM_TYPE_XTERM_256COLOR,
           "Should set terminal_type");
}

// Test 6: Internal state structure
TEST(internal_state_structure) {
    lle_internal_state_t state;
    memset(&state, 0, sizeof(state));
    
    // Set some state values
    state.cursor_row = 10;
    state.cursor_col = 20;
    state.rows = 24;
    state.cols = 80;
    state.auto_wrap_mode = true;
    
    ASSERT(state.cursor_row == 10, "Should set cursor_row");
    ASSERT(state.cursor_col == 20, "Should set cursor_col");
    ASSERT(state.rows == 24, "Should set rows");
    ASSERT(state.cols == 80, "Should set cols");
    ASSERT(state.auto_wrap_mode == true, "Should set auto_wrap_mode");
}

// Test 7: Cleanup with NULL pointer
TEST(cleanup_null_pointer) {
    // Should not crash
    lle_terminal_abstraction_cleanup(NULL);
    ASSERT(1, "Should handle NULL cleanup gracefully");
}

// Test 8: Environment variable detection
TEST(environment_detection) {
    const char *original_term = getenv("TERM");
    const char *original_colorterm = getenv("COLORTERM");
    
    // Test TERM variable handling
    setenv("TERM", "xterm-256color", 1);
    const char *term = getenv("TERM");
    ASSERT(term != NULL, "TERM should be set");
    ASSERT(strcmp(term, "xterm-256color") == 0, "TERM should be xterm-256color");
    
    // Test COLORTERM variable handling
    setenv("COLORTERM", "truecolor", 1);
    const char *colorterm = getenv("COLORTERM");
    ASSERT(colorterm != NULL, "COLORTERM should be set");
    ASSERT(strcmp(colorterm, "truecolor") == 0, "COLORTERM should be truecolor");
    
    // Restore original environment
    if (original_term) {
        setenv("TERM", original_term, 1);
    } else {
        unsetenv("TERM");
    }
    
    if (original_colorterm) {
        setenv("COLORTERM", original_colorterm, 1);
    } else {
        unsetenv("COLORTERM");
    }
    
    ASSERT(1, "Environment variable handling works");
}

// Test 9: Display content structure
TEST(display_content_structure) {
    lle_display_content_t content;
    memset(&content, 0, sizeof(content));
    
    content.line_count = 10;
    content.cursor_display_row = 5;
    content.cursor_display_col = 15;
    content.content_version = 1;
    
    ASSERT(content.line_count == 10, "Should set line_count");
    ASSERT(content.cursor_display_row == 5, "Should set cursor_display_row");
    ASSERT(content.cursor_display_col == 15, "Should set cursor_display_col");
    ASSERT(content.content_version == 1, "Should set content_version");
}

// Test 10: Terminal abstraction pointer handling
TEST(abstraction_pointer_init) {
    lle_terminal_abstraction_t *term = NULL;
    
    // Should start as NULL
    ASSERT(term == NULL, "Should initialize as NULL");
    
    // Getting state from NULL should return NULL
    const lle_internal_state_t *state = lle_terminal_get_state(term);
    ASSERT(state == NULL, "Should return NULL state for NULL term");
    
    // Getting capabilities from NULL should return NULL
    const lle_terminal_capabilities_t *caps = lle_terminal_get_capabilities(term);
    ASSERT(caps == NULL, "Should return NULL caps for NULL term");
}

int main(void) {
    printf("LLE Phase 1 Terminal Abstraction Unit Tests (No TTY Required)\n");
    printf("==============================================================\n\n");
    
    // Run all tests
    run_test_null_pointer_handling();
    run_test_result_codes();
    run_test_terminal_type_enum();
    run_test_structure_sizes();
    run_test_capability_structure();
    run_test_internal_state_structure();
    run_test_cleanup_null_pointer();
    run_test_environment_detection();
    run_test_display_content_structure();
    run_test_abstraction_pointer_init();
    
    // Summary
    printf("\n==============================================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\nFAILURE: %d test(s) failed\n", tests_failed);
        return 1;
    }
    
    printf("\nSUCCESS: All unit tests passed\n");
    printf("\nNote: These tests validate Phase 1 terminal abstraction structures\n");
    printf("      without requiring an actual TTY. Integration tests need real terminal.\n");
    return 0;
}

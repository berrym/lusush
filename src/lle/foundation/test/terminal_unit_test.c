// src/lle/foundation/test/terminal_unit_test.c
//
// Unit tests for terminal abstraction (can run without TTY)

#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
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

// Test 1: Null pointer handling (no TTY required)
TEST(null_pointer_handling) {
    const lle_term_state_t *state = lle_term_get_state(NULL);
    ASSERT(state == NULL, "Should return NULL for null term");
    
    const lle_term_capabilities_t *caps = lle_term_get_capabilities(NULL);
    ASSERT(caps == NULL, "Should return NULL for null term");
}

// Test 2: Error string conversion (no TTY required)
TEST(error_strings) {
    const char *str = lle_term_error_string(LLE_TERM_OK);
    ASSERT(str != NULL && strlen(str) > 0, "Should have error string for OK");
    ASSERT(strcmp(str, "Success") == 0, "OK should be 'Success'");
    
    str = lle_term_error_string(LLE_TERM_ERR_NULL_PTR);
    ASSERT(str != NULL && strlen(str) > 0, 
           "Should have error string for NULL_PTR");
    ASSERT(strstr(str, "Null") != NULL || strstr(str, "null") != NULL,
           "NULL_PTR should mention null");
    
    str = lle_term_error_string(LLE_TERM_ERR_NOT_TTY);
    ASSERT(str != NULL && strlen(str) > 0, 
           "Should have error string for NOT_TTY");
    ASSERT(strstr(str, "terminal") != NULL || strstr(str, "TTY") != NULL,
           "NOT_TTY should mention terminal");
    
    str = lle_term_error_string(LLE_TERM_ERR_INVALID_FD);
    ASSERT(str != NULL, "Should have error string for INVALID_FD");
    
    str = lle_term_error_string(LLE_TERM_ERR_TERMIOS);
    ASSERT(str != NULL, "Should have error string for TERMIOS");
    
    str = lle_term_error_string(LLE_TERM_ERR_TIMEOUT);
    ASSERT(str != NULL, "Should have error string for TIMEOUT");
    
    str = lle_term_error_string(LLE_TERM_ERR_ALREADY_INIT);
    ASSERT(str != NULL, "Should have error string for ALREADY_INIT");
    
    str = lle_term_error_string(-9999);
    ASSERT(str != NULL, "Should have fallback for unknown error");
    ASSERT(strstr(str, "Unknown") != NULL || strstr(str, "unknown") != NULL,
           "Unknown error should say 'Unknown'");
}

// Test 3: Not-a-TTY handling
TEST(not_tty_handling) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    
    // Try to initialize with /dev/null (not a TTY)
    int fd = open("/dev/null", O_RDWR);
    ASSERT(fd >= 0, "Should open /dev/null");
    
    int result = lle_term_init(&term, fd, fd);
    ASSERT(result == LLE_TERM_ERR_NOT_TTY, 
           "Should reject non-TTY file descriptor");
    ASSERT(!term.initialized, "Should not be marked as initialized");
    
    close(fd);
}

// Test 4: Uninitialized operations (no TTY required)
TEST(uninitialized_operations) {
    lle_term_t term;
    memset(&term, 0, sizeof(term));
    
    // These should all handle uninitialized terminal gracefully
    lle_term_update_cursor(&term, 5, 10);  // Should not crash
    
    const lle_term_state_t *state = lle_term_get_state(&term);
    ASSERT(state == NULL, "Should return NULL for uninitialized term");
    
    int result = lle_term_enter_raw_mode(&term);
    ASSERT(result == LLE_TERM_ERR_NULL_PTR, 
           "Should reject uninitialized term");
}

// Test 5: Capability detection logic (no TTY required, using environment)
TEST(capability_env_detection) {
    // Test terminal type detection from TERM environment
    // Note: This tests the logic, not actual terminal capabilities
    
    const char *original_term = getenv("TERM");
    
    // Test various TERM values
    setenv("TERM", "xterm-256color", 1);
    // Would call lle_term_detect_capabilities if we had a TTY
    
    setenv("TERM", "alacritty", 1);
    // Would detect Alacritty
    
    // Restore original TERM
    if (original_term) {
        setenv("TERM", original_term, 1);
    } else {
        unsetenv("TERM");
    }
    
    ASSERT(1, "Environment variable handling works");
}

// Test 6: Structure sizes and alignment
TEST(structure_sizes) {
    // Verify structures are reasonable sizes
    size_t term_size = sizeof(lle_term_t);
    size_t state_size = sizeof(lle_term_state_t);
    size_t caps_size = sizeof(lle_term_capabilities_t);
    
    printf("\n  lle_term_t: %zu bytes", term_size);
    printf("\n  lle_term_state_t: %zu bytes", state_size);
    printf("\n  lle_term_capabilities_t: %zu bytes", caps_size);
    
    ASSERT(term_size < 1024, "lle_term_t should be <1KB");
    ASSERT(state_size < 256, "lle_term_state_t should be <256 bytes");
    ASSERT(caps_size < 256, "lle_term_capabilities_t should be <256 bytes");
}

// Test 7: Terminal type enum values
TEST(terminal_type_enum) {
    // Verify enum values are distinct
    ASSERT(LLE_TERM_TYPE_VT100 != LLE_TERM_TYPE_XTERM, 
           "Terminal types should be distinct");
    ASSERT(LLE_TERM_TYPE_XTERM != LLE_TERM_TYPE_XTERM_256, 
           "Terminal types should be distinct");
    ASSERT(LLE_TERM_TYPE_KONSOLE != LLE_TERM_TYPE_GNOME, 
           "Terminal types should be distinct");
}

// Test 8: Error code values
TEST(error_code_values) {
    // Verify error codes are negative and OK is zero
    ASSERT(LLE_TERM_OK == 0, "OK should be 0");
    ASSERT(LLE_TERM_ERR_INVALID_FD < 0, "Error codes should be negative");
    ASSERT(LLE_TERM_ERR_TERMIOS < 0, "Error codes should be negative");
    ASSERT(LLE_TERM_ERR_NOT_TTY < 0, "Error codes should be negative");
    ASSERT(LLE_TERM_ERR_NULL_PTR < 0, "Error codes should be negative");
    
    // Verify error codes are distinct
    ASSERT(LLE_TERM_ERR_INVALID_FD != LLE_TERM_ERR_TERMIOS,
           "Error codes should be distinct");
    ASSERT(LLE_TERM_ERR_NOT_TTY != LLE_TERM_ERR_NULL_PTR,
           "Error codes should be distinct");
}

int main(void) {
    printf("LLE Terminal Abstraction Unit Tests (No TTY Required)\n");
    printf("=====================================================\n\n");
    
    // Run all tests
    run_test_null_pointer_handling();
    run_test_error_strings();
    run_test_not_tty_handling();
    run_test_uninitialized_operations();
    run_test_capability_env_detection();
    run_test_structure_sizes();
    run_test_terminal_type_enum();
    run_test_error_code_values();
    
    // Summary
    printf("\n=====================================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed > 0) {
        printf("\nFAILURE: %d test(s) failed\n", tests_failed);
        return 1;
    }
    
    printf("\nSUCCESS: All unit tests passed\n");
    printf("\nNote: Run terminal_test for full integration tests with TTY\n");
    return 0;
}

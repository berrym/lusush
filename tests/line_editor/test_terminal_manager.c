/**
 * @file test_terminal_manager.c
 * @brief Test suite for LLE terminal manager structures
 *
 * Tests for terminal manager structures, validation functions, and
 * capability management. Ensures proper structure definitions and
 * basic functionality without requiring actual terminal operations.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/terminal_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test) \
    do { \
        printf("Running test_%s...", #test); \
        fflush(stdout); \
        test_##test(); \
        tests_run++; \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

// =====================================
// LLE-009: Terminal Manager Structure Tests
// =====================================

// Test: Terminal manager structure initialization
LLE_TEST(terminal_manager_structure) {
    lle_terminal_manager_t tm;
    
    // Initialize structure with known values
    tm.geometry.width = 80;
    tm.geometry.height = 24;
    tm.geometry.prompt_width = 0;
    tm.geometry.prompt_height = 1;
    
    tm.capabilities = LLE_TERM_CAP_COLORS | LLE_TERM_CAP_CURSOR_MOVEMENT;
    tm.capabilities_initialized = true;
    tm.in_raw_mode = false;
    tm.geometry_valid = true;
    tm.stdin_fd = 0;
    tm.stdout_fd = 1;
    tm.stderr_fd = 2;
    
    // Verify fields are accessible and hold correct values
    LLE_ASSERT_EQ(tm.geometry.width, 80);
    LLE_ASSERT_EQ(tm.geometry.height, 24);
    LLE_ASSERT_EQ(tm.capabilities & LLE_TERM_CAP_COLORS, LLE_TERM_CAP_COLORS);
    LLE_ASSERT_EQ(tm.capabilities & LLE_TERM_CAP_CURSOR_MOVEMENT, LLE_TERM_CAP_CURSOR_MOVEMENT);
    LLE_ASSERT(tm.capabilities_initialized == true);
    LLE_ASSERT(tm.in_raw_mode == false);
    LLE_ASSERT(tm.geometry_valid == true);
    LLE_ASSERT_EQ(tm.stdin_fd, 0);
    LLE_ASSERT_EQ(tm.stdout_fd, 1);
    LLE_ASSERT_EQ(tm.stderr_fd, 2);
}

// Test: Terminal state structure
LLE_TEST(terminal_state_structure) {
    lle_terminal_state_t state;
    
    // Initialize structure
    state.original_termios = NULL;
    state.was_raw_mode = false;
    state.needs_restoration = false;
    state.original_flags = 0;
    
    // Verify fields are accessible
    LLE_ASSERT(state.original_termios == NULL);
    LLE_ASSERT(state.was_raw_mode == false);
    LLE_ASSERT(state.needs_restoration == false);
    LLE_ASSERT_EQ(state.original_flags, 0);
}

// Test: Terminal capability flags
LLE_TEST(terminal_capability_flags) {
    // Test individual capability flags
    uint32_t caps = 0;
    
    caps |= LLE_TERM_CAP_COLORS;
    LLE_ASSERT(caps & LLE_TERM_CAP_COLORS);
    LLE_ASSERT(!(caps & LLE_TERM_CAP_MOUSE));
    
    caps |= LLE_TERM_CAP_CURSOR_MOVEMENT;
    LLE_ASSERT(caps & LLE_TERM_CAP_COLORS);
    LLE_ASSERT(caps & LLE_TERM_CAP_CURSOR_MOVEMENT);
    
    caps |= LLE_TERM_CAP_UTF8;
    LLE_ASSERT(caps & LLE_TERM_CAP_UTF8);
    
    // Test clearing flags
    caps &= ~LLE_TERM_CAP_COLORS;
    LLE_ASSERT(!(caps & LLE_TERM_CAP_COLORS));
    LLE_ASSERT(caps & LLE_TERM_CAP_CURSOR_MOVEMENT);
    LLE_ASSERT(caps & LLE_TERM_CAP_UTF8);
}

// Test: Terminal initialization result codes
LLE_TEST(terminal_init_result_codes) {
    // Test that result codes are properly defined
    LLE_ASSERT_EQ(LLE_TERM_INIT_SUCCESS, 0);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_INVALID_FD < 0);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_NOT_TTY < 0);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_CAPABILITIES < 0);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_RAW_MODE < 0);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_GEOMETRY < 0);
    
    // Test that all error codes are different
    LLE_ASSERT(LLE_TERM_INIT_ERROR_INVALID_FD != LLE_TERM_INIT_ERROR_NOT_TTY);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_NOT_TTY != LLE_TERM_INIT_ERROR_CAPABILITIES);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_CAPABILITIES != LLE_TERM_INIT_ERROR_RAW_MODE);
    LLE_ASSERT(LLE_TERM_INIT_ERROR_RAW_MODE != LLE_TERM_INIT_ERROR_GEOMETRY);
}

// Test: Structure size and alignment
LLE_TEST(structure_properties) {
    // Check that structures have reasonable sizes
    LLE_ASSERT(sizeof(lle_terminal_manager_t) > 0);
    LLE_ASSERT(sizeof(lle_terminal_state_t) > 0);
    LLE_ASSERT(sizeof(lle_terminal_capabilities_t) > 0);
    LLE_ASSERT(sizeof(lle_terminal_init_result_t) > 0);
    
    // Structures should not be unreasonably large
    LLE_ASSERT(sizeof(lle_terminal_manager_t) < 10240); // Less than 10KB
    LLE_ASSERT(sizeof(lle_terminal_state_t) < 1024);    // Less than 1KB
    
    // Check that bool fields work correctly
    lle_terminal_manager_t tm;
    tm.capabilities_initialized = true;
    tm.in_raw_mode = false;
    tm.geometry_valid = true;
    
    LLE_ASSERT(tm.capabilities_initialized == true);
    LLE_ASSERT(tm.in_raw_mode == false);
    LLE_ASSERT(tm.geometry_valid == true);
}

// Test: Default terminal manager state
LLE_TEST(terminal_manager_defaults) {
    lle_terminal_manager_t tm = {0}; // Zero-initialize
    
    // Check default values after zero-initialization
    LLE_ASSERT_EQ(tm.geometry.width, 0);
    LLE_ASSERT_EQ(tm.geometry.height, 0);
    LLE_ASSERT_EQ(tm.capabilities, 0);
    LLE_ASSERT(tm.capabilities_initialized == false);
    LLE_ASSERT(tm.in_raw_mode == false);
    LLE_ASSERT(tm.geometry_valid == false);
    LLE_ASSERT_EQ(tm.stdin_fd, 0);
    LLE_ASSERT_EQ(tm.stdout_fd, 0);
    LLE_ASSERT_EQ(tm.stderr_fd, 0);
}

// Test: Capability combinations
LLE_TEST(capability_combinations) {
    lle_terminal_manager_t tm = {0};
    
    // Test basic terminal capabilities
    tm.capabilities = LLE_TERM_CAP_COLORS | LLE_TERM_CAP_CURSOR_MOVEMENT;
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_COLORS);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_CURSOR_MOVEMENT);
    LLE_ASSERT(!(tm.capabilities & LLE_TERM_CAP_MOUSE));
    
    // Test advanced terminal capabilities
    tm.capabilities |= LLE_TERM_CAP_256_COLORS | LLE_TERM_CAP_UTF8;
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_256_COLORS);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_UTF8);
    
    // Test full capability set
    tm.capabilities = LLE_TERM_CAP_COLORS | LLE_TERM_CAP_CURSOR_MOVEMENT | 
                     LLE_TERM_CAP_CLEAR_SCREEN | LLE_TERM_CAP_ALTERNATE_SCREEN |
                     LLE_TERM_CAP_MOUSE | LLE_TERM_CAP_BRACKETED_PASTE |
                     LLE_TERM_CAP_UTF8 | LLE_TERM_CAP_256_COLORS;
    
    // Verify all capabilities are set
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_COLORS);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_CURSOR_MOVEMENT);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_CLEAR_SCREEN);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_ALTERNATE_SCREEN);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_MOUSE);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_BRACKETED_PASTE);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_UTF8);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_256_COLORS);
}

// Test: File descriptor management
LLE_TEST(file_descriptor_management) {
    lle_terminal_manager_t tm;
    
    // Test standard file descriptors
    tm.stdin_fd = 0;
    tm.stdout_fd = 1;
    tm.stderr_fd = 2;
    
    LLE_ASSERT_EQ(tm.stdin_fd, 0);
    LLE_ASSERT_EQ(tm.stdout_fd, 1);
    LLE_ASSERT_EQ(tm.stderr_fd, 2);
    
    // Test custom file descriptors
    tm.stdin_fd = 10;
    tm.stdout_fd = 11;
    tm.stderr_fd = 12;
    
    LLE_ASSERT_EQ(tm.stdin_fd, 10);
    LLE_ASSERT_EQ(tm.stdout_fd, 11);
    LLE_ASSERT_EQ(tm.stderr_fd, 12);
    
    // Test invalid file descriptors
    tm.stdin_fd = -1;
    tm.stdout_fd = -1;
    tm.stderr_fd = -1;
    
    LLE_ASSERT_EQ(tm.stdin_fd, -1);
    LLE_ASSERT_EQ(tm.stdout_fd, -1);
    LLE_ASSERT_EQ(tm.stderr_fd, -1);
}

// Test: Complex terminal manager setup
LLE_TEST(complex_terminal_setup) {
    lle_terminal_manager_t tm;
    
    // Setup a realistic terminal configuration
    tm.geometry.width = 120;
    tm.geometry.height = 30;
    tm.geometry.prompt_width = 25;
    tm.geometry.prompt_height = 2;
    
    tm.capabilities = LLE_TERM_CAP_COLORS | LLE_TERM_CAP_CURSOR_MOVEMENT | 
                     LLE_TERM_CAP_CLEAR_SCREEN | LLE_TERM_CAP_UTF8 | 
                     LLE_TERM_CAP_256_COLORS;
    
    tm.capabilities_initialized = true;
    tm.in_raw_mode = true;
    tm.geometry_valid = true;
    tm.stdin_fd = 0;
    tm.stdout_fd = 1;
    tm.stderr_fd = 2;
    
    tm.saved_state.was_raw_mode = false;
    tm.saved_state.needs_restoration = true;
    tm.saved_state.original_flags = 0x12345678;
    
    // Verify the complete setup
    LLE_ASSERT(lle_validate_terminal_geometry(&tm.geometry));
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_COLORS);
    LLE_ASSERT(tm.capabilities & LLE_TERM_CAP_UTF8);
    LLE_ASSERT(tm.capabilities_initialized);
    LLE_ASSERT(tm.in_raw_mode);
    LLE_ASSERT(tm.geometry_valid);
    LLE_ASSERT(tm.saved_state.needs_restoration);
    LLE_ASSERT_EQ(tm.saved_state.original_flags, 0x12345678);
}

// Main test runner
int main(void) {
    printf("Running LLE Terminal Manager Tests\n");
    printf("==================================\n\n");
    
    RUN_TEST(terminal_manager_structure);
    RUN_TEST(terminal_state_structure);
    RUN_TEST(terminal_capability_flags);
    RUN_TEST(terminal_init_result_codes);
    RUN_TEST(structure_properties);
    RUN_TEST(terminal_manager_defaults);
    RUN_TEST(capability_combinations);
    RUN_TEST(file_descriptor_management);
    RUN_TEST(complex_terminal_setup);
    
    printf("\n==================================\n");
    printf("Tests completed: %d/%d passed\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
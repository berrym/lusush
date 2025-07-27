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
#include <unistd.h>

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

// =====================================
// LLE-010: Terminal Initialization Tests
// =====================================

// Test: Terminal initialization basic functionality
LLE_TEST(terminal_init_basic) {
    lle_terminal_manager_t tm;
    
    // Initialize terminal manager
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    
    // Should succeed or fail gracefully (depending on environment)
    if (result == LLE_TERM_INIT_SUCCESS) {
        // Verify initialization succeeded
        LLE_ASSERT(tm.stdin_fd == STDIN_FILENO);
        LLE_ASSERT(tm.stdout_fd == STDOUT_FILENO);
        LLE_ASSERT(tm.stderr_fd == STDERR_FILENO);
        LLE_ASSERT(tm.capabilities_initialized);
        LLE_ASSERT(lle_terminal_manager_is_valid(&tm));
        
        // Clean up
        LLE_ASSERT(lle_terminal_cleanup(&tm));
    } else {
        // Should be a valid error code
        LLE_ASSERT(result < 0);
    }
}

// Test: Terminal initialization with NULL pointer
LLE_TEST(terminal_init_null_pointer) {
    lle_terminal_init_result_t result = lle_terminal_init(NULL);
    LLE_ASSERT_EQ(result, LLE_TERM_INIT_ERROR_INVALID_FD);
}

// Test: Terminal cleanup with NULL pointer
LLE_TEST(terminal_cleanup_null_pointer) {
    // Should not crash
    LLE_ASSERT(!lle_terminal_cleanup(NULL));
}

// Test: Terminal size detection
LLE_TEST(terminal_get_size) {
    lle_terminal_manager_t tm = {0};
    tm.stdout_fd = STDOUT_FILENO;
    
    // Try to get terminal size
    bool result = lle_terminal_get_size(&tm);
    
    if (result) {
        // If successful, geometry should be reasonable
        LLE_ASSERT(tm.geometry.width > 0);
        LLE_ASSERT(tm.geometry.height > 0);
        LLE_ASSERT(tm.geometry_valid);
    }
    // If failed, should have default values
    // This test passes regardless since terminal size detection
    // may fail in CI environments
}

// Test: Terminal size with NULL pointer
LLE_TEST(terminal_get_size_null_pointer) {
    LLE_ASSERT(!lle_terminal_get_size(NULL));
}

// Test: Capability detection
LLE_TEST(terminal_detect_capabilities) {
    lle_terminal_manager_t tm = {0};
    
    // Initialize terminal manager properly first
    lle_terminal_init_result_t result = lle_terminal_init(&tm);
    LLE_ASSERT(result == LLE_TERM_INIT_SUCCESS || result == LLE_TERM_INIT_ERROR_NOT_TTY);
    
    if (result == LLE_TERM_INIT_SUCCESS) {
        LLE_ASSERT(tm.capabilities_initialized);
        LLE_ASSERT(tm.termcap_initialized);
        
        // Clean up
        lle_terminal_cleanup(&tm);
    }
    
    // Test direct capability detection (should fail without termcap init)
    lle_terminal_manager_t tm2 = {0};
    tm2.stdout_fd = STDOUT_FILENO;
    LLE_ASSERT(!lle_terminal_detect_capabilities(&tm2));
}

// Test: Capability detection with NULL pointer
LLE_TEST(terminal_detect_capabilities_null) {
    LLE_ASSERT(!lle_terminal_detect_capabilities(NULL));
}

// Test: Has capability function
LLE_TEST(terminal_has_capability) {
    lle_terminal_manager_t tm = {0};
    
    // Without initialization, should return false
    LLE_ASSERT(!lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS));
    
    // After capability detection
    tm.capabilities = LLE_TERM_CAP_COLORS | LLE_TERM_CAP_UTF8;
    tm.capabilities_initialized = true;
    
    LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_COLORS));
    LLE_ASSERT(lle_terminal_has_capability(&tm, LLE_TERM_CAP_UTF8));
    LLE_ASSERT(!lle_terminal_has_capability(&tm, LLE_TERM_CAP_MOUSE));
    
    // Test NULL pointer
    LLE_ASSERT(!lle_terminal_has_capability(NULL, LLE_TERM_CAP_COLORS));
}

// Test: Terminal manager validation
LLE_TEST(terminal_manager_validation) {
    lle_terminal_manager_t tm;
    
    // Test NULL pointer
    LLE_ASSERT(!lle_terminal_manager_is_valid(NULL));
    
    // Test valid terminal manager
    memset(&tm, 0, sizeof(tm));
    tm.stdin_fd = 0;
    tm.stdout_fd = 1;
    tm.stderr_fd = 2;
    tm.geometry.width = 80;
    tm.geometry.height = 24;
    tm.geometry_valid = true;
    
    LLE_ASSERT(lle_terminal_manager_is_valid(&tm));
    
    // Test invalid file descriptors
    tm.stdin_fd = -1;
    LLE_ASSERT(!lle_terminal_manager_is_valid(&tm));
    
    // Reset and test invalid geometry
    tm.stdin_fd = 0;
    tm.geometry.width = 5; // Too small
    LLE_ASSERT(!lle_terminal_manager_is_valid(&tm));
    
    // Test raw mode inconsistency
    tm.geometry.width = 80;
    tm.in_raw_mode = true;
    tm.saved_state.needs_restoration = false; // Inconsistent
    LLE_ASSERT(!lle_terminal_manager_is_valid(&tm));
}

// Test: Raw mode enter/exit (simulation)
LLE_TEST(terminal_raw_mode_simulation) {
    lle_terminal_manager_t tm = {0};
    tm.stdin_fd = STDIN_FILENO;
    
    // Test entering raw mode when not a TTY (should fail gracefully)
    // In CI environments, this may not be a TTY
    bool enter_result = lle_terminal_enter_raw_mode(&tm);
    
    if (enter_result) {
        // If we successfully entered raw mode
        LLE_ASSERT(tm.in_raw_mode);
        LLE_ASSERT(tm.saved_state.needs_restoration);
        LLE_ASSERT(tm.saved_state.original_termios != NULL);
        
        // Test exiting raw mode
        LLE_ASSERT(lle_terminal_exit_raw_mode(&tm));
        LLE_ASSERT(!tm.in_raw_mode);
        LLE_ASSERT(!tm.saved_state.needs_restoration);
        
        // Clean up
        if (tm.saved_state.original_termios) {
            free(tm.saved_state.original_termios);
        }
    }
    // If raw mode failed, that's okay in CI environments
}

// Test: Raw mode with NULL pointer
LLE_TEST(terminal_raw_mode_null_pointer) {
    LLE_ASSERT(!lle_terminal_enter_raw_mode(NULL));
    LLE_ASSERT(!lle_terminal_exit_raw_mode(NULL));
}

// Test: Capabilities string generation
LLE_TEST(terminal_capabilities_string) {
    lle_terminal_manager_t tm = {0};
    char buffer[256];
    
    // Test with uninitialized capabilities
    int result = lle_terminal_get_capabilities_string(&tm, buffer, sizeof(buffer));
    LLE_ASSERT(result > 0);
    LLE_ASSERT(strstr(buffer, "not initialized") != NULL);
    
    // Test with some capabilities
    tm.capabilities = LLE_TERM_CAP_COLORS | LLE_TERM_CAP_UTF8;
    tm.capabilities_initialized = true;
    
    result = lle_terminal_get_capabilities_string(&tm, buffer, sizeof(buffer));
    LLE_ASSERT(result > 0);
    LLE_ASSERT(strstr(buffer, "colors") != NULL);
    LLE_ASSERT(strstr(buffer, "utf8") != NULL);
    
    // Test NULL pointer
    LLE_ASSERT(lle_terminal_get_capabilities_string(NULL, buffer, sizeof(buffer)) == -1);
    LLE_ASSERT(lle_terminal_get_capabilities_string(&tm, NULL, sizeof(buffer)) == -1);
    LLE_ASSERT(lle_terminal_get_capabilities_string(&tm, buffer, 0) == -1);
}

// Test: Complete initialization and cleanup cycle
LLE_TEST(terminal_init_cleanup_cycle) {
    lle_terminal_manager_t tm;
    
    // Initialize
    lle_terminal_init_result_t init_result = lle_terminal_init(&tm);
    
    if (init_result == LLE_TERM_INIT_SUCCESS) {
        // Verify proper initialization
        LLE_ASSERT(lle_terminal_manager_is_valid(&tm));
        LLE_ASSERT(tm.capabilities_initialized);
        
        // Test cleanup
        LLE_ASSERT(lle_terminal_cleanup(&tm));
        
        // After cleanup, structure should be zeroed
        LLE_ASSERT_EQ(tm.capabilities, 0);
        LLE_ASSERT(!tm.capabilities_initialized);
        LLE_ASSERT(!tm.in_raw_mode);
        LLE_ASSERT(tm.saved_state.original_termios == NULL);
    }
    // If initialization failed (e.g., in CI), that's acceptable
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
    
    // LLE-010: Terminal initialization tests
    RUN_TEST(terminal_init_basic);
    RUN_TEST(terminal_init_null_pointer);
    RUN_TEST(terminal_cleanup_null_pointer);
    RUN_TEST(terminal_get_size);
    RUN_TEST(terminal_get_size_null_pointer);
    RUN_TEST(terminal_detect_capabilities);
    RUN_TEST(terminal_detect_capabilities_null);
    RUN_TEST(terminal_has_capability);
    RUN_TEST(terminal_manager_validation);
    RUN_TEST(terminal_raw_mode_simulation);
    RUN_TEST(terminal_raw_mode_null_pointer);
    RUN_TEST(terminal_capabilities_string);
    RUN_TEST(terminal_init_cleanup_cycle);
    
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
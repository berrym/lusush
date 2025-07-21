/**
 * @file test_cursor_math.c
 * @brief Test suite for LLE cursor mathematics structures
 *
 * Tests for cursor position structures and validation functions.
 * Ensures mathematical correctness and proper structure validation.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/cursor_math.h"
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
            fprintf(stderr, "ASSERTION FAILED: %s (%zu) != %s (%zu) at %s:%d\n", \
                    #actual, (size_t)(actual), #expected, (size_t)(expected), \
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
// LLE-006: Cursor Position Structure Tests
// =====================================

// Test: Cursor position structure initialization
LLE_TEST(cursor_position_structure) {
    lle_cursor_position_t pos;
    
    // Initialize structure
    pos.absolute_row = 5;
    pos.absolute_col = 10;
    pos.relative_row = 2;
    pos.relative_col = 8;
    pos.at_boundary = false;
    pos.valid = true;
    
    // Verify fields are accessible and hold correct values
    LLE_ASSERT_EQ(pos.absolute_row, 5);
    LLE_ASSERT_EQ(pos.absolute_col, 10);
    LLE_ASSERT_EQ(pos.relative_row, 2);
    LLE_ASSERT_EQ(pos.relative_col, 8);
    LLE_ASSERT(pos.at_boundary == false);
    LLE_ASSERT(pos.valid == true);
}

// Test: Terminal geometry structure initialization
LLE_TEST(terminal_geometry_structure) {
    lle_terminal_geometry_t geometry;
    
    // Initialize structure
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 1;
    
    // Verify fields are accessible and hold correct values
    LLE_ASSERT_EQ(geometry.width, 80);
    LLE_ASSERT_EQ(geometry.height, 24);
    LLE_ASSERT_EQ(geometry.prompt_width, 12);
    LLE_ASSERT_EQ(geometry.prompt_height, 1);
}

// Test: Terminal geometry validation with valid values
LLE_TEST(terminal_geometry_validation_valid) {
    lle_terminal_geometry_t geometry;
    
    // Test valid standard terminal
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 1;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
    
    // Test valid wide terminal
    geometry.width = 120;
    geometry.height = 30;
    geometry.prompt_width = 20;
    geometry.prompt_height = 2;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
    
    // Test minimum valid terminal
    geometry.width = LLE_MIN_TERMINAL_WIDTH;
    geometry.height = LLE_MIN_TERMINAL_HEIGHT;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
    
    // Test maximum valid terminal
    geometry.width = LLE_MAX_TERMINAL_WIDTH;
    geometry.height = LLE_MAX_TERMINAL_HEIGHT;
    geometry.prompt_width = LLE_MAX_TERMINAL_WIDTH;
    geometry.prompt_height = LLE_MAX_TERMINAL_HEIGHT;
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
}

// Test: Terminal geometry validation with invalid values
LLE_TEST(terminal_geometry_validation_invalid) {
    lle_terminal_geometry_t geometry;
    
    // Test NULL pointer
    LLE_ASSERT(!lle_validate_terminal_geometry(NULL));
    
    // Test width too small
    geometry.width = LLE_MIN_TERMINAL_WIDTH - 1;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test width too large
    geometry.width = LLE_MAX_TERMINAL_WIDTH + 1;
    geometry.height = 24;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test height too small
    geometry.width = 80;
    geometry.height = LLE_MIN_TERMINAL_HEIGHT - 1;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test height too large
    geometry.width = 80;
    geometry.height = LLE_MAX_TERMINAL_HEIGHT + 1;
    geometry.prompt_width = 0;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test prompt width exceeds terminal width
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 81;
    geometry.prompt_height = 1;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
    
    // Test prompt height exceeds terminal height
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 25;
    LLE_ASSERT(!lle_validate_terminal_geometry(&geometry));
}

// Test: Terminal geometry initialization
LLE_TEST(terminal_geometry_initialization) {
    lle_terminal_geometry_t geometry;
    
    // Initialize with defaults
    lle_init_terminal_geometry(&geometry);
    
    // Verify default values
    LLE_ASSERT_EQ(geometry.width, LLE_DEFAULT_TERMINAL_WIDTH);
    LLE_ASSERT_EQ(geometry.height, LLE_DEFAULT_TERMINAL_HEIGHT);
    LLE_ASSERT_EQ(geometry.prompt_width, 0);
    LLE_ASSERT_EQ(geometry.prompt_height, 1);
    
    // Verify initialized geometry is valid
    LLE_ASSERT(lle_validate_terminal_geometry(&geometry));
}

// Test: Terminal geometry initialization with NULL pointer
LLE_TEST(terminal_geometry_initialization_null) {
    // Should not crash
    lle_init_terminal_geometry(NULL);
}

// Test: Constants are properly defined
LLE_TEST(constants_defined) {
    // Check that all constants are defined and reasonable
    LLE_ASSERT(LLE_MAX_TERMINAL_WIDTH > 0);
    LLE_ASSERT(LLE_MAX_TERMINAL_HEIGHT > 0);
    LLE_ASSERT(LLE_MIN_TERMINAL_WIDTH > 0);
    LLE_ASSERT(LLE_MIN_TERMINAL_HEIGHT > 0);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_WIDTH > 0);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_HEIGHT > 0);
    
    // Check relationships between constants
    LLE_ASSERT(LLE_MIN_TERMINAL_WIDTH <= LLE_DEFAULT_TERMINAL_WIDTH);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_WIDTH <= LLE_MAX_TERMINAL_WIDTH);
    LLE_ASSERT(LLE_MIN_TERMINAL_HEIGHT <= LLE_DEFAULT_TERMINAL_HEIGHT);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_HEIGHT <= LLE_MAX_TERMINAL_HEIGHT);
    
    // Check reasonable values
    LLE_ASSERT(LLE_MIN_TERMINAL_WIDTH >= 10);
    LLE_ASSERT(LLE_MIN_TERMINAL_HEIGHT >= 3);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_WIDTH >= 80);
    LLE_ASSERT(LLE_DEFAULT_TERMINAL_HEIGHT >= 24);
}

// Test: Structure size and alignment
LLE_TEST(structure_properties) {
    // Check that structures have reasonable sizes
    LLE_ASSERT(sizeof(lle_cursor_position_t) > 0);
    LLE_ASSERT(sizeof(lle_terminal_geometry_t) > 0);
    
    // Structures should not be unreasonably large
    LLE_ASSERT(sizeof(lle_cursor_position_t) < 1024);
    LLE_ASSERT(sizeof(lle_terminal_geometry_t) < 1024);
    
    // Check that bool fields work correctly
    lle_cursor_position_t pos;
    pos.at_boundary = true;
    pos.valid = false;
    LLE_ASSERT(pos.at_boundary == true);
    LLE_ASSERT(pos.valid == false);
}

// Test: Mathematical invariants documentation
LLE_TEST(mathematical_invariants) {
    lle_cursor_position_t pos;
    lle_terminal_geometry_t geometry;
    
    // Test documented invariant: absolute_row >= relative_row
    pos.absolute_row = 10;
    pos.relative_row = 5;
    LLE_ASSERT(pos.absolute_row >= pos.relative_row);
    
    // Test boundary conditions
    pos.absolute_row = 0;
    pos.relative_row = 0;
    LLE_ASSERT(pos.absolute_row >= pos.relative_row);
    
    // Test geometry constraints
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 12;
    geometry.prompt_height = 2;
    
    // Documented invariants
    LLE_ASSERT(geometry.prompt_width <= geometry.width);
    LLE_ASSERT(geometry.prompt_height <= geometry.height);
}

// Main test runner
int main(void) {
    printf("Running LLE Cursor Math Tests\n");
    printf("=============================\n\n");
    
    RUN_TEST(cursor_position_structure);
    RUN_TEST(terminal_geometry_structure);
    RUN_TEST(terminal_geometry_validation_valid);
    RUN_TEST(terminal_geometry_validation_invalid);
    RUN_TEST(terminal_geometry_initialization);
    RUN_TEST(terminal_geometry_initialization_null);
    RUN_TEST(constants_defined);
    RUN_TEST(structure_properties);
    RUN_TEST(mathematical_invariants);
    
    printf("\n=============================\n");
    printf("Tests completed: %d/%d passed\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
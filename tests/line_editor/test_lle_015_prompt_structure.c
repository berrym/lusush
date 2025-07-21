/**
 * @file test_lle_015_prompt_structure.c
 * @brief Test suite for LLE-015: Prompt Structure Definition
 *
 * Tests the basic prompt structure functionality including initialization,
 * validation, and basic operations on the lle_prompt_t structure.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include "../src/line_editor/prompt.h"
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

#define LLE_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            fprintf(stderr, "ASSERTION FAILED: %s is not NULL at %s:%d\n", \
                    #ptr, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while(0)

// Test prompt structure definition and basic properties
LLE_TEST(prompt_structure_definition) {
    lle_prompt_t prompt;
    
    // Test that we can create the structure
    // This validates the structure is properly defined
    (void)prompt; // Suppress unused variable warning
    
    printf("Running test_prompt_structure_definition... ");
    printf("PASSED\n");
}

// Test prompt structure size and alignment
LLE_TEST(prompt_structure_properties) {
    // Verify the structure has reasonable size
    size_t size = sizeof(lle_prompt_t);
    LLE_ASSERT(size > 0);
    LLE_ASSERT(size < 1024); // Should be reasonable size
    
    // Test that all expected fields exist by checking offsets
    lle_prompt_t prompt;
    
    // These should compile without error, proving fields exist
    prompt.text = NULL;
    prompt.length = 0;
    prompt.has_ansi_codes = false;
    prompt.lines = NULL;
    prompt.line_count = 0;
    prompt.capacity = 0;
    
    // Geometry field should exist and be the right type
    prompt.geometry.width = 0;
    prompt.geometry.height = 0;
    prompt.geometry.last_line_width = 0;
    
    printf("Running test_prompt_structure_properties... ");
    printf("PASSED\n");
}

// Test prompt initialization
LLE_TEST(prompt_init_basic) {
    lle_prompt_t prompt;
    
    // Initialize with garbage data first
    memset(&prompt, 0xFF, sizeof(prompt));
    
    // Should successfully initialize
    bool result = lle_prompt_init(&prompt);
    LLE_ASSERT(result == true);
    
    // Check that fields are properly initialized
    LLE_ASSERT_NULL(prompt.text);
    LLE_ASSERT_EQ(prompt.length, 0);
    LLE_ASSERT_EQ(prompt.has_ansi_codes, false);
    LLE_ASSERT_NULL(prompt.lines);
    LLE_ASSERT_EQ(prompt.line_count, 0);
    LLE_ASSERT_EQ(prompt.capacity, 0);
    
    // Geometry should be zeroed
    LLE_ASSERT_EQ(prompt.geometry.width, 0);
    LLE_ASSERT_EQ(prompt.geometry.height, 0);
    LLE_ASSERT_EQ(prompt.geometry.last_line_width, 0);
    
    printf("Running test_prompt_init_basic... ");
    printf("PASSED\n");
}

// Test prompt initialization with NULL pointer
LLE_TEST(prompt_init_null_pointer) {
    bool result = lle_prompt_init(NULL);
    LLE_ASSERT(result == false);
    
    printf("Running test_prompt_init_null_pointer... ");
    printf("PASSED\n");
}

// Test prompt creation
LLE_TEST(prompt_create_basic) {
    lle_prompt_t *prompt = lle_prompt_create(4);
    LLE_ASSERT_NOT_NULL(prompt);
    
    // Should be properly initialized
    LLE_ASSERT_NULL(prompt->text);
    LLE_ASSERT_EQ(prompt->length, 0);
    LLE_ASSERT_EQ(prompt->has_ansi_codes, false);
    LLE_ASSERT_EQ(prompt->line_count, 0);
    LLE_ASSERT_EQ(prompt->capacity, 4);
    
    // Lines array should be allocated
    LLE_ASSERT_NOT_NULL(prompt->lines);
    
    lle_prompt_destroy(prompt);
    
    printf("Running test_prompt_create_basic... ");
    printf("PASSED\n");
}

// Test prompt creation with minimum capacity
LLE_TEST(prompt_create_minimum_capacity) {
    lle_prompt_t *prompt = lle_prompt_create(0);
    LLE_ASSERT_NOT_NULL(prompt);
    
    // Should enforce minimum capacity of 1
    LLE_ASSERT_EQ(prompt->capacity, 1);
    LLE_ASSERT_NOT_NULL(prompt->lines);
    
    lle_prompt_destroy(prompt);
    
    printf("Running test_prompt_create_minimum_capacity... ");
    printf("PASSED\n");
}

// Test prompt validation
LLE_TEST(prompt_validate_basic) {
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    
    // Newly initialized prompt should be valid
    bool result = lle_prompt_validate(&prompt);
    LLE_ASSERT(result == true);
    
    printf("Running test_prompt_validate_basic... ");
    printf("PASSED\n");
}

// Test prompt validation with NULL pointer
LLE_TEST(prompt_validate_null_pointer) {
    bool result = lle_prompt_validate(NULL);
    LLE_ASSERT(result == false);
    
    printf("Running test_prompt_validate_null_pointer... ");
    printf("PASSED\n");
}

// Test prompt clear
LLE_TEST(prompt_clear_basic) {
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    
    // Set some test data
    prompt.length = 10;
    prompt.has_ansi_codes = true;
    prompt.line_count = 2;
    prompt.geometry.width = 5;
    
    bool result = lle_prompt_clear(&prompt);
    LLE_ASSERT(result == true);
    
    // Should be cleared but structure intact
    LLE_ASSERT_EQ(prompt.length, 0);
    LLE_ASSERT_EQ(prompt.has_ansi_codes, false);
    LLE_ASSERT_EQ(prompt.line_count, 0);
    LLE_ASSERT_EQ(prompt.geometry.width, 0);
    
    printf("Running test_prompt_clear_basic... ");
    printf("PASSED\n");
}

// Test prompt clear with NULL pointer
LLE_TEST(prompt_clear_null_pointer) {
    bool result = lle_prompt_clear(NULL);
    LLE_ASSERT(result == false);
    
    printf("Running test_prompt_clear_null_pointer... ");
    printf("PASSED\n");
}

// Test prompt destroy with NULL (should not crash)
LLE_TEST(prompt_destroy_null_pointer) {
    // Should not crash with NULL pointer
    lle_prompt_destroy(NULL);
    
    printf("Running test_prompt_destroy_null_pointer... ");
    printf("PASSED\n");
}

// Test getter functions with uninitialized prompt
LLE_TEST(prompt_getters_empty) {
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    
    // All getters should return 0 for empty prompt
    LLE_ASSERT_EQ(lle_prompt_get_width(&prompt), 0);
    LLE_ASSERT_EQ(lle_prompt_get_height(&prompt), 0);
    LLE_ASSERT_EQ(lle_prompt_get_last_line_width(&prompt), 0);
    LLE_ASSERT_EQ(lle_prompt_has_ansi(&prompt), false);
    
    printf("Running test_prompt_getters_empty... ");
    printf("PASSED\n");
}

// Test getter functions with NULL pointer
LLE_TEST(prompt_getters_null_pointer) {
    LLE_ASSERT_EQ(lle_prompt_get_width(NULL), 0);
    LLE_ASSERT_EQ(lle_prompt_get_height(NULL), 0);
    LLE_ASSERT_EQ(lle_prompt_get_last_line_width(NULL), 0);
    LLE_ASSERT_EQ(lle_prompt_has_ansi(NULL), false);
    LLE_ASSERT_NULL(lle_prompt_get_line(NULL, 0));
    LLE_ASSERT_EQ(lle_prompt_copy_plain_text(NULL, NULL, 0), 0);
    
    printf("Running test_prompt_getters_null_pointer... ");
    printf("PASSED\n");
}

// Test integration with cursor math geometry
LLE_TEST(prompt_geometry_integration) {
    lle_prompt_t prompt;
    lle_prompt_init(&prompt);
    
    // Set geometry values
    prompt.geometry.width = 10;
    prompt.geometry.height = 2;
    prompt.geometry.last_line_width = 5;
    
    // Getters should return geometry values
    LLE_ASSERT_EQ(lle_prompt_get_width(&prompt), 10);
    LLE_ASSERT_EQ(lle_prompt_get_height(&prompt), 2);
    LLE_ASSERT_EQ(lle_prompt_get_last_line_width(&prompt), 5);
    
    printf("Running test_prompt_geometry_integration... ");
    printf("PASSED\n");
}

int main(void) {
    printf("Running LLE-015 Prompt Structure Definition Tests...\n");
    printf("==================================================\n\n");
    
    // Structure definition tests
    test_prompt_structure_definition();
    test_prompt_structure_properties();
    
    // Initialization tests
    test_prompt_init_basic();
    test_prompt_init_null_pointer();
    
    // Creation tests
    test_prompt_create_basic();
    test_prompt_create_minimum_capacity();
    
    // Validation tests
    test_prompt_validate_basic();
    test_prompt_validate_null_pointer();
    
    // Clear tests
    test_prompt_clear_basic();
    test_prompt_clear_null_pointer();
    
    // Destroy tests
    test_prompt_destroy_null_pointer();
    
    // Getter tests
    test_prompt_getters_empty();
    test_prompt_getters_null_pointer();
    
    // Integration tests
    test_prompt_geometry_integration();
    
    printf("\n==================================================\n");
    printf("All LLE-015 Prompt Structure Definition Tests Passed!\n");
    printf("Prompt structure properly defined and validated.\n");
    
    return 0;
}
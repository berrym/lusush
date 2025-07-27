/**
 * @file test_lle_035_syntax_highlighting_framework.c
 * @brief Tests for LLE-035: Syntax Highlighting Framework
 * 
 * Tests the syntax highlighting framework including structure validation,
 * basic highlighting functionality, shell syntax detection, configuration
 * management, and integration readiness for display systems.
 * 
 * @author Lusush Development Team
 * @version 1.0.0
 * @date 2024
 */

#include "test_framework.h"
#include "syntax.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// Test Helper Functions
// ============================================================================

/**
 * @brief Verify syntax region properties
 */
__attribute__((unused))
static bool verify_region(const lle_syntax_region_t *region,
                         size_t expected_start,
                         size_t expected_length,
                         lle_syntax_type_t expected_type) {
    if (!region) return false;
    
    return region->start == expected_start &&
           region->length == expected_length &&
           region->type == expected_type;
}

/**
 * @brief Count regions of a specific type
 */
static size_t count_regions_of_type(const lle_syntax_region_t *regions,
                                   size_t count,
                                   lle_syntax_type_t type) {
    if (!regions) return 0;
    
    size_t type_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (regions[i].type == type) {
            type_count++;
        }
    }
    return type_count;
}

/**
 * @brief Find first region of specific type
 */
static const lle_syntax_region_t *find_region_of_type(const lle_syntax_region_t *regions,
                                                     size_t count,
                                                     lle_syntax_type_t type) {
    if (!regions) return NULL;
    
    for (size_t i = 0; i < count; i++) {
        if (regions[i].type == type) {
            return &regions[i];
        }
    }
    return NULL;
}

// ============================================================================
// Structure and Initialization Tests
// ============================================================================

LLE_TEST(syntax_highlighter_creation) {
    printf("Testing syntax highlighter creation... ");
    
    // Test successful creation
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Verify initialization
    LLE_ASSERT(lle_syntax_is_enabled(highlighter));
    LLE_ASSERT(!lle_syntax_is_dirty(highlighter));
    
    // Cleanup
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(syntax_highlighter_initialization) {
    printf("Testing syntax highlighter initialization... ");
    
    lle_syntax_highlighter_t highlighter;
    
    // Test successful initialization
    LLE_ASSERT(lle_syntax_init(&highlighter, true, 1000));
    LLE_ASSERT(lle_syntax_is_enabled(&highlighter));
    
    // Test configuration - disable all highlighting types
    LLE_ASSERT(lle_syntax_configure_shell(&highlighter, false));
    LLE_ASSERT(lle_syntax_configure_strings(&highlighter, false));
    LLE_ASSERT(lle_syntax_configure_variables(&highlighter, false));
    LLE_ASSERT(lle_syntax_configure_comments(&highlighter, false));
    LLE_ASSERT(!lle_syntax_is_enabled(&highlighter));
    
    // Cleanup
    lle_syntax_cleanup(&highlighter);
    printf("PASSED\n");
}

LLE_TEST(syntax_highlighter_null_handling) {
    printf("Testing syntax highlighter NULL parameter handling... ");
    
    // Test NULL parameter handling
    LLE_ASSERT(!lle_syntax_init(NULL, true, 0));
    
    lle_syntax_destroy(NULL); // Should not crash
    lle_syntax_cleanup(NULL); // Should not crash
    
    // Test functions with NULL highlighter
    LLE_ASSERT(!lle_syntax_highlight_text(NULL, "test", 4));
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(NULL, 0), LLE_SYNTAX_NORMAL);
    LLE_ASSERT_NULL(lle_syntax_get_regions(NULL, NULL));
    LLE_ASSERT(!lle_syntax_is_enabled(NULL));
    LLE_ASSERT(!lle_syntax_is_dirty(NULL));
    
    printf("PASSED\n");
}

// ============================================================================
// Basic Highlighting Tests
// ============================================================================

LLE_TEST(basic_text_highlighting) {
    printf("Testing basic text highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test highlighting simple text
    const char *text = "echo hello world";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    // Get regions
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    LLE_ASSERT(region_count > 0);
    
    // Should have at least one region (the keyword "echo")
    const lle_syntax_region_t *keyword_region = find_region_of_type(regions, region_count, LLE_SYNTAX_KEYWORD);
    LLE_ASSERT_NOT_NULL(keyword_region);
    LLE_ASSERT_EQ(keyword_region->start, 0);
    LLE_ASSERT_EQ(keyword_region->length, 4); // "echo"
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(empty_text_highlighting) {
    printf("Testing empty text highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test empty text
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, "", 0));
    
    size_t region_count = 0;
    lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_EQ(region_count, 0);
    
    // Test whitespace-only text
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, "   \t  \n  ", 9));
    lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_EQ(region_count, 0); // Whitespace should not create regions
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(position_based_type_queries) {
    printf("Testing position-based syntax type queries... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    const char *text = "echo 'hello world'";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    // Test type at different positions
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 0), LLE_SYNTAX_KEYWORD); // 'e' in "echo"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 2), LLE_SYNTAX_KEYWORD); // 'h' in "echo"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 4), LLE_SYNTAX_NORMAL);  // space
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 5), LLE_SYNTAX_STRING);  // opening quote
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 10), LLE_SYNTAX_STRING); // 'o' in "hello"
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Shell Syntax Tests
// ============================================================================

LLE_TEST(shell_keyword_highlighting) {
    printf("Testing shell keyword highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test various shell keywords
    const char *text = "if test -f file; then echo found; fi";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have keyword regions for "if", "then", "fi"
    size_t keyword_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_KEYWORD);
    LLE_ASSERT(keyword_count >= 3);
    
    // Verify specific keywords
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 0), LLE_SYNTAX_KEYWORD); // "if"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 2), LLE_SYNTAX_NORMAL);  // space after "if"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 3), LLE_SYNTAX_KEYWORD); // "test" (keyword)
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(shell_string_highlighting) {
    printf("Testing shell string highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test different quote types
    const char *text = "echo 'single' \"double\" `backtick`";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have string regions
    size_t string_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_STRING);
    size_t command_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    size_t keyword_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_KEYWORD);
    LLE_ASSERT(string_count >= 2); // single and double quotes
    LLE_ASSERT(command_count >= 1); // backtick command
    LLE_ASSERT(keyword_count >= 1); // "echo" is now a keyword
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(shell_variable_highlighting) {
    printf("Testing shell variable highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test different variable syntaxes
    const char *text = "echo $VAR ${HOME} $? $1";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have variable regions
    size_t variable_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_VARIABLE);
    LLE_ASSERT(variable_count >= 4); // $VAR, ${HOME}, $?, $1
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(shell_comment_highlighting) {
    printf("Testing shell comment highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test comments
    const char *text = "echo hello # this is a comment\n# full line comment";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have comment regions
    size_t comment_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMENT);
    LLE_ASSERT(comment_count >= 2);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(shell_operator_highlighting) {
    printf("Testing shell operator highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test operators
    const char *text = "ls | grep test && echo found || echo not found";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have operator regions
    size_t operator_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR);
    LLE_ASSERT(operator_count >= 3); // |, &&, ||
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Configuration Tests
// ============================================================================

LLE_TEST(syntax_configuration_management) {
    printf("Testing syntax configuration management... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test initial state
    LLE_ASSERT(lle_syntax_is_enabled(highlighter));
    
    // Disable all syntax highlighting
    LLE_ASSERT(lle_syntax_configure_shell(highlighter, false));
    LLE_ASSERT(lle_syntax_configure_strings(highlighter, false));
    LLE_ASSERT(lle_syntax_configure_variables(highlighter, false));
    
    // Should be dirty after configuration changes
    LLE_ASSERT(lle_syntax_is_dirty(highlighter));
    
    // Re-enable
    LLE_ASSERT(lle_syntax_configure_shell(highlighter, true));
    LLE_ASSERT(lle_syntax_is_enabled(highlighter));
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(syntax_max_length_configuration) {
    printf("Testing syntax max length configuration... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Set max length
    LLE_ASSERT(lle_syntax_set_max_length(highlighter, 10));
    
    // Test with text longer than max length
    const char *text = "echo this is a very long command that exceeds the limit";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    // Should still work but may be limited
    size_t region_count = 0;
    lle_syntax_get_regions(highlighter, &region_count);
    // Just verify it doesn't crash and produces some result
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Utility Function Tests
// ============================================================================

LLE_TEST(syntax_type_name_utility) {
    printf("Testing syntax type name utility... ");
    
    // Test all syntax types have names
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_NORMAL), "normal");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_KEYWORD), "keyword");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_COMMAND), "command");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_STRING), "string");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_COMMENT), "comment");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_NUMBER), "number");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_OPERATOR), "operator");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_VARIABLE), "variable");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_PATH), "path");
    LLE_ASSERT_STR_EQ(lle_syntax_type_name(LLE_SYNTAX_ERROR), "error");
    
    // Test invalid type
    LLE_ASSERT_STR_EQ(lle_syntax_type_name((lle_syntax_type_t)999), "unknown");
    
    printf("PASSED\n");
}

LLE_TEST(syntax_region_clearing) {
    printf("Testing syntax region clearing... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Add some highlighting
    const char *text = "echo hello world";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT(region_count > 0);
    
    // Clear regions
    LLE_ASSERT(lle_syntax_clear_regions(highlighter));
    
    // Should have no regions now
    lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_EQ(region_count, 0);
    LLE_ASSERT(!lle_syntax_is_dirty(highlighter));
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Integration and Edge Case Tests
// ============================================================================

LLE_TEST(complex_shell_command_highlighting) {
    printf("Testing complex shell command highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Complex shell command with multiple elements
    const char *text = "if [ -f \"$HOME/.bashrc\" ]; then source ~/.bashrc && export PATH=$PATH:/usr/local/bin; fi";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    LLE_ASSERT(region_count > 5); // Should have multiple regions
    
    // Verify different types are present
    LLE_ASSERT(count_regions_of_type(regions, region_count, LLE_SYNTAX_KEYWORD) > 0);
    LLE_ASSERT(count_regions_of_type(regions, region_count, LLE_SYNTAX_STRING) > 0);
    LLE_ASSERT(count_regions_of_type(regions, region_count, LLE_SYNTAX_VARIABLE) > 0);
    LLE_ASSERT(count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR) > 0);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(syntax_update_region_functionality) {
    printf("Testing syntax update region functionality... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    const char *text = "echo hello world";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    // Test incremental update (currently should fall back to full re-highlighting)
    LLE_ASSERT(lle_syntax_update_region(highlighter, text, strlen(text), 5, 5));
    
    size_t region_count = 0;
    lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT(region_count > 0);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(syntax_highlighting_with_special_characters) {
    printf("Testing syntax highlighting with special characters... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Text with Unicode and special characters
    const char *text = "echo 'héllo wörld' | grep 'tëst'";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    LLE_ASSERT(region_count > 0);
    
    // Should handle special characters without crashing
    LLE_ASSERT(count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND) > 0);
    LLE_ASSERT(count_regions_of_type(regions, region_count, LLE_SYNTAX_STRING) > 0);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("=== LLE-035 Syntax Highlighting Framework Tests ===\n");
    
    // Structure and initialization tests
    test_syntax_highlighter_creation();
    test_syntax_highlighter_initialization();
    test_syntax_highlighter_null_handling();
    
    // Basic highlighting tests
    test_basic_text_highlighting();
    test_empty_text_highlighting();
    test_position_based_type_queries();
    
    // Shell syntax tests
    test_shell_keyword_highlighting();
    test_shell_string_highlighting();
    test_shell_variable_highlighting();
    test_shell_comment_highlighting();
    test_shell_operator_highlighting();
    
    // Configuration tests
    test_syntax_configuration_management();
    test_syntax_max_length_configuration();
    
    // Utility function tests
    test_syntax_type_name_utility();
    test_syntax_region_clearing();
    
    // Integration and edge case tests
    test_complex_shell_command_highlighting();
    test_syntax_update_region_functionality();
    test_syntax_highlighting_with_special_characters();
    
    printf("\n=== All LLE-035 Tests Completed Successfully ===\n");
    return 0;
}
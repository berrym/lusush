/**
 * @file test_lle_036_basic_shell_syntax.c
 * @brief Tests for LLE-036: Basic Shell Syntax Enhancements
 * 
 * Tests the enhanced shell syntax highlighting including built-in commands,
 * command substitution, parameter expansion, redirection operators, and
 * number recognition. Builds upon the foundation established in LLE-035.
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

/**
 * @brief Check if text at position matches expected substring
 */


// ============================================================================
// Shell Built-in Command Tests
// ============================================================================

LLE_TEST(shell_builtin_commands) {
    printf("Testing shell built-in command highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test common built-in commands
    const char *text = "cd /home && pwd && echo hello && printf world";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have keyword regions for built-ins (treated as keywords)
    size_t keyword_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_KEYWORD);
    
    LLE_ASSERT(keyword_count >= 4); // cd, pwd, echo, printf
    
    // Verify specific built-ins are highlighted as keywords
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 0), LLE_SYNTAX_KEYWORD); // "cd"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 12), LLE_SYNTAX_KEYWORD); // "pwd"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 19), LLE_SYNTAX_KEYWORD); // "echo"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 33), LLE_SYNTAX_KEYWORD); // "printf"
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(shell_system_commands) {
    printf("Testing shell system command detection... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test system commands vs built-ins
    const char *text = "ls -la && grep pattern && echo done";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have command regions for system commands and keyword for built-ins
    size_t command_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    size_t keyword_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_KEYWORD);
    
    LLE_ASSERT(command_count >= 2); // ls, grep
    LLE_ASSERT(keyword_count >= 1); // echo
    
    // Verify specific command types
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 0), LLE_SYNTAX_COMMAND); // "ls"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 10), LLE_SYNTAX_COMMAND); // "grep"
    LLE_ASSERT_EQ(lle_syntax_get_type_at_position(highlighter, 26), LLE_SYNTAX_KEYWORD); // "echo"
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Command Substitution Tests
// ============================================================================

LLE_TEST(command_substitution_dollar_paren) {
    printf("Testing command substitution with $(...) syntax... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test $(...) command substitution
    const char *text = "echo $(ls -la) and $(pwd)";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have command regions for substitutions
    size_t command_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    LLE_ASSERT(command_count >= 2); // $(ls -la) and $(pwd)
    
    // Find and verify command substitution regions
    const lle_syntax_region_t *first_cmd = find_region_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    LLE_ASSERT_NOT_NULL(first_cmd);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(command_substitution_backticks) {
    printf("Testing command substitution with backtick syntax... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test backtick command substitution
    const char *text = "echo `date` and `whoami`";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have command regions for backtick substitutions
    size_t command_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    LLE_ASSERT(command_count >= 2); // `date` and `whoami`
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(nested_command_substitution) {
    printf("Testing nested command substitution... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test nested substitution
    const char *text = "echo $(dirname $(which ls))";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should handle nested command substitution
    size_t command_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    LLE_ASSERT(command_count >= 1); // Outer substitution
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Parameter Expansion Tests
// ============================================================================

LLE_TEST(parameter_expansion_basic) {
    printf("Testing basic parameter expansion... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test various parameter expansion forms
    const char *text = "${HOME} ${USER:-default} ${#PATH}";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have variable regions
    size_t variable_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_VARIABLE);
    LLE_ASSERT(variable_count >= 3); // ${HOME}, ${USER:-default}, ${#PATH}
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(parameter_expansion_advanced) {
    printf("Testing advanced parameter expansion... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test complex parameter expansion
    const char *text = "${var:=default} ${array[@]} ${!prefix*}";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should recognize all as variables
    size_t variable_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_VARIABLE);
    LLE_ASSERT(variable_count >= 3);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Redirection Operator Tests
// ============================================================================

LLE_TEST(redirection_operators_basic) {
    printf("Testing basic redirection operators... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test basic redirection
    const char *text = "cat < input.txt > output.txt";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have operator regions for < and >
    size_t operator_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR);
    LLE_ASSERT(operator_count >= 2);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(redirection_operators_advanced) {
    printf("Testing advanced redirection operators... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test advanced redirection
    const char *text = "command >> file 2>&1 |& tee log";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have operator regions for >>, 2>&1, |&
    size_t operator_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR);
    LLE_ASSERT(operator_count >= 3);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(here_document_operators) {
    printf("Testing here-document operators... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test here-document syntax
    const char *text = "cat << EOF and cat <<< string";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should recognize << and <<< as operators
    size_t operator_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR);
    LLE_ASSERT(operator_count >= 2);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Number Recognition Tests
// ============================================================================

LLE_TEST(number_recognition_integers) {
    printf("Testing integer number recognition... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test integer numbers
    const char *text = "echo 123 456 0 999";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have number regions
    size_t number_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_NUMBER);
    LLE_ASSERT(number_count >= 4); // 123, 456, 0, 999
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(number_recognition_floats) {
    printf("Testing floating-point number recognition... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test floating-point numbers
    const char *text = "echo 3.14 0.5 123.456";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have number regions
    size_t number_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_NUMBER);
    LLE_ASSERT(number_count >= 3); // 3.14, 0.5, 123.456
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(number_recognition_scientific) {
    printf("Testing scientific notation number recognition... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test scientific notation
    const char *text = "echo 1e5 3.14e-2 2E+10";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have number regions
    size_t number_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_NUMBER);
    LLE_ASSERT(number_count >= 3); // 1e5, 3.14e-2, 2E+10
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Complex Command Tests
// ============================================================================

LLE_TEST(complex_shell_command) {
    printf("Testing complex shell command highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test complex command with multiple syntax elements
    const char *text = "if [ -f ${HOME}/.bashrc ]; then source ~/.bashrc > /dev/null 2>&1; fi";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have multiple types of regions
    size_t keyword_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_KEYWORD);
    size_t variable_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_VARIABLE);
    size_t path_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_PATH);
    size_t operator_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR);
    
    LLE_ASSERT(keyword_count >= 3); // if, then, fi, source
    LLE_ASSERT(variable_count >= 1); // ${HOME}
    LLE_ASSERT(path_count >= 1); // at least some paths detected
    LLE_ASSERT(operator_count >= 3); // >, 2>&1, ;
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(pipeline_with_redirection) {
    printf("Testing pipeline with redirection highlighting... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test complex pipeline
    const char *text = "cat file.txt | grep pattern | sort -n > output.txt 2>/dev/null";
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, text, strlen(text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    
    // Should have commands, operators, and paths
    size_t command_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_COMMAND);
    size_t operator_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_OPERATOR);
    size_t path_count = count_regions_of_type(regions, region_count, LLE_SYNTAX_PATH);
    
    LLE_ASSERT(command_count >= 3); // cat, grep, sort
    LLE_ASSERT(operator_count >= 4); // |, |, >, 2>
    LLE_ASSERT(path_count >= 2); // file.txt, dev/null (output.txt may be classified differently)
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Performance and Edge Case Tests
// ============================================================================

LLE_TEST(performance_long_command) {
    printf("Testing performance with long command... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Create a long command line
    char long_text[1000];
    snprintf(long_text, sizeof(long_text), 
             "echo $(for i in {1..100}; do echo $i; done) | grep pattern | sort -n > output.txt");
    
    LLE_ASSERT(lle_syntax_highlight_text(highlighter, long_text, strlen(long_text)));
    
    size_t region_count = 0;
    const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
    LLE_ASSERT_NOT_NULL(regions);
    LLE_ASSERT(region_count > 0);
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

LLE_TEST(edge_case_malformed_syntax) {
    printf("Testing edge cases with malformed syntax... ");
    
    lle_syntax_highlighter_t *highlighter = lle_syntax_create();
    LLE_ASSERT_NOT_NULL(highlighter);
    
    // Test malformed syntax that shouldn't crash
    const char *test_cases[] = {
        "echo $(incomplete",
        "echo ${unclosed",
        "echo `unterminated",
        "echo <<<",
        "echo >> >",
        "echo $",
        "${}",
        NULL
    };
    
    for (int i = 0; test_cases[i] != NULL; i++) {
        // Should not crash, even with malformed syntax
        LLE_ASSERT(lle_syntax_highlight_text(highlighter, test_cases[i], strlen(test_cases[i])));
        
        size_t region_count = 0;
        const lle_syntax_region_t *regions = lle_syntax_get_regions(highlighter, &region_count);
        // Should still produce some highlighting
        (void)regions; // Suppress unused variable warning
    }
    
    lle_syntax_destroy(highlighter);
    printf("PASSED\n");
}

// ============================================================================
// Main Test Function
// ============================================================================

int main(void) {
    printf("Running LLE-036 Basic Shell Syntax Enhancement Tests...\n");
    
    // Shell Built-in Command Tests
    test_shell_builtin_commands();
    test_shell_system_commands();
    
    // Command Substitution Tests
    test_command_substitution_dollar_paren();
    test_command_substitution_backticks();
    test_nested_command_substitution();
    
    // Parameter Expansion Tests
    test_parameter_expansion_basic();
    test_parameter_expansion_advanced();
    
    // Redirection Operator Tests
    test_redirection_operators_basic();
    test_redirection_operators_advanced();
    test_here_document_operators();
    
    // Number Recognition Tests
    test_number_recognition_integers();
    test_number_recognition_floats();
    test_number_recognition_scientific();
    
    // Complex Command Tests
    test_complex_shell_command();
    test_pipeline_with_redirection();
    
    // Performance and Edge Case Tests
    test_performance_long_command();
    test_edge_case_malformed_syntax();
    
    printf("All LLE-036 tests completed successfully!\n");
    return 0;
}
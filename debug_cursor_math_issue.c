/**
 * @file debug_cursor_math_issue.c
 * @brief Test program to debug why cursor math output still appears without LLE_DEBUG
 *
 * This program tests the cursor math debug output behavior to understand
 * why debug messages are appearing when LLE_DEBUG is not set.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include LLE headers
#include "src/line_editor/cursor_math.h"
#include "src/line_editor/text_buffer.h"

/**
 * Test the debug mode detection logic
 */
void test_debug_mode_detection(void) {
    printf("=== Debug Mode Detection Test ===\n");
    
    // Test current environment
    const char *debug_env = getenv("LLE_DEBUG");
    printf("LLE_DEBUG environment variable: '%s'\n", debug_env ? debug_env : "NULL");
    
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    printf("Debug mode evaluation: %s\n", debug_mode ? "TRUE" : "FALSE");
    
    // Test different environment values
    printf("\nTesting different LLE_DEBUG values:\n");
    
    const char *test_values[] = {"", "0", "1", "true", "false", "yes", "debug", NULL};
    for (int i = 0; test_values[i] != NULL; i++) {
        setenv("LLE_DEBUG", test_values[i], 1);
        const char *env = getenv("LLE_DEBUG");
        bool mode = env && (strcmp(env, "1") == 0 || strcmp(env, "true") == 0);
        printf("  LLE_DEBUG='%s' -> debug_mode=%s\n", test_values[i], mode ? "TRUE" : "FALSE");
    }
    
    // Restore original environment
    if (debug_env) {
        setenv("LLE_DEBUG", debug_env, 1);
    } else {
        unsetenv("LLE_DEBUG");
    }
    
    printf("\n");
}

/**
 * Test other environment variables that might affect debug output
 */
void test_other_debug_variables(void) {
    printf("=== Other Debug Variables Test ===\n");
    
    const char *debug_vars[] = {
        "LLE_DEBUG",
        "LLE_DEBUG_COMPLETION", 
        "LLE_DEBUG_CURSOR",
        "LLE_DEBUG_DISPLAY",
        "LLE_DEBUG_TERMINAL",
        "DEBUG",
        "VERBOSE",
        NULL
    };
    
    for (int i = 0; debug_vars[i] != NULL; i++) {
        const char *value = getenv(debug_vars[i]);
        printf("%s: '%s'\n", debug_vars[i], value ? value : "NULL");
    }
    
    printf("\n");
}

/**
 * Test cursor math function with controlled input
 */
void test_cursor_math_function(void) {
    printf("=== Cursor Math Function Test ===\n");
    
    // Create test components
    lle_text_buffer_t buffer;
    lle_terminal_geometry_t geometry;
    
    // Initialize buffer
    if (!lle_text_buffer_init(&buffer, 1024)) {
        printf("ERROR: Failed to initialize text buffer\n");
        return;
    }
    
    // Add test text
    const char *test_text = "echo test_completion";
    if (!lle_text_insert_at(&buffer, 0, test_text)) {
        printf("ERROR: Failed to insert test text\n");
        lle_text_buffer_clear(&buffer);
        return;
    }
    
    // Set cursor position
    buffer.cursor_pos = strlen(test_text);
    
    // Initialize geometry
    geometry.width = 80;
    geometry.height = 24;
    geometry.prompt_width = 77;  // Same as in the user's output
    
    printf("Before calling lle_calculate_cursor_position:\n");
    printf("  Buffer length: %zu\n", buffer.length);
    printf("  Cursor position: %zu\n", buffer.cursor_pos);
    printf("  Terminal width: %zu\n", geometry.width);
    printf("  Prompt width: %zu\n", geometry.prompt_width);
    printf("  LLE_DEBUG: '%s'\n", getenv("LLE_DEBUG") ? getenv("LLE_DEBUG") : "NULL");
    
    printf("\nCalling cursor math function (watch for debug output):\n");
    printf("--- START FUNCTION CALL ---\n");
    
    lle_cursor_position_t result = lle_calculate_cursor_position(&buffer, 77, &geometry);
    
    printf("--- END FUNCTION CALL ---\n");
    
    printf("Function result:\n");
    printf("  Valid: %s\n", result.valid ? "true" : "false");
    printf("  Relative row: %zu\n", result.relative_row);
    printf("  Relative col: %zu\n", result.relative_col);
    printf("  Absolute row: %zu\n", result.absolute_row);
    printf("  Absolute col: %zu\n", result.absolute_col);
    
    // Cleanup
    lle_text_buffer_clear(&buffer);
    
    printf("\n");
}

/**
 * Test direct debug mode simulation
 */
void test_debug_mode_simulation(void) {
    printf("=== Debug Mode Simulation Test ===\n");
    
    // Simulate the exact debug check from cursor_math.c
    const char *debug_env = getenv("LLE_DEBUG");
    bool debug_mode = debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0);
    
    printf("Current environment state:\n");
    printf("  debug_env pointer: %p\n", (void*)debug_env);
    printf("  debug_env value: '%s'\n", debug_env ? debug_env : "NULL");
    printf("  debug_mode result: %s\n", debug_mode ? "TRUE" : "FALSE");
    
    // Test the exact conditional logic
    if (debug_mode) {
        printf("DEBUG OUTPUT WOULD APPEAR: This is a test debug message\n");
    } else {
        printf("DEBUG OUTPUT SUPPRESSED: Debug mode is off\n");
    }
    
    printf("\n");
}

/**
 * Test if debug output is being forced elsewhere
 */
void test_forced_debug_output(void) {
    printf("=== Forced Debug Output Test ===\n");
    
    // Check if there are any global debug flags or forced output
    printf("Testing if debug output is being forced by other mechanisms...\n");
    
    // Temporarily disable all debug environment variables
    char *saved_debug = NULL;
    char *saved_debug_completion = NULL;
    char *saved_debug_cursor = NULL;
    
    const char *debug_val = getenv("LLE_DEBUG");
    if (debug_val) {
        saved_debug = strdup(debug_val);
        unsetenv("LLE_DEBUG");
    }
    
    debug_val = getenv("LLE_DEBUG_COMPLETION");
    if (debug_val) {
        saved_debug_completion = strdup(debug_val);
        unsetenv("LLE_DEBUG_COMPLETION");
    }
    
    debug_val = getenv("LLE_DEBUG_CURSOR");
    if (debug_val) {
        saved_debug_cursor = strdup(debug_val);
        unsetenv("LLE_DEBUG_CURSOR");
    }
    
    printf("All debug environment variables cleared\n");
    printf("LLE_DEBUG: '%s'\n", getenv("LLE_DEBUG") ? getenv("LLE_DEBUG") : "NULL");
    
    // Test cursor math with completely clean environment
    printf("Testing cursor math with clean environment...\n");
    test_cursor_math_function();
    
    // Restore environment variables
    if (saved_debug) {
        setenv("LLE_DEBUG", saved_debug, 1);
        free(saved_debug);
    }
    if (saved_debug_completion) {
        setenv("LLE_DEBUG_COMPLETION", saved_debug_completion, 1);
        free(saved_debug_completion);
    }
    if (saved_debug_cursor) {
        setenv("LLE_DEBUG_CURSOR", saved_debug_cursor, 1);
        free(saved_debug_cursor);
    }
    
    printf("Environment variables restored\n");
    printf("\n");
}

/**
 * Main test function
 */
int main(int argc, char *argv[]) {
    printf("=============================================\n");
    printf("Cursor Math Debug Output Investigation\n");
    printf("=============================================\n");
    printf("\nThis program tests why cursor math debug output\n");
    printf("appears even when LLE_DEBUG is not set.\n\n");
    
    test_other_debug_variables();
    test_debug_mode_detection();
    test_debug_mode_simulation();
    test_cursor_math_function();
    test_forced_debug_output();
    
    printf("=============================================\n");
    printf("Investigation Complete\n");
    printf("=============================================\n");
    
    printf("\nTo reproduce the issue:\n");
    printf("1. Compile: gcc -o debug_cursor_math debug_cursor_math_issue.c -Isrc -Lbuilddir -llle\n");
    printf("2. Run: ./debug_cursor_math\n");
    printf("3. Check if debug output appears in 'Cursor Math Function Test' section\n");
    printf("4. If it does, the issue is in the cursor math function itself\n");
    printf("5. If it doesn't, the issue is in how the shell calls the function\n");
    
    return 0;
}
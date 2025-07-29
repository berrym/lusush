/**
 * @file test_linux_compatibility.c
 * @brief Simple test to validate platform detection and Linux fallback strategy
 *
 * This test verifies that the Linux compatibility fixes work correctly,
 * including platform detection and conservative display update strategy.
 *
 * @author Lusush Development Team
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include LLE components
#include "src/line_editor/display.h"
#include "src/line_editor/terminal_manager.h"
#include "src/line_editor/text_buffer.h"
#include "src/line_editor/prompt.h"

/**
 * Test platform detection and display strategy selection
 */
void test_platform_detection(void) {
    printf("Testing platform detection...\n");
    
    // Enable debug mode to see platform detection
    setenv("LLE_DEBUG", "1", 1);
    
    // Initialize components
    lle_text_buffer_t buffer;
    lle_terminal_manager_t terminal;
    lle_prompt_t prompt;
    
    if (!lle_text_buffer_init(&buffer, 1024)) {
        printf("FAILED: Could not initialize text buffer\n");
        return;
    }
    
    if (lle_terminal_init(&terminal) != LLE_TERM_INIT_SUCCESS) {
        printf("WARNING: Terminal initialization failed (may be normal in CI)\n");
        // Continue test anyway
    }
    
    if (!lle_prompt_init(&prompt)) {
        printf("FAILED: Could not initialize prompt\n");
        goto cleanup;
    }
    
    // Parse a simple prompt
    if (!lle_prompt_parse(&prompt, "$ ")) {
        printf("FAILED: Could not parse prompt\n");
        goto cleanup;
    }
    
    // Add some test text to buffer
    if (!lle_text_insert_at(&buffer, 0, "hello")) {
        printf("FAILED: Could not insert text\n");
        goto cleanup;
    }
    
    // Create display state
    lle_display_state_t *state = lle_display_create(&prompt, &buffer, &terminal);
    if (!state) {
        printf("FAILED: Could not create display state\n");
        goto cleanup;
    }
    
    printf("Platform detection test setup complete.\n");
    
    // Test incremental update - this will show platform detection in debug output
    printf("Testing incremental display update with platform detection...\n");
    bool result = lle_display_update_incremental(state);
    
    if (result) {
        printf("PASSED: Incremental update completed successfully\n");
        
        #ifdef __linux__
        printf("INFO: Running on Linux - conservative strategy should be used\n");
        #elif defined(__APPLE__)
        printf("INFO: Running on macOS - standard strategy should be used\n");
        #else
        printf("INFO: Running on unknown platform\n");
        #endif
        
    } else {
        printf("FAILED: Incremental update failed\n");
    }
    
    // Test character-by-character input simulation
    printf("Testing character-by-character input simulation...\n");
    
    // Clear buffer and simulate typing "world"
    lle_text_buffer_clear(&buffer);
    
    const char *test_word = "world";
    for (size_t i = 0; i < strlen(test_word); i++) {
        char c = test_word[i];
        
        // Insert character
        if (!lle_text_insert_char(&buffer, c)) {
            printf("FAILED: Could not insert character '%c'\n", c);
            break;
        }
        
        // Update display incrementally
        if (!lle_display_update_incremental(state)) {
            printf("FAILED: Display update failed for character '%c'\n", c);
            break;
        }
        
        printf("Character '%c' processed successfully\n", c);
    }
    
    printf("Character-by-character test completed.\n");
    
    // Test backspace simulation
    printf("Testing backspace simulation...\n");
    
    // Remove characters one by one
    for (int i = 0; i < 3; i++) {
        if (buffer.length > 0) {
            if (!lle_text_delete_char(&buffer)) {
                printf("FAILED: Could not delete character\n");
                break;
            }
            
            if (!lle_display_update_incremental(state)) {
                printf("FAILED: Display update failed during backspace\n");
                break;
            }
            
            printf("Backspace %d processed successfully\n", i + 1);
        }
    }
    
    printf("Backspace test completed.\n");
    
    // Cleanup
    lle_display_destroy(state);
    
cleanup:
    lle_prompt_clear(&prompt);
    lle_text_buffer_clear(&buffer);
    lle_terminal_cleanup(&terminal);
    
    printf("Platform detection and compatibility test completed.\n\n");
}

/**
 * Test terminal escape sequence behavior
 */
void test_terminal_sequences(void) {
    printf("Testing terminal escape sequence behavior...\n");
    
    // Initialize terminal
    lle_terminal_manager_t terminal;
    if (lle_terminal_init(&terminal) != LLE_TERM_INIT_SUCCESS) {
        printf("WARNING: Terminal not available for sequence testing\n");
        return;
    }
    
    printf("Terminal initialized successfully.\n");
    
    // Test basic cursor movement
    printf("Testing cursor movement to column 10...\n");
    if (lle_terminal_move_cursor_to_column(&terminal, 10)) {
        printf("PASSED: Cursor movement succeeded\n");
    } else {
        printf("FAILED: Cursor movement failed\n");
    }
    
    // Test clear to end of line
    printf("Testing clear to end of line...\n");
    if (lle_terminal_clear_to_eol(&terminal)) {
        printf("PASSED: Clear to EOL succeeded\n");
    } else {
        printf("FAILED: Clear to EOL failed\n");
    }
    
    // Test writing text
    printf("Testing text output...\n");
    const char *test_text = "TEST";
    if (lle_terminal_write(&terminal, test_text, strlen(test_text))) {
        printf("PASSED: Text write succeeded\n");
    } else {
        printf("FAILED: Text write failed\n");
    }
    
    // Force output and add newline for clean display
    fflush(stdout);
    printf("\n");
    
    lle_terminal_cleanup(&terminal);
    printf("Terminal sequence test completed.\n\n");
}

/**
 * Main test function
 */
int main(int argc, char *argv[]) {
    printf("=======================================================\n");
    printf("Linux Compatibility Test for Lusush Line Editor (LLE)\n");
    printf("=======================================================\n\n");
    
    printf("This test validates:\n");
    printf("1. Platform detection (macOS vs Linux)\n");
    printf("2. Conservative display strategy for Linux\n");
    printf("3. Character duplication prevention\n");
    printf("4. Terminal escape sequence behavior\n\n");
    
    // Enable debug output
    setenv("LLE_DEBUG", "1", 1);
    
    // Run tests
    test_platform_detection();
    test_terminal_sequences();
    
    printf("=======================================================\n");
    printf("Linux Compatibility Test Completed\n");
    printf("=======================================================\n");
    
    printf("\nTo test on actual Linux system:\n");
    printf("1. Copy this test to Linux/Konsole environment\n");
    printf("2. Compile: gcc -o test_linux test_linux_compatibility.c -Isrc -Lbuilddir -llusush\n");
    printf("3. Run: LLE_DEBUG=1 ./test_linux\n");
    printf("4. Look for 'Platform detected: Linux' in debug output\n");
    printf("5. Verify no character duplication occurs\n\n");
    
    #ifdef __linux__
    printf("INFO: This test was compiled for Linux\n");
    #elif defined(__APPLE__)
    printf("INFO: This test was compiled for macOS\n");
    #else
    printf("INFO: This test was compiled for unknown platform\n");
    #endif
    
    return 0;
}
/**
 * @file debug_cursor_test.c
 * @brief Debug test for cursor movement visual feedback
 *
 * This test helps debug why Ctrl+A and Ctrl+E don't show visual cursor movement.
 * It directly tests the display cursor movement APIs to identify the issue.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "text_buffer.h"
#include "prompt.h"
#include "terminal_manager.h"

int main(void) {
    printf("🔍 Debug Test: Cursor Movement Visual Feedback\n");
    printf("==============================================\n");
    
    // Create components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;
    lle_display_state_t *display = NULL;
    
    // Initialize text buffer
    if (!lle_text_buffer_init(&buffer, LLE_DEFAULT_BUFFER_CAPACITY)) {
        printf("❌ Failed to initialize text buffer\n");
        return 1;
    }
    
    // Initialize prompt
    if (!lle_prompt_init(&prompt)) {
        printf("❌ Failed to initialize prompt\n");
        if (buffer.buffer) free(buffer.buffer);
        return 1;
    }
    
    // Set up a simple prompt
    if (!lle_prompt_set_text(&prompt, "test> ")) {
        printf("❌ Failed to set prompt text\n");
        goto cleanup;
    }
    
    // Initialize terminal
    lle_terminal_init_result_t term_result = lle_terminal_init(&terminal);
    if (term_result != LLE_TERM_INIT_SUCCESS && term_result != LLE_TERM_INIT_ERROR_NOT_TTY) {
        printf("❌ Failed to initialize terminal manager\n");
        goto cleanup;
    }
    
    printf("📊 Terminal Status:\n");
    printf("   - Is TTY: %s\n", isatty(STDIN_FILENO) ? "YES" : "NO");
    printf("   - Terminal init result: %d\n", term_result);
    
    // Create display state
    display = lle_display_create(&prompt, &buffer, &terminal);
    if (!display) {
        printf("❌ Failed to create display state\n");
        goto cleanup;
    }
    
    printf("✅ All components initialized successfully\n");
    printf("\n📝 Testing cursor movement with text: 'Hello World'\n");
    
    // Add some test text
    const char *test_text = "Hello World";
    for (size_t i = 0; i < strlen(test_text); i++) {
        if (!lle_text_insert_char(&buffer, test_text[i])) {
            printf("❌ Failed to insert character at position %zu\n", i);
            goto cleanup;
        }
    }
    
    printf("   - Buffer length: %zu\n", buffer.length);
    printf("   - Cursor position: %zu\n", buffer.cursor_pos);
    printf("   - Text: '%.*s'\n", (int)buffer.length, buffer.buffer);
    
    // Test initial display
    printf("\n🎨 Testing initial display render...\n");
    if (!lle_display_render(display)) {
        printf("❌ Failed to render initial display\n");
        goto cleanup;
    }
    printf("✅ Initial display rendered\n");
    
    // Check display state
    printf("\n📊 Display State Check:\n");
    printf("   - Cursor visible: %s\n", display->cursor_visible ? "YES" : "NO");
    printf("   - Display flags: 0x%x\n", display->display_flags);
    printf("   - Needs refresh: %s\n", display->needs_refresh ? "YES" : "NO");
    
    // Test cursor position calculation
    lle_cursor_position_t calc_pos;
    if (lle_display_calculate_cursor_position(display, &calc_pos)) {
        printf("   - Calculated cursor position: valid=%s, line=%zu, col=%zu\n", 
               calc_pos.valid ? "YES" : "NO", calc_pos.line, calc_pos.column);
    } else {
        printf("   - ❌ Failed to calculate cursor position\n");
    }
    
    if (isatty(STDIN_FILENO)) {
        printf("\n🔄 Testing Ctrl+A (move to home) - Watch for cursor movement...\n");
        printf("Press Enter to test Ctrl+A...");
        getchar();
        
        // Test move to home
        printf("Before Ctrl+A - Cursor at: %zu\n", buffer.cursor_pos);
        if (!lle_display_move_cursor_home(display)) {
            printf("❌ Failed to move cursor home\n");
        } else {
            printf("✅ Move cursor home completed\n");
            printf("After Ctrl+A - Cursor at: %zu\n", buffer.cursor_pos);
        }
        
        printf("\n🔄 Testing Ctrl+E (move to end) - Watch for cursor movement...\n");
        printf("Press Enter to test Ctrl+E...");
        getchar();
        
        // Test move to end
        printf("Before Ctrl+E - Cursor at: %zu\n", buffer.cursor_pos);
        if (!lle_display_move_cursor_end(display)) {
            printf("❌ Failed to move cursor end\n");
        } else {
            printf("✅ Move cursor end completed\n");
            printf("After Ctrl+E - Cursor at: %zu\n", buffer.cursor_pos);
        }
        
    } else {
        printf("\n⚠️  Not running in TTY - visual tests skipped\n");
        printf("Run this program directly in a terminal to see visual cursor movement\n");
        
        // Still test the API calls
        printf("\n🧪 Testing API calls without visual feedback...\n");
        
        printf("Testing move to home...\n");
        printf("Before: cursor at %zu\n", buffer.cursor_pos);
        if (lle_display_move_cursor_home(display)) {
            printf("✅ Move home API succeeded, cursor now at %zu\n", buffer.cursor_pos);
        } else {
            printf("❌ Move home API failed\n");
        }
        
        printf("Testing move to end...\n");
        printf("Before: cursor at %zu\n", buffer.cursor_pos);
        if (lle_display_move_cursor_end(display)) {
            printf("✅ Move end API succeeded, cursor now at %zu\n", buffer.cursor_pos);
        } else {
            printf("❌ Move end API failed\n");
        }
    }
    
    printf("\n🎯 DIAGNOSIS:\n");
    printf("If you can see this text but the cursor didn't move visually,\n");
    printf("then the issue is in the visual cursor positioning logic.\n");
    printf("\nTo debug further:\n");
    printf("1. Run: LLE_DEBUG=1 ./debug_cursor_test\n");
    printf("2. Check cursor positioning in lle_prompt_position_cursor()\n");
    printf("3. Verify terminal manager cursor operations\n");
    
    lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return 0;
    
cleanup:
    if (display) lle_display_destroy(display);
    lle_terminal_cleanup(&terminal);
    if (prompt.text) free(prompt.text);
    if (prompt.lines) free(prompt.lines);
    if (buffer.buffer) free(buffer.buffer);
    return 1;
}
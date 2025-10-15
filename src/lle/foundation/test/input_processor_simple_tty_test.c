// src/lle/foundation/test/input_processor_simple_tty_test.c
//
// Simple Interactive Test for Week 10 Input Processing
// Shows input → buffer → screen without full display system complexity

#include "../input/input_processor.h"
#include "../buffer/buffer_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void print_help(void) {
    printf("\n");
    printf("==================================================\n");
    printf("  Week 10 Input Processing Test\n");
    printf("==================================================\n\n");
    printf("Testing: Keyboard Input → Buffer Operations\n\n");
    printf("Controls:\n");
    printf("  Type characters  - Insert into buffer\n");
    printf("  Backspace        - Delete before cursor\n");
    printf("  Delete           - Delete at cursor\n");
    printf("  Arrow Left/Right - Move cursor\n");
    printf("  Home/End         - Line boundaries\n");
    printf("  Ctrl+A           - Beginning of line\n");
    printf("  Ctrl+E           - End of line\n");
    printf("  Ctrl+K           - Kill to end of line\n");
    printf("  Ctrl+U           - Kill to beginning\n");
    printf("  Ctrl+D           - Exit\n");
    printf("  Ctrl+C           - Exit\n\n");
    printf("Press Enter to start...\n");
    printf("==================================================\n\n");
}

// Simple display function that shows buffer content and cursor
static void simple_display_buffer(const lle_buffer_t *buffer) {
    // Clear line and return to start
    printf("\r\033[K");
    
    // Get buffer content
    char content[1024] = {0};
    lle_buffer_get_contents(buffer, content, sizeof(content) - 1);
    
    // Display prompt and content
    printf(">>> %s", content);
    
    // Show cursor position marker
    printf(" [cursor: %zu]", buffer->gap_start);
    
    fflush(stdout);
}

int main(void) {
    // Check if stdin is a TTY
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Error: This test requires a TTY\n");
        return 1;
    }
    
    print_help();
    getchar();
    
    // Initialize buffer manager
    lle_buffer_manager_t manager;
    if (lle_buffer_manager_init(&manager, 10, 1024) != 0) {
        fprintf(stderr, "Failed to initialize buffer manager\n");
        return 1;
    }
    
    // Create buffer
    uint32_t buffer_id;
    if (lle_buffer_manager_create_buffer(&manager, "main", &buffer_id) != 0) {
        fprintf(stderr, "Failed to create buffer\n");
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    
    // Initialize minimal input processor (without display/renderer)
    lle_simple_input_processor_t processor;
    memset(&processor, 0, sizeof(processor));
    processor.input_fd = STDIN_FILENO;
    processor.buffer_manager = &manager;
    processor.renderer = NULL;  // Not using display rendering
    processor.initialized = true;
    
    // Enable raw mode
    if (lle_simple_input_enable_raw_mode(&processor) != LLE_INPUT_OK) {
        fprintf(stderr, "Failed to enable raw mode\n");
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Clear screen
    printf("\033[2J\033[H");
    printf("Week 10 Input Test - Type away!\n\n");
    
    // Show initial empty buffer
    simple_display_buffer(&buf->buffer);
    
    // Input loop
    processor.running = true;
    while (processor.running) {
        lle_key_event_t event;
        int result = lle_simple_input_read_event(&processor, &event);
        
        if (result == LLE_INPUT_ERR_EOF) {
            break;
        }
        
        if (result != LLE_INPUT_OK) {
            continue;
        }
        
        // Process event (updates buffer)
        int action_result = LLE_INPUT_OK;
        processor.keys_processed++;
        
        switch (event.key) {
            case LLE_KEY_CHAR:
                action_result = lle_input_action_insert_char(&processor, event.ch);
                break;
            case LLE_KEY_BACKSPACE:
                action_result = lle_input_action_backspace(&processor);
                break;
            case LLE_KEY_DELETE:
                action_result = lle_input_action_delete(&processor);
                break;
            case LLE_KEY_ARROW_LEFT:
                action_result = lle_input_action_move_left(&processor);
                break;
            case LLE_KEY_ARROW_RIGHT:
                action_result = lle_input_action_move_right(&processor);
                break;
            case LLE_KEY_HOME:
                action_result = lle_input_action_move_home(&processor);
                break;
            case LLE_KEY_END:
                action_result = lle_input_action_move_end(&processor);
                break;
            case LLE_KEY_ENTER:
                action_result = lle_input_action_newline(&processor);
                break;
            case LLE_KEY_CTRL_A:
                action_result = lle_input_action_beginning_of_line(&processor);
                break;
            case LLE_KEY_CTRL_E:
                action_result = lle_input_action_end_of_line(&processor);
                break;
            case LLE_KEY_CTRL_K:
                action_result = lle_input_action_kill_line(&processor);
                break;
            case LLE_KEY_CTRL_U:
                action_result = lle_input_action_kill_backward(&processor);
                break;
            case LLE_KEY_CTRL_D:
                if (lle_buffer_size(&buf->buffer) == 0) {
                    processor.running = false;
                } else {
                    action_result = lle_input_action_delete(&processor);
                }
                break;
            case LLE_KEY_CTRL_C:
                processor.running = false;
                break;
            default:
                break;
        }
        
        // Redisplay buffer after action
        if (action_result == LLE_INPUT_OK && processor.running) {
            simple_display_buffer(&buf->buffer);
        }
    }
    
    printf("\n\n");
    
    // Show statistics
    uint64_t keys, inserted, deleted, moves;
    lle_simple_input_get_stats(&processor, &keys, &inserted, &deleted, &moves);
    
    printf("==================================================\n");
    printf("  Session Statistics\n");
    printf("==================================================\n");
    printf("Keys processed:      %lu\n", keys);
    printf("Characters inserted: %lu\n", inserted);
    printf("Characters deleted:  %lu\n", deleted);
    printf("Cursor movements:    %lu\n", moves);
    printf("==================================================\n\n");
    
    // Cleanup
    lle_simple_input_cleanup(&processor);
    lle_buffer_manager_cleanup(&manager);
    
    printf("Test completed.\n");
    return 0;
}

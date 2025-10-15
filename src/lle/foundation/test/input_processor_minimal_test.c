// Minimal input processor test - just test escape sequence parsing
// This avoids the getchar() issues from the full TTY test

#include "../input/input_processor.h"
#include "../buffer/buffer_manager.h"
#include "../display/display_buffer.h"
#include "../display/display.h"
#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    // Check for TTY
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Error: This test requires a TTY\n");
        return 1;
    }

    printf("Minimal Input Processor Test\n");
    printf("============================\n\n");
    printf("This test shows raw escape sequence parsing.\n");
    printf("Type arrow keys, Ctrl combos, or regular chars.\n");
    printf("Ctrl+D to exit.\n\n");
    printf("Starting in 2 seconds...\n");
    sleep(2);
    
    // Init terminal abstraction with stdout for output
    lle_terminal_abstraction_t term;
    memset(&term, 0, sizeof(term));
    
    // Set up minimal unix interface for display output
    lle_unix_interface_t unix_interface;
    memset(&unix_interface, 0, sizeof(unix_interface));
    unix_interface.output_fd = STDOUT_FILENO;
    term.unix_interface = &unix_interface;
    
    // Init buffer manager
    lle_buffer_manager_t manager;
    if (lle_buffer_manager_init(&manager, 10, 1024) != 0) {
        fprintf(stderr, "Failed to initialize buffer manager\n");
        return 1;
    }
    
    // Create buffer
    uint32_t buffer_id;
    if (lle_buffer_manager_create_buffer(&manager, "test", &buffer_id) != 0) {
        fprintf(stderr, "Failed to create buffer\n");
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Init display (24x80)
    lle_display_t display;
    if (lle_display_init(&display, &term, 24, 80) != LLE_DISPLAY_OK) {
        fprintf(stderr, "Failed to initialize display\n");
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Init renderer
    lle_display_buffer_renderer_t renderer;
    if (lle_display_buffer_init(&renderer, &manager, &display) != LLE_DISPLAY_BUFFER_OK) {
        fprintf(stderr, "Failed to initialize renderer\n");
        lle_display_cleanup(&display);
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Init input processor
    lle_simple_input_processor_t processor;
    if (lle_simple_input_init(&processor, STDIN_FILENO, &manager, &renderer) != LLE_INPUT_OK) {
        fprintf(stderr, "Failed to initialize input processor\n");
        lle_display_buffer_cleanup(&renderer);
        lle_display_cleanup(&display);
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Enable raw mode
    if (lle_simple_input_enable_raw_mode(&processor) != LLE_INPUT_OK) {
        fprintf(stderr, "Failed to enable raw mode\n");
        lle_simple_input_cleanup(&processor);
        lle_display_buffer_cleanup(&renderer);
        lle_display_cleanup(&display);
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    printf("\n\n=== RAW MODE ENABLED ===\n");
    printf("Debug logs will show on stderr.\n\n");
    
    // Read and process events manually
    processor.running = true;
    while (processor.running) {
        lle_key_event_t event;
        int result = lle_simple_input_read_event(&processor, &event);
        
        if (result == LLE_INPUT_ERR_EOF) {
            printf("\nEOF detected\n");
            break;
        }
        
        if (result != LLE_INPUT_OK) {
            fprintf(stderr, "[ERROR] read_event returned: %d (%s)\n", 
                    result, lle_input_error_string(result));
            continue;
        }
        
        // Process the event
        if (event.key == LLE_KEY_CTRL_D) {
            printf("\nCtrl+D - exiting\n");
            processor.running = false;
            break;
        }
        
        if (event.key == LLE_KEY_CTRL_C) {
            printf("\nCtrl+C - exiting\n");
            processor.running = false;
            break;
        }
        
        // Call process_event which has debug logging
        lle_simple_input_process_event(&processor, &event);
        
        // Show buffer content (using simple text output, no ANSI escapes)
        lle_managed_buffer_t *mbuf = lle_buffer_manager_get_buffer(&manager, buffer_id);
        if (mbuf) {
            char content[256] = {0};
            lle_buffer_get_contents(&mbuf->buffer, content, sizeof(content) - 1);
            printf("Buffer: [%s] (cursor at %zu)\n", content, mbuf->buffer.gap_start);
        }
    }
    
    // Cleanup
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    printf("\n\nTest completed.\n");
    return 0;
}

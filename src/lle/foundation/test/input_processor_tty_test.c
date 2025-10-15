// src/lle/foundation/test/input_processor_tty_test.c
//
// LLE Input Processor Interactive TTY Test
//
// This test requires a real TTY and lets you test keyboard input interactively.
//
// Usage: ./input_processor_tty_test
//
// Controls:
// - Type characters to insert them
// - Arrow keys to move cursor
// - Backspace/Delete to delete
// - Home/End to move to line boundaries
// - Ctrl+A/E for beginning/end of line
// - Ctrl+K to kill to end of line
// - Ctrl+U to kill to beginning of line
// - Ctrl+L to clear screen
// - Ctrl+D on empty line to exit
// - Ctrl+C to exit

#include "../input/input_processor.h"
#include "../buffer/buffer_manager.h"
#include "../display/display_buffer.h"
#include "../display/display.h"
#include "../terminal/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static void print_help(void) {
    printf("\n");
    printf("==================================================\n");
    printf("  LLE Input Processor Interactive Test\n");
    printf("==================================================\n\n");
    printf("This test demonstrates keyboard input processing.\n\n");
    printf("Controls:\n");
    printf("  Character keys   - Insert characters\n");
    printf("  Arrow keys       - Move cursor\n");
    printf("  Backspace/Delete - Delete characters\n");
    printf("  Home/End         - Move to line boundaries\n");
    printf("  Page Up/Down     - Scroll viewport\n");
    printf("  Enter            - New line\n");
    printf("  Tab              - Insert tab\n\n");
    printf("Ctrl combinations (Emacs-style):\n");
    printf("  Ctrl+A           - Beginning of line\n");
    printf("  Ctrl+E           - End of line\n");
    printf("  Ctrl+K           - Kill to end of line\n");
    printf("  Ctrl+U           - Kill to beginning of line\n");
    printf("  Ctrl+L           - Clear screen\n");
    printf("  Ctrl+D           - Exit (on empty line)\n");
    printf("  Ctrl+C           - Exit\n\n");
    printf("Press any key to start...\n");
    printf("==================================================\n\n");
}

int main(void) {
    // Check if stdin is a TTY
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Error: This test requires a TTY (interactive terminal)\n");
        fprintf(stderr, "Run directly in a terminal, not with input redirection.\n");
        return 1;
    }
    
    print_help();
    getchar();  // Wait for keypress
    
    // Initialize components
    lle_terminal_abstraction_t term;
    lle_unix_interface_t unix_interface;
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_input_parser_system_t *processor = NULL;
    
    memset(&term, 0, sizeof(term));
    memset(&unix_interface, 0, sizeof(unix_interface));
    
    // Set up unix interface for display output
    unix_interface.output_fd = STDOUT_FILENO;
    term.unix_interface = &unix_interface;
    
    // Init buffer manager
    if (lle_buffer_manager_init(&manager, 10, 1024) != 0) {
        fprintf(stderr, "Failed to initialize buffer manager\n");
        return 1;
    }
    
    // Create initial buffer
    uint32_t buffer_id;
    if (lle_buffer_manager_create_buffer(&manager, "main", &buffer_id) != 0) {
        fprintf(stderr, "Failed to create buffer\n");
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Init display (24 lines x 80 columns)
    if (lle_display_init(&display, &term, 24, 80) != LLE_DISPLAY_OK) {
        fprintf(stderr, "Failed to initialize display\n");
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Init renderer
    if (lle_display_buffer_init(&renderer, &manager, &display) != LLE_DISPLAY_BUFFER_OK) {
        fprintf(stderr, "Failed to initialize renderer\n");
        lle_display_cleanup(&display);
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Init input processor
    if (lle_input_parser_system_init_simple(&processor, STDIN_FILENO, &manager, &renderer) != LLE_INPUT_OK) {
        fprintf(stderr, "Failed to initialize input processor\n");
        lle_display_buffer_cleanup(&renderer);
        lle_display_cleanup(&display);
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Enable raw mode
    if (lle_input_parser_enable_raw_mode(processor) != LLE_INPUT_OK) {
        fprintf(stderr, "Failed to enable raw mode\n");
        lle_input_parser_system_cleanup(processor);
        lle_display_buffer_cleanup(&renderer);
        lle_display_cleanup(&display);
        lle_buffer_manager_cleanup(&manager);
        return 1;
    }
    
    // Clear screen and show initial state
    printf("\033[2J\033[H");  // Clear screen and move to top
    printf("LLE Input Processor Test - Ready!\n");
    printf("(Ctrl+D or Ctrl+C to exit)\n\n");
    
    // Run input loop
    int result = lle_input_parser_run(processor);
    
    // Show statistics
    uint64_t keys, inserted, deleted, moves;
    lle_input_parser_get_stats(processor, &keys, &inserted, &deleted, &moves);
    
    printf("\n\n");
    printf("==================================================\n");
    printf("  Session Statistics\n");
    printf("==================================================\n");
    printf("Keys processed:     %lu\n", keys);
    printf("Characters inserted: %lu\n", inserted);
    printf("Characters deleted:  %lu\n", deleted);
    printf("Cursor movements:    %lu\n", moves);
    printf("==================================================\n\n");
    
    // Cleanup
    lle_input_parser_system_cleanup(processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    if (result != LLE_INPUT_OK) {
        fprintf(stderr, "Input loop error: %s\n", lle_input_error_string(result));
        return 1;
    }
    
    printf("Test completed successfully.\n");
    return 0;
}

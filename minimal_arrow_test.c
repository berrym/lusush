/**
 * @file minimal_arrow_test.c
 * @brief Minimal arrow key detection test to isolate the switch statement issue
 * 
 * This is a standalone test program that replicates the exact arrow key
 * detection logic from the line editor to identify why the switch cases
 * are not being reached.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "src/line_editor/input_handler.h"
#include "src/line_editor/terminal_manager.h"

int main(void) {
    printf("=== MINIMAL ARROW KEY TEST ===\n");
    printf("This test isolates the arrow key detection issue.\n");
    printf("Press arrow keys, then Ctrl+C to exit.\n");
    printf("Debug output will show if switch cases are reached.\n\n");
    
    // Setup terminal manager (minimal)
    lle_terminal_manager_t tm;
    tm.stdin_fd = STDIN_FILENO;
    tm.stdout_fd = STDOUT_FILENO;
    tm.stderr_fd = STDERR_FILENO;
    
    // Set raw mode manually
    struct termios old_termios, new_termios;
    if (tcgetattr(STDIN_FILENO, &old_termios) != 0) {
        perror("tcgetattr");
        return 1;
    }
    
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) != 0) {
        perror("tcsetattr");
        return 1;
    }
    
    printf("Raw mode enabled. Press arrow keys...\n");
    
    // Main test loop - exact replica of line editor logic
    while (1) {
        lle_key_event_t event;
        
        printf("\n[TEST] About to read key event\n");
        
        if (!lle_input_read_key(&tm, &event)) {
            printf("[TEST] lle_input_read_key failed\n");
            break;
        }
        
        printf("[TEST] Read key event type: %d\n", event.type);
        printf("[TEST] About to enter switch statement with type %d\n", event.type);
        printf("[TEST] ENTERING SWITCH STATEMENT with event.type=%d\n", event.type);
        
        // Exact replica of line editor switch statement structure
        switch (event.type) {
            case LLE_KEY_ENTER:
            case LLE_KEY_CTRL_M:
            case LLE_KEY_CTRL_J:
                printf("[TEST] ENTER case executed\n");
                break;
                
            case LLE_KEY_CTRL_C:
                printf("[TEST] CTRL+C case executed - exiting\n");
                goto cleanup;
                
            case LLE_KEY_BACKSPACE:
            case LLE_KEY_CTRL_H:
                printf("[TEST] BACKSPACE case executed\n");
                break;
                
            case LLE_KEY_DELETE:
                printf("[TEST] DELETE case executed\n");
                break;
                
            case LLE_KEY_ARROW_LEFT:
            case LLE_KEY_CTRL_B:
                printf("[TEST] ARROW_LEFT case executed (type=%d)\n", event.type);
                break;
                
            case LLE_KEY_ARROW_RIGHT:
            case LLE_KEY_CTRL_F:
                printf("[TEST] ARROW_RIGHT case executed (type=%d)\n", event.type);
                break;
                
            case LLE_KEY_ARROW_UP:
            case LLE_KEY_CTRL_P:
                printf("[TEST] *** ARROW_UP case executed (type=%d) ***\n", event.type);
                printf("[TEST] *** THIS IS THE CASE WE'RE LOOKING FOR ***\n");
                break;
                
            case LLE_KEY_ARROW_DOWN:
            case LLE_KEY_CTRL_N:
                printf("[TEST] *** ARROW_DOWN case executed (type=%d) ***\n", event.type);
                printf("[TEST] *** THIS IS THE CASE WE'RE LOOKING FOR ***\n");
                break;
                
            case LLE_KEY_CHAR:
                printf("[TEST] CHAR case executed, character: 0x%02x ('%c')\n", 
                       event.character, 
                       (event.character >= 32 && event.character <= 126) ? event.character : '?');
                break;
                
            default:
                printf("[TEST] DEFAULT case executed for event.type=%d\n", event.type);
                printf("[TEST] This means the arrow key case was not reached!\n");
                break;
        }
        
        printf("[TEST] Switch statement completed, about to read next key\n");
    }
    
cleanup:
    // Restore terminal
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_termios);
    printf("\nTerminal restored. Test complete.\n");
    
    printf("\n=== TEST ANALYSIS ===\n");
    printf("If arrow keys work:\n");
    printf("- You should see '*** ARROW_UP case executed ***' when pressing UP\n");
    printf("- You should see '*** ARROW_DOWN case executed ***' when pressing DOWN\n");
    printf("\nIf arrow keys don't work:\n");
    printf("- You'll see 'DEFAULT case executed' instead\n");
    printf("- This indicates the switch statement issue\n");
    
    return 0;
}
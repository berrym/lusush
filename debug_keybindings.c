/*
 * Debug program to test keybinding detection in LLE
 * 
 * This program tests if Ctrl+A, Ctrl+E, and Ctrl+R keys are being
 * properly detected and processed by the line editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "src/line_editor/line_editor.h"
#include "src/line_editor/input_handler.h"

static struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

const char* key_type_name(lle_key_type_t type) {
    switch (type) {
        case LLE_KEY_CHAR: return "CHAR";
        case LLE_KEY_CTRL_A: return "CTRL_A";
        case LLE_KEY_CTRL_E: return "CTRL_E";
        case LLE_KEY_CTRL_R: return "CTRL_R";
        case LLE_KEY_HOME: return "HOME";
        case LLE_KEY_END: return "END";
        case LLE_KEY_ARROW_LEFT: return "LEFT";
        case LLE_KEY_ARROW_RIGHT: return "RIGHT";
        case LLE_KEY_ENTER: return "ENTER";
        case LLE_KEY_ESCAPE: return "ESCAPE";
        case LLE_KEY_BACKSPACE: return "BACKSPACE";
        case LLE_KEY_DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

int main() {
    printf("Keybinding Debug Program\n");
    printf("========================\n");
    printf("Press keys to test detection. Press Ctrl+C to exit.\n");
    printf("Testing: Ctrl+A (home), Ctrl+E (end), Ctrl+R (reverse search)\n\n");
    
    // Initialize LLE terminal manager for key reading
    lle_terminal_manager_t terminal;
    if (!lle_terminal_init(&terminal)) {
        fprintf(stderr, "Failed to initialize terminal\n");
        return 1;
    }
    
    enable_raw_mode();
    
    lle_key_event_t event;
    
    while (1) {
        // Read key event
        if (lle_input_read_key(&terminal, &event)) {
            printf("Key detected: type=%s", key_type_name(event.type));
            
            if (event.type == LLE_KEY_CHAR) {
                printf(", char=0x%02x", (unsigned char)event.character);
                if (event.character >= 32 && event.character <= 126) {
                    printf(" ('%c')", event.character);
                }
                
                // Check for specific control characters
                if (event.character == 0x01) printf(" [This is Ctrl+A]");
                if (event.character == 0x05) printf(" [This is Ctrl+E]");
                if (event.character == 0x12) printf(" [This is Ctrl+R]");
                if (event.character == 0x03) {
                    printf(" [Ctrl+C - exiting]\n");
                    break;
                }
            }
            
            if (event.ctrl) printf(", ctrl=true");
            if (event.alt) printf(", alt=true");
            if (event.shift) printf(", shift=true");
            
            printf("\n");
            
            // Test specific keybindings
            switch (event.type) {
                case LLE_KEY_CTRL_A:
                    printf("  → Ctrl+A detected! Should move cursor to beginning of line\n");
                    break;
                case LLE_KEY_CTRL_E:
                    printf("  → Ctrl+E detected! Should move cursor to end of line\n");
                    break;
                case LLE_KEY_CTRL_R:
                    printf("  → Ctrl+R detected! Should start reverse history search\n");
                    break;
                case LLE_KEY_HOME:
                    printf("  → Home key detected! Should move cursor to beginning\n");
                    break;
                case LLE_KEY_END:
                    printf("  → End key detected! Should move cursor to end\n");
                    break;
                case LLE_KEY_CHAR:
                    if (event.character == 0x01) {
                        printf("  → Raw Ctrl+A (0x01) detected as CHAR - might be mapping issue\n");
                    } else if (event.character == 0x05) {
                        printf("  → Raw Ctrl+E (0x05) detected as CHAR - might be mapping issue\n");
                    } else if (event.character == 0x12) {
                        printf("  → Raw Ctrl+R (0x12) detected as CHAR - NOT IMPLEMENTED\n");
                    }
                    break;
                default:
                    break;
            }
        }
    }
    
    lle_terminal_cleanup(&terminal);
    disable_raw_mode();
    
    printf("\nKeybinding test complete.\n");
    return 0;
}
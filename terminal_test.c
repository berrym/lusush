/*
 * Terminal Capability Test for LLE Platform Differences
 * 
 * This utility tests and reports terminal capabilities to help debug
 * platform-specific differences in tab completion and syntax highlighting
 * between macOS/iTerm2/Zed and Linux/Konsole environments.
 *
 * Usage: gcc -o terminal_test terminal_test.c && ./terminal_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

// Test color output capabilities
void test_color_support(void) {
    printf("=== COLOR SUPPORT TEST ===\n");
    
    // Test basic ANSI colors
    printf("Basic ANSI colors:\n");
    printf("\033[31mRed\033[0m ");
    printf("\033[32mGreen\033[0m ");
    printf("\033[34mBlue\033[0m ");
    printf("\033[33mYellow\033[0m\n");
    
    // Test 256-color support
    printf("256-color test: ");
    for (int i = 16; i < 22; i++) {
        printf("\033[38;5;%dm●\033[0m", i);
    }
    printf("\n");
    
    // Test truecolor support
    printf("Truecolor test: ");
    printf("\033[38;2;255;128;0m●\033[0m"); // Orange
    printf("\033[38;2;128;0;255m●\033[0m"); // Purple
    printf("\033[38;2;0;255;128m●\033[0m"); // Green
    printf("\n");
    
    // Test command highlighting (blue like LLE uses)
    printf("Command highlighting test: ");
    printf("\033[34mecho\033[0m hello world\n");
    
    printf("\n");
}

// Test terminal environment variables
void test_environment(void) {
    printf("=== TERMINAL ENVIRONMENT ===\n");
    
    const char *env_vars[] = {
        "TERM",
        "TERM_PROGRAM", 
        "TERM_PROGRAM_VERSION",
        "COLORTERM",
        "ITERM_SESSION_ID",
        "TMUX",
        "SSH_TTY",
        "DISPLAY",
        "TERM_FEATURES"
    };
    
    for (int i = 0; i < sizeof(env_vars)/sizeof(env_vars[0]); i++) {
        const char *value = getenv(env_vars[i]);
        printf("%-20s: %s\n", env_vars[i], value ? value : "(unset)");
    }
    printf("\n");
}

// Test terminal size and capabilities
void test_terminal_info(void) {
    printf("=== TERMINAL INFO ===\n");
    
    // Test if stdout is a TTY
    printf("isatty(STDOUT_FILENO): %s\n", isatty(STDOUT_FILENO) ? "yes" : "no");
    printf("isatty(STDIN_FILENO):  %s\n", isatty(STDIN_FILENO) ? "yes" : "no");
    
    // Get terminal size
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        printf("Terminal size:         %dx%d\n", ws.ws_col, ws.ws_row);
    } else {
        printf("Terminal size:         unable to detect\n");
    }
    
    // Test terminal attributes
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0) {
        printf("Terminal attributes:   available\n");
        printf("Input flags:           0x%lx\n", (unsigned long)term.c_iflag);
        printf("Output flags:          0x%lx\n", (unsigned long)term.c_oflag);
        printf("Control flags:         0x%lx\n", (unsigned long)term.c_cflag);
        printf("Local flags:           0x%lx\n", (unsigned long)term.c_lflag);
    } else {
        printf("Terminal attributes:   unavailable\n");
    }
    
    printf("\n");
}

// Test cursor positioning and movement
void test_cursor_capabilities(void) {
    printf("=== CURSOR CAPABILITIES TEST ===\n");
    
    // Save cursor position
    printf("Testing cursor save/restore: ");
    printf("\033[s");  // Save cursor
    printf("MOVED");
    printf("\033[u");  // Restore cursor
    printf("BACK\n");
    
    // Test cursor positioning
    printf("Testing absolute positioning:\n");
    printf("\033[3;10HPositioned at row 3, col 10");
    printf("\033[1;1H"); // Return to start
    printf("\n\n\n\n"); // Move down to clear the positioned text
    
    // Test cursor movement
    printf("Testing relative movement: ");
    printf("START");
    printf("\033[5D");  // Move left 5
    printf("MID");
    printf("\033[2C");  // Move right 2  
    printf("END\n");
    
    printf("\n");
}

// Test key detection capabilities
void test_key_detection(void) {
    printf("=== KEY DETECTION TEST ===\n");
    printf("This test requires user input. Press keys as prompted.\n");
    printf("Press Ctrl+C to skip this test.\n\n");
    
    struct termios orig_term, raw_term;
    
    // Save original terminal settings
    if (tcgetattr(STDIN_FILENO, &orig_term) != 0) {
        printf("Cannot get terminal attributes for key test\n");
        return;
    }
    
    // Set up raw mode for character-by-character input
    raw_term = orig_term;
    raw_term.c_lflag &= ~(ICANON | ECHO);
    raw_term.c_cc[VMIN] = 1;
    raw_term.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term) != 0) {
        printf("Cannot set raw mode for key test\n");
        return;
    }
    
    printf("Press Tab key (shows as '^I' or raw bytes): ");
    fflush(stdout);
    
    unsigned char buf[10];
    int bytes = read(STDIN_FILENO, buf, sizeof(buf));
    if (bytes > 0) {
        printf("Received %d bytes: ", bytes);
        for (int i = 0; i < bytes; i++) {
            if (buf[i] >= 32 && buf[i] <= 126) {
                printf("'%c' ", buf[i]);
            } else {
                printf("0x%02x ", buf[i]);
            }
        }
        printf("\n");
    }
    
    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
    
    printf("\n");
}

// Test escape sequence handling
void test_escape_sequences(void) {
    printf("=== ESCAPE SEQUENCE TEST ===\n");
    
    // Test various escape sequences that LLE might use
    printf("Testing clear line: ");
    printf("BEFORE");
    printf("\033[K");  // Clear to end of line
    printf("AFTER\n");
    
    printf("Testing color reset: ");
    printf("\033[31mRED\033[0mNORMAL\n");
    
    printf("Testing bold: ");
    printf("\033[1mBOLD\033[22mNORMAL\n");
    
    printf("Testing underline: ");
    printf("\033[4mUNDERLINE\033[24mNORMAL\n");
    
    printf("\n");
}

// Test specific LLE-like functionality
void test_lle_simulation(void) {
    printf("=== LLE FUNCTIONALITY SIMULATION ===\n");
    
    // Simulate command prompt with syntax highlighting
    printf("Simulating command prompt:\n");
    printf("$ ");
    printf("\033[34mecho\033[0m ");  // Blue command like LLE should do
    printf("\"hello world\"\n");
    
    // Simulate tab completion
    printf("Simulating tab completion:\n");
    printf("$ ls te\t");
    printf("\033[K");  // Clear to end of line
    printf("$ ls test.txt");
    printf("\n");
    
    // Test if backspace works properly
    printf("Testing backspace simulation: ");
    printf("WRONG");
    printf("\b\b\b\b\b");  // Backspace 5 chars
    printf("RIGHT");
    printf("\n");
    
    printf("\n");
}

int main(void) {
    printf("Terminal Capability Test for LLE Platform Differences\n");
    printf("=====================================================\n\n");
    
    test_environment();
    test_terminal_info();
    test_color_support();
    test_cursor_capabilities();
    test_escape_sequences();
    test_lle_simulation();
    
    printf("=== SUMMARY ===\n");
    printf("If you see:\n");
    printf("- Colors displayed correctly: Terminal supports ANSI colors\n");
    printf("- 'echo' in blue: Command highlighting should work\n");
    printf("- Cursor movements work: Terminal positioning is functional\n");
    printf("- Environment shows your terminal type\n\n");
    
    printf("Run this test on both macOS/Zed and Linux/Konsole to compare!\n");
    
    return 0;
}
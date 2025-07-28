/*
 * Enhanced Terminal Detection Test Utility
 * 
 * This utility tests the enhanced terminal detection system to verify
 * proper identification of terminals like Zed, iTerm2, Konsole, and
 * their capabilities including color support and interactive mode detection.
 *
 * Usage: gcc -o enhanced_terminal_test enhanced_terminal_test.c enhanced_terminal_detection.c && ./enhanced_terminal_test
 */

#include "src/line_editor/enhanced_terminal_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Test color output
void test_color_output(void) {
    printf("\n=== COLOR OUTPUT TEST ===\n");
    
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    printf("Color Support Status:\n");
    printf("  Basic colors: %s\n", info->supports_colors ? "YES" : "NO");
    printf("  256 colors:   %s\n", info->supports_256_colors ? "YES" : "NO");
    printf("  Truecolor:    %s\n", info->supports_truecolor ? "YES" : "NO");
    
    if (info->supports_colors) {
        printf("\nBasic ANSI colors test:\n");
        printf("  \033[31mRed\033[0m ");
        printf("\033[32mGreen\033[0m ");
        printf("\033[34mBlue\033[0m ");
        printf("\033[33mYellow\033[0m\n");
        
        printf("\nCommand syntax highlighting test (like LLE should do):\n");
        printf("  $ \033[34mecho\033[0m \"hello world\"\n");
        printf("  $ \033[34mls\033[0m -la /home\n");
        printf("  $ \033[34mgit\033[0m status\n");
    }
    
    if (info->supports_256_colors) {
        printf("\n256-color test: ");
        for (int i = 16; i < 26; i++) {
            printf("\033[38;5;%dm●\033[0m", i);
        }
        printf("\n");
    }
    
    if (info->supports_truecolor) {
        printf("\nTruecolor test: ");
        printf("\033[38;2;255;128;0m●\033[0m");  // Orange
        printf("\033[38;2;128;0;255m●\033[0m");  // Purple
        printf("\033[38;2;0;255;128m●\033[0m");  // Green
        printf("\033[38;2;255;0;128m●\033[0m");  // Pink
        printf("\n");
    }
}

// Test interactive mode detection
void test_interactive_detection(void) {
    printf("\n=== INTERACTIVE MODE DETECTION ===\n");
    
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    printf("TTY Status:\n");
    printf("  stdin is TTY:  %s\n", info->stdin_is_tty ? "YES" : "NO");
    printf("  stdout is TTY: %s\n", info->stdout_is_tty ? "YES" : "NO");
    printf("  stderr is TTY: %s\n", info->stderr_is_tty ? "YES" : "NO");
    
    printf("\nInteractive Mode Analysis:\n");
    printf("  Detected mode: ");
    switch (info->interactive_mode) {
        case LLE_INTERACTIVE_NONE:
            printf("NONE (non-interactive)\n");
            break;
        case LLE_INTERACTIVE_ENHANCED:
            printf("ENHANCED (editor terminal)\n");
            break;
        case LLE_INTERACTIVE_NATIVE:
            printf("NATIVE (traditional TTY)\n");
            break;
        case LLE_INTERACTIVE_MULTIPLEXED:
            printf("MULTIPLEXED (tmux/screen)\n");
            break;
        default:
            printf("UNKNOWN (%d)\n", info->interactive_mode);
    }
    
    printf("  Should be interactive: %s\n", 
           lle_enhanced_terminal_should_be_interactive() ? "YES" : "NO");
    
    printf("  Traditional isatty() would say: %s\n",
           (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) ? "interactive" : "non-interactive");
    
    if (lle_enhanced_terminal_should_be_interactive() != 
        (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))) {
        printf("  *** ENHANCED DETECTION DIFFERS FROM TRADITIONAL! ***\n");
        printf("  This explains why LLE features may not work as expected.\n");
    }
}

// Test terminal identification
void test_terminal_identification(void) {
    printf("\n=== TERMINAL IDENTIFICATION ===\n");
    
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    printf("Environment Variables:\n");
    printf("  TERM:         %s\n", info->term_name);
    printf("  TERM_PROGRAM: %s\n", info->term_program);
    printf("  COLORTERM:    %s\n", info->colorterm);
    
    printf("\nTerminal Classification:\n");
    if (info->terminal_signature) {
        printf("  Identified as: %s\n", info->terminal_signature->name);
        printf("  Force interactive: %s\n", 
               info->terminal_signature->force_interactive ? "YES" : "NO");
    } else {
        printf("  Identified as: Unknown terminal\n");
    }
    
    printf("  Capability level: ");
    switch (info->capability_level) {
        case LLE_TERMINAL_CAPABILITY_NONE:
            printf("NONE\n");
            break;
        case LLE_TERMINAL_CAPABILITY_BASIC:
            printf("BASIC\n");
            break;
        case LLE_TERMINAL_CAPABILITY_STANDARD:
            printf("STANDARD\n");
            break;
        case LLE_TERMINAL_CAPABILITY_FULL:
            printf("FULL\n");
            break;
        case LLE_TERMINAL_CAPABILITY_PREMIUM:
            printf("PREMIUM\n");
            break;
        default:
            printf("UNKNOWN (%d)\n", info->capability_level);
    }
}

// Test cursor and positioning capabilities
void test_cursor_capabilities(void) {
    printf("\n=== CURSOR CAPABILITIES TEST ===\n");
    
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    printf("Cursor Support:\n");
    printf("  Cursor queries:    %s\n", info->supports_cursor_queries ? "YES" : "NO");
    printf("  Cursor positioning: %s\n", info->supports_cursor_positioning ? "YES" : "NO");
    printf("  Terminal resize:   %s\n", info->supports_terminal_resize ? "YES" : "NO");
    
    printf("\nTerminal Dimensions:\n");
    printf("  Width:  %d columns\n", info->terminal_width);
    printf("  Height: %d rows\n", info->terminal_height);
    
    if (info->supports_cursor_positioning) {
        printf("\nCursor positioning test:\n");
        printf("  Moving to position (5, 10): ");
        printf("\033[5;10H");
        printf("HERE");
        printf("\033[1;1H");  // Return to start
        printf("\n\n\n\n\n");  // Move down to clear
        printf("  Cursor save/restore test: ");
        printf("\033[s");      // Save cursor
        printf("MOVED");
        printf("\033[u");      // Restore cursor
        printf("BACK\n");
    }
}

// Test specific LLE compatibility
void test_lle_compatibility(void) {
    printf("\n=== LLE COMPATIBILITY ANALYSIS ===\n");
    
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    printf("LLE Feature Compatibility:\n");
    
    // Syntax highlighting compatibility
    printf("  Syntax highlighting: ");
    if (info->supports_colors && lle_enhanced_terminal_should_be_interactive()) {
        printf("SHOULD WORK ✓\n");
    } else if (!info->supports_colors) {
        printf("NO - no color support ✗\n");
    } else if (!lle_enhanced_terminal_should_be_interactive()) {
        printf("NO - non-interactive mode ✗\n");
    } else {
        printf("UNKNOWN\n");
    }
    
    // Tab completion compatibility
    printf("  Tab completion:    ");
    if (lle_enhanced_terminal_should_be_interactive()) {
        printf("SHOULD WORK ✓\n");
    } else {
        printf("NO - non-interactive mode ✗\n");
    }
    
    // Cursor movement compatibility
    printf("  Cursor movement:   ");
    if (info->supports_cursor_positioning && lle_enhanced_terminal_should_be_interactive()) {
        printf("SHOULD WORK ✓\n");
    } else {
        printf("LIMITED or NO ✗\n");
    }
    
    // Overall LLE compatibility
    printf("\nOverall LLE Compatibility: ");
    if (lle_enhanced_terminal_should_be_interactive() && 
        info->supports_colors && 
        info->supports_cursor_positioning) {
        printf("EXCELLENT ✓\n");
        printf("  All LLE features should work properly.\n");
    } else if (lle_enhanced_terminal_should_be_interactive()) {
        printf("PARTIAL ⚠\n");
        printf("  Some LLE features may have issues.\n");
    } else {
        printf("POOR ✗\n");
        printf("  LLE will run in non-interactive mode.\n");
        printf("  Most advanced features will be disabled.\n");
    }
}

// Show recommendations for fixing issues
void show_recommendations(void) {
    printf("\n=== RECOMMENDATIONS ===\n");
    
    const lle_enhanced_terminal_info_t *info = lle_enhanced_terminal_get_info();
    
    if (!lle_enhanced_terminal_should_be_interactive()) {
        printf("Issue: Terminal not detected as interactive\n");
        printf("Solutions:\n");
        printf("  1. Force interactive mode: lusush -i\n");
        printf("  2. Use a native terminal (iTerm2, Terminal.app, Konsole)\n");
        printf("  3. Update LLE to use enhanced detection\n");
    }
    
    if (!info->supports_colors) {
        printf("Issue: No color support detected\n");
        printf("Solutions:\n");
        printf("  1. Set COLORTERM=truecolor environment variable\n");
        printf("  2. Use a terminal with better color support\n");
        printf("  3. Check terminal color settings\n");
    }
    
    if (strcmp(info->term_program, "zed") == 0 && !info->stdin_is_tty) {
        printf("Zed Terminal Detected:\n");
        printf("  Issue: stdin is not TTY, causing non-interactive mode\n");
        printf("  This explains missing syntax highlighting\n");
        printf("  Solution: Enhanced detection should force interactive mode\n");
    }
    
    if (info->supports_colors && lle_enhanced_terminal_should_be_interactive()) {
        printf("Good News:\n");
        printf("  Your terminal supports all necessary LLE features!\n");
        printf("  If LLE isn't working properly, it's likely a detection bug.\n");
    }
}

int main(void) {
    printf("Enhanced Terminal Detection Test Utility\n");
    printf("========================================\n");
    
    // Initialize enhanced detection
    if (!lle_enhanced_terminal_detection_init()) {
        printf("ERROR: Failed to initialize enhanced terminal detection\n");
        return 1;
    }
    
    // Run all tests
    test_terminal_identification();
    test_interactive_detection();
    test_color_output();
    test_cursor_capabilities();
    test_lle_compatibility();
    show_recommendations();
    
    // Show complete debug information
    printf("\n=== COMPLETE DEBUG INFORMATION ===\n");
    printf("%s\n", lle_enhanced_terminal_get_debug_info());
    
    // Cleanup
    lle_enhanced_terminal_detection_cleanup();
    
    printf("\n=== TEST COMPLETE ===\n");
    printf("Compare results between macOS/Zed and Linux/Konsole!\n");
    
    return 0;
}
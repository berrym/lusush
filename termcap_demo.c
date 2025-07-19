/**
 * termcap_demo.c - Standalone Demo Program for Enhanced Termcap Capabilities
 *
 * This program demonstrates the enhanced termcap functionality in lusush,
 * including colors, cursor control, and advanced terminal features.
 * 
 * Usage:
 *   gcc -o termcap_demo termcap_demo.c src/termcap.c src/termcap_test.c -Iinclude
 *   ./termcap_demo [--test|--demo|--capabilities]
 */

#include "include/termcap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* External test functions */
extern int termcap_run_all_tests(void);
extern int termcap_interactive_demo(void);
extern void termcap_dump_capabilities(void);

static void print_usage(const char *prog_name) {
    printf("Enhanced Termcap Demo for Lusush Shell\n");
    printf("======================================\n\n");
    printf("Usage: %s [option]\n\n", prog_name);
    printf("Options:\n");
    printf("  --test         Run comprehensive test suite\n");
    printf("  --demo         Run interactive demonstration\n");
    printf("  --capabilities Dump terminal capabilities\n");
    printf("  --help         Show this help message\n\n");
    printf("Without options, runs a quick showcase.\n");
}

static void quick_showcase(void) {
    printf("Quick Termcap Showcase\n");
    printf("======================\n\n");
    
    // Initialize
    termcap_init();
    termcap_detect_capabilities();
    
    const terminal_info_t *info = termcap_get_info();
    
    // Basic info
    printf("Terminal: %s (%dx%d)\n", info->term_type, info->cols, info->rows);
    printf("Detected: %s\n\n", info->terminal_name);
    
    // Color showcase
    if (termcap_supports_colors()) {
        printf("Color Support: ");
        termcap_print_colored(TERMCAP_RED, TERMCAP_DEFAULT, "Red ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "Green ");
        termcap_print_colored(TERMCAP_BLUE, TERMCAP_DEFAULT, "Blue ");
        termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "Yellow ");
        termcap_print_colored(TERMCAP_MAGENTA, TERMCAP_DEFAULT, "Magenta ");
        termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, "Cyan ");
        printf("\n\n");
    }
    
    // Text formatting
    printf("Text Formatting: ");
    termcap_print_bold("Bold ");
    termcap_print_italic("Italic ");
    termcap_set_underline(true);
    printf("Underlined");
    termcap_reset_attributes();
    printf("\n\n");
    
    // Status messages
    printf("Status Messages:\n");
    printf("  ");
    termcap_print_success("✓ Success message");
    printf("\n  ");
    termcap_print_warning("⚠ Warning message");
    printf("\n  ");
    termcap_print_error("✗ Error message");
    printf("\n  ");
    termcap_print_info("ℹ Info message");
    printf("\n\n");
    
    // Feature summary
    printf("Available Features:\n");
    printf("  Basic Colors:     %s\n", termcap_supports_colors() ? "✓" : "✗");
    printf("  256 Colors:       %s\n", termcap_supports_256_colors() ? "✓" : "✗");
    printf("  True Color:       %s\n", termcap_supports_truecolor() ? "✓" : "✗");
    printf("  Mouse Support:    %s\n", termcap_supports_mouse() ? "✓" : "✗");
    printf("  Bracketed Paste:  %s\n", termcap_supports_bracketed_paste() ? "✓" : "✗");
    printf("  Alternate Screen: %s\n", termcap_supports_alternate_screen() ? "✓" : "✗");
    
    // True color gradient (if supported)
    if (termcap_supports_truecolor()) {
        printf("\nTrue Color Gradient:\n  ");
        for (int i = 0; i < 40; i++) {
            int r = (i * 255) / 39;
            int g = 128 + (i * 127) / 39;
            int b = 255 - (i * 255) / 39;
            termcap_set_background_rgb(r, g, b);
            printf(" ");
        }
        termcap_reset_colors();
        printf("\n");
    }
    
    printf("\nRun with --demo for interactive demonstration\n");
    printf("Run with --test for comprehensive testing\n");
    printf("Run with --capabilities for detailed capability report\n");
    
    termcap_cleanup();
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--test") == 0) {
            return termcap_run_all_tests();
        } else if (strcmp(argv[1], "--demo") == 0) {
            return termcap_interactive_demo();
        } else if (strcmp(argv[1], "--capabilities") == 0) {
            termcap_dump_capabilities();
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    quick_showcase();
    return 0;
}
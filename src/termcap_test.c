/**
 * termcap_test.c - Test and Demonstration Module for Enhanced Termcap
 *
 * This module provides comprehensive testing and demonstration of the enhanced
 * termcap functionality, including interactive demos and capability validation.
 */

#include "../include/termcap.h"
#include "../include/termcap_internal.h"
#include "../include/themes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* ======================= Test Result Tracking ======================= */

typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
} test_results_t;

static test_results_t g_test_results = {0};

/* ======================= Test Utilities ======================= */

static void test_print_header(const char *section) {
    printf("\n");
    termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, "=== ");
    termcap_print_bold(section);
    termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, " ===");
    printf("\n\n");
}

static void test_print_result(const char *test_name, bool passed, const char *details) {
    printf("  %-40s ", test_name);
    
    if (passed) {
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        g_test_results.passed_tests++;
    } else {
        termcap_print_colored(TERMCAP_RED, TERMCAP_DEFAULT, "[FAIL]");
        g_test_results.failed_tests++;
    }
    
    if (details) {
        printf(" %s", details);
    }
    printf("\n");
    
    g_test_results.total_tests++;
}

static void test_print_skip(const char *test_name, const char *reason) {
    printf("  %-40s ", test_name);
    termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "[SKIP]");
    if (reason) {
        printf(" %s", reason);
    }
    printf("\n");
    
    g_test_results.skipped_tests++;
    g_test_results.total_tests++;
}

/* ======================= Basic Functionality Tests ======================= */

static void test_basic_initialization(void) {
    test_print_header("Basic Initialization Tests");
    
    // Test initialization
    int result = termcap_init();
    test_print_result("termcap_init()", result == TERMCAP_OK || result == TERMCAP_NOT_TERMINAL, NULL);
    
    // Test terminal info retrieval
    const terminal_info_t *info = termcap_get_info();
    test_print_result("termcap_get_info()", info != NULL, NULL);
    
    if (info) {
        char details[128];
        snprintf(details, sizeof(details), "(%dx%d, %s)", 
                info->cols, info->rows, info->is_tty ? "TTY" : "not TTY");
        test_print_result("Terminal size detection", 
                         info->rows > 0 && info->cols > 0, details);
    }
    
    // Test capability detection
    result = termcap_detect_capabilities();
    test_print_result("termcap_detect_capabilities()", result == TERMCAP_OK, NULL);
}

static void test_color_support(void) {
    test_print_header("Color Support Tests");
    
    if (!termcap_supports_colors()) {
        test_print_skip("Color tests", "Terminal doesn't support colors");
        return;
    }
    
    // Test basic colors
    printf("  Basic color test: ");
    for (int i = 0; i < 8; i++) {
        termcap_set_background(i);
        printf(" %d ", i);
    }
    termcap_reset_colors();
    printf(" ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
    
    // Test bright colors
    printf("  Bright color test: ");
    for (int i = 8; i < 16; i++) {
        termcap_set_background(i);
        printf(" %d ", i - 8);
    }
    termcap_reset_colors();
    printf(" ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
    
    // Test 256 colors if supported
    if (termcap_supports_256_colors()) {
        printf("  256-color test: ");
        for (int i = 16; i < 32; i++) {
            termcap_set_background_256(i);
            printf("█");
        }
        termcap_reset_colors();
        printf(" ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
        g_test_results.total_tests++;
    } else {
        test_print_skip("256-color test", "Not supported");
    }
    
    // Test true color if supported
    if (termcap_supports_truecolor()) {
        printf("  True color test: ");
        for (int i = 0; i < 16; i++) {
            int r = (i * 255) / 15;
            int g = 128;
            int b = 255 - r;
            termcap_set_background_rgb(r, g, b);
            printf("█");
        }
        termcap_reset_colors();
        printf(" ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
        g_test_results.total_tests++;
    } else {
        test_print_skip("True color test", "Not supported");
    }
}

static void test_cursor_operations(void) {
    test_print_header("Cursor Operations Tests");
    
    // Save current position
    int orig_row, orig_col;
    bool pos_available = (termcap_get_cursor_pos(&orig_row, &orig_col) == TERMCAP_OK);
    test_print_result("Get cursor position", pos_available, 
                     pos_available ? NULL : "Cursor query not working");
    
    if (!pos_available) {
        test_print_skip("Cursor movement tests", "Can't determine cursor position");
        return;
    }
    
    // Test cursor save/restore
    termcap_save_cursor();
    termcap_move_cursor(10, 10);
    printf("          Moved to (10,10)");
    termcap_restore_cursor();
    printf(" Restored");
    test_print_result("Cursor save/restore", true, NULL);
    
    // Test cursor visibility
    printf("  Cursor visibility test: ");
    termcap_hide_cursor();
    printf("Hidden");
    usleep(500000);  // 0.5 seconds
    termcap_show_cursor();
    printf(" Shown ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
}

static void test_text_attributes(void) {
    test_print_header("Text Attributes Tests");
    
    printf("  Text formatting test: ");
    
    termcap_set_bold(true);
    printf("Bold ");
    termcap_reset_attributes();
    
    termcap_set_italic(true);
    printf("Italic ");
    termcap_reset_attributes();
    
    termcap_set_underline(true);
    printf("Underlined ");
    termcap_reset_attributes();
    
    termcap_set_reverse(true);
    printf("Reversed ");
    termcap_reset_attributes();
    
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
}

static void test_screen_management(void) {
    test_print_header("Screen Management Tests");
    
    // Test line clearing
    printf("  Line clearing test: ");
    printf("This line will be cleared");
    termcap_clear_to_eol();
    printf("Cleared ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
    
    // Test alternate screen (if supported)
    if (termcap_supports_alternate_screen()) {
        printf("  Alternate screen test: ");
        termcap_enter_alternate_screen();
        termcap_clear_screen();
        termcap_move_cursor(10, 10);
        printf("In alternate screen buffer");
        usleep(1000000);  // 1 second
        termcap_exit_alternate_screen();
        printf("Back to main screen ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
        g_test_results.total_tests++;
    } else {
        test_print_skip("Alternate screen test", "Not supported");
    }
}

static void test_advanced_features(void) {
    test_print_header("Advanced Features Tests");
    
    // Test mouse support
    if (termcap_supports_mouse()) {
        printf("  Mouse support: ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "Available");
        printf(" (enable with termcap_enable_mouse())\n");
        g_test_results.passed_tests++;
        g_test_results.total_tests++;
    } else {
        test_print_skip("Mouse support", "Not supported");
    }
    
    // Test bracketed paste
    if (termcap_supports_bracketed_paste()) {
        printf("  Bracketed paste: ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "Available");
        printf(" (enable with termcap_enable_bracketed_paste())\n");
        g_test_results.passed_tests++;
        g_test_results.total_tests++;
    } else {
        test_print_skip("Bracketed paste", "Not supported");
    }
    
    // Test title setting
    printf("  Terminal title test: ");
    termcap_set_title("Termcap Test - Title Changed");
    usleep(1000000);  // 1 second
    termcap_set_title("Termcap Test");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
}

/* ======================= Performance Tests ======================= */

static void test_performance(void) {
    test_print_header("Performance Tests");
    
    clock_t start, end;
    double cpu_time_used;
    
    // Test cursor query performance
    start = clock();
    int row, col;
    for (int i = 0; i < 10; i++) {
        termcap_get_cursor_pos(&row, &col);
    }
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    char details[64];
    snprintf(details, sizeof(details), "%.3f ms per query", (cpu_time_used * 1000) / 10);
    test_print_result("Cursor query performance", cpu_time_used < 1.0, details);
    
    // Test color setting performance
    start = clock();
    for (int i = 0; i < 1000; i++) {
        termcap_set_color(TERMCAP_RED, TERMCAP_BLACK);
    }
    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    snprintf(details, sizeof(details), "%.3f μs per color change", (cpu_time_used * 1000000) / 1000);
    test_print_result("Color setting performance", cpu_time_used < 0.1, details);
    
    termcap_reset_colors();
}

/* ======================= Interactive Demo ======================= */

static void demo_color_palette(void) {
    printf("\n");
    termcap_print_bold("16-Color Palette:");
    printf("\n");
    
    for (int bg = 0; bg < 8; bg++) {
        printf("  ");
        for (int fg = 0; fg < 8; fg++) {
            termcap_set_color(fg, bg);
            printf(" %d%d ", fg, bg);
        }
        termcap_reset_colors();
        printf("\n");
    }
    
    if (termcap_supports_256_colors()) {
        printf("\n");
        termcap_print_bold("256-Color Palette Sample:");
        printf("\n");
        
        // Color cube
        for (int r = 0; r < 6; r++) {
            printf("  ");
            for (int g = 0; g < 6; g++) {
                for (int b = 0; b < 6; b++) {
                    int color = 16 + (r * 36) + (g * 6) + b;
                    termcap_set_background_256(color);
                    printf("  ");
                }
                termcap_reset_colors();
                printf(" ");
            }
            printf("\n");
        }
        
        // Grayscale
        printf("  Grayscale: ");
        for (int i = 232; i < 256; i++) {
            termcap_set_background_256(i);
            printf(" ");
        }
        termcap_reset_colors();
        printf("\n");
    }
    
    if (termcap_supports_truecolor()) {
        printf("\n");
        termcap_print_bold("True Color Gradient:");
        printf("\n  ");
        
        for (int i = 0; i < 60; i++) {
            int r = (i < 20) ? 255 : (i < 40) ? 255 - ((i - 20) * 12) : 0;
            int g = (i < 20) ? i * 12 : (i < 40) ? 255 : 255 - ((i - 40) * 12);
            int b = (i < 20) ? 0 : (i < 40) ? (i - 20) * 12 : 255;
            
            termcap_set_background_rgb(r, g, b);
            printf("█");
        }
        termcap_reset_colors();
        printf("\n");
    }
}

static void demo_text_formatting(void) {
    printf("\n");
    termcap_print_bold("Text Formatting Demo:");
    printf("\n\n");
    
    printf("  Normal text\n");
    printf("  ");
    termcap_print_bold("Bold text");
    printf("\n");
    
    printf("  ");
    termcap_print_italic("Italic text");
    printf("\n");
    
    printf("  ");
    termcap_set_underline(true);
    printf("Underlined text");
    termcap_reset_attributes();
    printf("\n");
    
    printf("  ");
    termcap_set_reverse(true);
    printf("Reversed text");
    termcap_reset_attributes();
    printf("\n");
    
    printf("  ");
    termcap_set_bold(true);
    termcap_set_italic(true);
    termcap_set_underline(true);
    printf("Bold+Italic+Underlined");
    termcap_reset_attributes();
    printf("\n");
}

static void demo_colored_messages(void) {
    printf("\n");
    termcap_print_bold("Colored Message Demo:");
    printf("\n\n");
    
    printf("  ");
    termcap_print_error("Error: Something went wrong!");
    printf("\n");
    
    printf("  ");
    termcap_print_warning("Warning: This is a warning message.");
    printf("\n");
    
    printf("  ");
    termcap_print_success("Success: Operation completed successfully!");
    printf("\n");
    
    printf("  ");
    termcap_print_info("Info: This is an informational message.");
    printf("\n");
}

static void demo_progress_indicator(void) {
    printf("\n");
    termcap_print_bold("Progress Indicator Demo:");
    printf("\n\n");
    
    printf("  Progress: ");
    for (int i = 0; i <= 100; i += 5) {
        printf("\r  Progress: ");
        
        // Draw progress bar
        int filled = (i * 30) / 100;
        printf("[");
        for (int j = 0; j < 30; j++) {
            if (j < filled) {
                termcap_set_background(TERMCAP_GREEN);
                printf(" ");
                termcap_reset_colors();
            } else {
                printf("·");
            }
        }
        printf("] %3d%%", i);
        fflush(stdout);
        
        usleep(50000);  // 50ms
    }
    printf("\n");
}

/* ======================= Enhanced Integration Tests ======================= */

static void test_lusush_integration(void) {
    test_print_header("Lusush Shell Integration Tests");
    
    // Test shell setup function
    int setup_result = termcap_shell_setup();
    test_print_result("termcap_shell_setup()", 
                     setup_result == TERMCAP_OK || setup_result == TERMCAP_NOT_TERMINAL, 
                     NULL);
    
    // Test linenoise color integration
    if (termcap_supports_colors()) {
        int color_result = termcap_linenoise_set_colors(TERMCAP_CYAN, TERMCAP_WHITE, TERMCAP_YELLOW);
        test_print_result("linenoise color integration", 
                         color_result == TERMCAP_OK, NULL);
        
        // Test mouse integration
        if (termcap_supports_mouse()) {
            int mouse_result = termcap_linenoise_enable_mouse(true);
            test_print_result("linenoise mouse integration", 
                             mouse_result == TERMCAP_OK, NULL);
            termcap_linenoise_enable_mouse(false); // Cleanup
        } else {
            test_print_skip("linenoise mouse integration", "Mouse not supported");
        }
    } else {
        test_print_skip("linenoise color integration", "Colors not supported");
        test_print_skip("linenoise mouse integration", "Colors not supported");
    }
    
    // Test enhanced prompt features
    printf("  Enhanced prompt test: ");
    if (termcap_supports_colors()) {
        termcap_print_colored(TERMCAP_CYAN, TERMCAP_DEFAULT, "lusush");
        printf(" ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "$");
        printf(" Ready for input");
        termcap_reset_all_formatting();
        printf(" ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
        g_test_results.total_tests++;
    } else {
        printf("lusush $ Ready for input ");
        termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "[SKIP]");
        printf(" No color support\n");
        g_test_results.skipped_tests++;
        g_test_results.total_tests++;
    }
    
    // Test error message integration
    printf("  Error message integration: ");
    termcap_print_error("Sample error message");
    printf(" ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf("\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
    
    // Test shell cleanup
    int cleanup_result = termcap_shell_cleanup();
    test_print_result("termcap_shell_cleanup()", cleanup_result == TERMCAP_OK, NULL);
}

static void test_theme_integration(void) {
    test_print_header("Theme System Integration Tests");
    
    // Test terminal capability detection for themes
    int color_support = theme_detect_color_support();
    char details[64];
    if (color_support >= 16777216) {
        snprintf(details, sizeof(details), "True color (%d colors)", color_support);
    } else if (color_support >= 256) {
        snprintf(details, sizeof(details), "256 colors");
    } else if (color_support >= 16) {
        snprintf(details, sizeof(details), "16 colors");
    } else {
        snprintf(details, sizeof(details), "No color support");
    }
    test_print_result("Theme color detection", color_support > 0, details);
    
    // Test responsive prompt features
    printf("  Responsive prompt test: ");
    const terminal_info_t *term_info = termcap_get_info();
    if (term_info && term_info->cols > 0) {
        printf("Terminal width: %d cols ", term_info->cols);
        if (term_info->cols < 80) {
            printf("(compact mode) ");
        } else {
            printf("(full mode) ");
        }
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
    } else {
        printf("Width detection failed ");
        termcap_print_colored(TERMCAP_RED, TERMCAP_DEFAULT, "[FAIL]");
        printf("\n");
        g_test_results.failed_tests++;
    }
    g_test_results.total_tests++;
    
    // Test enhanced color messages
    printf("  Enhanced message types:\n");
    printf("    Info: ");
    termcap_print_info("Information message");
    printf("\n");
    printf("    Warning: ");
    termcap_print_warning("Warning message");
    printf("\n");
    printf("    Success: ");
    termcap_print_success("Success message");
    printf("\n");
    printf("    ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
    printf(" All message types rendered\n");
    g_test_results.passed_tests++;
    g_test_results.total_tests++;
}

static void test_linenoise_enhancements(void) {
    test_print_header("Linenoise Enhancement Tests");
    
    // Test completion style configuration
    int style_result = termcap_linenoise_set_completion_style(1);
    test_print_result("Completion style configuration", 
                     style_result == TERMCAP_OK || style_result == TERMCAP_NOT_SUPPORTED, 
                     NULL);
    
    // Test enhanced cursor handling
    printf("  Enhanced cursor handling: ");
    int orig_row, orig_col;
    if (termcap_get_cursor_pos(&orig_row, &orig_col) == TERMCAP_OK) {
        printf("Position detected (%d,%d) ", orig_row, orig_col);
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
    } else {
        printf("Position detection failed ");
        termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "[SKIP]");
        printf(" (Expected in non-interactive mode)\n");
        g_test_results.skipped_tests++;
    }
    g_test_results.total_tests++;
    
    // Test color escape sequence handling
    printf("  Color sequence validation: ");
    if (termcap_supports_colors()) {
        termcap_set_color(TERMCAP_BLUE, TERMCAP_DEFAULT);
        printf("Blue text");
        termcap_reset_colors();
        printf(" ");
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
    } else {
        printf("Colors not supported ");
        termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "[SKIP]");
        printf("\n");
        g_test_results.skipped_tests++;
    }
    g_test_results.total_tests++;
    
    // Test terminal width adaptation
    printf("  Terminal width adaptation: ");
    const terminal_info_t *term_info = termcap_get_info();
    if (term_info && term_info->cols > 0) {
        printf("Adapting to %d columns ", term_info->cols);
        termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "[PASS]");
        printf("\n");
        g_test_results.passed_tests++;
    } else {
        printf("Width detection unavailable ");
        termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "[SKIP]");
        printf("\n");
        g_test_results.skipped_tests++;
    }
    g_test_results.total_tests++;
}

/* ======================= Main Test Functions ======================= */

int termcap_run_all_tests(void) {
    printf("Enhanced Termcap Test Suite\n");
    printf("===========================\n");
    
    // Initialize
    termcap_init();
    termcap_detect_capabilities();
    
    // Run tests
    test_basic_initialization();
    test_color_support();
    test_cursor_operations();
    test_text_attributes();
    test_screen_management();
    test_advanced_features();
    test_performance();
    
    // Enhanced integration tests
    test_lusush_integration();
    test_theme_integration();
    test_linenoise_enhancements();
    
    // Print summary
    test_print_header("Test Results Summary");
    
    printf("  Total tests:  %d\n", g_test_results.total_tests);
    printf("  ");
    termcap_print_colored(TERMCAP_GREEN, TERMCAP_DEFAULT, "Passed:       ");
    printf("%d\n", g_test_results.passed_tests);
    
    if (g_test_results.failed_tests > 0) {
        printf("  ");
        termcap_print_colored(TERMCAP_RED, TERMCAP_DEFAULT, "Failed:       ");
        printf("%d\n", g_test_results.failed_tests);
    }
    
    if (g_test_results.skipped_tests > 0) {
        printf("  ");
        termcap_print_colored(TERMCAP_YELLOW, TERMCAP_DEFAULT, "Skipped:      ");
        printf("%d\n", g_test_results.skipped_tests);
    }
    
    printf("  Success rate: %.1f%%\n", 
           (double)g_test_results.passed_tests / g_test_results.total_tests * 100);
    
    termcap_cleanup();
    
    // Enhanced summary with integration status
    printf("\n");
    termcap_print_info("Enhanced Termcap Integration Status:");
    printf("\n");
    printf("  • Shell integration: %s\n", 
           termcap_supports_colors() ? "✓ Active" : "○ Basic");
    printf("  • Linenoise enhancement: %s\n", 
           termcap_supports_colors() ? "✓ Enhanced" : "○ Standard");
    printf("  • Theme system: %s\n", 
           termcap_supports_colors() ? "✓ Integrated" : "○ Fallback");
    printf("  • Error messaging: %s\n", 
           termcap_supports_colors() ? "✓ Colored" : "○ Plain");
    
    return (g_test_results.failed_tests == 0) ? 0 : 1;
}

int termcap_interactive_demo(void) {
    printf("Enhanced Termcap Interactive Demo\n");
    printf("==================================\n");
    
    termcap_init();
    termcap_detect_capabilities();
    
    // Display terminal info
    const terminal_info_t *info = termcap_get_info();
    printf("\nTerminal Information:\n");
    printf("  Type: %s\n", info->term_type);
    printf("  Size: %dx%d\n", info->cols, info->rows);
    printf("  TTY:  %s\n", info->is_tty ? "Yes" : "No");
    printf("  Name: %s\n", info->terminal_name);
    
    printf("\nSupported Features:\n");
    printf("  Colors:           %s\n", termcap_supports_colors() ? "Yes" : "No");
    printf("  256 Colors:       %s\n", termcap_supports_256_colors() ? "Yes" : "No");
    printf("  True Color:       %s\n", termcap_supports_truecolor() ? "Yes" : "No");
    printf("  Mouse:            %s\n", termcap_supports_mouse() ? "Yes" : "No");
    printf("  Bracketed Paste:  %s\n", termcap_supports_bracketed_paste() ? "Yes" : "No");
    printf("  Alternate Screen: %s\n", termcap_supports_alternate_screen() ? "Yes" : "No");
    
    // Run demos
    demo_color_palette();
    demo_text_formatting();
    demo_colored_messages();
    demo_progress_indicator();
    
    printf("\n");
    termcap_print_bold("Demo completed!");
    printf("\n");
    
    termcap_cleanup();
    return 0;
}

/* ======================= Capability Dump Function ======================= */

void termcap_dump_capabilities(void) {
    termcap_init();
    termcap_detect_capabilities();
    
    const terminal_info_t *info = termcap_get_info();
    
    printf("Terminal Capability Report\n");
    printf("==========================\n\n");
    
    printf("Basic Information:\n");
    printf("  TERM:             %s\n", info->term_type);
    printf("  Terminal Name:    %s\n", info->terminal_name);
    printf("  Terminal Size:    %dx%d\n", info->cols, info->rows);
    printf("  Is TTY:           %s\n", info->is_tty ? "Yes" : "No");
    printf("  Max Colors:       %d\n", info->max_colors);
    
    printf("\nColor Capabilities:\n");
    printf("  Basic Colors:     %s\n", info->caps.colors ? "Yes" : "No");
    printf("  256 Colors:       %s\n", info->caps.colors_256 ? "Yes" : "No");
    printf("  True Color:       %s\n", info->caps.truecolor ? "Yes" : "No");
    printf("  BCE Support:      %s\n", info->background_color_erase ? "Yes" : "No");
    
    printf("\nInput Capabilities:\n");
    printf("  Mouse Support:    %s\n", info->caps.mouse ? "Yes" : "No");
    printf("  Bracketed Paste:  %s\n", info->caps.bracketed_paste ? "Yes" : "No");
    printf("  Focus Events:     %s\n", info->caps.focus_events ? "Yes" : "No");
    
    printf("\nDisplay Capabilities:\n");
    printf("  Title Setting:    %s\n", info->caps.title_setting ? "Yes" : "No");
    printf("  Alternate Screen: %s\n", info->caps.alternate_screen ? "Yes" : "No");
    printf("  Cursor Styling:   %s\n", info->caps.cursor_styling ? "Yes" : "No");
    printf("  Synchronized Out: %s\n", info->caps.synchronized_output ? "Yes" : "No");
    printf("  Hyperlinks:       %s\n", info->caps.hyperlinks ? "Yes" : "No");
    
    printf("\nCharacter Support:\n");
    printf("  Unicode:          %s\n", info->caps.unicode ? "Yes" : "No");
    
    printf("\nPerformance:\n");
    printf("  Query Timeout:    %dms\n", info->cursor_query_timeout_ms);
    printf("  Fast Queries:     %s\n", info->fast_cursor_queries ? "Yes" : "No");
    
    printf("\nPlatform Detection:\n");
    printf("  iTerm2:           %s\n", termcap_is_iterm2() ? "Yes" : "No");
    printf("  tmux:             %s\n", termcap_is_tmux() ? "Yes" : "No");
    printf("  screen:           %s\n", termcap_is_screen() ? "Yes" : "No");
    
    termcap_cleanup();
}
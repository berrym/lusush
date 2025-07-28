/*
 * Enhanced Terminal Integration Test
 * 
 * This utility tests the enhanced terminal integration system to verify
 * proper functionality and demonstrate the improvements over traditional
 * terminal detection. It shows how the integration handles different
 * terminal environments and provides configuration recommendations.
 *
 * Usage: gcc -I. -o enhanced_integration_test enhanced_integration_test.c \
 *        src/line_editor/enhanced_terminal_integration.c \
 *        src/line_editor/enhanced_terminal_detection.c && \
 *        ./enhanced_integration_test
 */

#include "src/line_editor/enhanced_terminal_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Test colors for visual verification
void test_color_output(void) {
    printf("\n=== COLOR OUTPUT TEST ===\n");
    
    printf("Color support: %s\n", lle_enhanced_supports_colors() ? "YES" : "NO");
    printf("Advanced features: %s\n", lle_enhanced_supports_advanced_features() ? "YES" : "NO");
    
    if (lle_enhanced_supports_colors()) {
        printf("\nSyntax highlighting simulation:\n");
        printf("  $ \033[34mecho\033[0m \"hello world\"  # Command in blue\n");
        printf("  $ \033[34mls\033[0m -la /home         # Another command\n");
        printf("  $ \033[34mgit\033[0m status          # Git command\n");
        
        printf("\nBasic ANSI colors:\n");
        printf("  \033[31mRed\033[0m \033[32mGreen\033[0m \033[34mBlue\033[0m \033[33mYellow\033[0m\n");
    } else {
        printf("No color support detected - syntax highlighting would be disabled\n");
    }
}

// Test interactive detection differences
void test_interactive_detection(void) {
    printf("\n=== INTERACTIVE DETECTION COMPARISON ===\n");
    
    bool traditional = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    bool enhanced = lle_enhanced_is_interactive_terminal();
    bool detection_differs = lle_enhanced_integration_test_detection_differences();
    
    printf("Detection Results:\n");
    printf("  Traditional isatty(): %s\n", traditional ? "interactive" : "non-interactive");
    printf("  Enhanced detection:   %s\n", enhanced ? "interactive" : "non-interactive");
    printf("  Detection differs:    %s\n", detection_differs ? "YES" : "NO");
    
    if (detection_differs) {
        printf("\n*** ENHANCED DETECTION PROVIDES DIFFERENT RESULT ***\n");
        printf("This explains why LLE features may not work as expected with traditional detection.\n");
        printf("The enhanced detection is likely identifying an editor terminal that should\n");
        printf("be treated as interactive despite having non-TTY stdin.\n");
    } else {
        printf("\nEnhanced and traditional detection agree.\n");
    }
}

// Test shell integration helper
void test_shell_integration(void) {
    printf("\n=== SHELL INTEGRATION TEST ===\n");
    
    // Test different shell scenarios
    struct {
        const char *scenario;
        bool forced_interactive;
        bool has_script_file;
        bool stdin_mode;
        bool expected_interactive;
    } test_cases[] = {
        {"Normal terminal use", false, false, false, true},
        {"Forced interactive (-i)", true, false, false, true},
        {"Script execution", false, true, false, false},
        {"Stdin mode (-s)", false, false, true, false},
        {"Forced interactive with script", true, true, false, false}, // Script overrides
        {"Forced interactive with stdin", true, false, true, false}   // Stdin overrides
    };
    
    printf("Shell Interactive Mode Decision Matrix:\n");
    printf("%-30s | %-10s | Expected | Actual\n", "Scenario", "Enhanced");
    printf("------------------------------------------------------------\n");
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        bool result = lle_enhanced_should_shell_be_interactive(
            test_cases[i].forced_interactive,
            test_cases[i].has_script_file,
            test_cases[i].stdin_mode
        );
        
        // For normal terminal use, expected result depends on actual detection
        bool expected = test_cases[i].expected_interactive;
        if (i == 0) { // Normal terminal use case
            expected = lle_enhanced_is_interactive_terminal();
        }
        
        const char *status = (result == expected) ? "✓" : "✗";
        
        printf("%-30s | %-10s | %-8s | %-6s %s\n",
               test_cases[i].scenario,
               lle_enhanced_is_interactive_terminal() ? "yes" : "no",
               expected ? "yes" : "no",
               result ? "yes" : "no",
               status);
    }
}

// Test LLE feature recommendations
void test_lle_configuration(void) {
    printf("\n=== LLE CONFIGURATION RECOMMENDATIONS ===\n");
    
    lle_enhanced_config_recommendation_t config;
    lle_enhanced_get_recommended_config(&config);
    
    printf("Recommended LLE Configuration:\n");
    printf("  Enable LLE:              %s\n", config.enable_lle ? "YES" : "NO");
    printf("  Syntax highlighting:     %s\n", config.enable_syntax_highlighting ? "YES" : "NO");
    printf("  Tab completion:          %s\n", config.enable_tab_completion ? "YES" : "NO");
    printf("  Command history:         %s\n", config.enable_history ? "YES" : "NO");
    printf("  Multiline editing:       %s\n", config.enable_multiline ? "YES" : "NO");
    printf("  Undo/redo:               %s\n", config.enable_undo ? "YES" : "NO");
    printf("  Force interactive mode:  %s\n", config.force_interactive_mode ? "YES" : "NO");
    printf("  Color support level:     %d (0=none, 1=basic, 2=256, 3=truecolor)\n", 
           config.color_support_level);
    
    printf("\nConfiguration Analysis:\n");
    if (config.enable_lle) {
        printf("  ✓ LLE should be enabled - all features available\n");
        if (config.enable_syntax_highlighting) {
            printf("  ✓ Syntax highlighting recommended - terminal supports colors\n");
        } else {
            printf("  ⚠ Syntax highlighting not recommended - limited color support\n");
        }
        if (config.force_interactive_mode) {
            printf("  ⚠ Interactive mode needs to be forced - editor terminal detected\n");
        }
    } else {
        printf("  ✗ LLE not recommended - non-interactive terminal\n");
        printf("  ➤ Shell will run in simple line-reading mode\n");
    }
}

// Test terminal information summary
void test_terminal_summary(void) {
    printf("\n=== TERMINAL INFORMATION SUMMARY ===\n");
    
    printf("Terminal Summary: %s\n", lle_enhanced_get_terminal_summary());
    
    printf("\nFeature Support Summary:\n");
    printf("  Interactive terminal:    %s\n", lle_enhanced_is_interactive_terminal() ? "YES" : "NO");
    printf("  Color support:           %s\n", lle_enhanced_supports_colors() ? "YES" : "NO");
    printf("  Advanced features:       %s\n", lle_enhanced_supports_advanced_features() ? "YES" : "NO");
    printf("  LLE features should work: %s\n", lle_enhanced_should_enable_lle_features() ? "YES" : "NO");
}

// Test integration with debug output
void test_debug_integration(void) {
    printf("\n=== DEBUG INTEGRATION TEST ===\n");
    
    // Test with debug mode (if enabled)
    const char *debug_env = getenv("LLE_DEBUG");
    if (debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) {
        printf("Debug mode is enabled - detailed output should appear above.\n");
    } else {
        printf("Debug mode not enabled. Set LLE_DEBUG=1 to see detailed output.\n");
        printf("Example: LLE_DEBUG=1 ./enhanced_integration_test\n");
    }
    
    printf("\nQuick debug info test:\n");
    printf("Using LLE_TERMINAL_INFO() macro: %s\n", LLE_TERMINAL_INFO());
    printf("Using LLE_IS_INTERACTIVE() macro: %s\n", LLE_IS_INTERACTIVE() ? "yes" : "no");
    printf("Using LLE_USE_COLORS() macro: %s\n", LLE_USE_COLORS() ? "yes" : "no");
}

// Show practical integration examples
void show_integration_examples(void) {
    printf("\n=== PRACTICAL INTEGRATION EXAMPLES ===\n");
    
    printf("Example 1: Simple shell interactive detection replacement\n");
    printf("  // OLD CODE:\n");
    printf("  // bool interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);\n");
    printf("  \n");
    printf("  // NEW CODE:\n");
    printf("  lle_enhanced_integration_init(true);\n");
    printf("  bool interactive = lle_enhanced_is_interactive_terminal();\n");
    printf("  \n");
    printf("  Result: %s → %s\n", 
           (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) ? "interactive" : "non-interactive",
           lle_enhanced_is_interactive_terminal() ? "interactive" : "non-interactive");
    
    printf("\nExample 2: LLE configuration based on terminal capabilities\n");
    printf("  lle_enhanced_config_recommendation_t rec;\n");
    printf("  lle_enhanced_get_recommended_config(&rec);\n");
    printf("  \n");
    printf("  // Use rec.enable_syntax_highlighting, rec.enable_tab_completion, etc.\n");
    
    lle_enhanced_config_recommendation_t rec;
    lle_enhanced_get_recommended_config(&rec);
    printf("  \n");
    printf("  Actual recommendations for this terminal:\n");
    printf("    syntax_highlighting = %s\n", rec.enable_syntax_highlighting ? "true" : "false");
    printf("    tab_completion = %s\n", rec.enable_tab_completion ? "true" : "false");
    printf("    force_interactive = %s\n", rec.force_interactive_mode ? "true" : "false");
    
    printf("\nExample 3: Conditional feature enabling\n");
    printf("  if (lle_enhanced_should_enable_lle_features()) {\n");
    printf("      // Initialize full LLE with all features\n");
    printf("  } else {\n");
    printf("      // Use simple line reading\n");
    printf("  }\n");
    printf("  \n");
    printf("  Result: %s\n", 
           lle_enhanced_should_enable_lle_features() ? "Enable full LLE" : "Use simple mode");
}

// Show recommendations for fixing common issues
void show_issue_recommendations(void) {
    printf("\n=== ISSUE DIAGNOSIS AND RECOMMENDATIONS ===\n");
    
    bool traditional = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    bool enhanced = lle_enhanced_is_interactive_terminal();
    bool colors = lle_enhanced_supports_colors();
    
    if (!enhanced) {
        printf("⚠ ISSUE: Terminal not detected as interactive\n");
        printf("  Impact: LLE features will be disabled\n");
        printf("  Solutions:\n");
        printf("    1. Use 'lusush -i' to force interactive mode\n");
        printf("    2. Switch to a native terminal (iTerm2, Terminal.app, Konsole)\n");
        printf("    3. Integrate enhanced detection into Lusush shell\n");
    } else if (!traditional) {
        printf("✓ GOOD NEWS: Enhanced detection fixes interactive mode!\n");
        printf("  Traditional detection would disable LLE features\n");
        printf("  Enhanced detection correctly identifies capable terminal\n");
        printf("  This explains why current LLE may not work properly\n");
    } else {
        printf("✓ Terminal properly detected as interactive by both methods\n");
    }
    
    if (!colors) {
        printf("\n⚠ ISSUE: No color support detected\n");
        printf("  Impact: Syntax highlighting will be disabled\n");
        printf("  Solutions:\n");
        printf("    1. Set COLORTERM=truecolor environment variable\n");
        printf("    2. Use a terminal with better color support\n");
        printf("    3. Check terminal color configuration\n");
    } else {
        printf("\n✓ Color support detected - syntax highlighting should work\n");
    }
    
    // Check specific terminal types
    const char *term_program = getenv("TERM_PROGRAM");
    if (term_program && strcmp(term_program, "zed") == 0) {
        if (!traditional && enhanced) {
            printf("\n✓ ZED TERMINAL OPTIMIZATION:\n");
            printf("  Zed terminal detected with enhanced interactive mode\n");
            printf("  This fixes the stdin non-TTY issue\n");
            printf("  All LLE features should work properly\n");
        }
    }
    
    printf("\nOverall Assessment:\n");
    lle_enhanced_config_recommendation_t config;
    lle_enhanced_get_recommended_config(&config);
    
    if (config.enable_lle && config.enable_syntax_highlighting) {
        printf("  ✓ EXCELLENT: All LLE features should work perfectly\n");
    } else if (config.enable_lle) {
        printf("  ⚠ GOOD: Basic LLE features should work, some limitations\n");
    } else {
        printf("  ✗ LIMITED: LLE will run in basic mode only\n");
    }
}

int main(void) {
    printf("Enhanced Terminal Integration Test\n");
    printf("==================================\n");
    
    // Initialize the integration system
    if (!lle_enhanced_integration_init(true)) {
        printf("ERROR: Failed to initialize enhanced integration\n");
        return 1;
    }
    
    // Run all tests
    test_interactive_detection();
    test_color_output();
    test_shell_integration();
    test_lle_configuration();
    test_terminal_summary();
    test_debug_integration();
    show_integration_examples();
    show_issue_recommendations();
    
    // Optionally show full debug information
    const char *debug_env = getenv("LLE_DEBUG");
    if (debug_env && (strcmp(debug_env, "1") == 0 || strcmp(debug_env, "true") == 0)) {
        printf("\n=== FULL DEBUG INFORMATION ===\n");
        lle_enhanced_integration_debug_print();
    }
    
    // Cleanup
    lle_enhanced_integration_cleanup();
    
    printf("\n=== TEST COMPLETE ===\n");
    printf("Key Findings:\n");
    if (lle_enhanced_integration_test_detection_differences()) {
        printf("  ✓ Enhanced detection provides improved results!\n");
        printf("  ➤ Integration would fix LLE functionality issues\n");
    } else {
        printf("  ℹ Enhanced and traditional detection agree\n");
        printf("  ➤ Integration would maintain current behavior\n");
    }
    
    printf("\nNext Steps:\n");
    printf("  1. Compare results on different terminals (macOS/iTerm2 vs Linux/Konsole)\n");
    printf("  2. Integrate enhanced detection into Lusush shell initialization\n");
    printf("  3. Update LLE configuration to use capability-based recommendations\n");
    printf("  4. Test tab completion and syntax highlighting improvements\n");
    
    return 0;
}
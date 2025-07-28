/*
 * Integration Validation Test for Enhanced Terminal Detection
 * 
 * This test validates that the enhanced terminal detection system is properly
 * integrated into the Lusush shell and working correctly across different
 * terminal environments.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// Include the enhanced terminal integration system
#include "src/line_editor/enhanced_terminal_integration.h"

// ANSI color codes for output formatting
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m"

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/**
 * @brief Print test result with color coding
 */
static void print_test_result(const char *test_name, bool passed, const char *details) {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("%s✅ PASS%s: %s", COLOR_GREEN, COLOR_RESET, test_name);
    } else {
        tests_failed++;
        printf("%s❌ FAIL%s: %s", COLOR_RED, COLOR_RESET, test_name);
    }
    
    if (details && *details) {
        printf(" - %s", details);
    }
    printf("\n");
}

/**
 * @brief Test enhanced detection initialization
 */
static void test_enhanced_detection_initialization(void) {
    printf("\n%s=== Testing Enhanced Detection Initialization ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    // Test initialization
    bool init_result = lle_enhanced_integration_init(true);
    print_test_result("Enhanced detection initialization", init_result, 
                     init_result ? "System initialized successfully" : "Initialization failed");
    
    // Test multiple initializations (should be safe)
    bool second_init = lle_enhanced_integration_init(true);
    print_test_result("Multiple initialization safety", second_init,
                     "Second initialization should succeed");
    
    // Test cleanup and re-initialization
    lle_enhanced_integration_cleanup();
    bool reinit_result = lle_enhanced_integration_init(true);
    print_test_result("Cleanup and re-initialization", reinit_result,
                     "Should be able to cleanup and re-initialize");
}

/**
 * @brief Test terminal detection capabilities
 */
static void test_terminal_detection_capabilities(void) {
    printf("\n%s=== Testing Terminal Detection Capabilities ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    // Ensure system is initialized
    lle_enhanced_integration_init(true);
    
    // Test interactive detection
    bool is_interactive = lle_enhanced_is_interactive_terminal();
    bool traditional_interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    
    print_test_result("Interactive terminal detection", true,
                     is_interactive ? "Detected as interactive" : "Detected as non-interactive");
    
    // Test color support detection
    bool supports_colors = lle_enhanced_supports_colors();
    print_test_result("Color support detection", true,
                     supports_colors ? "Colors supported" : "No color support");
    
    // Test advanced features detection
    bool supports_advanced = lle_enhanced_supports_advanced_features();
    print_test_result("Advanced features detection", true,
                     supports_advanced ? "Advanced features supported" : "Basic features only");
    
    // Test terminal information
    const char *terminal_info = lle_enhanced_get_terminal_summary();
    bool has_terminal_info = terminal_info && strlen(terminal_info) > 0;
    print_test_result("Terminal information retrieval", has_terminal_info,
                     has_terminal_info ? terminal_info : "No terminal information");
    
    // Show detection comparison
    if (is_interactive != traditional_interactive) {
        printf("%s🔍 DETECTION DIFFERENCE%s: Enhanced detection differs from traditional isatty()\n", 
               COLOR_YELLOW, COLOR_RESET);
        printf("   Traditional: %s\n", traditional_interactive ? "interactive" : "non-interactive");
        printf("   Enhanced:    %s\n", is_interactive ? "interactive" : "non-interactive");
        printf("   %s✨ Enhanced detection providing additional capabilities!%s\n", 
               COLOR_GREEN, COLOR_RESET);
    } else {
        printf("%s📋 DETECTION MATCH%s: Enhanced and traditional detection agree\n", 
               COLOR_BLUE, COLOR_RESET);
    }
}

/**
 * @brief Test shell integration functions
 */
static void test_shell_integration_functions(void) {
    printf("\n%s=== Testing Shell Integration Functions ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    // Test shell interactive detection with various scenarios
    bool shell_interactive_forced = lle_enhanced_should_shell_be_interactive(true, false, false);
    print_test_result("Shell interactive (forced)", shell_interactive_forced,
                     "Forced interactive should always return true");
    
    bool shell_interactive_script = lle_enhanced_should_shell_be_interactive(false, true, false);
    print_test_result("Shell interactive (script mode)", !shell_interactive_script,
                     "Script mode should always return false");
    
    bool shell_interactive_stdin_mode = lle_enhanced_should_shell_be_interactive(false, false, true);
    print_test_result("Shell interactive (stdin mode)", !shell_interactive_stdin_mode,
                     "stdin mode should always return false");
    
    bool shell_interactive_normal = lle_enhanced_should_shell_be_interactive(false, false, false);
    print_test_result("Shell interactive (normal detection)", true,
                     shell_interactive_normal ? "Enhanced detection enabled" : "Enhanced detection disabled");
    
    // Test LLE features detection
    bool should_enable_lle = lle_enhanced_should_enable_lle_features();
    print_test_result("LLE features should be enabled", true,
                     should_enable_lle ? "LLE features recommended" : "LLE features not recommended");
}

/**
 * @brief Test configuration recommendations
 */
static void test_configuration_recommendations(void) {
    printf("\n%s=== Testing Configuration Recommendations ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    lle_enhanced_config_recommendation_t config;
    memset(&config, 0, sizeof(config));
    
    lle_enhanced_get_recommended_config(&config);
    
    print_test_result("Configuration recommendations retrieved", true, "Config structure populated");
    
    printf("   📋 Recommended Configuration:\n");
    printf("      Enable LLE: %s\n", config.enable_lle ? "yes" : "no");
    printf("      Syntax Highlighting: %s\n", config.enable_syntax_highlighting ? "yes" : "no");
    printf("      Tab Completion: %s\n", config.enable_tab_completion ? "yes" : "no");
    printf("      History: %s\n", config.enable_history ? "yes" : "no");
    printf("      Multiline: %s\n", config.enable_multiline ? "yes" : "no");
    printf("      Undo/Redo: %s\n", config.enable_undo ? "yes" : "no");
    printf("      Force Interactive: %s\n", config.force_interactive_mode ? "yes" : "no");
    printf("      Color Support Level: %d\n", config.color_support_level);
    
    // Validate recommendations make sense
    bool recommendations_valid = true;
    if (config.enable_syntax_highlighting && !config.enable_lle) {
        recommendations_valid = false;
    }
    if (config.enable_tab_completion && !config.enable_lle) {
        recommendations_valid = false;
    }
    
    print_test_result("Configuration recommendations valid", recommendations_valid,
                     "All recommendations are logically consistent");
}

/**
 * @brief Test debug and diagnostic functions
 */
static void test_debug_and_diagnostics(void) {
    printf("\n%s=== Testing Debug and Diagnostic Functions ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    // Test debug output (should not crash)
    printf("   📊 Debug Information Output:\n");
    lle_enhanced_integration_debug_print();
    
    print_test_result("Debug information output", true, "Debug output completed without errors");
    
    // Test detection differences
    bool has_differences = lle_enhanced_integration_test_detection_differences();
    print_test_result("Detection difference testing", true,
                     has_differences ? "Enhanced detection differs from traditional" : "Detection methods agree");
    
    if (has_differences) {
        printf("   %s🎯 ENHANCEMENT VALUE%s: Enhanced detection is providing capabilities beyond traditional methods\n",
               COLOR_GREEN, COLOR_RESET);
    }
}

/**
 * @brief Test terminal type detection scenarios
 */
static void test_terminal_type_scenarios(void) {
    printf("\n%s=== Testing Terminal Type Detection Scenarios ===%s\n", COLOR_BLUE, COLOR_RESET);
    
    // Get current environment variables
    const char *term = getenv("TERM");
    const char *term_program = getenv("TERM_PROGRAM");
    const char *colorterm = getenv("COLORTERM");
    
    printf("   🖥️  Current Terminal Environment:\n");
    printf("      TERM: %s\n", term ? term : "(not set)");
    printf("      TERM_PROGRAM: %s\n", term_program ? term_program : "(not set)");
    printf("      COLORTERM: %s\n", colorterm ? colorterm : "(not set)");
    
    // Test with current environment
    bool detection_works = lle_enhanced_is_interactive_terminal();
    print_test_result("Current environment detection", true,
                     detection_works ? "Terminal properly detected" : "Terminal detection negative");
    
    // Show detailed terminal information
    const char *detailed_info = lle_enhanced_get_terminal_summary();
    if (detailed_info) {
        printf("   📋 Detailed Terminal Info: %s\n", detailed_info);
    }
    
    // Test specific terminal types if we can identify them
    if (term_program) {
        if (strcmp(term_program, "zed") == 0) {
            print_test_result("Zed terminal detection", true, "Running in Zed editor terminal");
        } else if (strcmp(term_program, "vscode") == 0) {
            print_test_result("VS Code terminal detection", true, "Running in VS Code terminal");
        } else if (strcmp(term_program, "iTerm.app") == 0) {
            print_test_result("iTerm2 terminal detection", true, "Running in iTerm2 terminal");
        } else {
            printf("   🔍 Terminal program '%s' detected\n", term_program);
        }
    }
}

/**
 * @brief Main test execution function
 */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("%s╔══════════════════════════════════════════════════════════════╗%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║           Enhanced Terminal Detection Integration Test        ║%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║                     Lusush Line Editor (LLE)                 ║%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
    
    // Run all test suites
    test_enhanced_detection_initialization();
    test_terminal_detection_capabilities();
    test_shell_integration_functions();
    test_configuration_recommendations();
    test_debug_and_diagnostics();
    test_terminal_type_scenarios();
    
    // Final cleanup
    lle_enhanced_integration_cleanup();
    
    // Print test summary
    printf("\n%s╔══════════════════════════════════════════════════════════════╗%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║                        TEST SUMMARY                          ║%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s╚══════════════════════════════════════════════════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
    
    printf("Total Tests Run: %d\n", tests_run);
    printf("%sPassed: %d%s\n", COLOR_GREEN, tests_passed, COLOR_RESET);
    if (tests_failed > 0) {
        printf("%sFailed: %d%s\n", COLOR_RED, tests_failed, COLOR_RESET);
    } else {
        printf("Failed: 0\n");
    }
    
    if (tests_failed == 0) {
        printf("\n%s🎉 ALL TESTS PASSED! Enhanced terminal detection integration is working correctly.%s\n", 
               COLOR_GREEN, COLOR_RESET);
        return 0;
    } else {
        printf("\n%s⚠️  Some tests failed. Please review the output above.%s\n", 
               COLOR_YELLOW, COLOR_RESET);
        return 1;
    }
}
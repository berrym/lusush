/*
 * Comprehensive Unit Tests for Completion Menu Theme Integration (Phase 3)
 * 
 * Test Coverage:
 * - Theme initialization
 * - Color mapping
 * - Symbol selection (Unicode vs ASCII)
 * - Themed formatting functions
 * - Fallback configurations
 * - Theme updates
 */

#include "../../include/completion_types.h"
#include "../../include/completion_menu.h"
#include "../../include/completion_menu_theme.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// Mock implementations
// ============================================================================

typedef struct builtin_s {
    const char *name;
    const char *doc;
    void *func;
} builtin;

builtin builtins[] = {
    {"cd", "Change directory", NULL},
    {"echo", "Echo arguments", NULL},
};

const size_t builtins_count = sizeof(builtins) / sizeof(builtins[0]);

char *lookup_alias(const char *name) {
    (void)name;
    return NULL;
}

void lusush_add_completion(lusush_completions_t *lc, const char *completion) {
    (void)lc;
    (void)completion;
}

// Mock theme functions
theme_definition_t *theme_get_active(void) {
    return NULL;  // No active theme
}

int theme_detect_color_support(void) {
    return 256;  // Simulate 256-color support
}

symbol_compatibility_t symbol_get_compatibility_mode(void) {
    return SYMBOL_MODE_UNICODE;
}

// ============================================================================
// Test Utilities
// ============================================================================

static int test_count = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void name(void); \
    static void run_##name(void) { \
        test_count++; \
        printf("  Test %d: %s ... ", test_count, #name); \
        fflush(stdout); \
        name(); \
        tests_passed++; \
        printf("PASS\n"); \
    } \
    static void name(void)

#define RUN_TEST(name) run_##name()

// ============================================================================
// Theme Initialization Tests
// ============================================================================

TEST(theme_init_success) {
    completion_menu_theme_cleanup();
    bool success = completion_menu_theme_init();
    assert(success == true);
}

TEST(theme_init_idempotent) {
    completion_menu_theme_cleanup();
    bool success1 = completion_menu_theme_init();
    bool success2 = completion_menu_theme_init();
    assert(success1 == true);
    assert(success2 == true);
}

TEST(theme_get_config_not_null) {
    completion_menu_theme_cleanup();
    completion_menu_theme_init();
    const completion_menu_theme_config_t *config = completion_menu_theme_get_config();
    assert(config != NULL);
}

TEST(theme_get_config_auto_init) {
    completion_menu_theme_cleanup();
    // Should auto-initialize if not initialized
    const completion_menu_theme_config_t *config = completion_menu_theme_get_config();
    assert(config != NULL);
}

TEST(theme_update_success) {
    completion_menu_theme_init();
    bool success = completion_menu_theme_update();
    assert(success == true);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(config_has_colors_enabled) {
    completion_menu_theme_cleanup();
    completion_menu_theme_init();
    const completion_menu_theme_config_t *config = completion_menu_theme_get_config();
    // With 256-color support, colors should be enabled
    assert(config->use_colors == true);
}

TEST(config_has_unicode_enabled) {
    completion_menu_theme_cleanup();
    completion_menu_theme_init();
    const completion_menu_theme_config_t *config = completion_menu_theme_get_config();
    // With SYMBOL_MODE_UNICODE, unicode should be enabled
    assert(config->use_unicode == true);
}

TEST(config_has_symbols) {
    completion_menu_theme_cleanup();
    completion_menu_theme_init();
    const completion_menu_theme_config_t *config = completion_menu_theme_get_config();
    
    assert(config->symbols.builtin_indicator != NULL);
    assert(config->symbols.command_indicator != NULL);
    assert(config->symbols.file_indicator != NULL);
    assert(config->symbols.directory_indicator != NULL);
    assert(config->symbols.variable_indicator != NULL);
    assert(config->symbols.alias_indicator != NULL);
    assert(config->symbols.history_indicator != NULL);
}

TEST(config_has_colors) {
    completion_menu_theme_cleanup();
    completion_menu_theme_init();
    const completion_menu_theme_config_t *config = completion_menu_theme_get_config();
    
    // Check that color strings are not empty
    assert(strlen(config->colors.category_header) > 0);
    assert(strlen(config->colors.item_selected) > 0);
    assert(strlen(config->colors.type_builtin) > 0);
    assert(strlen(config->colors.type_command) > 0);
}

// ============================================================================
// Color Mapping Tests
// ============================================================================

TEST(get_type_color_builtin) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_BUILTIN);
    assert(color != NULL);
    assert(strlen(color) > 0);
}

TEST(get_type_color_command) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_COMMAND);
    assert(color != NULL);
    assert(strlen(color) > 0);
}

TEST(get_type_color_file) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_FILE);
    assert(color != NULL);
}

TEST(get_type_color_directory) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_DIRECTORY);
    assert(color != NULL);
    assert(strlen(color) > 0);
}

TEST(get_type_color_variable) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_VARIABLE);
    assert(color != NULL);
    assert(strlen(color) > 0);
}

TEST(get_type_color_alias) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_ALIAS);
    assert(color != NULL);
    assert(strlen(color) > 0);
}

TEST(get_type_color_history) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_HISTORY);
    assert(color != NULL);
}

TEST(get_type_color_unknown) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_type_color(COMPLETION_TYPE_UNKNOWN);
    assert(color != NULL);
}

// ============================================================================
// Symbol Mapping Tests
// ============================================================================

TEST(get_type_symbol_builtin) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_BUILTIN);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_command) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_COMMAND);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_file) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_FILE);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_directory) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_DIRECTORY);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_variable) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_VARIABLE);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_alias) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_ALIAS);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_history) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_HISTORY);
    assert(symbol != NULL);
    assert(strlen(symbol) > 0);
}

TEST(get_type_symbol_unknown) {
    completion_menu_theme_init();
    const char *symbol = completion_menu_theme_get_type_symbol(COMPLETION_TYPE_UNKNOWN);
    assert(symbol != NULL);
}

// ============================================================================
// Header and Selection Colors
// ============================================================================

TEST(get_header_color) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_header_color();
    assert(color != NULL);
}

TEST(get_selection_color) {
    completion_menu_theme_init();
    const char *color = completion_menu_theme_get_selection_color();
    assert(color != NULL);
}

// ============================================================================
// Themed Formatting Tests
// ============================================================================

TEST(format_item_themed_normal) {
    completion_menu_theme_init();
    
    completion_item_t item = {
        .text = "test_cmd",
        .suffix = " ",
        .type = COMPLETION_TYPE_COMMAND,
        .relevance_score = 800,
        .description = NULL,
        .owns_text = false,
        .owns_description = false
    };
    
    char buffer[512];
    size_t len = completion_menu_format_item_themed(&item, false, buffer, sizeof(buffer));
    
    assert(len > 0);
    assert(strlen(buffer) > 0);
    assert(strstr(buffer, "test_cmd") != NULL);
}

TEST(format_item_themed_selected) {
    completion_menu_theme_init();
    
    completion_item_t item = {
        .text = "test_cmd",
        .suffix = " ",
        .type = COMPLETION_TYPE_COMMAND,
        .relevance_score = 800,
        .description = NULL,
        .owns_text = false,
        .owns_description = false
    };
    
    char buffer[512];
    size_t len = completion_menu_format_item_themed(&item, true, buffer, sizeof(buffer));
    
    assert(len > 0);
    assert(strlen(buffer) > 0);
    assert(strstr(buffer, "test_cmd") != NULL);
}

TEST(format_item_themed_null_item) {
    completion_menu_theme_init();
    
    char buffer[512];
    size_t len = completion_menu_format_item_themed(NULL, false, buffer, sizeof(buffer));
    
    assert(len == 0);
}

TEST(format_item_themed_null_buffer) {
    completion_menu_theme_init();
    
    completion_item_t item = {
        .text = "test_cmd",
        .suffix = " ",
        .type = COMPLETION_TYPE_COMMAND,
        .relevance_score = 800,
        .description = NULL,
        .owns_text = false,
        .owns_description = false
    };
    
    size_t len = completion_menu_format_item_themed(&item, false, NULL, 512);
    
    assert(len == 0);
}

TEST(format_header_themed) {
    completion_menu_theme_init();
    
    char buffer[256];
    size_t len = completion_menu_format_header_themed(COMPLETION_TYPE_BUILTIN, 5,
                                                      buffer, sizeof(buffer));
    
    assert(len > 0);
    assert(strlen(buffer) > 0);
    assert(strstr(buffer, "BUILT-INS") != NULL);
    assert(strstr(buffer, "5") != NULL);
}

TEST(format_header_themed_null_buffer) {
    completion_menu_theme_init();
    
    size_t len = completion_menu_format_header_themed(COMPLETION_TYPE_BUILTIN, 5,
                                                      NULL, 256);
    
    assert(len == 0);
}

TEST(format_scroll_indicator_down) {
    completion_menu_theme_init();
    
    char buffer[128];
    size_t len = completion_menu_format_scroll_indicator_themed(1, buffer, sizeof(buffer));
    
    assert(len > 0);
    assert(strlen(buffer) > 0);
}

TEST(format_scroll_indicator_up) {
    completion_menu_theme_init();
    
    char buffer[128];
    size_t len = completion_menu_format_scroll_indicator_themed(-1, buffer, sizeof(buffer));
    
    assert(len > 0);
    assert(strlen(buffer) > 0);
}

TEST(format_scroll_indicator_null_buffer) {
    completion_menu_theme_init();
    
    size_t len = completion_menu_format_scroll_indicator_themed(1, NULL, 128);
    
    assert(len == 0);
}

// ============================================================================
// Themed Display Tests
// ============================================================================

TEST(display_themed_basic) {
    completion_menu_theme_init();
    
    completion_result_t *result = completion_result_create(8);
    completion_result_add(result, "cmd1", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "cmd2", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_sort(result);
    
    completion_menu_t *menu = completion_menu_create(result, NULL);
    assert(menu != NULL);
    
    // Should not crash
    bool success = completion_menu_display_themed(menu);
    assert(success == true);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(display_themed_null_menu) {
    completion_menu_theme_init();
    
    bool success = completion_menu_display_themed(NULL);
    assert(success == false);
}

// ============================================================================
// Utility Tests
// ============================================================================

TEST(theme_is_supported) {
    bool supported = completion_menu_theme_is_supported();
    // With 256-color support, should be supported
    assert(supported == true);
}

TEST(theme_get_fallback) {
    completion_menu_theme_config_t fallback = completion_menu_theme_get_fallback();
    
    assert(fallback.use_unicode == false);  // ASCII fallback
    assert(fallback.symbols.builtin_indicator != NULL);
    assert(fallback.symbols.command_indicator != NULL);
}

TEST(theme_cleanup_works) {
    completion_menu_theme_init();
    completion_menu_theme_cleanup();
    // Should not crash
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Completion Menu Theme Integration Tests (Phase 3)            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Running Theme Initialization Tests...\n");
    RUN_TEST(theme_init_success);
    RUN_TEST(theme_init_idempotent);
    RUN_TEST(theme_get_config_not_null);
    RUN_TEST(theme_get_config_auto_init);
    RUN_TEST(theme_update_success);
    
    printf("\nRunning Configuration Tests...\n");
    RUN_TEST(config_has_colors_enabled);
    RUN_TEST(config_has_unicode_enabled);
    RUN_TEST(config_has_symbols);
    RUN_TEST(config_has_colors);
    
    printf("\nRunning Color Mapping Tests...\n");
    RUN_TEST(get_type_color_builtin);
    RUN_TEST(get_type_color_command);
    RUN_TEST(get_type_color_file);
    RUN_TEST(get_type_color_directory);
    RUN_TEST(get_type_color_variable);
    RUN_TEST(get_type_color_alias);
    RUN_TEST(get_type_color_history);
    RUN_TEST(get_type_color_unknown);
    
    printf("\nRunning Symbol Mapping Tests...\n");
    RUN_TEST(get_type_symbol_builtin);
    RUN_TEST(get_type_symbol_command);
    RUN_TEST(get_type_symbol_file);
    RUN_TEST(get_type_symbol_directory);
    RUN_TEST(get_type_symbol_variable);
    RUN_TEST(get_type_symbol_alias);
    RUN_TEST(get_type_symbol_history);
    RUN_TEST(get_type_symbol_unknown);
    
    printf("\nRunning Header and Selection Color Tests...\n");
    RUN_TEST(get_header_color);
    RUN_TEST(get_selection_color);
    
    printf("\nRunning Themed Formatting Tests...\n");
    RUN_TEST(format_item_themed_normal);
    RUN_TEST(format_item_themed_selected);
    RUN_TEST(format_item_themed_null_item);
    RUN_TEST(format_item_themed_null_buffer);
    RUN_TEST(format_header_themed);
    RUN_TEST(format_header_themed_null_buffer);
    RUN_TEST(format_scroll_indicator_down);
    RUN_TEST(format_scroll_indicator_up);
    RUN_TEST(format_scroll_indicator_null_buffer);
    
    printf("\nRunning Themed Display Tests...\n");
    RUN_TEST(display_themed_basic);
    RUN_TEST(display_themed_null_menu);
    
    printf("\nRunning Utility Tests...\n");
    RUN_TEST(theme_is_supported);
    RUN_TEST(theme_get_fallback);
    RUN_TEST(theme_cleanup_works);
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Test Summary                                                  ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Total tests:  %d\n", test_count);
    printf("  Passed:       %d\n", tests_passed);
    printf("  Failed:       %d\n", tests_failed);
    printf("\n");
    
    if (tests_failed == 0) {
        printf("  ✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("  ✗ Some tests failed\n\n");
        return 1;
    }
}

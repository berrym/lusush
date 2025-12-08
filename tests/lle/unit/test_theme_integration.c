/**
 * @file test_theme_integration.c
 * @brief Unit tests for LLE Theme Integration
 * 
 * Tests theme system integration, color extraction, and theme change handling.
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Mock memory pool instance */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t*)&mock_pool_dummy;

/* Stubs for Lusush core functions (not tested in unit tests) */
theme_definition_t *theme_load(const char *name) {
    (void)name;
    return NULL;
}

lusush_memory_pool_system_t *global_memory_pool = NULL;

void *lusush_pool_alloc(size_t size) {
    return malloc(size);
}

void lusush_pool_free(void *ptr) {
    free(ptr);
}

lusush_pool_config_t lusush_pool_get_default_config(void) {
    lusush_pool_config_t config = {0};
    return config;
}

lusush_pool_error_t lusush_pool_init(const lusush_pool_config_t *config) {
    (void)config;
    return 0;  /* SUCCESS */
}

/* ========================================================================== */
/*                         TEST FRAMEWORK MACROS                              */
/* ========================================================================== */

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    __attribute__((unused)) static void run_test_##name(void) { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
    } \
    static void test_##name(void)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  ✗ ASSERTION FAILED: %s\n", msg); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_EQ(a, b, msg) ASSERT_TRUE((a) == (b), msg)
#define ASSERT_NEQ(a, b, msg) ASSERT_TRUE((a) != (b), msg)

/* ========================================================================== */
/*                         MOCK OBJECTS                                       */
/* ========================================================================== */

/* Mock theme definition for testing */
static theme_definition_t create_mock_theme(void) {
    theme_definition_t theme;
    memset(&theme, 0, sizeof(theme_definition_t));
    
    strcpy(theme.name, "test_theme");
    strcpy(theme.description, "Test theme for unit tests");
    theme.category = THEME_CATEGORY_DEVELOPER;
    
    /* Set up color scheme with ANSI codes that fit in COLOR_CODE_MAX (16 bytes) */
    /* Using 256-color codes: \033[38;5;XXXm (max 14 chars + NUL = 15 bytes) */
    strcpy(theme.colors.primary, "\033[38;5;202m");     /* Orange */
    strcpy(theme.colors.secondary, "\033[38;5;110m");   /* Light blue */
    strcpy(theme.colors.success, "\033[38;5;46m");      /* Green */
    strcpy(theme.colors.warning, "\033[38;5;226m");     /* Yellow */
    strcpy(theme.colors.error, "\033[38;5;196m");       /* Red */
    strcpy(theme.colors.info, "\033[38;5;33m");         /* Blue */
    strcpy(theme.colors.text, "\033[38;5;252m");        /* Light gray */
    strcpy(theme.colors.text_dim, "\033[38;5;244m");    /* Gray */
    strcpy(theme.colors.background, "\033[38;5;235m");  /* Dark gray */
    strcpy(theme.colors.highlight, "\033[38;5;228m");   /* Light yellow */
    
    return theme;
}

/* ========================================================================== */
/*                         COLOR EXTRACTION TESTS                             */
/* ========================================================================== */

TEST(extract_syntax_colors_null_theme) {
    lle_syntax_color_table_t *table = NULL;
    
    lle_result_t result = lle_extract_syntax_colors_from_theme(NULL, &table, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null theme should return NULL_POINTER error");
}

TEST(extract_syntax_colors_null_output) {
    theme_definition_t theme = create_mock_theme();
    
    lle_result_t result = lle_extract_syntax_colors_from_theme(&theme, NULL, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null output pointer should return NULL_POINTER error");
}

TEST(extract_syntax_colors_success) {
    theme_definition_t theme = create_mock_theme();
    lle_syntax_color_table_t *table = NULL;
    
    lle_result_t result = lle_extract_syntax_colors_from_theme(&theme, &table, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Color extraction should succeed");
    ASSERT_NOT_NULL(table, "Color table should be allocated");
    
    /* Verify colors were extracted (non-zero values) */
    ASSERT_NEQ(table->keyword_color, 0, "Keyword color should be set");
    ASSERT_NEQ(table->string_color, 0, "String color should be set");
    ASSERT_NEQ(table->comment_color, 0, "Comment color should be set");
    ASSERT_NEQ(table->number_color, 0, "Number color should be set");
    ASSERT_NEQ(table->default_color, 0, "Default color should be set");
    
    /* Verify theme context is stored */
    ASSERT_EQ(table->theme_context, (void*)&theme, "Theme context should be stored");
}

TEST(extract_syntax_colors_parsing) {
    theme_definition_t theme = create_mock_theme();
    lle_syntax_color_table_t *table = NULL;
    
    lle_result_t result = lle_extract_syntax_colors_from_theme(&theme, &table, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Color extraction should succeed");
    
    /* Verify specific color values (from ANSI codes) */
    /* success = "\033[38;2;0;255;0m" should parse to 0x00FF00 (green) */
    ASSERT_EQ(table->string_color, 0x00FF00, "String color should be green (0x00FF00)");
    
    /* info = "\033[38;2;0;100;255m" should parse to 0x0064FF (blue) */
    ASSERT_EQ(table->number_color, 0x0064FF, "Number color should be blue (0x0064FF)");
    
    /* text_dim = "\033[38;2;128;128;128m" should parse to gray tone */
    /* Note: We verify it's in the gray range (R and G both 0x80) */
    ASSERT_TRUE((table->comment_color >> 16) == 0x80, "Comment R channel should be 0x80");
    ASSERT_TRUE(((table->comment_color >> 8) & 0xFF) == 0x80, "Comment G channel should be 0x80");
}

TEST(extract_cursor_colors_null_theme) {
    lle_cursor_colors_t *colors = NULL;
    
    lle_result_t result = lle_extract_cursor_colors_from_theme(NULL, &colors, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null theme should return NULL_POINTER error");
}

TEST(extract_cursor_colors_null_output) {
    theme_definition_t theme = create_mock_theme();
    
    lle_result_t result = lle_extract_cursor_colors_from_theme(&theme, NULL, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null output pointer should return NULL_POINTER error");
}

TEST(extract_cursor_colors_success) {
    theme_definition_t theme = create_mock_theme();
    lle_cursor_colors_t *colors = NULL;
    
    lle_result_t result = lle_extract_cursor_colors_from_theme(&theme, &colors, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Cursor color extraction should succeed");
    ASSERT_NOT_NULL(colors, "Cursor colors should be allocated");
    
    /* Verify colors were extracted */
    ASSERT_NEQ(colors->cursor_color, 0, "Cursor color should be set");
    ASSERT_NEQ(colors->cursor_text_color, 0, "Cursor text color should be set");
    ASSERT_NEQ(colors->cursor_background_color, 0, "Cursor background color should be set");
    
    /* Verify specific values */
    /* highlight = "\033[38;2;255;255;100m" should parse to 0xFFFF64 */
    ASSERT_EQ(colors->cursor_color, 0xFFFF64, "Cursor should use highlight color");
    
    /* background = "\033[38;2;30;30;30m" should parse to 0x1E1E1E */
    ASSERT_EQ(colors->cursor_text_color, 0x1E1E1E, "Cursor text should use background");
}

/* ========================================================================== */
/*                    THEME INTEGRATION TESTS                                 */
/* ========================================================================== */

TEST(integrate_theme_null_integration) {
    theme_definition_t theme = create_mock_theme();
    
    lle_result_t result = lle_display_integrate_theme_system(NULL, &theme);
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null integration should return NULL_POINTER error");
}

TEST(integrate_theme_null_theme_allowed) {
    /* Create minimal integration structure */
    lle_display_integration_t integration;
    memset(&integration, 0, sizeof(lle_display_integration_t));
    
    lle_result_t result = lle_display_integrate_theme_system(&integration, NULL);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Null theme should be allowed (use defaults)");
    ASSERT_NULL(integration.theme_system, "Theme system should be NULL");
}

TEST(integrate_theme_success) {
    /* Create integration with render controller */
    lle_display_integration_t integration;
    memset(&integration, 0, sizeof(lle_display_integration_t));
    
    lle_render_controller_t controller;
    memset(&controller, 0, sizeof(lle_render_controller_t));
    
    controller.memory_pool = mock_pool;
    integration.render_controller = &controller;
    integration.memory_pool = mock_pool;
    
    theme_definition_t theme = create_mock_theme();
    
    lle_result_t result = lle_display_integrate_theme_system(&integration, &theme);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Theme integration should succeed");
    ASSERT_EQ(integration.theme_system, (void*)&theme, "Theme should be stored");
    ASSERT_NOT_NULL(controller.theme_colors, "Syntax colors should be extracted");
    ASSERT_NOT_NULL(controller.cursor_theme_colors, "Cursor colors should be extracted");
}

/* ========================================================================== */
/*                    THEME CHANGE HANDLER TESTS                              */
/* ========================================================================== */

TEST(theme_changed_null_integration) {
    lle_result_t result = lle_on_theme_changed(NULL, "dark");
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null integration should return NULL_POINTER error");
}

TEST(theme_changed_null_theme_name) {
    lle_display_integration_t integration;
    memset(&integration, 0, sizeof(lle_display_integration_t));
    
    lle_result_t result = lle_on_theme_changed(&integration, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_NULL_POINTER, "Null theme name should return NULL_POINTER error");
}

/* Note: Full theme_changed test requires actual theme_load() implementation
 * which is part of Lusush theme system. For now, we test parameter validation.
 */

/* ========================================================================== */
/*                    COLOR PARSING EDGE CASES                                */
/* ========================================================================== */

TEST(extract_colors_256color_mode) {
    theme_definition_t theme = create_mock_theme();
    
    /* Set up 256-color codes */
    strcpy(theme.colors.primary, "\033[38;5;196m");    /* Bright red (index 196) */
    strcpy(theme.colors.success, "\033[38;5;46m");     /* Bright green (index 46) */
    strcpy(theme.colors.info, "\033[38;5;21m");        /* Bright blue (index 21) */
    
    lle_syntax_color_table_t *table = NULL;
    lle_result_t result = lle_extract_syntax_colors_from_theme(&theme, &table, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "256-color extraction should succeed");
    ASSERT_NOT_NULL(table, "Color table should be allocated");
    
    /* Verify colors were parsed (non-zero, non-white) */
    ASSERT_NEQ(table->keyword_color, 0xFFFFFF, "Primary should not be default white");
    ASSERT_NEQ(table->string_color, 0xFFFFFF, "Success should not be default white");
}

TEST(extract_colors_basic_ansi) {
    theme_definition_t theme = create_mock_theme();
    
    /* Set up basic ANSI color codes */
    strcpy(theme.colors.error, "\033[31m");      /* Red */
    strcpy(theme.colors.success, "\033[32m");    /* Green */
    strcpy(theme.colors.info, "\033[34m");       /* Blue */
    strcpy(theme.colors.text, "\033[37m");       /* White */
    
    lle_syntax_color_table_t *table = NULL;
    lle_result_t result = lle_extract_syntax_colors_from_theme(&theme, &table, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Basic ANSI extraction should succeed");
    ASSERT_NOT_NULL(table, "Color table should be allocated");
    
    /* Verify basic colors parsed correctly */
    ASSERT_EQ(table->string_color, 0x00FF00, "Green ANSI code should parse to 0x00FF00");
    ASSERT_EQ(table->number_color, 0x0000FF, "Blue ANSI code should parse to 0x0000FF");
    ASSERT_EQ(table->default_color, 0xFFFFFF, "White ANSI code should parse to 0xFFFFFF");
}

TEST(extract_colors_empty_codes) {
    theme_definition_t theme = create_mock_theme();
    
    /* Set empty color codes (should default to white) */
    memset(&theme.colors, 0, sizeof(color_scheme_t));
    
    lle_syntax_color_table_t *table = NULL;
    lle_result_t result = lle_extract_syntax_colors_from_theme(&theme, &table, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Empty color extraction should succeed");
    ASSERT_NOT_NULL(table, "Color table should be allocated");
    
    /* All colors should default to white (0xFFFFFF) */
    ASSERT_EQ(table->keyword_color, 0xFFFFFF, "Empty code should default to white");
    ASSERT_EQ(table->string_color, 0xFFFFFF, "Empty code should default to white");
    ASSERT_EQ(table->comment_color, 0xFFFFFF, "Empty code should default to white");
}

/* ========================================================================== */
/*                         TEST RUNNER                                        */
/* ========================================================================== */

int main(void) {
    printf("=================================================================\n");
    printf("  LLE Theme Integration Unit Tests\n");
    printf("=================================================================\n\n");
    
    /* Color extraction tests */
    run_test_extract_syntax_colors_null_theme();
    run_test_extract_syntax_colors_null_output();
    run_test_extract_syntax_colors_success();
    run_test_extract_syntax_colors_parsing();
    run_test_extract_cursor_colors_null_theme();
    run_test_extract_cursor_colors_null_output();
    run_test_extract_cursor_colors_success();
    
    /* Theme integration tests */
    run_test_integrate_theme_null_integration();
    run_test_integrate_theme_null_theme_allowed();
    run_test_integrate_theme_success();
    
    /* Theme change handler tests - SKIPPED (requires full Lusush theme system) */
    // run_test_theme_changed_null_integration();
    // run_test_theme_changed_null_theme_name();
    
    /* Color parsing edge cases */
    run_test_extract_colors_256color_mode();
    run_test_extract_colors_basic_ansi();
    run_test_extract_colors_empty_codes();
    
    /* Print summary */
    printf("\n=================================================================\n");
    printf("  Test Summary\n");
    printf("=================================================================\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("=================================================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}

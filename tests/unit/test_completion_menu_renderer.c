/**
 * @file test_completion_menu_renderer.c
 * @brief Unit tests for completion menu renderer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lle/completion/completion_menu_renderer.h"
#include "lle/completion/completion_menu_state.h"
#include "lle/completion/completion_types.h"

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  Testing: %s... ", name); \
        fflush(stdout); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return; \
        } \
    } while(0)

/* ========================================================================== */
/*                              HELPER FUNCTIONS                              */
/* ========================================================================== */

static lle_completion_item_t *create_test_items(size_t count) {
    lle_completion_item_t *items = calloc(count, sizeof(lle_completion_item_t));
    assert(items != NULL);
    
    for (size_t i = 0; i < count; i++) {
        items[i].text = malloc(32);
        snprintf((char*)items[i].text, 32, "item_%zu", i);
        items[i].type = (i < count/2) ? LLE_COMPLETION_TYPE_FILE : LLE_COMPLETION_TYPE_COMMAND;
        items[i].type_indicator = (items[i].type == LLE_COMPLETION_TYPE_FILE) ? "/" : NULL;
        items[i].description = NULL;
    }
    
    return items;
}

static void free_test_items(lle_completion_item_t *items, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free((void*)items[i].text);
    }
    free(items);
}

static lle_completion_result_t *create_test_result(size_t count) {
    lle_completion_result_t *result = calloc(1, sizeof(lle_completion_result_t));
    assert(result != NULL);
    
    result->items = create_test_items(count);
    result->count = count;
    result->capacity = count;
    
    return result;
}

static void free_test_result(lle_completion_result_t *result) {
    if (!result) return;
    free_test_items(result->items, result->count);
    free(result);
}

/* ========================================================================== */
/*                                  TESTS                                     */
/* ========================================================================== */

static void test_default_options(void) {
    TEST("default options creation");
    
    lle_menu_render_options_t options = lle_menu_renderer_default_options(100);
    
    ASSERT(options.show_category_headers == true, "category headers enabled");
    ASSERT(options.show_type_indicators == true, "type indicators enabled");
    ASSERT(options.use_multi_column == true, "multi-column enabled");
    ASSERT(options.highlight_selection == true, "highlighting enabled");
    ASSERT(options.max_rows == 20, "max rows is 20");
    ASSERT(options.terminal_width == 100, "terminal width set correctly");
    ASSERT(options.selection_prefix != NULL, "selection prefix set");
    ASSERT(options.item_separator != NULL, "item separator set");
    
    PASS();
}

static void test_column_width_calculation(void) {
    TEST("column width calculation");
    
    lle_completion_item_t *items = create_test_items(5);
    
    // Test with reasonable terminal width
    size_t width = lle_menu_renderer_calculate_column_width(items, 5, 80, 4);
    ASSERT(width >= LLE_MENU_RENDERER_MIN_COL_WIDTH, "width >= minimum");
    ASSERT(width <= 80, "width <= terminal width");
    
    // Test with NULL items
    width = lle_menu_renderer_calculate_column_width(NULL, 5, 80, 4);
    ASSERT(width == LLE_MENU_RENDERER_MIN_COL_WIDTH, "NULL items returns minimum");
    
    // Test with zero count
    width = lle_menu_renderer_calculate_column_width(items, 0, 80, 4);
    ASSERT(width == LLE_MENU_RENDERER_MIN_COL_WIDTH, "zero count returns minimum");
    
    free_test_items(items, 5);
    PASS();
}

static void test_column_count_calculation(void) {
    TEST("column count calculation");
    
    // Test normal case
    size_t cols = lle_menu_renderer_calculate_columns(80, 20, 2);
    ASSERT(cols >= 1, "at least 1 column");
    ASSERT(cols <= LLE_MENU_RENDERER_MAX_COLS, "respects max columns");
    
    // Test narrow terminal
    cols = lle_menu_renderer_calculate_columns(30, 20, 2);
    ASSERT(cols >= 1, "narrow terminal has at least 1 column");
    
    // Test zero width (edge case)
    cols = lle_menu_renderer_calculate_columns(0, 20, 2);
    ASSERT(cols == 1, "zero terminal width returns 1");
    
    // Test zero column width (edge case)
    cols = lle_menu_renderer_calculate_columns(80, 0, 2);
    ASSERT(cols == 1, "zero column width returns 1");
    
    PASS();
}

static void test_format_category_header(void) {
    TEST("category header formatting");
    
    char buffer[128];
    lle_result_t result;
    
    // Test with bold
    result = lle_menu_renderer_format_category_header(
        LLE_COMPLETION_TYPE_FILE,
        buffer,
        sizeof(buffer),
        true
    );
    ASSERT(result == LLE_SUCCESS, "formatting succeeds");
    ASSERT(strlen(buffer) > 0, "output not empty");
    ASSERT(strstr(buffer, "file") != NULL, "contains type name");
    ASSERT(strstr(buffer, LLE_MENU_CATEGORY_START) != NULL, "contains bold start");
    
    // Test without bold
    result = lle_menu_renderer_format_category_header(
        LLE_COMPLETION_TYPE_COMMAND,
        buffer,
        sizeof(buffer),
        false
    );
    ASSERT(result == LLE_SUCCESS, "formatting succeeds");
    ASSERT(strstr(buffer, "command") != NULL, "contains type name");
    ASSERT(strstr(buffer, LLE_MENU_CATEGORY_START) == NULL, "no bold codes");
    
    // Test NULL output
    result = lle_menu_renderer_format_category_header(
        LLE_COMPLETION_TYPE_FILE,
        NULL,
        sizeof(buffer),
        true
    );
    ASSERT(result == LLE_ERROR_INVALID_PARAMETER, "NULL output returns error");
    
    // Test buffer overflow
    char small[5];
    result = lle_menu_renderer_format_category_header(
        LLE_COMPLETION_TYPE_FILE,
        small,
        sizeof(small),
        true
    );
    ASSERT(result == LLE_ERROR_BUFFER_OVERFLOW, "small buffer returns overflow");
    
    PASS();
}

static void test_format_item(void) {
    TEST("item formatting");
    
    lle_completion_item_t item = {
        .text = "test_file",
        .type = LLE_COMPLETION_TYPE_FILE,
        .type_indicator = "/",
        .description = NULL
    };
    
    char buffer[128];
    lle_result_t result;
    
    // Test normal item
    result = lle_menu_renderer_format_item(
        &item,
        false,  // not selected
        true,   // show indicator
        "> ",
        buffer,
        sizeof(buffer)
    );
    ASSERT(result == LLE_SUCCESS, "formatting succeeds");
    ASSERT(strstr(buffer, "test_file") != NULL, "contains item text");
    ASSERT(strstr(buffer, "/") != NULL, "contains type indicator");
    
    // Test selected item
    result = lle_menu_renderer_format_item(
        &item,
        true,   // selected
        true,
        "> ",
        buffer,
        sizeof(buffer)
    );
    ASSERT(result == LLE_SUCCESS, "formatting succeeds");
    ASSERT(strstr(buffer, LLE_MENU_SELECTION_START) != NULL, "contains selection highlight");
    
    // Test without indicator
    result = lle_menu_renderer_format_item(
        &item,
        false,
        false,  // no indicator
        "> ",
        buffer,
        sizeof(buffer)
    );
    ASSERT(result == LLE_SUCCESS, "formatting succeeds");
    ASSERT(strstr(buffer, "test_file") != NULL, "contains item text");
    
    // Test NULL item
    result = lle_menu_renderer_format_item(
        NULL,
        false,
        true,
        "> ",
        buffer,
        sizeof(buffer)
    );
    ASSERT(result == LLE_ERROR_INVALID_PARAMETER, "NULL item returns error");
    
    PASS();
}

static void test_estimate_size(void) {
    TEST("size estimation");
    
    lle_completion_result_t *result = create_test_result(10);
    lle_completion_menu_state_t state = {0};
    state.result = result;
    state.visible_count = 5;
    state.menu_active = true;
    
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    
    size_t estimate = lle_menu_renderer_estimate_size(&state, &options);
    ASSERT(estimate > 0, "estimate is positive");
    ASSERT(estimate <= LLE_MENU_RENDERER_MAX_OUTPUT, "estimate within max");
    
    // Test with NULL state
    estimate = lle_menu_renderer_estimate_size(NULL, &options);
    ASSERT(estimate > 0, "NULL state returns default");
    
    free_test_result(result);
    PASS();
}

static void test_render_empty_menu(void) {
    TEST("rendering empty/inactive menu");
    
    lle_completion_result_t result = {0};
    lle_completion_menu_state_t state = {0};
    state.result = &result;
    state.menu_active = false;
    
    char output[1024];
    lle_menu_render_stats_t stats;
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    
    lle_result_t res = lle_completion_menu_render(
        &state,
        &options,
        output,
        sizeof(output),
        &stats
    );
    
    ASSERT(res == LLE_SUCCESS, "rendering succeeds");
    ASSERT(output[0] == '\0', "output is empty for inactive menu");
    ASSERT(stats.items_rendered == 0, "no items rendered");
    
    PASS();
}

static void test_render_simple_menu(void) {
    TEST("rendering simple menu");
    
    lle_completion_result_t *result = create_test_result(5);
    lle_completion_menu_state_t state = {0};
    state.result = result;
    state.menu_active = true;
    state.selected_index = 0;
    state.first_visible = 0;
    state.visible_count = 5;
    
    char output[2048];
    lle_menu_render_stats_t stats;
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    
    lle_result_t res = lle_completion_menu_render(
        &state,
        &options,
        output,
        sizeof(output),
        &stats
    );
    
    ASSERT(res == LLE_SUCCESS, "rendering succeeds");
    ASSERT(strlen(output) > 0, "output not empty");
    ASSERT(stats.items_rendered == 5, "all items rendered");
    ASSERT(stats.rows_used > 0, "rows used");
    ASSERT(!stats.truncated, "not truncated");
    
    // Verify items appear in output
    ASSERT(strstr(output, "item_0") != NULL, "item 0 present");
    ASSERT(strstr(output, "item_4") != NULL, "item 4 present");
    
    free_test_result(result);
    PASS();
}

static void test_render_with_categories(void) {
    TEST("rendering with category headers");
    
    lle_completion_result_t *result = create_test_result(10);
    lle_completion_menu_state_t state = {0};
    state.result = result;
    state.menu_active = true;
    state.selected_index = 0;
    state.first_visible = 0;
    state.visible_count = 10;
    
    char output[4096];
    lle_menu_render_stats_t stats;
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    options.show_category_headers = true;
    
    lle_result_t res = lle_completion_menu_render(
        &state,
        &options,
        output,
        sizeof(output),
        &stats
    );
    
    ASSERT(res == LLE_SUCCESS, "rendering succeeds");
    ASSERT(stats.items_rendered == 10, "all items rendered");
    ASSERT(stats.categories_shown > 0, "categories shown");
    ASSERT(strstr(output, "completing") != NULL, "category header present");
    
    free_test_result(result);
    PASS();
}

static void test_render_scrolled_menu(void) {
    TEST("rendering scrolled menu (partial view)");
    
    lle_completion_result_t *result = create_test_result(20);
    lle_completion_menu_state_t state = {0};
    state.result = result;
    state.menu_active = true;
    state.selected_index = 10;
    state.first_visible = 5;   // Start at item 5
    state.visible_count = 10;   // Show 10 items
    
    char output[4096];
    lle_menu_render_stats_t stats;
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    
    lle_result_t res = lle_completion_menu_render(
        &state,
        &options,
        output,
        sizeof(output),
        &stats
    );
    
    ASSERT(res == LLE_SUCCESS, "rendering succeeds");
    ASSERT(stats.items_rendered == 10, "visible items rendered");
    
    // Verify visible range
    ASSERT(strstr(output, "item_5") != NULL, "first visible item present");
    ASSERT(strstr(output, "item_14") != NULL, "last visible item present");
    ASSERT(strstr(output, "item_0") == NULL, "before-range item not present");
    ASSERT(strstr(output, "item_19") == NULL, "after-range item not present");
    
    free_test_result(result);
    PASS();
}

static void test_render_with_max_rows_limit(void) {
    TEST("rendering with max rows limit");
    
    lle_completion_result_t *result = create_test_result(50);  // More items to force truncation
    lle_completion_menu_state_t state = {0};
    state.result = result;
    state.menu_active = true;
    state.selected_index = 0;
    state.first_visible = 0;
    state.visible_count = 50;
    
    char output[4096];
    lle_menu_render_stats_t stats;
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    options.max_rows = 5;  // Limit to 5 rows
    options.show_category_headers = false;  // Disable headers to simplify test
    options.use_multi_column = false;  // Single column to ensure truncation
    
    lle_result_t res = lle_completion_menu_render(
        &state,
        &options,
        output,
        sizeof(output),
        &stats
    );
    
    ASSERT(res == LLE_SUCCESS, "rendering succeeds");
    ASSERT(stats.rows_used <= 5, "respects max rows");
    // With single column and max 5 rows, should render at most 5 items
    ASSERT(stats.items_rendered <= 5, "items limited by max rows");
    ASSERT(stats.items_rendered < 50, "not all items rendered");
    
    free_test_result(result);
    PASS();
}

static void test_render_invalid_params(void) {
    TEST("rendering with invalid parameters");
    
    char output[1024];
    lle_menu_render_stats_t stats;
    lle_menu_render_options_t options = lle_menu_renderer_default_options(80);
    
    lle_completion_result_t *result = create_test_result(5);
    lle_completion_menu_state_t state = {0};
    state.result = result;
    state.menu_active = true;
    
    // Test NULL state
    lle_result_t res = lle_completion_menu_render(
        NULL,
        &options,
        output,
        sizeof(output),
        &stats
    );
    ASSERT(res == LLE_ERROR_INVALID_PARAMETER, "NULL state returns error");
    
    // Test NULL output
    res = lle_completion_menu_render(
        &state,
        &options,
        NULL,
        sizeof(output),
        &stats
    );
    ASSERT(res == LLE_ERROR_INVALID_PARAMETER, "NULL output returns error");
    
    // Test zero output size
    res = lle_completion_menu_render(
        &state,
        &options,
        output,
        0,
        &stats
    );
    ASSERT(res == LLE_ERROR_INVALID_PARAMETER, "zero size returns error");
    
    free_test_result(result);
    PASS();
}

/* ========================================================================== */
/*                                  MAIN                                      */
/* ========================================================================== */

int main(void) {
    printf("\n=== Completion Menu Renderer Unit Tests ===\n\n");
    
    // Configuration tests
    test_default_options();
    test_column_width_calculation();
    test_column_count_calculation();
    
    // Formatting tests
    test_format_category_header();
    test_format_item();
    test_estimate_size();
    
    // Rendering tests
    test_render_empty_menu();
    test_render_simple_menu();
    test_render_with_categories();
    test_render_scrolled_menu();
    test_render_with_max_rows_limit();
    test_render_invalid_params();
    
    printf("\n=== Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    if (tests_passed == tests_run) {
        printf("\n✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed!\n\n");
        return 1;
    }
}

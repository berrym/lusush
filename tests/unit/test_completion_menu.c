/*
 * Comprehensive Unit Tests for Interactive Completion Menu (Phase 2)
 * 
 * Test Coverage:
 * - Menu lifecycle (create/free)
 * - Menu configuration
 * - Navigation (up/down/left/right/page/home/end)
 * - Selection management
 * - Display rendering
 * - Category handling
 * - Scrolling viewport
 * - Terminal dimension handling
 * - Edge cases and error handling
 */

#include "../../include/completion_types.h"
#include "../../include/completion_menu.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// Mock implementations to avoid linking with full lusush
// ============================================================================

typedef struct builtin_s {
    const char *name;
    const char *doc;
    void *func;
} builtin;

builtin builtins[] = {
    {"cd", "Change directory", NULL},
    {"echo", "Echo arguments", NULL},
    {"pwd", "Print working directory", NULL},
    {"exit", "Exit shell", NULL},
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

// Create sample completion result for testing
static completion_result_t* create_test_completions(void) {
    completion_result_t *result = completion_result_create(16);
    
    // Add builtins
    completion_result_add(result, "cd", " ", COMPLETION_TYPE_BUILTIN, 900);
    completion_result_add(result, "echo", " ", COMPLETION_TYPE_BUILTIN, 900);
    completion_result_add(result, "pwd", " ", COMPLETION_TYPE_BUILTIN, 900);
    
    // Add commands
    completion_result_add(result, "ls", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "grep", " ", COMPLETION_TYPE_COMMAND, 800);
    
    // Add files
    completion_result_add(result, "file1.txt", " ", COMPLETION_TYPE_FILE, 600);
    completion_result_add(result, "file2.c", " ", COMPLETION_TYPE_FILE, 600);
    
    // Add directory
    completion_result_add(result, "src/", "/", COMPLETION_TYPE_DIRECTORY, 700);
    
    completion_result_sort(result);
    return result;
}

// ============================================================================
// Menu Lifecycle Tests
// ============================================================================

TEST(menu_create_default_config) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    assert(menu != NULL);
    assert(menu->result == result);
    assert(menu->selected_index == 0);
    assert(menu->first_visible == 0);
    // Menu is created but not yet active (not displayed)
    assert(menu->menu_active == false);
    
    // Check default config
    assert(menu->config.max_visible_items > 0);
    assert(menu->config.show_category_headers == true);
    assert(menu->config.show_type_indicators == true);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(menu_create_custom_config) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 5,
        .show_category_headers = false,
        .show_type_indicators = false,
        .show_descriptions = false,
        .enable_scrolling = false,
        .highlight_selection = false,
        .min_items_for_menu = 2
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    assert(menu != NULL);
    assert(menu->config.max_visible_items == 5);
    assert(menu->config.show_category_headers == false);
    assert(menu->config.show_type_indicators == false);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(menu_create_null_result) {
    completion_menu_t *menu = completion_menu_create(NULL, NULL);
    assert(menu == NULL);
}

TEST(menu_free_null) {
    completion_menu_free(NULL);  // Should not crash
}

TEST(menu_create_empty_result) {
    completion_result_t *result = completion_result_create(8);
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Empty results return NULL (no menu to show)
    assert(menu == NULL);
    
    completion_result_free(result);
}

// ============================================================================
// Navigation Tests - Vertical Movement
// ============================================================================

TEST(navigate_down_basic) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    assert(menu->selected_index == 0);
    
    bool changed = completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(changed == true);
    assert(menu->selected_index == 1);
    
    changed = completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(changed == true);
    assert(menu->selected_index == 2);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_up_basic) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Move down first
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(menu->selected_index == 2);
    
    // Move up
    bool changed = completion_menu_navigate(menu, MENU_NAV_UP);
    assert(changed == true);
    assert(menu->selected_index == 1);
    
    changed = completion_menu_navigate(menu, MENU_NAV_UP);
    assert(changed == true);
    assert(menu->selected_index == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_down_wraps_to_start) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    size_t last_index = result->count - 1;
    
    // Move to last item
    while (menu->selected_index < last_index) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    assert(menu->selected_index == last_index);
    
    // Navigate down should wrap to start
    bool changed = completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(changed == true);
    assert(menu->selected_index == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_up_wraps_to_end) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    assert(menu->selected_index == 0);
    
    // Navigate up from start should wrap to end
    bool changed = completion_menu_navigate(menu, MENU_NAV_UP);
    assert(changed == true);
    assert(menu->selected_index == result->count - 1);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_home_end) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Move to middle
    for (int i = 0; i < 3; i++) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    assert(menu->selected_index == 3);
    
    // Home should go to start
    bool changed = completion_menu_navigate(menu, MENU_NAV_HOME);
    assert(changed == true);
    assert(menu->selected_index == 0);
    
    // End should go to last
    changed = completion_menu_navigate(menu, MENU_NAV_END);
    assert(changed == true);
    assert(menu->selected_index == result->count - 1);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_page_down) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 3,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = true,
        .enable_scrolling = true,
        .highlight_selection = true,
        .min_items_for_menu = 1
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    assert(menu->selected_index == 0);
    
    // Page down should advance by max_visible_items
    bool changed = completion_menu_navigate(menu, MENU_NAV_PAGE_DOWN);
    assert(changed == true);
    assert(menu->selected_index == 3);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_page_up) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 3,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = true,
        .enable_scrolling = true,
        .highlight_selection = true,
        .min_items_for_menu = 1
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    // Move to middle first
    for (int i = 0; i < 4; i++) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    assert(menu->selected_index == 4);
    
    // Page up should go back by max_visible_items
    bool changed = completion_menu_navigate(menu, MENU_NAV_PAGE_UP);
    assert(changed == true);
    assert(menu->selected_index == 1);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

// ============================================================================
// Navigation Tests - Horizontal Movement (Categories)
// ============================================================================

TEST(navigate_right_to_next_category) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    assert(menu->selected_index == 0);
    assert(menu->result->items[0].type == COMPLETION_TYPE_BUILTIN);
    
    // Navigate right should jump to next category
    bool changed = completion_menu_navigate(menu, MENU_NAV_RIGHT);
    assert(changed == true);
    
    // Should now be on a different type
    completion_type_t new_type = menu->result->items[menu->selected_index].type;
    assert(new_type != COMPLETION_TYPE_BUILTIN);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_left_to_prev_category) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Move to second category first
    completion_menu_navigate(menu, MENU_NAV_RIGHT);
    size_t second_category_index = menu->selected_index;
    completion_type_t second_category_type = menu->result->items[second_category_index].type;
    
    // Navigate left should return to first category
    bool changed = completion_menu_navigate(menu, MENU_NAV_LEFT);
    assert(changed == true);
    
    completion_type_t current_type = menu->result->items[menu->selected_index].type;
    assert(current_type != second_category_type);
    assert(current_type == COMPLETION_TYPE_BUILTIN);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_right_wraps_categories) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    completion_type_t first_type = menu->result->items[0].type;
    
    // Navigate right through all categories
    size_t iterations = 0;
    const size_t max_iterations = 20;  // Safety limit
    completion_type_t current_type;
    
    do {
        completion_menu_navigate(menu, MENU_NAV_RIGHT);
        iterations++;
        current_type = menu->result->items[menu->selected_index].type;
    } while (current_type != first_type && iterations < max_iterations);
    
    // Should have wrapped back to first category type
    assert(current_type == first_type);
    assert(iterations < max_iterations);
    assert(iterations >= menu->category_count);  // Should have visited all categories
    
    completion_menu_free(menu);
    completion_result_free(result);
}

// ============================================================================
// Selection Management Tests
// ============================================================================

TEST(get_selected_item) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    const completion_item_t *item = completion_menu_get_selected(menu);
    assert(item != NULL);
    assert(item == &result->items[0]);
    
    // Navigate and check again
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    item = completion_menu_get_selected(menu);
    assert(item != NULL);
    assert(item == &result->items[1]);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(get_selected_text) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    const char *text = completion_menu_get_selected_text(menu);
    assert(text != NULL);
    assert(strcmp(text, result->items[0].text) == 0);
    
    // Navigate and check again
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    text = completion_menu_get_selected_text(menu);
    assert(text != NULL);
    assert(strcmp(text, result->items[2].text) == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(get_selected_from_null_menu) {
    const completion_item_t *item = completion_menu_get_selected(NULL);
    assert(item == NULL);
    
    const char *text = completion_menu_get_selected_text(NULL);
    assert(text == NULL);
}

TEST(get_selected_from_empty_menu) {
    completion_result_t *result = completion_result_create(8);
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Empty result returns NULL menu
    assert(menu == NULL);
    
    const completion_item_t *item = completion_menu_get_selected(menu);
    assert(item == NULL);
    
    const char *text = completion_menu_get_selected_text(menu);
    assert(text == NULL);
    
    completion_result_free(result);
}

// ============================================================================
// Scrolling and Viewport Tests
// ============================================================================

TEST(scrolling_follows_selection_down) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 3,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = true,
        .enable_scrolling = true,
        .highlight_selection = true,
        .min_items_for_menu = 1
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    assert(menu->first_visible == 0);
    
    // Navigate down beyond visible area
    for (int i = 0; i < 4; i++) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    
    // Viewport should have scrolled
    assert(menu->first_visible > 0);
    assert(menu->selected_index >= menu->first_visible);
    assert(menu->selected_index < menu->first_visible + menu->visible_count);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(scrolling_follows_selection_up) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 3,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = true,
        .enable_scrolling = true,
        .highlight_selection = true,
        .min_items_for_menu = 1
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    // Navigate down to scroll viewport
    for (int i = 0; i < 5; i++) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    
    size_t previous_first = menu->first_visible;
    size_t previous_selected = menu->selected_index;
    
    // Navigate up multiple times to trigger scrolling back
    for (int i = 0; i < 4; i++) {
        completion_menu_navigate(menu, MENU_NAV_UP);
    }
    
    // Selection should have moved up and viewport should follow
    assert(menu->selected_index < previous_selected);
    assert(menu->selected_index >= menu->first_visible);
    assert(menu->selected_index < menu->first_visible + menu->visible_count);
    // If we scrolled back far enough, first_visible should be less
    if (menu->selected_index < previous_first) {
        assert(menu->first_visible <= menu->selected_index);
    }
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(scrolling_disabled) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 3,
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = true,
        .enable_scrolling = false,
        .highlight_selection = true,
        .min_items_for_menu = 1
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    assert(menu->first_visible == 0);
    
    // Navigate down
    for (int i = 0; i < 5; i++) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    
    // Viewport should not have scrolled
    assert(menu->first_visible == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

// ============================================================================
// Display and Rendering Tests
// ============================================================================

TEST(display_menu_basic) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Should not crash - output goes to stdout
    bool success = completion_menu_display(menu);
    assert(success == true);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(display_null_menu) {
    bool success = completion_menu_display(NULL);
    assert(success == false);
}

TEST(clear_menu) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Display then clear - should not crash
    completion_menu_display(menu);
    completion_menu_clear(menu);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(clear_null_menu) {
    completion_menu_clear(NULL);  // Should not crash
}

TEST(refresh_menu) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    completion_menu_display(menu);
    
    // Navigate and refresh
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    bool success = completion_menu_refresh(menu);
    assert(success == true);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(refresh_null_menu) {
    bool success = completion_menu_refresh(NULL);
    assert(success == false);
}

// ============================================================================
// Menu State Tests
// ============================================================================

TEST(menu_is_active) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Menu is created but not yet displayed, so not active
    assert(completion_menu_is_active(menu) == false);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(menu_is_active_null) {
    assert(completion_menu_is_active(NULL) == false);
}

TEST(menu_get_item_count) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    size_t count = completion_menu_get_item_count(menu);
    assert(count == result->count);
    assert(count == 8);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(menu_get_item_count_null) {
    size_t count = completion_menu_get_item_count(NULL);
    assert(count == 0);
}

TEST(menu_get_selected_index) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    assert(completion_menu_get_selected_index(menu) == 0);
    
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(completion_menu_get_selected_index(menu) == 2);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(menu_get_selected_index_null) {
    size_t index = completion_menu_get_selected_index(NULL);
    assert(index == 0);
}

// ============================================================================
// Terminal Dimension Tests
// ============================================================================

TEST(terminal_dimensions_are_set) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Terminal dimensions should be detected or use defaults
    assert(menu->terminal_width > 0);
    assert(menu->terminal_height > 0);
    
    // Default fallback values should be reasonable
    assert(menu->terminal_width >= 80);
    assert(menu->terminal_height >= 24);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST(navigate_single_item) {
    completion_result_t *result = completion_result_create(8);
    completion_result_add(result, "single", " ", COMPLETION_TYPE_COMMAND, 800);
    
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    assert(menu->selected_index == 0);
    
    // Navigation should not change anything with single item
    completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(menu->selected_index == 0);
    
    completion_menu_navigate(menu, MENU_NAV_UP);
    assert(menu->selected_index == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(navigate_empty_menu) {
    completion_result_t *result = completion_result_create(8);
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Empty result returns NULL menu
    assert(menu == NULL);
    
    // Navigation on NULL menu should return false
    bool changed = completion_menu_navigate(menu, MENU_NAV_DOWN);
    assert(changed == false);
    
    changed = completion_menu_navigate(menu, MENU_NAV_UP);
    assert(changed == false);
    
    completion_result_free(result);
}

TEST(navigate_null_menu) {
    bool changed = completion_menu_navigate(NULL, MENU_NAV_DOWN);
    assert(changed == false);
}

TEST(large_menu_navigation) {
    completion_result_t *result = completion_result_create(64);
    
    // Create large menu
    for (int i = 0; i < 50; i++) {
        char name[32];
        snprintf(name, sizeof(name), "item%d", i);
        completion_result_add(result, name, " ", COMPLETION_TYPE_COMMAND, 800);
    }
    
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Navigate through entire menu
    for (int i = 0; i < 50; i++) {
        completion_menu_navigate(menu, MENU_NAV_DOWN);
    }
    
    // Should wrap back to start
    assert(menu->selected_index == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(config_with_zero_max_visible) {
    completion_result_t *result = create_test_completions();
    
    completion_menu_config_t config = {
        .max_visible_items = 0,  // Invalid
        .show_category_headers = true,
        .show_type_indicators = true,
        .show_descriptions = true,
        .enable_scrolling = true,
        .highlight_selection = true,
        .min_items_for_menu = 1
    };
    
    completion_menu_t *menu = completion_menu_create(result, &config);
    
    // Should handle gracefully with default
    assert(menu != NULL);
    assert(menu->config.max_visible_items > 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

// ============================================================================
// Category Position Tests
// ============================================================================

TEST(category_positions_calculated) {
    completion_result_t *result = create_test_completions();
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Menu should have category positions calculated
    assert(menu->category_count > 0);
    assert(menu->category_positions != NULL);
    
    // First category should start at 0
    assert(menu->category_positions[0] == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

TEST(single_category_positions) {
    completion_result_t *result = completion_result_create(8);
    
    // Add items of single type
    completion_result_add(result, "cmd1", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "cmd2", " ", COMPLETION_TYPE_COMMAND, 800);
    completion_result_add(result, "cmd3", " ", COMPLETION_TYPE_COMMAND, 800);
    
    completion_menu_t *menu = completion_menu_create(result, NULL);
    
    // Should have single category
    assert(menu->category_count == 1);
    assert(menu->category_positions[0] == 0);
    
    completion_menu_free(menu);
    completion_result_free(result);
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  Completion Menu Unit Tests (Phase 2)                         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Running Menu Lifecycle Tests...\n");
    RUN_TEST(menu_create_default_config);
    RUN_TEST(menu_create_custom_config);
    RUN_TEST(menu_create_null_result);
    RUN_TEST(menu_free_null);
    RUN_TEST(menu_create_empty_result);
    
    printf("\nRunning Navigation Tests - Vertical...\n");
    RUN_TEST(navigate_down_basic);
    RUN_TEST(navigate_up_basic);
    RUN_TEST(navigate_down_wraps_to_start);
    RUN_TEST(navigate_up_wraps_to_end);
    RUN_TEST(navigate_home_end);
    RUN_TEST(navigate_page_down);
    RUN_TEST(navigate_page_up);
    
    printf("\nRunning Navigation Tests - Horizontal...\n");
    RUN_TEST(navigate_right_to_next_category);
    RUN_TEST(navigate_left_to_prev_category);
    RUN_TEST(navigate_right_wraps_categories);
    
    printf("\nRunning Selection Management Tests...\n");
    RUN_TEST(get_selected_item);
    RUN_TEST(get_selected_text);
    RUN_TEST(get_selected_from_null_menu);
    RUN_TEST(get_selected_from_empty_menu);
    
    printf("\nRunning Scrolling and Viewport Tests...\n");
    RUN_TEST(scrolling_follows_selection_down);
    RUN_TEST(scrolling_follows_selection_up);
    RUN_TEST(scrolling_disabled);
    
    printf("\nRunning Display and Rendering Tests...\n");
    RUN_TEST(display_menu_basic);
    RUN_TEST(display_null_menu);
    RUN_TEST(clear_menu);
    RUN_TEST(clear_null_menu);
    RUN_TEST(refresh_menu);
    RUN_TEST(refresh_null_menu);
    
    printf("\nRunning Menu State Tests...\n");
    RUN_TEST(menu_is_active);
    RUN_TEST(menu_is_active_null);
    RUN_TEST(menu_get_item_count);
    RUN_TEST(menu_get_item_count_null);
    RUN_TEST(menu_get_selected_index);
    RUN_TEST(menu_get_selected_index_null);
    
    printf("\nRunning Terminal Dimension Tests...\n");
    RUN_TEST(terminal_dimensions_are_set);
    
    printf("\nRunning Edge Cases and Error Handling...\n");
    RUN_TEST(navigate_single_item);
    RUN_TEST(navigate_empty_menu);
    RUN_TEST(navigate_null_menu);
    RUN_TEST(large_menu_navigation);
    RUN_TEST(config_with_zero_max_visible);
    
    printf("\nRunning Category Position Tests...\n");
    RUN_TEST(category_positions_calculated);
    RUN_TEST(single_category_positions);
    
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

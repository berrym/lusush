/*
 * Tests for LLE-031: Completion Display
 * 
 * This file contains comprehensive tests for the completion display system
 * including display configuration, navigation, formatting, and integration
 * with the terminal manager.
 *
 * Copyright (c) 2024 Lusush Project
 * SPDX-License-Identifier: MIT
 */

#include "test_framework.h"
#include "../src/line_editor/completion.h"
#include "../src/line_editor/terminal_manager.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

/* Test completion display creation and initialization */
LLE_TEST(completion_display_create_and_destroy) {
    printf("Testing completion display creation and destruction... ");
    
    lle_completion_list_t *list = lle_completion_list_create(10);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add some test items
    LLE_ASSERT(lle_completion_list_add(list, "test1.txt", "Test file 1", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "test2.c", "Test file 2", LLE_COMPLETION_PRIORITY_HIGH));
    LLE_ASSERT(lle_completion_list_add(list, "example.h", "Header file", LLE_COMPLETION_PRIORITY_LOW));
    
    lle_completion_display_t *display = lle_completion_display_create(list, 5);
    LLE_ASSERT_NOT_NULL(display);
    LLE_ASSERT(display->completions == list);
    LLE_ASSERT_EQ(display->max_display_items, 5);
    LLE_ASSERT(display->show_descriptions == true);
    LLE_ASSERT(display->show_selection == true);
    LLE_ASSERT(display->use_colors == true);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display initialization */
LLE_TEST(completion_display_init_and_clear) {
    printf("Testing completion display initialization and clearing... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    
    LLE_ASSERT(lle_completion_list_add(list, "file1", "Description 1", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "file2", "Description 2", LLE_COMPLETION_PRIORITY_HIGH));
    
    lle_completion_display_t display;
    bool result = lle_completion_display_init(&display, list, 3);
    LLE_ASSERT(result == true);
    LLE_ASSERT(display.completions == list);
    LLE_ASSERT_EQ(display.max_display_items, 3);
    LLE_ASSERT_EQ(display.display_start, 0);
    LLE_ASSERT(display.show_descriptions == true);
    LLE_ASSERT(display.show_selection == true);
    
    lle_completion_display_clear(&display);
    LLE_ASSERT(display.completions == NULL);
    LLE_ASSERT_EQ(display.max_display_items, 0);
    
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display with null parameters */
LLE_TEST(completion_display_null_parameters) {
    printf("Testing completion display with null parameters... ");
    
    // Test create with NULL list
    lle_completion_display_t *display = lle_completion_display_create(NULL, 5);
    LLE_ASSERT_NULL(display);
    
    // Test init with NULL parameters
    lle_completion_display_t test_display;
    LLE_ASSERT(lle_completion_display_init(NULL, NULL, 5) == false);
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    LLE_ASSERT(lle_completion_display_init(NULL, list, 5) == false);
    LLE_ASSERT(lle_completion_display_init(&test_display, NULL, 5) == false);
    
    // Test clear and destroy with NULL
    lle_completion_display_clear(NULL); // Should not crash
    lle_completion_display_destroy(NULL); // Should not crash
    
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display viewport calculation */
LLE_TEST(completion_display_viewport_update) {
    printf("Testing completion display viewport updates... ");
    
    lle_completion_list_t *list = lle_completion_list_create(10);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add 8 items to test scrolling
    for (int i = 0; i < 8; i++) {
        char text[16], desc[32];
        snprintf(text, sizeof(text), "item%d", i);
        snprintf(desc, sizeof(desc), "Description %d", i);
        LLE_ASSERT(lle_completion_list_add(list, text, desc, LLE_COMPLETION_PRIORITY_NORMAL));
    }
    
    lle_completion_display_t *display = lle_completion_display_create(list, 3);
    LLE_ASSERT_NOT_NULL(display);
    
    // Initially should show items 0-2
    LLE_ASSERT_EQ(display->display_start, 0);
    LLE_ASSERT_EQ(display->completions->selected, 0);
    
    // Select item 4 (should scroll to show items 2-4)
    LLE_ASSERT(lle_completion_list_set_selected(list, 4));
    LLE_ASSERT(lle_completion_display_update_viewport(display));
    LLE_ASSERT_EQ(display->display_start, 2);
    
    // Select item 7 (should scroll to show items 5-7)
    LLE_ASSERT(lle_completion_list_set_selected(list, 7));
    LLE_ASSERT(lle_completion_display_update_viewport(display));
    LLE_ASSERT_EQ(display->display_start, 5);
    
    // Select item 0 (should scroll back to show items 0-2)
    LLE_ASSERT(lle_completion_list_set_selected(list, 0));
    LLE_ASSERT(lle_completion_display_update_viewport(display));
    LLE_ASSERT_EQ(display->display_start, 0);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display navigation */
LLE_TEST(completion_display_navigation) {
    printf("Testing completion display navigation... ");
    
    lle_completion_list_t *list = lle_completion_list_create(10);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add 5 items
    LLE_ASSERT(lle_completion_list_add(list, "first", "First item", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "second", "Second item", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "third", "Third item", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "fourth", "Fourth item", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "fifth", "Fifth item", LLE_COMPLETION_PRIORITY_NORMAL));
    
    lle_completion_display_t *display = lle_completion_display_create(list, 3);
    LLE_ASSERT_NOT_NULL(display);
    
    // Start at item 0
    LLE_ASSERT_EQ(list->selected, 0);
    
    // Navigate down
    LLE_ASSERT(lle_completion_display_navigate(display, 1) == true);
    LLE_ASSERT_EQ(list->selected, 1);
    
    LLE_ASSERT(lle_completion_display_navigate(display, 1) == true);
    LLE_ASSERT_EQ(list->selected, 2);
    
    // Navigate up
    LLE_ASSERT(lle_completion_display_navigate(display, -1) == true);
    LLE_ASSERT_EQ(list->selected, 1);
    
    // Test no change with direction 0
    LLE_ASSERT(lle_completion_display_navigate(display, 0) == false);
    LLE_ASSERT_EQ(list->selected, 1);
    
    // Navigate to end and wrap around
    LLE_ASSERT(lle_completion_list_set_selected(list, 4));
    LLE_ASSERT(lle_completion_display_navigate(display, 1) == true);
    LLE_ASSERT_EQ(list->selected, 0); // Should wrap to beginning
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display color configuration */
LLE_TEST(completion_display_color_configuration) {
    printf("Testing completion display color configuration... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    
    LLE_ASSERT(lle_completion_list_add(list, "test", "Test item", LLE_COMPLETION_PRIORITY_NORMAL));
    
    lle_completion_display_t *display = lle_completion_display_create(list, 5);
    LLE_ASSERT_NOT_NULL(display);
    
    // Test color configuration
    const char *selection_color = "\033[1;32m";
    const char *text_color = "\033[0;37m";
    const char *desc_color = "\033[0;90m";
    
    bool result = lle_completion_display_configure_colors(
        display, true, selection_color, text_color, desc_color);
    LLE_ASSERT(result == true);
    LLE_ASSERT(display->use_colors == true);
    LLE_ASSERT(display->selection_color == selection_color);
    LLE_ASSERT(display->text_color == text_color);
    LLE_ASSERT(display->desc_color == desc_color);
    
    // Test disabling colors
    result = lle_completion_display_configure_colors(display, false, NULL, NULL, NULL);
    LLE_ASSERT(result == true);
    LLE_ASSERT(display->use_colors == false);
    
    // Test with NULL display
    LLE_ASSERT(lle_completion_display_configure_colors(NULL, true, NULL, NULL, NULL) == false);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display layout configuration */
LLE_TEST(completion_display_layout_configuration) {
    printf("Testing completion display layout configuration... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    
    LLE_ASSERT(lle_completion_list_add(list, "test", "Test item", LLE_COMPLETION_PRIORITY_NORMAL));
    
    lle_completion_display_t *display = lle_completion_display_create(list, 5);
    LLE_ASSERT_NOT_NULL(display);
    
    // Test layout configuration
    bool result = lle_completion_display_configure_layout(
        display, false, true, ">>", 8);
    LLE_ASSERT(result == true);
    LLE_ASSERT(display->show_descriptions == false);
    LLE_ASSERT(display->show_selection == true);
    LLE_ASSERT_EQ(display->max_display_items, 8);
    LLE_ASSERT_STR_EQ(display->selection_indicator, ">>");
    
    // Test with different configuration
    result = lle_completion_display_configure_layout(
        display, true, false, "*", 0);
    LLE_ASSERT(result == true);
    LLE_ASSERT(display->show_descriptions == true);
    LLE_ASSERT(display->show_selection == false);
    LLE_ASSERT_STR_EQ(display->selection_indicator, "*");
    // max_items 0 should not change the current value
    LLE_ASSERT_EQ(display->max_display_items, 8);
    
    // Test with NULL display
    LLE_ASSERT(lle_completion_display_configure_layout(NULL, true, true, ">>", 5) == false);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display statistics */
LLE_TEST(completion_display_statistics) {
    printf("Testing completion display statistics... ");
    
    lle_completion_list_t *list = lle_completion_list_create(10);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add 6 items
    for (int i = 0; i < 6; i++) {
        char text[16];
        snprintf(text, sizeof(text), "item%d", i);
        LLE_ASSERT(lle_completion_list_add(list, text, "Description", LLE_COMPLETION_PRIORITY_NORMAL));
    }
    
    lle_completion_display_t *display = lle_completion_display_create(list, 4);
    LLE_ASSERT_NOT_NULL(display);
    
    // Set selection to item 3 (should be visible in initial viewport 0-3, no change needed)
    LLE_ASSERT(lle_completion_list_set_selected(list, 3));
    lle_completion_display_update_viewport(display);  // May return false if no change needed
    
    size_t total_items, visible_items, selected_index, display_start;
    bool result = lle_completion_display_get_stats(
        display, &total_items, &visible_items, &selected_index, &display_start);
    
    LLE_ASSERT(result == true);
    LLE_ASSERT_EQ(total_items, 6);
    LLE_ASSERT_EQ(selected_index, 3);
    
    // Test with NULL parameters (should still work)
    result = lle_completion_display_get_stats(display, NULL, NULL, NULL, NULL);
    LLE_ASSERT(result == true);
    
    // Test with NULL display
    LLE_ASSERT(lle_completion_display_get_stats(NULL, &total_items, NULL, NULL, NULL) == false);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display with empty list */
LLE_TEST(completion_display_empty_list) {
    printf("Testing completion display with empty list... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    LLE_ASSERT_EQ(list->count, 0);
    
    lle_completion_display_t *display = lle_completion_display_create(list, 5);
    LLE_ASSERT_NOT_NULL(display);
    LLE_ASSERT_EQ(display->display_count, 0);
    
    // Navigation should fail with empty list
    LLE_ASSERT(lle_completion_display_navigate(display, 1) == false);
    LLE_ASSERT(lle_completion_display_navigate(display, -1) == false);
    
    // Viewport update should succeed but not change anything
    LLE_ASSERT(lle_completion_display_update_viewport(display) == false);
    
    // Statistics should work with empty list
    size_t total_items;
    LLE_ASSERT(lle_completion_display_get_stats(display, &total_items, NULL, NULL, NULL));
    LLE_ASSERT_EQ(total_items, 0);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display with single item */
LLE_TEST(completion_display_single_item) {
    printf("Testing completion display with single item... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    
    LLE_ASSERT(lle_completion_list_add(list, "single_item", "Only item", LLE_COMPLETION_PRIORITY_NORMAL));
    
    lle_completion_display_t *display = lle_completion_display_create(list, 5);
    LLE_ASSERT_NOT_NULL(display);
    LLE_ASSERT_EQ(list->count, 1);
    LLE_ASSERT_EQ(list->selected, 0);
    
    // Navigation should wrap around with single item
    LLE_ASSERT(lle_completion_display_navigate(display, 1) == true);
    LLE_ASSERT_EQ(list->selected, 0); // Should stay at 0 (wraps around)
    
    LLE_ASSERT(lle_completion_display_navigate(display, -1) == true);
    LLE_ASSERT_EQ(list->selected, 0); // Should stay at 0 (wraps around)
    
    // Viewport should not need updates
    LLE_ASSERT(lle_completion_display_update_viewport(display) == false);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display large list scrolling */
LLE_TEST(completion_display_large_list_scrolling) {
    printf("Testing completion display with large list scrolling... ");
    
    lle_completion_list_t *list = lle_completion_list_create(20);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add 15 items
    for (int i = 0; i < 15; i++) {
        char text[16], desc[32];
        snprintf(text, sizeof(text), "item_%02d", i);
        snprintf(desc, sizeof(desc), "Description for item %d", i);
        LLE_ASSERT(lle_completion_list_add(list, text, desc, LLE_COMPLETION_PRIORITY_NORMAL));
    }
    
    lle_completion_display_t *display = lle_completion_display_create(list, 5);
    LLE_ASSERT_NOT_NULL(display);
    
    // Navigate through the list and check scrolling
    LLE_ASSERT_EQ(display->display_start, 0);
    
    // Move to item 7 (should trigger scrolling)
    for (int i = 0; i < 7; i++) {
        lle_completion_display_navigate(display, 1);
    }
    LLE_ASSERT_EQ(list->selected, 7);
    LLE_ASSERT_EQ(display->display_start, 3); // Should show items 3-7
    
    // Move to item 12 (should scroll more)
    for (int i = 0; i < 5; i++) {
        lle_completion_display_navigate(display, 1);
    }
    LLE_ASSERT_EQ(list->selected, 12);
    LLE_ASSERT_EQ(display->display_start, 8); // Should show items 8-12
    
    // Move to last item
    for (int i = 0; i < 2; i++) {
        lle_completion_display_navigate(display, 1);
    }
    LLE_ASSERT_EQ(list->selected, 14);
    LLE_ASSERT_EQ(display->display_start, 10); // Should show items 10-14
    
    // Navigate back up
    for (int i = 0; i < 14; i++) {
        lle_completion_display_navigate(display, -1);
    }
    LLE_ASSERT_EQ(list->selected, 0);
    LLE_ASSERT_EQ(display->display_start, 0); // Should show items 0-4
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display auto-sizing */
LLE_TEST(completion_display_auto_sizing) {
    printf("Testing completion display auto-sizing... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add 3 items
    LLE_ASSERT(lle_completion_list_add(list, "item1", "Description 1", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "item2", "Description 2", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "item3", "Description 3", LLE_COMPLETION_PRIORITY_NORMAL));
    
    // Create display with auto-sizing (0 max_items)
    lle_completion_display_t *display = lle_completion_display_create(list, 0);
    LLE_ASSERT_NOT_NULL(display);
    LLE_ASSERT_EQ(display->max_display_items, 10); // Should use default
    
    // Configure with specific max items
    LLE_ASSERT(lle_completion_display_configure_layout(display, true, true, NULL, 2));
    LLE_ASSERT_EQ(display->max_display_items, 2);
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Test completion display boundary conditions */
LLE_TEST(completion_display_boundary_conditions) {
    printf("Testing completion display boundary conditions... ");
    
    lle_completion_list_t *list = lle_completion_list_create(5);
    LLE_ASSERT_NOT_NULL(list);
    
    // Add items with various text lengths
    LLE_ASSERT(lle_completion_list_add(list, "a", "Short", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "very_long_filename_that_might_cause_issues.txt", 
                                      "A very long description that might overflow terminal width and cause formatting problems", 
                                      LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "single_char", "Single character prefix", LLE_COMPLETION_PRIORITY_NORMAL));
    LLE_ASSERT(lle_completion_list_add(list, "normal.txt", NULL, LLE_COMPLETION_PRIORITY_NORMAL)); // No description
    
    lle_completion_display_t *display = lle_completion_display_create(list, 4);
    LLE_ASSERT_NOT_NULL(display);
    
    // Test navigation through boundary cases
    for (size_t i = 0; i < list->count; i++) {
        LLE_ASSERT(lle_completion_list_set_selected(list, i));
        lle_completion_display_update_viewport(display);  // May return false if no change needed
        
        size_t total, visible, selected, start;
        LLE_ASSERT(lle_completion_display_get_stats(display, &total, &visible, &selected, &start));
        LLE_ASSERT_EQ(selected, i);
    }
    
    lle_completion_display_destroy(display);
    lle_completion_list_destroy(list);
    printf("PASSED\n");
}

/* Main test function */
int main(void) {
    printf("Running LLE-031 Completion Display Tests...\n\n");
    
    test_completion_display_create_and_destroy();
    test_completion_display_init_and_clear();
    test_completion_display_null_parameters();
    test_completion_display_viewport_update();
    test_completion_display_navigation();
    test_completion_display_color_configuration();
    test_completion_display_layout_configuration();
    test_completion_display_statistics();
    test_completion_display_empty_list();
    test_completion_display_single_item();
    test_completion_display_large_list_scrolling();
    test_completion_display_auto_sizing();
    test_completion_display_boundary_conditions();
    
    printf("\nAll LLE-031 Completion Display tests passed!\n");
    return 0;
}
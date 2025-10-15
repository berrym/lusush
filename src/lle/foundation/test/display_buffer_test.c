// src/lle/foundation/test/display_buffer_test.c
//
// LLE Display-Buffer Integration Tests

#include "../display/display_buffer.h"
#include "../buffer/buffer.h"
#include "../buffer/buffer_manager.h"
#include "../display/display.h"
#include "../terminal/terminal.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define TEST_PASS printf("[PASS] ")
#define TEST_FAIL printf("[FAIL] ")

static int test_count = 0;
static int pass_count = 0;

// Helper to run a test
#define RUN_TEST(test) do { \
    printf("Running test: %s... ", #test); \
    test_count++; \
    if (test()) { \
        TEST_PASS; \
        pass_count++; \
    } else { \
        TEST_FAIL; \
    } \
    printf("%s\n", #test); \
} while (0)

// Mock terminal for testing (minimal implementation)
static lle_terminal_abstraction_t test_term;

static void setup_test_terminal(void) {
    memset(&test_term, 0, sizeof(test_term));
}

// Test: Init and cleanup
static bool test_init_cleanup(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    
    // Init buffer manager
    int result = lle_buffer_manager_init(&manager, 10, 256);
    assert(result == 0);
    
    // Init display
    result = lle_display_init(&display, &test_term, 24, 80);
    assert(result == LLE_DISPLAY_OK);
    
    // Init renderer
    result = lle_display_buffer_init(&renderer, &manager, &display);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.initialized);
    assert(renderer.buffer_manager == &manager);
    assert(renderer.display == &display);
    
    // Check default settings
    assert(renderer.auto_scroll);
    assert(renderer.wrap_long_lines);
    assert(renderer.tab_width == 4);
    
    // Check viewport
    assert(renderer.viewport.visible_lines == 24);
    assert(renderer.viewport.visible_cols == 80);
    assert(renderer.viewport.top_line == 0);
    assert(renderer.viewport.left_column == 0);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Configuration options
static bool test_configuration(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Test auto-scroll
    lle_display_buffer_set_auto_scroll(&renderer, false);
    assert(!renderer.auto_scroll);
    lle_display_buffer_set_auto_scroll(&renderer, true);
    assert(renderer.auto_scroll);
    
    // Test wrap lines
    lle_display_buffer_set_wrap_lines(&renderer, false);
    assert(!renderer.wrap_long_lines);
    lle_display_buffer_set_wrap_lines(&renderer, true);
    assert(renderer.wrap_long_lines);
    
    // Test tab width
    lle_display_buffer_set_tab_width(&renderer, 8);
    assert(renderer.tab_width == 8);
    lle_display_buffer_set_tab_width(&renderer, 2);
    assert(renderer.tab_width == 2);
    
    // Test colors
    lle_display_buffer_set_colors(&renderer, 15, 0);
    assert(renderer.default_fg_color == 15);
    assert(renderer.default_bg_color == 0);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Viewport management
static bool test_viewport(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create a buffer for scroll operations to work
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "viewport_test", &buffer_id);
    
    // Add enough lines to test scrolling (50 lines)
    lle_managed_buffer_t *managed = lle_buffer_manager_get_current(&manager);
    for (int i = 0; i < 50; i++) {
        char line[32];
        snprintf(line, sizeof(line), "Line %d\n", i);
        lle_buffer_insert_string(&managed->buffer, managed->buffer.gap_start, line, strlen(line));
    }
    
    // Set viewport
    int result = lle_display_buffer_set_viewport(&renderer, 10, 5);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line == 10);
    assert(renderer.viewport.left_column == 5);
    
    // Scroll down
    result = lle_display_buffer_scroll_down(&renderer, 3);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line == 13);
    assert(renderer.scroll_count == 1);
    
    // Scroll up
    result = lle_display_buffer_scroll_up(&renderer, 5);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line == 8);
    assert(renderer.scroll_count == 2);
    
    // Scroll up past top (should clamp to 0)
    result = lle_display_buffer_scroll_up(&renderer, 100);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line == 0);
    
    // Scroll right
    result = lle_display_buffer_scroll_right(&renderer, 10);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.left_column == 15);
    
    // Scroll left
    result = lle_display_buffer_scroll_left(&renderer, 5);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.left_column == 10);
    
    // Scroll left past left edge (should clamp to 0)
    result = lle_display_buffer_scroll_left(&renderer, 100);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.left_column == 0);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Coordinate conversion
static bool test_coordinate_conversion(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Set viewport to (10, 5)
    lle_display_buffer_set_viewport(&renderer, 10, 5);
    
    // Buffer-to-screen conversion
    uint16_t screen_row, screen_col;
    int result = lle_display_buffer_buffer_to_screen(&renderer, 15, 10,
                                                     &screen_row, &screen_col);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(screen_row == 5);  // 15 - 10
    assert(screen_col == 5);  // 10 - 5
    
    // Screen-to-buffer conversion
    size_t buffer_line, buffer_col;
    result = lle_display_buffer_screen_to_buffer(&renderer, 5, 5,
                                                 &buffer_line, &buffer_col);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(buffer_line == 15);  // 10 + 5
    assert(buffer_col == 10);   // 5 + 5
    
    // Out of viewport (above)
    result = lle_display_buffer_buffer_to_screen(&renderer, 5, 10,
                                                 &screen_row, &screen_col);
    assert(result == LLE_DISPLAY_BUFFER_ERR_INVALID_VIEWPORT);
    
    // Out of viewport (below)
    result = lle_display_buffer_buffer_to_screen(&renderer, 100, 10,
                                                 &screen_row, &screen_col);
    assert(result == LLE_DISPLAY_BUFFER_ERR_INVALID_VIEWPORT);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Cursor synchronization
static bool test_cursor_sync(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create a buffer
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    // Set viewport
    lle_display_buffer_set_viewport(&renderer, 0, 0);
    
    // Set buffer cursor position
    renderer.buffer_cursor_line = 5;
    renderer.buffer_cursor_col = 10;
    
    // Sync cursor to screen
    int result = lle_display_buffer_sync_cursor_to_screen(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.screen_cursor_row == 5);
    assert(renderer.screen_cursor_col == 10);
    assert(renderer.cursor_sync_count == 1);
    
    // Sync cursor from screen
    result = lle_display_buffer_sync_cursor_to_buffer(&renderer, 8, 15);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.buffer_cursor_line == 8);
    assert(renderer.buffer_cursor_col == 15);
    assert(renderer.cursor_sync_count == 2);
    
    // Get cursor positions
    size_t line, col;
    lle_display_buffer_get_buffer_cursor(&renderer, &line, &col);
    assert(line == 8);
    assert(col == 15);
    
    uint16_t row, col_u16;
    lle_display_buffer_get_screen_cursor(&renderer, &row, &col_u16);
    assert(row == 8);
    assert(col_u16 == 15);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Render empty buffer
static bool test_render_empty_buffer(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create empty buffer
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "empty", &buffer_id);
    
    // Render
    int result = lle_display_buffer_render(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.render_count == 1);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Render simple text
static bool test_render_simple_text(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create buffer with content
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "text", &buffer_id);
    
    const char *text = "Hello, World!";
    lle_managed_buffer_t *managed = lle_buffer_manager_get_current(&manager);
    lle_buffer_insert_string(&managed->buffer, managed->buffer.gap_start, text, strlen(text));
    
    // Render
    int result = lle_display_buffer_render(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.render_count == 1);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Render multiline text
static bool test_render_multiline(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create buffer with multiline content
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "multiline", &buffer_id);
    
    const char *text = "Line 1\nLine 2\nLine 3\nLine 4\n";
    lle_managed_buffer_t *managed = lle_buffer_manager_get_current(&manager);
    lle_buffer_insert_string(&managed->buffer, managed->buffer.gap_start, text, strlen(text));
    
    // Render
    int result = lle_display_buffer_render(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Auto-scroll with cursor movement
static bool test_auto_scroll(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Enable auto-scroll
    lle_display_buffer_set_auto_scroll(&renderer, true);
    
    // Create buffer with many lines
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "scroll", &buffer_id);
    
    // Add 50 lines
    lle_managed_buffer_t *managed = lle_buffer_manager_get_current(&manager);
    for (int i = 0; i < 50; i++) {
        char line[64];
        snprintf(line, sizeof(line), "Line %d\n", i);
        lle_buffer_insert_string(&managed->buffer, managed->buffer.gap_start, line, strlen(line));
    }
    
    // Set cursor below viewport
    renderer.buffer_cursor_line = 30;
    renderer.buffer_cursor_col = 0;
    
    // Ensure cursor visible should scroll
    int result = lle_display_buffer_ensure_cursor_visible(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line > 0);
    assert(renderer.buffer_cursor_line >= renderer.viewport.top_line);
    assert(renderer.buffer_cursor_line < renderer.viewport.top_line + renderer.viewport.visible_lines);
    
    // Set cursor above viewport
    renderer.buffer_cursor_line = 5;
    result = lle_display_buffer_ensure_cursor_visible(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line <= 5);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Handle resize
static bool test_handle_resize(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create buffer
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "resize", &buffer_id);
    
    // Initial size
    assert(renderer.viewport.visible_lines == 24);
    assert(renderer.viewport.visible_cols == 80);
    
    // Resize
    int result = lle_display_buffer_handle_resize(&renderer, 30, 100);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.visible_lines == 30);
    assert(renderer.viewport.visible_cols == 100);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Clear display
static bool test_clear(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Set viewport somewhere
    lle_display_buffer_set_viewport(&renderer, 10, 5);
    
    // Clear should reset viewport
    int result = lle_display_buffer_clear(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_OK);
    assert(renderer.viewport.top_line == 0);
    assert(renderer.viewport.left_column == 0);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Performance metrics
static bool test_metrics(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create buffer
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "metrics", &buffer_id);
    
    // Perform operations
    lle_display_buffer_render(&renderer);
    lle_display_buffer_scroll_down(&renderer, 5);
    lle_display_buffer_sync_cursor_to_screen(&renderer);
    
    // Check metrics
    uint64_t render_count, scroll_count, cursor_sync_count;
    lle_display_buffer_get_metrics(&renderer, &render_count, &scroll_count, &cursor_sync_count);
    
    assert(render_count == 1);
    assert(scroll_count == 1);
    assert(cursor_sync_count == 2);  // render() calls sync_cursor internally + explicit call
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Error handling
static bool test_error_handling(void) {
    lle_display_buffer_renderer_t renderer;
    
    // Operations on uninitialized renderer
    int result = lle_display_buffer_render(&renderer);
    assert(result == LLE_DISPLAY_BUFFER_ERR_NOT_INIT);
    
    result = lle_display_buffer_set_viewport(&renderer, 0, 0);
    assert(result == LLE_DISPLAY_BUFFER_ERR_NOT_INIT);
    
    // NULL pointer errors
    result = lle_display_buffer_init(NULL, NULL, NULL);
    assert(result == LLE_DISPLAY_BUFFER_ERR_NULL_PTR);
    
    // Error string conversion
    const char *msg = lle_display_buffer_error_string(LLE_DISPLAY_BUFFER_OK);
    assert(strcmp(msg, "Success") == 0);
    
    msg = lle_display_buffer_error_string(LLE_DISPLAY_BUFFER_ERR_NULL_PTR);
    assert(strcmp(msg, "Null pointer") == 0);
    
    msg = lle_display_buffer_error_string(LLE_DISPLAY_BUFFER_ERR_NOT_INIT);
    assert(strcmp(msg, "Not initialized") == 0);
    
    return true;
}

// Test: Get viewport
static bool test_get_viewport(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Set viewport
    lle_display_buffer_set_viewport(&renderer, 15, 20);
    
    // Get viewport
    const lle_viewport_t *vp = lle_display_buffer_get_viewport(&renderer);
    assert(vp != NULL);
    assert(vp->top_line == 15);
    assert(vp->left_column == 20);
    assert(vp->visible_lines == 24);
    assert(vp->visible_cols == 80);
    
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

int main(void) {
    printf("\nLLE Display-Buffer Integration Tests\n");
    printf("=====================================\n\n");
    
    RUN_TEST(test_init_cleanup);
    RUN_TEST(test_configuration);
    RUN_TEST(test_viewport);
    RUN_TEST(test_coordinate_conversion);
    RUN_TEST(test_cursor_sync);
    RUN_TEST(test_render_empty_buffer);
    RUN_TEST(test_render_simple_text);
    RUN_TEST(test_render_multiline);
    RUN_TEST(test_auto_scroll);
    RUN_TEST(test_handle_resize);
    RUN_TEST(test_clear);
    RUN_TEST(test_metrics);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_get_viewport);
    
    printf("\n=====================================\n");
    printf("Tests run: %d\n", test_count);
    printf("Tests passed: %d\n", pass_count);
    printf("Tests failed: %d\n\n", test_count - pass_count);
    
    if (pass_count == test_count) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed.\n");
        return 1;
    }
}

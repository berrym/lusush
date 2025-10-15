// src/lle/foundation/test/input_processor_test.c
//
// LLE Input Processor Tests (Automated - simulated input)

#include "../input/input_processor.h"
#include "../buffer/buffer.h"
#include "../buffer/buffer_manager.h"
#include "../display/display.h"
#include "../display/display_buffer.h"
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

// Mock terminal for testing
static lle_terminal_abstraction_t test_term;

static void setup_test_terminal(void) {
    memset(&test_term, 0, sizeof(test_term));
}

// Test: Init and cleanup
static bool test_init_cleanup(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    int result = lle_simple_input_init(&processor, 0, &manager, &renderer);
    assert(result == LLE_INPUT_OK);
    assert(processor.initialized);
    assert(processor.buffer_manager == &manager);
    assert(processor.renderer == &renderer);
    assert(!processor.raw_mode_enabled);
    assert(!processor.running);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Event parsing (simulated - no actual terminal needed)
static bool test_event_structure(void) {
    lle_key_event_t event;
    
    // Test event initialization
    memset(&event, 0, sizeof(event));
    assert(event.key == LLE_KEY_CHAR);
    assert(event.ch == 0);
    assert(!event.ctrl);
    assert(!event.alt);
    assert(!event.shift);
    
    // Test key code setting
    event.key = LLE_KEY_ARROW_LEFT;
    assert(event.key == LLE_KEY_ARROW_LEFT);
    
    event.key = LLE_KEY_CHAR;
    event.ch = 'a';
    assert(event.ch == 'a');
    
    return true;
}

// Test: Insert character action
static bool test_action_insert_char(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    // Create buffer
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Insert some characters
    lle_input_action_insert_char(&processor, 'H');
    lle_input_action_insert_char(&processor, 'e');
    lle_input_action_insert_char(&processor, 'l');
    lle_input_action_insert_char(&processor, 'l');
    lle_input_action_insert_char(&processor, 'o');
    
    // Check buffer content
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    assert(buf != NULL);
    assert(lle_buffer_size(&buf->buffer) == 5);
    assert(processor.chars_inserted == 5);
    
    char content[10] = {0};
    int result = lle_buffer_get_contents(&buf->buffer, content, sizeof(content));
    assert(result == 0);  // Success
    content[5] = '\0';  // Null-terminate after 5 chars
    assert(strcmp(content, "Hello") == 0);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Backspace action
static bool test_action_backspace(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Insert then backspace
    lle_input_action_insert_char(&processor, 'a');
    lle_input_action_insert_char(&processor, 'b');
    lle_input_action_insert_char(&processor, 'c');
    lle_input_action_backspace(&processor);
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    assert(lle_buffer_size(&buf->buffer) == 2);
    assert(processor.chars_deleted == 1);
    
    char content[10] = {0};
    int result = lle_buffer_get_contents(&buf->buffer, content, sizeof(content));
    assert(result == 0);  // Success
    content[2] = '\0';  // Null-terminate after 2 chars
    assert(strcmp(content, "ab") == 0);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Cursor movement
static bool test_action_cursor_movement(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Insert "abc"
    lle_input_action_insert_char(&processor, 'a');
    lle_input_action_insert_char(&processor, 'b');
    lle_input_action_insert_char(&processor, 'c');
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    assert(buf->buffer.gap_start == 3);  // Cursor at end
    
    // Move left
    lle_input_action_move_left(&processor);
    assert(buf->buffer.gap_start == 2);
    
    lle_input_action_move_left(&processor);
    assert(buf->buffer.gap_start == 1);
    
    // Move right
    lle_input_action_move_right(&processor);
    assert(buf->buffer.gap_start == 2);
    
    assert(processor.cursor_moves == 3);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Home/End actions
static bool test_action_home_end(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Insert "Hello\nWorld"
    lle_input_action_insert_char(&processor, 'H');
    lle_input_action_insert_char(&processor, 'e');
    lle_input_action_insert_char(&processor, 'l');
    lle_input_action_insert_char(&processor, 'l');
    lle_input_action_insert_char(&processor, 'o');
    lle_input_action_newline(&processor);
    lle_input_action_insert_char(&processor, 'W');
    lle_input_action_insert_char(&processor, 'o');
    lle_input_action_insert_char(&processor, 'r');
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    size_t cursor_before_home = buf->buffer.gap_start;
    
    // Home should move to beginning of "World" line
    lle_input_action_move_home(&processor);
    assert(buf->buffer.gap_start == 6);  // After "Hello\n"
    
    // End should move to end of "World" line (where we were)
    lle_input_action_move_end(&processor);
    assert(buf->buffer.gap_start == cursor_before_home);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Delete forward action
static bool test_action_delete(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Insert "abc", move to middle, delete
    lle_input_action_insert_char(&processor, 'a');
    lle_input_action_insert_char(&processor, 'b');
    lle_input_action_insert_char(&processor, 'c');
    lle_input_action_move_left(&processor);  // Between 'b' and 'c'
    lle_input_action_delete(&processor);     // Delete 'c'
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    assert(lle_buffer_size(&buf->buffer) == 2);
    
    char content[10] = {0};
    int result = lle_buffer_get_contents(&buf->buffer, content, sizeof(content));
    assert(result == 0);  // Success
    content[2] = '\0';  // Null-terminate after 2 chars
    assert(strcmp(content, "ab") == 0);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Kill line (Ctrl+K)
static bool test_action_kill_line(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Insert "Hello World", move to middle, kill to end
    const char *text = "Hello World";
    for (const char *p = text; *p; p++) {
        lle_input_action_insert_char(&processor, *p);
    }
    
    // Move to 'W' (position 6)
    while (lle_buffer_manager_get_current(&manager)->buffer.gap_start > 6) {
        lle_input_action_move_left(&processor);
    }
    
    lle_input_action_kill_line(&processor);
    
    lle_managed_buffer_t *buf = lle_buffer_manager_get_current(&manager);
    char content[20] = {0};
    int result = lle_buffer_get_contents(&buf->buffer, content, sizeof(content));
    assert(result == 0);  // Success
    content[6] = '\0';  // Null-terminate after 6 chars ("Hello ")
    assert(strcmp(content, "Hello ") == 0);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Statistics
static bool test_statistics(void) {
    lle_buffer_manager_t manager;
    lle_display_t display;
    lle_display_buffer_renderer_t renderer;
    lle_simple_input_processor_t processor;
    
    setup_test_terminal();
    lle_buffer_manager_init(&manager, 10, 256);
    lle_display_init(&display, &test_term, 24, 80);
    lle_display_buffer_init(&renderer, &manager, &display);
    
    uint32_t buffer_id;
    lle_buffer_manager_create_buffer(&manager, "test", &buffer_id);
    
    lle_simple_input_init(&processor, 0, &manager, &renderer);
    
    // Perform various operations
    lle_input_action_insert_char(&processor, 'a');
    lle_input_action_insert_char(&processor, 'b');
    lle_input_action_backspace(&processor);
    lle_input_action_move_left(&processor);
    
    uint64_t keys, inserted, deleted, moves;
    lle_simple_input_get_stats(&processor, &keys, &inserted, &deleted, &moves);
    
    assert(inserted == 2);
    assert(deleted == 1);
    assert(moves == 1);
    
    lle_simple_input_cleanup(&processor);
    lle_display_buffer_cleanup(&renderer);
    lle_display_cleanup(&display);
    lle_buffer_manager_cleanup(&manager);
    
    return true;
}

// Test: Error strings
static bool test_error_strings(void) {
    const char *msg;
    
    msg = lle_input_error_string(LLE_INPUT_OK);
    assert(strcmp(msg, "Success") == 0);
    
    msg = lle_input_error_string(LLE_INPUT_ERR_NULL_PTR);
    assert(strcmp(msg, "Null pointer") == 0);
    
    msg = lle_input_error_string(LLE_INPUT_ERR_NOT_INIT);
    assert(strcmp(msg, "Not initialized") == 0);
    
    return true;
}

// Test: Key code strings
static bool test_key_code_strings(void) {
    const char *msg;
    
    msg = lle_key_code_string(LLE_KEY_CHAR);
    assert(strcmp(msg, "CHAR") == 0);
    
    msg = lle_key_code_string(LLE_KEY_ARROW_LEFT);
    assert(strcmp(msg, "ARROW_LEFT") == 0);
    
    msg = lle_key_code_string(LLE_KEY_CTRL_A);
    assert(strcmp(msg, "CTRL+A") == 0);
    
    return true;
}

int main(void) {
    printf("\nLLE Input Processor Tests\n");
    printf("==========================\n\n");
    
    RUN_TEST(test_init_cleanup);
    RUN_TEST(test_event_structure);
    RUN_TEST(test_action_insert_char);
    RUN_TEST(test_action_backspace);
    RUN_TEST(test_action_cursor_movement);
    RUN_TEST(test_action_home_end);
    RUN_TEST(test_action_delete);
    RUN_TEST(test_action_kill_line);
    RUN_TEST(test_statistics);
    RUN_TEST(test_error_strings);
    RUN_TEST(test_key_code_strings);
    
    printf("\n==========================\n");
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

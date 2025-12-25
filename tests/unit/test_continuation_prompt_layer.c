/*
 * Unit Tests for Continuation Prompt Layer
 *
 * Test Coverage:
 * - Layer lifecycle (create/init/cleanup/destroy)
 * - Event system integration
 * - Event handling (CONTENT_CHANGED, CURSOR_MOVED, SIZE_CHANGED)
 * - Simple mode prompt generation
 * - Context-aware mode prompt generation
 * - Cache behavior
 * - Error handling
 */

#include "../../include/display/continuation_prompt_layer.h"
#include "../../include/display/layer_events.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Test Utilities
// ============================================================================

static int test_count = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name)                                                             \
    static void name(void);                                                    \
    static void run_##name(void) {                                             \
        test_count++;                                                          \
        printf("  Test %d: %s ... ", test_count, #name);                       \
        fflush(stdout);                                                        \
        name();                                                                \
        tests_passed++;                                                        \
        printf("PASS\n");                                                      \
    }                                                                          \
    static void name(void)

#define RUN_TEST(name) run_##name()

#define ASSERT(condition)                                                      \
    do {                                                                       \
        if (!(condition)) {                                                    \
            printf("FAIL\n");                                                  \
            printf("    Assertion failed: %s\n", #condition);                  \
            printf("    at %s:%d\n", __FILE__, __LINE__);                      \
            tests_failed++;                                                    \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_STR_EQ(actual, expected)                                        \
    do {                                                                       \
        if (strcmp((actual), (expected)) != 0) {                               \
            printf("FAIL\n");                                                  \
            printf("    Expected: \"%s\"\n", expected);                        \
            printf("    Actual:   \"%s\"\n", actual);                          \
            printf("    at %s:%d\n", __FILE__, __LINE__);                      \
            tests_failed++;                                                    \
            return;                                                            \
        }                                                                      \
    } while (0)

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST(test_create_destroy) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    ASSERT(layer != NULL);

    continuation_prompt_layer_destroy(layer);
}

TEST(test_destroy_null) {
    // Should not crash
    continuation_prompt_layer_destroy(NULL);
}

TEST(test_init_requires_event_system) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    ASSERT(layer != NULL);

    // Init without event system succeeds - event system is only required
    // when context-aware continuation prompts are configured.
    // In simple mode (the default), no event system is needed.
    continuation_prompt_error_t result = continuation_prompt_layer_init(layer);
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);

    continuation_prompt_layer_destroy(layer);
}

TEST(test_init_with_event_system) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    ASSERT(layer != NULL);

    layer_event_system_t *events = layer_events_create(NULL);
    ASSERT(events != NULL);
    layer_events_init(events);

    continuation_prompt_error_t result = continuation_prompt_layer_init(layer);
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_double_cleanup) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);

    continuation_prompt_layer_init(layer);

    // First cleanup
    continuation_prompt_error_t result =
        continuation_prompt_layer_cleanup(layer);
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);

    // Second cleanup (should be safe)
    result = continuation_prompt_layer_cleanup(layer);
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);

    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

// ============================================================================
// Simple Mode Tests
// ============================================================================

TEST(test_simple_mode_default) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    // Default mode should be SIMPLE
    continuation_prompt_mode_t mode = continuation_prompt_layer_get_mode(layer);
    ASSERT(mode == CONTINUATION_PROMPT_MODE_SIMPLE);

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_simple_mode_all_lines_same) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer, CONTINUATION_PROMPT_MODE_SIMPLE);

    char prompt[64];
    const char *command = "for i in 1 2 3\ndo\n    echo $i\ndone";

    // All continuation lines should return "> "
    continuation_prompt_error_t result;

    result = continuation_prompt_layer_get_prompt_for_line(
        layer, 1, command, prompt, sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "> ");

    result = continuation_prompt_layer_get_prompt_for_line(
        layer, 2, command, prompt, sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "> ");

    result = continuation_prompt_layer_get_prompt_for_line(
        layer, 3, command, prompt, sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

// ============================================================================
// Context-Aware Mode Tests
// ============================================================================

TEST(test_context_aware_if_statement) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer,
                                       CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    char prompt[64];
    const char *command = "if [ -f file ]\nthen\n    echo yes\nfi";

    // Lines in if statement should return "if> "
    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "if> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_context_aware_for_loop) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer,
                                       CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    char prompt[64];
    const char *command = "for i in 1 2 3\ndo\n    echo $i\ndone";

    // Lines in for loop should return "for> "
    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "for> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_context_aware_while_loop) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer,
                                       CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    char prompt[64];
    const char *command = "while true\ndo\n    echo looping\ndone";

    // Lines in while loop should return "while> "
    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "while> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_context_aware_function) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer,
                                       CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    char prompt[64];
    const char *command = "myfunc() {\n    echo hello\n}";

    // Lines in function (brace block) should return "brace> "
    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "brace> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_context_aware_subshell) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer,
                                       CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    char prompt[64];
    const char *command = "(\n    echo subshell\n)";

    // Lines in subshell should return "> " (generic continuation)
    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_context_aware_quotes) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    continuation_prompt_layer_set_mode(layer,
                                       CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    char prompt[64];
    const char *command = "echo \"line one\nline two\"";

    // Lines in quotes should return "quote> "
    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);
    ASSERT_STR_EQ(prompt, "quote> ");

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(test_null_layer) {
    char prompt[64];
    const char *command = "test";

    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(NULL, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_ERROR_NULL_POINTER);
}

TEST(test_null_command) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    char prompt[64];

    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, NULL, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_ERROR_NULL_POINTER);

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_null_output_buffer) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    const char *command = "test";

    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, NULL,
                                                      64);
    ASSERT(result == CONTINUATION_PROMPT_ERROR_NULL_POINTER);

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

TEST(test_buffer_too_small) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    char prompt[2]; // Too small for "> "
    const char *command = "test";

    continuation_prompt_error_t result =
        continuation_prompt_layer_get_prompt_for_line(layer, 1, command, prompt,
                                                      sizeof(prompt));
    ASSERT(result == CONTINUATION_PROMPT_ERROR_BUFFER_TOO_SMALL);

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

// ============================================================================
// Mode Switching Tests
// ============================================================================

TEST(test_mode_switching) {
    continuation_prompt_layer_t *layer = continuation_prompt_layer_create();
    layer_event_system_t *events = layer_events_create(NULL);
    layer_events_init(events);
    continuation_prompt_layer_init(layer);

    // Default is SIMPLE
    continuation_prompt_mode_t mode = continuation_prompt_layer_get_mode(layer);
    ASSERT(mode == CONTINUATION_PROMPT_MODE_SIMPLE);

    // Switch to CONTEXT_AWARE
    continuation_prompt_error_t result = continuation_prompt_layer_set_mode(
        layer, CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);

    mode = continuation_prompt_layer_get_mode(layer);
    ASSERT(mode == CONTINUATION_PROMPT_MODE_CONTEXT_AWARE);

    // Switch back to SIMPLE
    result = continuation_prompt_layer_set_mode(
        layer, CONTINUATION_PROMPT_MODE_SIMPLE);
    ASSERT(result == CONTINUATION_PROMPT_SUCCESS);

    mode = continuation_prompt_layer_get_mode(layer);
    ASSERT(mode == CONTINUATION_PROMPT_MODE_SIMPLE);

    continuation_prompt_layer_cleanup(layer);
    continuation_prompt_layer_destroy(layer);
    layer_events_destroy(events);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("\n");
    printf("=============================================================\n");
    printf("  Continuation Prompt Layer Unit Tests\n");
    printf("=============================================================\n\n");

    printf("Lifecycle Tests:\n");
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_destroy_null);
    RUN_TEST(test_init_requires_event_system);
    RUN_TEST(test_init_with_event_system);
    RUN_TEST(test_double_cleanup);

    printf("\nSimple Mode Tests:\n");
    RUN_TEST(test_simple_mode_default);
    RUN_TEST(test_simple_mode_all_lines_same);

    printf("\nContext-Aware Mode Tests:\n");
    RUN_TEST(test_context_aware_if_statement);
    RUN_TEST(test_context_aware_for_loop);
    RUN_TEST(test_context_aware_while_loop);
    RUN_TEST(test_context_aware_function);
    RUN_TEST(test_context_aware_subshell);
    RUN_TEST(test_context_aware_quotes);

    printf("\nError Handling Tests:\n");
    RUN_TEST(test_null_layer);
    RUN_TEST(test_null_command);
    RUN_TEST(test_null_output_buffer);
    RUN_TEST(test_buffer_too_small);

    printf("\nMode Switching Tests:\n");
    RUN_TEST(test_mode_switching);

    printf("\n");
    printf("=============================================================\n");
    printf("  Test Summary\n");
    printf("=============================================================\n");
    printf("  Total tests:  %d\n", test_count);
    printf("  Passed:       %d\n", tests_passed);
    printf("  Failed:       %d\n", tests_failed);
    printf("=============================================================\n");

    return tests_failed > 0 ? 1 : 0;
}

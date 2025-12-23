/**
 * @file test_widget_hooks.c
 * @brief Unit tests for LLE Widget Hooks Manager
 * @date 2025-01-17
 *
 * Tests for widget hooks manager functionality including hook registration,
 * triggering, and lifecycle management.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "../functional/test_memory_mock.h"
#include "lle/widget_hooks.h"
#include "lle/widget_system.h"
#include "lle/lle_editor.h"

/* Test state tracking */
static int g_hook_callback_called = 0;
static int g_hook_callback_count = 0;
static lle_editor_t *g_hook_editor_arg = NULL;
static lle_widget_hook_type_t g_last_hook_type = 0;

/* Test widget callbacks */
static lle_result_t test_hook_widget_callback(lle_editor_t *editor, void *user_data) {
    (void)user_data;
    g_hook_callback_called = 1;
    g_hook_callback_count++;
    g_hook_editor_arg = editor;
    return LLE_SUCCESS;
}

static lle_result_t test_hook_widget_error(lle_editor_t *editor, void *user_data) {
    (void)editor;
    (void)user_data;
    g_hook_callback_called = 1;
    g_hook_callback_count++;
    return LLE_ERROR_INVALID_STATE;
}

/* Helper to reset test state */
static void reset_test_state(void) {
    g_hook_callback_called = 0;
    g_hook_callback_count = 0;
    g_hook_editor_arg = NULL;
    g_last_hook_type = 0;
}

/* Test: Initialize hooks manager */
static void test_hooks_manager_init(void) {
    printf("Running: test_hooks_manager_init\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;

    /* Initialize registry first */
    lle_result_t result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    /* Initialize hooks manager */
    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);
    assert(manager != NULL);

    /* Verify all hook lists are empty */
    for (int i = 0; i < LLE_HOOK_COUNT; i++) {
        int count = lle_widget_hook_get_count(manager, (lle_widget_hook_type_t)i);
        assert(count == 0);
    }

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Invalid parameter checks for init */
static void test_hooks_manager_init_invalid_params(void) {
    printf("Running: test_hooks_manager_init_invalid_params\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_result_t result;

    /* Initialize registry for valid tests */
    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    /* NULL manager pointer */
    result = lle_widget_hooks_manager_init(NULL, registry, pool);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* NULL registry */
    result = lle_widget_hooks_manager_init(&manager, NULL, pool);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* NULL pool */
    result = lle_widget_hooks_manager_init(&manager, registry, NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Register hook */
static void test_hook_register(void) {
    printf("Running: test_hook_register\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;

    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    /* Register a widget */
    result = lle_widget_register(registry, "test-hook-widget", 
                                 test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register widget to hook */
    result = lle_widget_hook_register(manager, LLE_HOOK_LINE_INIT, "test-hook-widget");
    assert(result == LLE_SUCCESS);

    /* Verify hook count */
    int count = lle_widget_hook_get_count(manager, LLE_HOOK_LINE_INIT);
    assert(count == 1);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Register multiple hooks */
static void test_hook_register_multiple(void) {
    printf("Running: test_hook_register_multiple\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    /* Register widgets */
    result = lle_widget_register(registry, "widget1", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_register(registry, "widget2", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_register(registry, "widget3", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register all to same hook */
    result = lle_widget_hook_register(manager, LLE_HOOK_BUFFER_MODIFIED, "widget1");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_BUFFER_MODIFIED, "widget2");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_BUFFER_MODIFIED, "widget3");
    assert(result == LLE_SUCCESS);

    /* Verify count */
    int count = lle_widget_hook_get_count(manager, LLE_HOOK_BUFFER_MODIFIED);
    assert(count == 3);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Register duplicate hook */
static void test_hook_register_duplicate(void) {
    printf("Running: test_hook_register_duplicate\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    /* Register widget */
    result = lle_widget_register(registry, "dup-widget", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register to hook twice */
    result = lle_widget_hook_register(manager, LLE_HOOK_PRE_COMMAND, "dup-widget");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_PRE_COMMAND, "dup-widget");
    assert(result == LLE_ERROR_ALREADY_EXISTS);

    /* Should still only have one */
    int count = lle_widget_hook_get_count(manager, LLE_HOOK_PRE_COMMAND);
    assert(count == 1);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Trigger hook */
static void test_hook_trigger(void) {
    printf("Running: test_hook_trigger\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_editor_t editor = {0};
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    /* Set up editor with registry */
    editor.widget_registry = registry;
    editor.widget_hooks_manager = manager;

    /* Register widget */
    result = lle_widget_register(registry, "trigger-test", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register to hook */
    result = lle_widget_hook_register(manager, LLE_HOOK_LINE_INIT, "trigger-test");
    assert(result == LLE_SUCCESS);

    /* Trigger hook */
    result = lle_widget_hook_trigger(manager, LLE_HOOK_LINE_INIT, &editor);
    assert(result == LLE_SUCCESS);

    /* Verify callback was called */
    assert(g_hook_callback_called == 1);
    assert(g_hook_callback_count == 1);
    assert(g_hook_editor_arg == &editor);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Trigger multiple hooks */
static void test_hook_trigger_multiple(void) {
    printf("Running: test_hook_trigger_multiple\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_editor_t editor = {0};
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    editor.widget_registry = registry;
    editor.widget_hooks_manager = manager;

    /* Register three widgets */
    result = lle_widget_register(registry, "hook1", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_register(registry, "hook2", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_register(registry, "hook3", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register all to same hook */
    result = lle_widget_hook_register(manager, LLE_HOOK_POST_COMMAND, "hook1");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_POST_COMMAND, "hook2");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_POST_COMMAND, "hook3");
    assert(result == LLE_SUCCESS);

    /* Trigger hook */
    result = lle_widget_hook_trigger(manager, LLE_HOOK_POST_COMMAND, &editor);
    assert(result == LLE_SUCCESS);

    /* Verify all three callbacks were called */
    assert(g_hook_callback_count == 3);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Trigger hook with error widget (should continue) */
static void test_hook_trigger_with_error(void) {
    printf("Running: test_hook_trigger_with_error\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_editor_t editor = {0};
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    editor.widget_registry = registry;
    editor.widget_hooks_manager = manager;

    /* Register normal widget, error widget, then normal widget */
    result = lle_widget_register(registry, "normal1", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_register(registry, "error", test_hook_widget_error,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_register(registry, "normal2", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register to hook */
    result = lle_widget_hook_register(manager, LLE_HOOK_COMPLETION_START, "normal1");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_COMPLETION_START, "error");
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_COMPLETION_START, "normal2");
    assert(result == LLE_SUCCESS);

    /* Trigger hook - should continue despite error */
    result = lle_widget_hook_trigger(manager, LLE_HOOK_COMPLETION_START, &editor);
    assert(result == LLE_SUCCESS); /* Hook manager continues on error */

    /* All three should have been called */
    assert(g_hook_callback_count == 3);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Unregister hook */
static void test_hook_unregister(void) {
    printf("Running: test_hook_unregister\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    /* Register widget */
    result = lle_widget_register(registry, "unreg-test", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    /* Register to hook */
    result = lle_widget_hook_register(manager, LLE_HOOK_HISTORY_SEARCH, "unreg-test");
    assert(result == LLE_SUCCESS);

    assert(lle_widget_hook_get_count(manager, LLE_HOOK_HISTORY_SEARCH) == 1);

    /* Unregister */
    result = lle_widget_hook_unregister(manager, LLE_HOOK_HISTORY_SEARCH, "unreg-test");
    assert(result == LLE_SUCCESS);

    assert(lle_widget_hook_get_count(manager, LLE_HOOK_HISTORY_SEARCH) == 0);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Enable/disable hooks globally */
static void test_hook_enable_disable(void) {
    printf("Running: test_hook_enable_disable\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_editor_t editor = {0};
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    editor.widget_registry = registry;
    editor.widget_hooks_manager = manager;

    /* Register widget */
    result = lle_widget_register(registry, "enable-test", test_hook_widget_callback,
                                 LLE_WIDGET_BUILTIN, NULL);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hook_register(manager, LLE_HOOK_TERMINAL_RESIZE, "enable-test");
    assert(result == LLE_SUCCESS);

    /* Verify hooks are enabled by default */
    assert(lle_widget_hooks_enabled(manager) == true);

    /* Trigger - should execute */
    result = lle_widget_hook_trigger(manager, LLE_HOOK_TERMINAL_RESIZE, &editor);
    assert(result == LLE_SUCCESS);
    assert(g_hook_callback_count == 1);

    /* Disable hooks globally */
    result = lle_widget_hooks_disable(manager);
    assert(result == LLE_SUCCESS);
    assert(lle_widget_hooks_enabled(manager) == false);

    /* Trigger - should NOT execute */
    reset_test_state();
    result = lle_widget_hook_trigger(manager, LLE_HOOK_TERMINAL_RESIZE, &editor);
    assert(result == LLE_SUCCESS);
    assert(g_hook_callback_count == 0); /* Not called */

    /* Re-enable hooks globally */
    result = lle_widget_hooks_enable(manager);
    assert(result == LLE_SUCCESS);
    assert(lle_widget_hooks_enabled(manager) == true);

    /* Trigger - should execute again */
    result = lle_widget_hook_trigger(manager, LLE_HOOK_TERMINAL_RESIZE, &editor);
    assert(result == LLE_SUCCESS);
    assert(g_hook_callback_count == 1);

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Test: Hook count */
static void test_hook_count(void) {
    printf("Running: test_hook_count\n");
    reset_test_state();

    lle_memory_pool_t *pool = lle_pool_create();
    lle_widget_registry_t *registry = NULL;
    lle_widget_hooks_manager_t *manager = NULL;
    lle_result_t result;

    result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);

    result = lle_widget_hooks_manager_init(&manager, registry, pool);
    assert(result == LLE_SUCCESS);

    /* Initially zero */
    assert(lle_widget_hook_get_count(manager, LLE_HOOK_LINE_FINISH) == 0);

    /* Register widgets */
    for (int i = 0; i < 5; i++) {
        char name[32];
        snprintf(name, sizeof(name), "count-widget-%d", i);
        result = lle_widget_register(registry, name, test_hook_widget_callback,
                                     LLE_WIDGET_BUILTIN, NULL);
        assert(result == LLE_SUCCESS);

        result = lle_widget_hook_register(manager, LLE_HOOK_LINE_FINISH, name);
        assert(result == LLE_SUCCESS);

        assert(lle_widget_hook_get_count(manager, LLE_HOOK_LINE_FINISH) == (size_t)(i + 1));
    }

    /* Cleanup */
    lle_pool_destroy(pool);

    printf("  PASSED\n");
}

/* Main test runner */
int main(void) {
    printf("=== Widget Hooks Manager Tests ===\n\n");

    test_hooks_manager_init();
    test_hooks_manager_init_invalid_params();
    test_hook_register();
    test_hook_register_multiple();
    test_hook_register_duplicate();
    test_hook_trigger();
    test_hook_trigger_multiple();
    test_hook_trigger_with_error();
    test_hook_unregister();
    test_hook_enable_disable();
    test_hook_count();

    printf("\n=== All Widget Hooks Tests Passed ===\n");
    return 0;
}

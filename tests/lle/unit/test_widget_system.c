/**
 * @file test_widget_system.c
 * @brief Unit tests for LLE Widget System
 * 
 * Comprehensive test coverage for widget registry functionality including
 * registration, lookup, execution, and lifecycle management.
 */

#include "../functional/test_memory_mock.h"
#include "lle/widget_system.h"
#include "lle/lle_editor.h"
#include "lle/buffer_management.h"
#include "lle/error_handling.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * TEST HELPERS
 * ============================================================================ */

static int g_test_widget_called = 0;
static int g_test_widget_call_count = 0;
static lle_editor_t *g_test_editor_arg = NULL;
static void *g_test_user_data_arg = NULL;

/**
 * Simple test widget that sets a flag when called
 */
static lle_result_t test_widget_callback(lle_editor_t *editor, void *user_data) {
    g_test_widget_called = 1;
    g_test_widget_call_count++;
    g_test_editor_arg = editor;
    g_test_user_data_arg = user_data;
    return LLE_SUCCESS;
}

/**
 * Test widget that returns an error
 */
static lle_result_t test_widget_error_callback(lle_editor_t *editor, void *user_data) {
    (void)editor;
    (void)user_data;
    return LLE_ERROR_INVALID_STATE;
}

/**
 * Reset test globals
 */
static void reset_test_globals(void) {
    g_test_widget_called = 0;
    g_test_widget_call_count = 0;
    g_test_editor_arg = NULL;
    g_test_user_data_arg = NULL;
}

/**
 * Create minimal test editor
 */
static lle_editor_t* create_test_editor(void) {
    lle_editor_t *editor = malloc(sizeof(lle_editor_t));
    memset(editor, 0, sizeof(lle_editor_t));
    return editor;
}

/**
 * Free test editor
 */
static void free_test_editor(lle_editor_t *editor) {
    if (editor) {
        free(editor);
    }
}

/* ============================================================================
 * TEST CASES
 * ============================================================================ */

/**
 * Test widget registry initialization
 */
static void test_widget_registry_init(void) {
    printf("  test_widget_registry_init...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    lle_result_t result = lle_widget_registry_init(&registry, pool);
    assert(result == LLE_SUCCESS);
    assert(registry != NULL);
    assert(registry->registry_active == true);
    assert(registry->widget_count == 0);
    assert(registry->widget_list == NULL);
    assert(registry->widgets != NULL);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget registry init with NULL parameters
 */
static void test_widget_registry_init_null_params(void) {
    printf("  test_widget_registry_init_null_params...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    /* NULL registry pointer */
    lle_result_t result = lle_widget_registry_init(NULL, pool);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
    
    /* NULL memory pool */
    result = lle_widget_registry_init(&registry, NULL);
    assert(result == LLE_ERROR_INVALID_PARAMETER);
    assert(registry == NULL);
    
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget registration
 */
static void test_widget_register(void) {
    printf("  test_widget_register...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    
    /* Register a widget */
    lle_result_t result = lle_widget_register(registry, "test-widget", 
                                              test_widget_callback,
                                              LLE_WIDGET_USER, NULL);
    assert(result == LLE_SUCCESS);
    assert(registry->widget_count == 1);
    assert(registry->widget_list != NULL);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget registration with duplicate name
 */
static void test_widget_register_duplicate(void) {
    printf("  test_widget_register_duplicate...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    
    /* Register first widget */
    assert(lle_widget_register(registry, "test-widget", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    
    /* Try to register with same name */
    lle_result_t result = lle_widget_register(registry, "test-widget", 
                                              test_widget_callback,
                                              LLE_WIDGET_USER, NULL);
    assert(result == LLE_ERROR_ALREADY_EXISTS);
    assert(registry->widget_count == 1);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget lookup
 */
static void test_widget_lookup(void) {
    printf("  test_widget_lookup...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "test-widget", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    
    /* Lookup existing widget */
    lle_widget_t *widget = lle_widget_lookup(registry, "test-widget");
    assert(widget != NULL);
    assert(strcmp(widget->name, "test-widget") == 0);
    assert(widget->callback == test_widget_callback);
    assert(widget->type == LLE_WIDGET_USER);
    assert(widget->enabled == true);
    
    /* Lookup non-existent widget */
    widget = lle_widget_lookup(registry, "nonexistent");
    assert(widget == NULL);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget execution
 */
static void test_widget_execute(void) {
    printf("  test_widget_execute...");
    
    reset_test_globals();
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    lle_editor_t *editor = create_test_editor();
    int user_data = 42;
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "test-widget", test_widget_callback,
                              LLE_WIDGET_USER, &user_data) == LLE_SUCCESS);
    
    /* Execute widget */
    lle_result_t result = lle_widget_execute(registry, "test-widget", editor);
    assert(result == LLE_SUCCESS);
    assert(g_test_widget_called == 1);
    assert(g_test_widget_call_count == 1);
    assert(g_test_editor_arg == editor);
    assert(g_test_user_data_arg == &user_data);
    
    /* Verify execution count updated */
    lle_widget_t *widget = lle_widget_lookup(registry, "test-widget");
    assert(widget->execution_count == 1);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    free_test_editor(editor);
    
    printf(" PASSED\n");
}

/**
 * Test widget execution with error
 */
static void test_widget_execute_error(void) {
    printf("  test_widget_execute_error...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    lle_editor_t *editor = create_test_editor();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "error-widget", test_widget_error_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    
    /* Execute widget that returns error */
    lle_result_t result = lle_widget_execute(registry, "error-widget", editor);
    assert(result == LLE_ERROR_INVALID_STATE);
    
    /* Verify execution count still updated */
    lle_widget_t *widget = lle_widget_lookup(registry, "error-widget");
    assert(widget->execution_count == 1);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    free_test_editor(editor);
    
    printf(" PASSED\n");
}

/**
 * Test widget unregistration
 */
static void test_widget_unregister(void) {
    printf("  test_widget_unregister...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "test-widget", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    assert(registry->widget_count == 1);
    
    /* Unregister widget */
    lle_result_t result = lle_widget_unregister(registry, "test-widget");
    assert(result == LLE_SUCCESS);
    assert(registry->widget_count == 0);
    
    /* Verify widget no longer exists */
    lle_widget_t *widget = lle_widget_lookup(registry, "test-widget");
    assert(widget == NULL);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget enable/disable
 */
static void test_widget_enable_disable(void) {
    printf("  test_widget_enable_disable...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    lle_editor_t *editor = create_test_editor();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "test-widget", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    
    /* Widget starts enabled */
    lle_widget_t *widget = lle_widget_lookup(registry, "test-widget");
    assert(widget->enabled == true);
    
    /* Disable widget */
    assert(lle_widget_disable(registry, "test-widget") == LLE_SUCCESS);
    assert(widget->enabled == false);
    
    /* Try to execute disabled widget */
    reset_test_globals();
    lle_result_t result = lle_widget_execute(registry, "test-widget", editor);
    assert(result == LLE_ERROR_DISABLED);
    assert(g_test_widget_called == 0);
    
    /* Re-enable widget */
    assert(lle_widget_enable(registry, "test-widget") == LLE_SUCCESS);
    assert(widget->enabled == true);
    
    /* Execute now works */
    result = lle_widget_execute(registry, "test-widget", editor);
    assert(result == LLE_SUCCESS);
    assert(g_test_widget_called == 1);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    free_test_editor(editor);
    
    printf(" PASSED\n");
}

/**
 * Test multiple widget types
 */
static void test_multiple_widget_types(void) {
    printf("  test_multiple_widget_types...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    
    /* Register widgets of different types */
    assert(lle_widget_register(registry, "builtin-widget", test_widget_callback,
                              LLE_WIDGET_BUILTIN, NULL) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "user-widget", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    assert(lle_widget_register(registry, "plugin-widget", test_widget_callback,
                              LLE_WIDGET_PLUGIN, NULL) == LLE_SUCCESS);
    
    assert(registry->widget_count == 3);
    
    /* Verify types */
    assert(lle_widget_lookup(registry, "builtin-widget")->type == LLE_WIDGET_BUILTIN);
    assert(lle_widget_lookup(registry, "user-widget")->type == LLE_WIDGET_USER);
    assert(lle_widget_lookup(registry, "plugin-widget")->type == LLE_WIDGET_PLUGIN);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget exists
 */
static void test_widget_exists(void) {
    printf("  test_widget_exists...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    
    assert(lle_widget_exists(registry, "test-widget") == false);
    
    assert(lle_widget_register(registry, "test-widget", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    
    assert(lle_widget_exists(registry, "test-widget") == true);
    assert(lle_widget_exists(registry, "nonexistent") == false);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/**
 * Test widget count
 */
static void test_widget_count(void) {
    printf("  test_widget_count...");
    
    lle_widget_registry_t *registry = NULL;
    lle_memory_pool_t *pool = lle_pool_create();
    
    assert(lle_widget_registry_init(&registry, pool) == LLE_SUCCESS);
    assert(lle_widget_registry_get_count(registry) == 0);
    
    assert(lle_widget_register(registry, "widget1", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    assert(lle_widget_registry_get_count(registry) == 1);
    
    assert(lle_widget_register(registry, "widget2", test_widget_callback,
                              LLE_WIDGET_USER, NULL) == LLE_SUCCESS);
    assert(lle_widget_registry_get_count(registry) == 2);
    
    assert(lle_widget_unregister(registry, "widget1") == LLE_SUCCESS);
    assert(lle_widget_registry_get_count(registry) == 1);
    
    lle_widget_registry_destroy(registry);
    lle_pool_destroy(pool);
    
    printf(" PASSED\n");
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("Running Widget System Tests:\n");
    printf("========================================\n");
    
    test_widget_registry_init();
    test_widget_registry_init_null_params();
    test_widget_register();
    test_widget_register_duplicate();
    test_widget_lookup();
    test_widget_execute();
    test_widget_execute_error();
    test_widget_unregister();
    test_widget_enable_disable();
    test_multiple_widget_types();
    test_widget_exists();
    test_widget_count();
    
    printf("========================================\n");
    printf("All Widget System tests PASSED!\n");
    
    return 0;
}

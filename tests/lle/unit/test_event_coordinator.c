/**
 * @file test_event_coordinator.c
 * @brief Unit Tests for LLE Event Coordinator
 * 
 * Tests the event coordinator component that manages event flow between LLE's
 * internal event system and Lusush's layered display architecture.
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION: src/lle/event_coordinator.c
 * 
 * TEST COVERAGE:
 * - Coordinator initialization and cleanup
 * - Sub-component initialization (translator, router, filter, queue, metrics)
 * - Parameter validation
 * - Event processing pipeline
 * - Thread safety
 * - Memory management
 * 
 * NOTE: Uses mock memory pool (test_memory_mock.c) for standalone testing
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "display/display_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Mock memory pool instance - just a non-NULL pointer for validation */
/* The test_memory_mock.c provides lle_pool_alloc/free that don't actually use this */
/* We cast a dummy value to satisfy the pointer requirement */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t*)&mock_pool_dummy;

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
        printf("  ✓ PASSED\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  ✗ ASSERTION FAILED: %s\n", message); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr, message) \
    ASSERT((ptr) != NULL, message)

#define ASSERT_NULL(ptr, message) \
    ASSERT((ptr) == NULL, message)

#define ASSERT_EQ(actual, expected, message) \
    ASSERT((actual) == (expected), message)

#define ASSERT_TRUE(condition, message) \
    ASSERT((condition), message)

#define ASSERT_FALSE(condition, message) \
    ASSERT(!(condition), message)

/* ========================================================================== */
/*                            MOCK OBJECTS                                    */
/* ========================================================================== */

/**
 * Mock implementation of display_controller_get_event_system
 * Returns NULL since we don't need a real event system for unit tests
 */
layer_event_system_t* display_controller_get_event_system(const display_controller_t *controller) {
    (void)controller; /* Unused */
    return NULL;
}

/**
 * Mock display controller for testing
 * Uses real display_controller structure but minimal initialization
 */
static display_controller_t* create_mock_display_controller(void) {
    display_controller_t *display = calloc(1, sizeof(display_controller_t));
    if (!display) return NULL;
    
    /* Create mock composition engine */
    display->compositor = calloc(1, sizeof(composition_engine_t));
    if (!display->compositor) {
        free(display);
        return NULL;
    }
    
    /* Initialize minimal compositor state */
    display->compositor->initialized = false;
    
    return display;
}

static void destroy_mock_display_controller(display_controller_t *display) {
    if (display) {
        if (display->compositor) {
            free(display->compositor);
        }
        free(display);
    }
}

/**
 * Mock editor context (opaque pointer for testing)
 */
static void* create_mock_editor(void) {
    /* Just return a non-NULL pointer for validation tests */
    static int dummy = 42;
    return &dummy;
}

/* ========================================================================== */
/*                    EVENT COORDINATOR INITIALIZATION TESTS                  */
/* ========================================================================== */

TEST(coordinator_init_success) {
    lle_event_coordinator_t *coordinator = NULL;
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    /* Test successful initialization */
    lle_result_t result = lle_event_coordinator_init(&coordinator, editor, display, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Coordinator init should succeed");
    ASSERT_NOT_NULL(coordinator, "Coordinator should be allocated");
    
    /* Verify sub-components are initialized */
    ASSERT_NOT_NULL(coordinator->translator, "Translator should be initialized");
    ASSERT_NOT_NULL(coordinator->router, "Router should be initialized");
    ASSERT_NOT_NULL(coordinator->filter, "Filter should be initialized");
    ASSERT_NOT_NULL(coordinator->queue, "Queue should be initialized");
    ASSERT_NOT_NULL(coordinator->metrics, "Metrics should be initialized");
    ASSERT_NOT_NULL(coordinator->memory_pool, "Memory pool reference should be set");
    
    /* Cleanup */
    lle_event_coordinator_cleanup(coordinator);
    lle_pool_free(coordinator);
    destroy_mock_display_controller(display);
}

TEST(coordinator_init_null_coordinator_pointer) {
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    /* Test NULL coordinator pointer */
    lle_result_t result = lle_event_coordinator_init(NULL, editor, display, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should fail with NULL coordinator pointer");
    
    /* Cleanup */
    destroy_mock_display_controller(display);
}

TEST(coordinator_init_null_display) {
    lle_event_coordinator_t *coordinator = NULL;
    void *editor = create_mock_editor();
    
    /* Test NULL display */
    lle_result_t result = lle_event_coordinator_init(&coordinator, editor, NULL, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should fail with NULL display");
    ASSERT_NULL(coordinator, "Coordinator should not be allocated on failure");
}

TEST(coordinator_init_null_memory_pool) {
    lle_event_coordinator_t *coordinator = NULL;
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    /* Test NULL memory pool */
    lle_result_t result = lle_event_coordinator_init(&coordinator, editor, display, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should fail with NULL memory pool");
    ASSERT_NULL(coordinator, "Coordinator should not be allocated on failure");
    
    /* Cleanup */
    destroy_mock_display_controller(display);
}

TEST(coordinator_init_null_editor_allowed) {
    lle_event_coordinator_t *coordinator = NULL;
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    /* Test that NULL editor is allowed (editor type not fully defined yet) */
    lle_result_t result = lle_event_coordinator_init(&coordinator, NULL, display, mock_pool);
    
    /* This should succeed - editor can be NULL */
    ASSERT_EQ(result, LLE_SUCCESS, "Should succeed with NULL editor");
    ASSERT_NOT_NULL(coordinator, "Coordinator should be allocated");
    
    /* Cleanup */
    lle_event_coordinator_cleanup(coordinator);
    lle_pool_free(coordinator);
    destroy_mock_display_controller(display);
}

/* ========================================================================== */
/*                       COORDINATOR CLEANUP TESTS                            */
/* ========================================================================== */

TEST(coordinator_cleanup_success) {
    lle_event_coordinator_t *coordinator = NULL;
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    /* Initialize coordinator */
    lle_result_t result = lle_event_coordinator_init(&coordinator, editor, display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Coordinator init should succeed");
    ASSERT_NOT_NULL(coordinator, "Coordinator should be allocated");
    
    /* Test cleanup */
    result = lle_event_coordinator_cleanup(coordinator);
    ASSERT_EQ(result, LLE_SUCCESS, "Coordinator cleanup should succeed");
    
    /* Verify all sub-components are cleared */
    ASSERT_NULL(coordinator->translator, "Translator should be cleared");
    ASSERT_NULL(coordinator->router, "Router should be cleared");
    ASSERT_NULL(coordinator->filter, "Filter should be cleared");
    ASSERT_NULL(coordinator->queue, "Queue should be cleared");
    ASSERT_NULL(coordinator->metrics, "Metrics should be cleared");
    
    /* Cleanup */
    lle_pool_free(coordinator);
    destroy_mock_display_controller(display);
}

TEST(coordinator_cleanup_null_coordinator) {
    /* Test cleanup with NULL pointer - should handle gracefully */
    lle_result_t result = lle_event_coordinator_cleanup(NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should handle NULL gracefully");
}

/* ========================================================================== */
/*                    SUB-COMPONENT INITIALIZATION TESTS                      */
/* ========================================================================== */

TEST(translator_init_success) {
    lle_event_translator_t *translator = NULL;
    
    lle_result_t result = lle_event_translator_init(&translator, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Translator init should succeed");
    ASSERT_NOT_NULL(translator, "Translator should be allocated");
    ASSERT_NOT_NULL(translator->memory_pool, "Memory pool should be set");
    
    /* Cleanup */
    lle_pool_free(translator);
}

TEST(router_init_success) {
    lle_event_router_t *router = NULL;
    
    lle_result_t result = lle_event_router_init(&router, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Router init should succeed");
    ASSERT_NOT_NULL(router, "Router should be allocated");
    ASSERT_EQ(router->route_count, 0, "Initial route count should be 0");
    ASSERT_EQ(router->route_capacity, 0, "Initial route capacity should be 0");
    
    /* Cleanup */
    pthread_mutex_destroy(&router->router_lock);
    lle_pool_free(router);
}

TEST(filter_init_success) {
    lle_event_filter_t *filter = NULL;
    
    lle_result_t result = lle_event_filter_init(&filter, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Filter init should succeed");
    ASSERT_NOT_NULL(filter, "Filter should be allocated");
    ASSERT_NOT_NULL(filter->should_process, "Filter function should be set");
    ASSERT_EQ(filter->events_filtered, 0, "Initial filtered count should be 0");
    
    /* Cleanup */
    lle_pool_free(filter);
}

TEST(queue_init_success) {
    lle_coord_queue_t *queue = NULL;
    
    lle_result_t result = lle_coord_queue_init(&queue, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Queue init should succeed");
    ASSERT_NOT_NULL(queue, "Queue should be allocated");
    ASSERT_NOT_NULL(queue->requests, "Requests array should be allocated");
    ASSERT_EQ(queue->capacity, 32, "Initial capacity should be 32");
    ASSERT_EQ(queue->count, 0, "Initial count should be 0");
    ASSERT_EQ(queue->head, 0, "Initial head should be 0");
    ASSERT_EQ(queue->tail, 0, "Initial tail should be 0");
    
    /* Cleanup */
    pthread_mutex_destroy(&queue->lock);
    lle_pool_free(queue->requests);
    lle_pool_free(queue);
}

TEST(metrics_init_success) {
    lle_event_metrics_t *metrics = NULL;
    
    lle_result_t result = lle_event_metrics_init(&metrics, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Metrics init should succeed");
    ASSERT_NOT_NULL(metrics, "Metrics should be allocated");
    ASSERT_EQ(metrics->events_processed, 0, "Initial processed count should be 0");
    ASSERT_EQ(metrics->events_filtered, 0, "Initial filtered count should be 0");
    ASSERT_EQ(metrics->avg_processing_time_ns, 0, "Initial avg time should be 0");
    
    /* Cleanup */
    lle_pool_free(metrics);
}

/* ========================================================================== */
/*                       EVENT PROCESSING TESTS                               */
/* ========================================================================== */

TEST(process_event_null_coordinator) {
    lle_event_t event = {0};
    
    lle_result_t result = lle_event_coordinator_process_event(NULL, &event);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should fail with NULL coordinator");
}

TEST(process_event_null_event) {
    lle_event_coordinator_t *coordinator = NULL;
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    lle_result_t result = lle_event_coordinator_init(&coordinator, editor, display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Coordinator init should succeed");
    
    result = lle_event_coordinator_process_event(coordinator, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should fail with NULL event");
    
    /* Cleanup */
    lle_event_coordinator_cleanup(coordinator);
    lle_pool_free(coordinator);
    destroy_mock_display_controller(display);
}

TEST(process_event_success) {
    lle_event_coordinator_t *coordinator = NULL;
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    
    ASSERT_NOT_NULL(display, "Mock display creation failed");
    
    lle_result_t result = lle_event_coordinator_init(&coordinator, editor, display, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Coordinator init should succeed");
    
    /* Create a test event */
    lle_event_t event = {
        .type = LLE_DISPLAY_EVENT_BUFFER_CHANGE,
        .timestamp = 1234567890,
        .sequence_number = 1,
        .data = NULL,
        .data_size = 0
    };
    
    result = lle_event_coordinator_process_event(coordinator, &event);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Event processing should succeed");
    ASSERT_EQ(coordinator->metrics->events_processed, 1, "Processed count should be 1");
    
    /* Cleanup */
    lle_event_coordinator_cleanup(coordinator);
    lle_pool_free(coordinator);
    destroy_mock_display_controller(display);
}

/* ========================================================================== */
/*                       ROUTER FUNCTIONALITY TESTS                           */
/* ========================================================================== */

TEST(router_add_route_success) {
    lle_event_router_t *router = NULL;
    
    lle_result_t result = lle_event_router_init(&router, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Router init should succeed");
    
    /* Dummy handler function */
    lle_result_t (*handler)(lle_event_t*, void*) = NULL;
    handler = (lle_result_t (*)(lle_event_t*, void*))(void*)0x12345;
    
    result = lle_event_router_add_route(router, LLE_DISPLAY_EVENT_BUFFER_CHANGE, handler);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Adding route should succeed");
    ASSERT_EQ(router->route_count, 1, "Route count should be 1");
    
    /* Cleanup */
    pthread_mutex_destroy(&router->router_lock);
    lle_pool_free(router);
}

/* ========================================================================== */
/*                       FILTER FUNCTIONALITY TESTS                           */
/* ========================================================================== */

TEST(filter_should_process_default) {
    lle_event_filter_t *filter = NULL;
    
    lle_result_t result = lle_event_filter_init(&filter, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Filter init should succeed");
    
    /* Create a test event */
    lle_event_t event = {
        .type = LLE_DISPLAY_EVENT_BUFFER_CHANGE,
        .timestamp = 1234567890,
        .sequence_number = 1,
        .data = NULL,
        .data_size = 0
    };
    
    /* Default filter should accept all events */
    bool should_process = lle_event_filter_should_process(filter, &event);
    
    ASSERT_TRUE(should_process, "Default filter should accept all events");
    
    /* Cleanup */
    lle_pool_free(filter);
}

/* ========================================================================== */
/*                             TEST RUNNER                                    */
/* ========================================================================== */

int main(void) {
    printf("=================================================================\n");
    printf("LLE Event Coordinator Unit Tests\n");
    printf("=================================================================\n\n");
    
    /* Coordinator initialization tests */
    printf("Coordinator Initialization Tests:\n");
    printf("-----------------------------------------------------------------\n");
    run_test_coordinator_init_success();
    run_test_coordinator_init_null_coordinator_pointer();
    run_test_coordinator_init_null_display();
    run_test_coordinator_init_null_memory_pool();
    run_test_coordinator_init_null_editor_allowed();
    
    /* Coordinator cleanup tests */
    printf("\nCoordinator Cleanup Tests:\n");
    printf("-----------------------------------------------------------------\n");
    run_test_coordinator_cleanup_success();
    run_test_coordinator_cleanup_null_coordinator();
    
    /* Sub-component initialization tests */
    printf("\nSub-Component Initialization Tests:\n");
    printf("-----------------------------------------------------------------\n");
    run_test_translator_init_success();
    run_test_router_init_success();
    run_test_filter_init_success();
    run_test_queue_init_success();
    run_test_metrics_init_success();
    
    /* Event processing tests */
    printf("\nEvent Processing Tests:\n");
    printf("-----------------------------------------------------------------\n");
    run_test_process_event_null_coordinator();
    run_test_process_event_null_event();
    run_test_process_event_success();
    
    /* Router functionality tests */
    printf("\nRouter Functionality Tests:\n");
    printf("-----------------------------------------------------------------\n");
    run_test_router_add_route_success();
    
    /* Filter functionality tests */
    printf("\nFilter Functionality Tests:\n");
    printf("-----------------------------------------------------------------\n");
    run_test_filter_should_process_default();
    
    /* Print summary */
    printf("\n=================================================================\n");
    printf("Test Summary:\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("=================================================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}

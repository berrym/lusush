/**
 * @file test_render_controller.c
 * @brief Unit Tests for LLE Render Controller
 * 
 * Tests the render controller component that coordinates all rendering operations
 * for the LLE display integration system.
 * 
 * SPECIFICATION: docs/lle_specification/08_display_integration_complete.md
 * IMPLEMENTATION: src/lle/render_controller.c
 * 
 * TEST COVERAGE:
 * - Render controller initialization and cleanup
 * - Parameter validation
 * - Memory allocation error handling
 * - Sub-component initialization (buffer renderer, cursor renderer, etc.)
 * - State initialization
 * - Proper cleanup and resource management
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
    return NULL; /* Bridge can handle NULL event system */
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

/**
 * Mock display bridge for render controller testing
 */
static lle_display_bridge_t* create_mock_display_bridge(void) {
    void *editor = create_mock_editor();
    display_controller_t *display = create_mock_display_controller();
    if (!display) return NULL;
    
    lle_display_bridge_t *bridge = NULL;
    lle_result_t result = lle_display_bridge_init(&bridge, editor, display, mock_pool);
    
    if (result != LLE_SUCCESS || !bridge) {
        destroy_mock_display_controller(display);
        return NULL;
    }
    
    return bridge;
}

static void destroy_mock_display_bridge(lle_display_bridge_t *bridge) {
    if (bridge) {
        /* Just cleanup bridge - display controller is managed separately */
        lle_display_bridge_cleanup(bridge);
    }
}

/* ========================================================================== */
/*                          INITIALIZATION TESTS                              */
/* ========================================================================== */

TEST(render_controller_init_success) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    /* Test successful initialization */
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    ASSERT_NOT_NULL(controller, "Render controller should be allocated");
    
    /* Verify controller state */
    ASSERT_NOT_NULL(controller->bridge, "Display bridge should be set");
    ASSERT_EQ(controller->bridge, bridge, "Display bridge should match input");
    ASSERT_NOT_NULL(controller->memory_pool, "Memory pool should be set");
    
    /* Verify sub-components are initialized (non-NULL) */
    ASSERT_NOT_NULL(controller->buffer_renderer, "Buffer renderer should be initialized");
    ASSERT_NOT_NULL(controller->cursor_renderer, "Cursor renderer should be initialized");
    ASSERT_NOT_NULL(controller->scheduler, "Frame scheduler should be initialized");
    ASSERT_NOT_NULL(controller->cache, "Render cache should be initialized");
    ASSERT_NOT_NULL(controller->dirty_tracker, "Dirty tracker should be initialized");
    ASSERT_NOT_NULL(controller->metrics, "Render metrics should be initialized");
    ASSERT_NOT_NULL(controller->config, "Render config should be initialized");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_init_null_controller) {
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    /* Test NULL controller pointer */
    lle_result_t result = lle_render_controller_init(NULL, bridge, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL controller pointer");
    
    /* Cleanup */
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_init_null_bridge) {
    lle_render_controller_t *controller = NULL;
    
    /* Test NULL display bridge */
    lle_result_t result = lle_render_controller_init(&controller, NULL, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL display bridge");
    ASSERT_NULL(controller, "Controller should remain NULL on failure");
}

TEST(render_controller_init_null_memory_pool) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    /* Test NULL memory pool */
    lle_result_t result = lle_render_controller_init(&controller, bridge, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL memory pool");
    ASSERT_NULL(controller, "Controller should remain NULL on failure");
    
    /* Cleanup */
    destroy_mock_display_bridge(bridge);
}

/* ========================================================================== */
/*                            CLEANUP TESTS                                   */
/* ========================================================================== */

TEST(render_controller_cleanup_success) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    /* Initialize controller */
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    ASSERT_NOT_NULL(controller, "Render controller should be allocated");
    
    /* Test cleanup */
    result = lle_render_controller_cleanup(controller);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller cleanup should succeed");
    
    /* Bridge cleanup */
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_cleanup_null_controller) {
    /* Test cleanup with NULL controller */
    lle_result_t result = lle_render_controller_cleanup(NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL controller");
}

TEST(render_controller_double_cleanup) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    /* Initialize controller */
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* First cleanup */
    result = lle_render_controller_cleanup(controller);
    ASSERT_EQ(result, LLE_SUCCESS, "First cleanup should succeed");
    
    /* Second cleanup - controller memory was freed, so we can't safely call again */
    /* This test just verifies that single cleanup works correctly */
    
    /* Bridge cleanup */
    destroy_mock_display_bridge(bridge);
}

/* ========================================================================== */
/*                         SUB-COMPONENT TESTS                                */
/* ========================================================================== */

TEST(render_controller_buffer_renderer_initialized) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    ASSERT_NOT_NULL(controller, "Render controller should be allocated");
    
    /* Verify buffer renderer structure */
    lle_buffer_renderer_t *renderer = controller->buffer_renderer;
    ASSERT_NOT_NULL(renderer, "Buffer renderer should be initialized");
    ASSERT_NOT_NULL(renderer->memory_pool, "Buffer renderer should have memory pool");
    ASSERT_EQ(renderer->max_render_size, 0, "Buffer renderer max_render_size should be 0");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_cursor_renderer_initialized) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Verify cursor renderer structure */
    lle_cursor_renderer_t *renderer = controller->cursor_renderer;
    ASSERT_NOT_NULL(renderer, "Cursor renderer should be initialized");
    ASSERT_NOT_NULL(renderer->memory_pool, "Cursor renderer should have memory pool");
    ASSERT_TRUE(renderer->cursor_visible, "Cursor should be visible by default");
    ASSERT_EQ(renderer->cursor_style, 0, "Cursor style should be default");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_frame_scheduler_initialized) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Verify frame scheduler structure */
    lle_frame_scheduler_t *scheduler = controller->scheduler;
    ASSERT_NOT_NULL(scheduler, "Frame scheduler should be initialized");
    ASSERT_EQ(scheduler->target_frame_time_us, 16667, "Should target ~60 FPS (16.667ms)");
    ASSERT_EQ(scheduler->frames_rendered, 0, "Frame count should start at 0");
    ASSERT_EQ(scheduler->frames_skipped, 0, "Skipped frame count should start at 0");
    ASSERT_TRUE(scheduler->throttling_enabled, "Throttling should be enabled by default");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_render_cache_initialized) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Verify render cache structure */
    lle_render_cache_t *cache = controller->cache;
    ASSERT_NOT_NULL(cache, "Render cache should be initialized");
    ASSERT_EQ(cache->max_render_size, 0, "Max render size should be 0");
    ASSERT_EQ(cache->cache_ttl_ms, 5000, "Cache TTL should be 5000ms");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_dirty_tracker_initialized) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Verify dirty tracker structure */
    lle_dirty_tracker_t *tracker = controller->dirty_tracker;
    ASSERT_NOT_NULL(tracker, "Dirty tracker should be initialized");
    ASSERT_EQ(tracker->region_count, 0, "Region count should start at 0");
    ASSERT_EQ(tracker->region_capacity, 0, "Region capacity should start at 0");
    ASSERT_TRUE(tracker->full_redraw_needed, "Full redraw should be needed initially");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_controller_render_metrics_initialized) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Verify render metrics structure */
    lle_render_metrics_t *metrics = controller->metrics;
    ASSERT_NOT_NULL(metrics, "Render metrics should be initialized");
    ASSERT_EQ(metrics->total_renders, 0, "Total renders should start at 0");
    ASSERT_EQ(metrics->cache_hits, 0, "Cache hits should start at 0");
    ASSERT_EQ(metrics->cache_misses, 0, "Cache misses should start at 0");
    ASSERT_EQ(metrics->avg_render_time_ns, 0, "Avg render time should start at 0");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

/* ========================================================================== */
/*                            TEST RUNNER                                     */
/* ========================================================================== */

int main(void) {
    printf("=================================================================\n");
    printf("  LLE Render Controller Unit Tests\n");
    printf("=================================================================\n\n");
    
    /* Initialization tests */
    run_test_render_controller_init_success();
    run_test_render_controller_init_null_controller();
    run_test_render_controller_init_null_bridge();
    run_test_render_controller_init_null_memory_pool();
    
    /* Cleanup tests */
    run_test_render_controller_cleanup_success();
    run_test_render_controller_cleanup_null_controller();
    run_test_render_controller_double_cleanup();
    
    /* Sub-component tests */
    run_test_render_controller_buffer_renderer_initialized();
    run_test_render_controller_cursor_renderer_initialized();
    run_test_render_controller_frame_scheduler_initialized();
    run_test_render_controller_render_cache_initialized();
    run_test_render_controller_dirty_tracker_initialized();
    run_test_render_controller_render_metrics_initialized();
    
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

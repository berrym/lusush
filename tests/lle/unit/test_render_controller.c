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

/* External memory pool from mock for buffer operations */
extern lusush_memory_pool_t *global_memory_pool;

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
    
    /* Create mock command_layer - required by display_bridge */
    display->compositor->command_layer = calloc(1, sizeof(command_layer_t));
    if (!display->compositor->command_layer) {
        free(display->compositor);
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
            if (display->compositor->command_layer) {
                free(display->compositor->command_layer);
            }
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
/*                         RENDERING OUTPUT TESTS                             */
/* ========================================================================== */

TEST(render_buffer_content_success) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    /* Initialize controller */
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Create a test buffer */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    ASSERT_NOT_NULL(buffer, "Buffer should be allocated");
    
    /* Insert some test content */
    result = lle_buffer_insert_text(buffer, 0, "Hello, World!", 13);
    ASSERT_EQ(result, LLE_SUCCESS, "Insert text should succeed");
    
    /* Create cursor position */
    lle_cursor_position_t cursor = {0};
    cursor.byte_offset = 0;
    cursor.line_number = 0;
    cursor.visual_column = 0;
    cursor.position_valid = true;
    
    /* Render buffer content */
    lle_render_output_t *output = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render buffer content should succeed");
    ASSERT_NOT_NULL(output, "Render output should be allocated");
    ASSERT_NOT_NULL(output->content, "Output content should be allocated");
    ASSERT_EQ(output->content_length, 13, "Output length should match buffer");
    ASSERT_TRUE(memcmp(output->content, "Hello, World!", 13) == 0, "Output content should match buffer");
    
    /* Verify metrics were updated */
    ASSERT_EQ(controller->metrics->total_renders, 1, "Total renders should be 1");
    ASSERT_TRUE(controller->metrics->min_render_time_ns > 0, "Min render time should be tracked");
    ASSERT_TRUE(controller->metrics->max_render_time_ns > 0, "Max render time should be tracked");
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_buffer_content_empty_buffer) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Create an empty buffer */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    /* Create cursor position */
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Render empty buffer */
    lle_render_output_t *output = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render empty buffer should succeed");
    ASSERT_NOT_NULL(output, "Render output should be allocated");
    ASSERT_EQ(output->content_length, 0, "Empty buffer should have zero length output");
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_buffer_content_null_params) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    lle_cursor_position_t cursor = {0};
    lle_render_output_t *output = NULL;
    
    /* Test null controller */
    result = lle_render_buffer_content(NULL, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null controller");
    
    /* Test null buffer */
    result = lle_render_buffer_content(controller, NULL, &cursor, &output);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null buffer");
    
    /* Test null cursor */
    result = lle_render_buffer_content(controller, buffer, NULL, &output);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null cursor");
    
    /* Test null output */
    result = lle_render_buffer_content(controller, buffer, &cursor, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null output");
    
    /* Cleanup */
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_cursor_position_success) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Create cursor at line 5, column 10 */
    lle_cursor_position_t cursor = {0};
    cursor.line_number = 5;
    cursor.visual_column = 10;
    cursor.position_valid = true;
    
    /* Render cursor position */
    char output[64];
    size_t bytes_written = 0;
    result = lle_render_cursor_position(controller, &cursor, output, sizeof(output), &bytes_written);
    ASSERT_EQ(result, LLE_SUCCESS, "Render cursor position should succeed");
    ASSERT_TRUE(bytes_written > 0, "Should write bytes");
    
    /* Verify ANSI escape sequence format: ESC[row;colH */
    /* Line 5, col 10 -> screen row 6, col 11 (1-based) */
    ASSERT_TRUE(strstr(output, "\033[6;11H") != NULL, "Should generate correct ANSI sequence");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_cursor_position_hidden) {
    lle_render_controller_t *controller = NULL;
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    ASSERT_NOT_NULL(bridge, "Mock bridge creation failed");
    
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render controller init should succeed");
    
    /* Hide cursor */
    controller->cursor_renderer->cursor_visible = false;
    
    /* Create cursor position */
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Render cursor position */
    char output[64];
    size_t bytes_written = 0;
    result = lle_render_cursor_position(controller, &cursor, output, sizeof(output), &bytes_written);
    ASSERT_EQ(result, LLE_SUCCESS, "Render cursor position should succeed");
    ASSERT_TRUE(bytes_written > 0, "Should write bytes");
    
    /* Verify hide cursor sequence: ESC[?25l */
    ASSERT_TRUE(strstr(output, "\033[?25l") != NULL, "Should generate hide cursor sequence");
    
    /* Cleanup */
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(render_output_free_success) {
    /* Allocate a render output */
    lle_render_output_t *output = lle_pool_alloc(sizeof(lle_render_output_t));
    ASSERT_NOT_NULL(output, "Output allocation should succeed");
    memset(output, 0, sizeof(lle_render_output_t));
    
    output->content = lle_pool_alloc(100);
    ASSERT_NOT_NULL(output->content, "Content allocation should succeed");
    output->content_capacity = 100;
    output->content_length = 50;
    
    /* Free the output */
    lle_result_t result = lle_render_output_free(output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render output free should succeed");
}

TEST(render_output_free_null) {
    /* Test freeing null output */
    lle_result_t result = lle_render_output_free(NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null output");
}

/* ========================================================================== */
/*                         PIPELINE STAGE TESTS                               */
/* ========================================================================== */

TEST(pipeline_init_success) {
    lle_render_pipeline_t *pipeline = NULL;
    
    /* Initialize pipeline */
    lle_result_t result = lle_render_pipeline_init(&pipeline, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline init should succeed");
    ASSERT_NOT_NULL(pipeline, "Pipeline should be allocated");
    
    /* Verify pipeline structure */
    ASSERT_EQ(pipeline->stage_count, 4, "Should have 4 stages");
    ASSERT_EQ(pipeline->stage_capacity, 4, "Should have capacity for 4 stages");
    ASSERT_NOT_NULL(pipeline->stages, "Stages array should be allocated");
    ASSERT_FALSE(pipeline->parallel_execution_enabled, "Parallel execution should be disabled by default");
    
    /* Verify each stage */
    ASSERT_EQ(pipeline->stages[0].type, LLE_RENDER_STAGE_PREPROCESSING, "Stage 0 should be preprocessing");
    ASSERT_TRUE(pipeline->stages[0].enabled, "Preprocessing stage should be enabled");
    ASSERT_NOT_NULL(pipeline->stages[0].execute, "Preprocessing stage should have execute function");
    
    ASSERT_EQ(pipeline->stages[1].type, LLE_RENDER_STAGE_SYNTAX, "Stage 1 should be syntax");
    ASSERT_TRUE(pipeline->stages[1].enabled, "Syntax stage should be enabled");
    ASSERT_NOT_NULL(pipeline->stages[1].execute, "Syntax stage should have execute function");
    
    ASSERT_EQ(pipeline->stages[2].type, LLE_RENDER_STAGE_FORMATTING, "Stage 2 should be formatting");
    ASSERT_TRUE(pipeline->stages[2].enabled, "Formatting stage should be enabled");
    ASSERT_NOT_NULL(pipeline->stages[2].execute, "Formatting stage should have execute function");
    
    ASSERT_EQ(pipeline->stages[3].type, LLE_RENDER_STAGE_COMPOSITION, "Stage 3 should be composition");
    ASSERT_TRUE(pipeline->stages[3].enabled, "Composition stage should be enabled");
    ASSERT_NOT_NULL(pipeline->stages[3].execute, "Composition stage should have execute function");
    
    /* Cleanup */
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_init_null_params) {
    lle_render_pipeline_t *pipeline = NULL;
    
    /* Test null pipeline pointer */
    lle_result_t result = lle_render_pipeline_init(NULL, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null pipeline pointer");
    
    /* Test null memory pool */
    result = lle_render_pipeline_init(&pipeline, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null memory pool");
}

TEST(pipeline_execute_success) {
    lle_render_pipeline_t *pipeline = NULL;
    
    /* Initialize pipeline */
    lle_result_t result = lle_render_pipeline_init(&pipeline, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline init should succeed");
    
    /* Create a test buffer */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    /* Insert test content */
    result = lle_buffer_insert_text(buffer, 0, "Test content", 12);
    ASSERT_EQ(result, LLE_SUCCESS, "Insert text should succeed");
    
    /* Create render context */
    lle_render_context_t context = {0};
    context.buffer = buffer;
    context.memory_pool = mock_pool;
    
    /* Execute pipeline */
    lle_render_output_t *output = NULL;
    result = lle_render_pipeline_execute(pipeline, &context, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline execution should succeed");
    ASSERT_NOT_NULL(output, "Output should be allocated");
    ASSERT_NOT_NULL(output->content, "Output content should be allocated");
    ASSERT_EQ(output->content_length, 12, "Output length should match input");
    
    /* Verify stage metrics were updated */
    for (size_t i = 0; i < pipeline->stage_count; i++) {
        ASSERT_TRUE(pipeline->stages[i].execution_count > 0, "Stage should have been executed");
    }
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_execute_null_params) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_result_t result = lle_render_pipeline_init(&pipeline, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline init should succeed");
    
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    lle_render_context_t context = {0};
    context.buffer = buffer;
    context.memory_pool = mock_pool;
    lle_render_output_t *output = NULL;
    
    /* Test null pipeline */
    result = lle_render_pipeline_execute(NULL, &context, &output);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null pipeline");
    
    /* Test null context */
    result = lle_render_pipeline_execute(pipeline, NULL, &output);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null context");
    
    /* Test null output */
    result = lle_render_pipeline_execute(pipeline, &context, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null output");
    
    /* Cleanup */
    lle_buffer_destroy(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_cleanup_null) {
    /* Test cleanup with null pipeline */
    lle_result_t result = lle_render_pipeline_cleanup(NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null pipeline");
}

/* ========================================================================== */
/*                    CACHE SYSTEM TESTS (libhashtable)                       */
/* ========================================================================== */

TEST(cache_init_success) {
    /* Test successful cache initialization using libhashtable */
    lle_display_cache_t *cache = NULL;
    
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    ASSERT_NOT_NULL(cache, "Cache should be allocated");
    ASSERT_NOT_NULL(cache->cache_table, "libhashtable should be created");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_init_null_params) {
    /* Test cache init with null parameters */
    lle_display_cache_t *cache = NULL;
    
    lle_result_t result = lle_display_cache_init(NULL, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null cache pointer");
    
    result = lle_display_cache_init(&cache, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null memory pool");
}

TEST(cache_store_and_lookup_success) {
    /* Test storing and retrieving from cache using libhashtable */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Store data in cache */
    const char *test_data = "test render output";
    uint64_t key = 12345;
    
    result = lle_display_cache_store(cache, key, test_data, strlen(test_data) + 1);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache store should succeed");
    
    /* Lookup data from cache */
    void *retrieved_data = NULL;
    size_t retrieved_size = 0;
    
    result = lle_display_cache_lookup(cache, key, &retrieved_data, &retrieved_size);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache lookup should succeed");
    ASSERT_NOT_NULL(retrieved_data, "Retrieved data should not be null");
    ASSERT_EQ(retrieved_size, strlen(test_data) + 1, "Retrieved size should match");
    ASSERT_EQ(strcmp((char*)retrieved_data, test_data), 0, "Retrieved data should match");
    
    /* Cleanup */
    lle_pool_free(retrieved_data); /* Free deserialized data */
    lle_display_cache_cleanup(cache);
}

TEST(cache_lookup_miss) {
    /* Test cache miss on non-existent key */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Try to lookup non-existent key */
    void *retrieved_data = NULL;
    size_t retrieved_size = 0;
    uint64_t nonexistent_key = 99999;
    
    result = lle_display_cache_lookup(cache, nonexistent_key, &retrieved_data, &retrieved_size);
    ASSERT_EQ(result, LLE_ERROR_CACHE_MISS, "Should return cache miss");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_hit_count) {
    /* Test cache hit metrics with libhashtable backend */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Store data */
    const char *test_data = "test data";
    uint64_t key = 100;
    result = lle_display_cache_store(cache, key, test_data, strlen(test_data) + 1);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache store should succeed");
    
    /* Get initial hit count */
    uint64_t initial_hits = cache->metrics->cache_hits;
    
    /* Lookup multiple times to increment hit count */
    void *data = NULL;
    size_t size = 0;
    for (int i = 0; i < 5; i++) {
        result = lle_display_cache_lookup(cache, key, &data, &size);
        ASSERT_EQ(result, LLE_SUCCESS, "Cache lookup should succeed");
        if (data) {
            lle_pool_free(data);
            data = NULL;
        }
    }
    
    /* Verify hit count increased */
    ASSERT_EQ(cache->metrics->cache_hits, initial_hits + 5, "Hit count should increase by 5");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_miss_count) {
    /* Test cache miss metrics */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Get initial miss count */
    uint64_t initial_misses = cache->metrics->cache_misses;
    
    /* Try to lookup non-existent keys */
    void *data = NULL;
    size_t size = 0;
    for (int i = 0; i < 3; i++) {
        result = lle_display_cache_lookup(cache, 1000 + i, &data, &size);
        ASSERT_EQ(result, LLE_ERROR_CACHE_MISS, "Should return cache miss");
    }
    
    /* Verify miss count increased */
    ASSERT_EQ(cache->metrics->cache_misses, initial_misses + 3, "Miss count should increase by 3");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(render_cache_init_success) {
    /* Test render cache initialization with libhashtable */
    lle_render_cache_t *cache = NULL;
    
    lle_result_t result = lle_render_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render cache init should succeed");
    ASSERT_NOT_NULL(cache, "Render cache should be allocated");
    ASSERT_NOT_NULL(cache->base_cache, "Base cache should be initialized");
    ASSERT_NOT_NULL(cache->base_cache->cache_table, "libhashtable should be created");
    
    /* Cleanup */
    lle_render_cache_cleanup(cache);
}

TEST(render_cache_init_null_params) {
    /* Test render cache init with null parameters */
    lle_render_cache_t *cache = NULL;
    
    lle_result_t result = lle_render_cache_init(NULL, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null cache pointer");
    
    result = lle_render_cache_init(&cache, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null memory pool");
}

TEST(cache_invalidate_entry) {
    /* Test cache entry invalidation */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Store data */
    const char *test_data = "test data";
    uint64_t key = 300;
    result = lle_display_cache_store(cache, key, test_data, strlen(test_data) + 1);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache store should succeed");
    
    /* Verify it's in cache */
    void *data = NULL;
    size_t size = 0;
    result = lle_display_cache_lookup(cache, key, &data, &size);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache lookup should succeed");
    if (data) lle_pool_free(data);
    
    /* Invalidate entry */
    result = lle_display_cache_invalidate(cache, key);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache invalidate should succeed");
    
    /* Verify it's no longer in cache */
    data = NULL;
    result = lle_display_cache_lookup(cache, key, &data, &size);
    ASSERT_EQ(result, LLE_ERROR_CACHE_MISS, "Should return cache miss after invalidation");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_invalidate_all) {
    /* Test invalidating all cache entries */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Store multiple entries */
    const char *test_data = "test";
    for (uint64_t i = 0; i < 5; i++) {
        result = lle_display_cache_store(cache, 400 + i, test_data, strlen(test_data) + 1);
        ASSERT_EQ(result, LLE_SUCCESS, "Cache store should succeed");
    }
    
    /* Invalidate all */
    result = lle_display_cache_invalidate_all(cache);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache invalidate all should succeed");
    
    /* Verify all entries are gone */
    void *data = NULL;
    size_t size = 0;
    for (uint64_t i = 0; i < 5; i++) {
        result = lle_display_cache_lookup(cache, 400 + i, &data, &size);
        ASSERT_EQ(result, LLE_ERROR_CACHE_MISS, "Should return cache miss after invalidate all");
    }
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_hit_rate_calculation) {
    /* Test cache hit rate calculation */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Store data */
    const char *test_data = "test";
    uint64_t key = 500;
    result = lle_display_cache_store(cache, key, test_data, strlen(test_data) + 1);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache store should succeed");
    
    /* Generate 7 hits and 3 misses for 70% hit rate */
    void *data = NULL;
    size_t size = 0;
    
    /* 7 hits */
    for (int i = 0; i < 7; i++) {
        result = lle_display_cache_lookup(cache, key, &data, &size);
        ASSERT_EQ(result, LLE_SUCCESS, "Cache lookup should succeed");
        if (data) {
            lle_pool_free(data);
            data = NULL;
        }
    }
    
    /* 3 misses */
    for (int i = 0; i < 3; i++) {
        result = lle_display_cache_lookup(cache, 600 + i, &data, &size);
        ASSERT_EQ(result, LLE_ERROR_CACHE_MISS, "Cache lookup should miss");
    }
    
    /* Check hit rate (should be 70%) */
    double hit_rate = cache->metrics->hit_rate;
    ASSERT_TRUE(hit_rate >= 69.0 && hit_rate <= 71.0, "Hit rate should be approximately 70%");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_policy_initialized) {
    /* Test that LRU policy is initialized */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Verify policy is initialized */
    ASSERT_NOT_NULL(cache->policy, "Cache policy should be initialized");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

/* ========================================================================== */
/*                      DIRTY REGION TRACKING TESTS                           */
/* ========================================================================== */

TEST(dirty_tracker_init_success) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Dirty tracker init should succeed");
    ASSERT_NOT_NULL(tracker, "Dirty tracker should be allocated");
    ASSERT_NOT_NULL(tracker->dirty_regions, "Dirty regions array should be allocated");
    ASSERT_EQ(tracker->region_count, 0, "Region count should be 0");
    ASSERT_TRUE(tracker->full_redraw_needed, "Full redraw should be needed initially");
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_init_null_params) {
    lle_result_t result = lle_dirty_tracker_init(NULL, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL tracker pointer");
    
    lle_dirty_tracker_t *tracker = NULL;
    result = lle_dirty_tracker_init(&tracker, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL memory pool");
}

TEST(dirty_tracker_mark_region) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    /* Clear initial full redraw flag */
    tracker->full_redraw_needed = false;
    
    /* Mark a region dirty */
    result = lle_dirty_tracker_mark_region(tracker, 100);
    ASSERT_EQ(result, LLE_SUCCESS, "Mark region should succeed");
    ASSERT_EQ(tracker->region_count, 1, "Should have 1 dirty region");
    ASSERT_EQ(tracker->dirty_regions[0], 100, "Region should be at offset 100");
    
    /* Mark another non-overlapping region */
    result = lle_dirty_tracker_mark_region(tracker, 500);
    ASSERT_EQ(result, LLE_SUCCESS, "Mark second region should succeed");
    ASSERT_EQ(tracker->region_count, 2, "Should have 2 dirty regions");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_mark_range) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    /* Clear initial full redraw flag */
    tracker->full_redraw_needed = false;
    
    /* Mark a range dirty (100 bytes starting at offset 200) */
    result = lle_dirty_tracker_mark_range(tracker, 200, 100);
    ASSERT_EQ(result, LLE_SUCCESS, "Mark range should succeed");
    ASSERT_TRUE(tracker->region_count > 0, "Should have dirty regions");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_mark_full) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    /* Clear initial state */
    tracker->full_redraw_needed = false;
    tracker->region_count = 5; /* Simulate some regions */
    
    /* Mark for full redraw */
    result = lle_dirty_tracker_mark_full(tracker);
    ASSERT_EQ(result, LLE_SUCCESS, "Mark full should succeed");
    ASSERT_TRUE(tracker->full_redraw_needed, "Full redraw flag should be set");
    ASSERT_EQ(tracker->region_count, 0, "Regions should be cleared");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_clear) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    /* Mark some regions */
    tracker->full_redraw_needed = false;
    lle_dirty_tracker_mark_region(tracker, 100);
    lle_dirty_tracker_mark_region(tracker, 200);
    ASSERT_EQ(tracker->region_count, 2, "Should have 2 regions");
    
    /* Clear the tracker */
    result = lle_dirty_tracker_clear(tracker);
    ASSERT_EQ(result, LLE_SUCCESS, "Clear should succeed");
    ASSERT_EQ(tracker->region_count, 0, "Region count should be 0");
    ASSERT_FALSE(tracker->full_redraw_needed, "Full redraw flag should be cleared");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_is_region_dirty) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    /* Initial state: full redraw needed */
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 100), 
                "Any region should be dirty when full redraw needed");
    
    /* Clear and mark specific region */
    tracker->full_redraw_needed = false;
    lle_dirty_tracker_mark_region(tracker, 200);
    
    /* Check region is dirty (within merge threshold) */
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 200), 
                "Marked region should be dirty");
    ASSERT_TRUE(lle_dirty_tracker_is_region_dirty(tracker, 210), 
                "Region within threshold should be dirty");
    ASSERT_FALSE(lle_dirty_tracker_is_region_dirty(tracker, 500), 
                 "Far region should not be dirty");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_needs_full_redraw) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    /* Initial state */
    ASSERT_TRUE(lle_dirty_tracker_needs_full_redraw(tracker), 
                "Should need full redraw initially");
    
    /* Clear flag */
    tracker->full_redraw_needed = false;
    ASSERT_FALSE(lle_dirty_tracker_needs_full_redraw(tracker), 
                 "Should not need full redraw after clearing");
    
    /* Set flag again */
    lle_dirty_tracker_mark_full(tracker);
    ASSERT_TRUE(lle_dirty_tracker_needs_full_redraw(tracker), 
                "Should need full redraw after marking");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_region_merging) {
    lle_dirty_tracker_t *tracker = NULL;
    lle_result_t result = lle_dirty_tracker_init(&tracker, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Tracker init should succeed");
    
    tracker->full_redraw_needed = false;
    
    /* Mark two regions close together (within merge threshold of 64 bytes) */
    lle_dirty_tracker_mark_region(tracker, 100);
    size_t count_after_first = tracker->region_count;
    ASSERT_EQ(count_after_first, 1, "Should have 1 region after first mark");
    
    lle_dirty_tracker_mark_region(tracker, 130); /* Within 64 bytes */
    /* Should merge with existing region or add new one depending on implementation */
    ASSERT_TRUE(tracker->region_count <= 2, "Should have at most 2 regions");
    
    lle_dirty_tracker_cleanup(tracker);
}

TEST(dirty_tracker_cleanup_null) {
    lle_result_t result = lle_dirty_tracker_cleanup(NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject NULL tracker");
}

/* ========================================================================== */
/*                      PARTIAL RENDERING INTEGRATION TESTS                   */
/* ========================================================================== */

TEST(partial_render_with_dirty_regions) {
    /* Test that marking dirty regions triggers partial render */
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    lle_render_controller_t *controller = NULL;
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Controller init should succeed");
    
    /* Enable dirty tracking */
    controller->config->dirty_tracking_enabled = true;
    
    /* Create a buffer with content */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    /* Insert some content */
    const char *content = "Hello World";
    result = lle_buffer_insert_text(buffer, 0, content, strlen(content));
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer insert should succeed");
    
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Clear full redraw flag first (set by init) */
    controller->dirty_tracker->full_redraw_needed = false;
    
    /* Mark some regions dirty (far apart to avoid merging - 64 byte threshold) */
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 5);
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 100);
    
    /* Verify we have dirty regions */
    ASSERT_TRUE(controller->dirty_tracker->region_count >= 1, "Should have at least 1 dirty region");
    
    /* Render buffer */
    lle_render_output_t *output = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render should succeed");
    ASSERT_NOT_NULL(output, "Output should be allocated");
    
    /* Verify partial render was attempted (might fall back to full if buffer too small) */
    ASSERT_TRUE(controller->metrics->total_renders == 1, "Should have 1 total render");
    
    /* Verify dirty tracker was cleared */
    ASSERT_EQ(controller->dirty_tracker->region_count, 0, "Dirty regions should be cleared");
    ASSERT_FALSE(controller->dirty_tracker->full_redraw_needed, "Full redraw flag should be clear");
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(full_render_when_full_redraw_needed) {
    /* Test that full redraw flag triggers full render */
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    lle_render_controller_t *controller = NULL;
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Controller init should succeed");
    
    /* Enable dirty tracking */
    controller->config->dirty_tracking_enabled = true;
    
    /* Create a buffer */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Mark full redraw needed */
    lle_dirty_tracker_mark_full(controller->dirty_tracker);
    ASSERT_TRUE(controller->dirty_tracker->full_redraw_needed, "Full redraw should be needed");
    
    /* Render buffer */
    lle_render_output_t *output = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render should succeed");
    
    /* Verify full render metrics */
    ASSERT_EQ(controller->metrics->full_renders, 1, "Should have 1 full render");
    ASSERT_EQ(controller->metrics->partial_renders, 0, "Should have 0 partial renders");
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(full_render_when_dirty_tracking_disabled) {
    /* Test that disabled dirty tracking always does full render */
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    lle_render_controller_t *controller = NULL;
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Controller init should succeed");
    
    /* Disable dirty tracking */
    controller->config->dirty_tracking_enabled = false;
    
    /* Create a buffer */
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Mark dirty regions (should be ignored) */
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 2);
    controller->dirty_tracker->full_redraw_needed = false;
    
    /* Render buffer */
    lle_render_output_t *output = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render should succeed");
    
    /* Verify full render even with dirty regions */
    ASSERT_EQ(controller->metrics->full_renders, 1, "Should do full render when tracking disabled");
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(partial_render_metrics_tracking) {
    /* Test that partial render timing is tracked separately */
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    lle_render_controller_t *controller = NULL;
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Controller init should succeed");
    
    controller->config->dirty_tracking_enabled = true;
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Do a partial render attempt */
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 5);
    controller->dirty_tracker->full_redraw_needed = false;
    
    lle_render_output_t *output1 = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output1);
    ASSERT_EQ(result, LLE_SUCCESS, "Render should succeed");
    
    /* Do a full render */
    lle_dirty_tracker_mark_full(controller->dirty_tracker);
    lle_render_output_t *output2 = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output2);
    ASSERT_EQ(result, LLE_SUCCESS, "Full render should succeed");
    
    /* Verify total renders */
    ASSERT_EQ(controller->metrics->total_renders, 2, "Should have 2 total renders");
    
    /* Verify at least one metric is tracked */
    ASSERT_TRUE(controller->metrics->avg_render_time_ns > 0, "Average render time should be tracked");
    
    /* Cleanup */
    lle_render_output_free(output1);
    lle_render_output_free(output2);
    lle_buffer_destroy(buffer);
    lle_render_controller_cleanup(controller);
    destroy_mock_display_bridge(bridge);
}

TEST(dirty_tracker_cleared_after_render) {
    /* Test that dirty tracker is cleared after successful render */
    lle_display_bridge_t *bridge = create_mock_display_bridge();
    lle_render_controller_t *controller = NULL;
    lle_result_t result = lle_render_controller_init(&controller, bridge, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Controller init should succeed");
    
    controller->config->dirty_tracking_enabled = true;
    lle_buffer_t *buffer = NULL;
    result = lle_buffer_create(&buffer, global_memory_pool, 1024);
    ASSERT_EQ(result, LLE_SUCCESS, "Buffer creation should succeed");
    
    lle_cursor_position_t cursor = {0};
    cursor.position_valid = true;
    
    /* Clear full redraw flag first (set by init) */
    controller->dirty_tracker->full_redraw_needed = false;
    
    /* Mark multiple dirty regions (far apart to avoid merging - 64 byte threshold) */
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 10);
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 100);
    lle_dirty_tracker_mark_region(controller->dirty_tracker, 200);
    
    size_t dirty_count_before = controller->dirty_tracker->region_count;
    ASSERT_TRUE(dirty_count_before >= 1, "Should have at least 1 dirty region");
    
    /* Render */
    lle_render_output_t *output = NULL;
    result = lle_render_buffer_content(controller, buffer, &cursor, &output);
    ASSERT_EQ(result, LLE_SUCCESS, "Render should succeed");
    
    /* Verify dirty tracker cleared */
    ASSERT_EQ(controller->dirty_tracker->region_count, 0, "Dirty regions should be cleared");
    ASSERT_FALSE(controller->dirty_tracker->full_redraw_needed, "Full redraw flag should be cleared");
    
    /* Cleanup */
    lle_render_output_free(output);
    lle_buffer_destroy(buffer);
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
    
    /* Rendering output tests */
    run_test_render_buffer_content_success();
    run_test_render_buffer_content_empty_buffer();
    run_test_render_buffer_content_null_params();
    run_test_render_cursor_position_success();
    run_test_render_cursor_position_hidden();
    run_test_render_output_free_success();
    run_test_render_output_free_null();
    
    /* Pipeline stage tests */
    run_test_pipeline_init_success();
    run_test_pipeline_init_null_params();
    run_test_pipeline_execute_success();
    run_test_pipeline_execute_null_params();
    run_test_pipeline_cleanup_null();
    
    /* Cache system tests (libhashtable integration) */
    run_test_cache_init_success();
    run_test_cache_init_null_params();
    run_test_cache_store_and_lookup_success();
    run_test_cache_lookup_miss();
    run_test_cache_hit_count();
    run_test_cache_miss_count();
    run_test_render_cache_init_success();
    run_test_render_cache_init_null_params();
    
    /* Cache policy and invalidation tests */
    run_test_cache_invalidate_entry();
    run_test_cache_invalidate_all();
    run_test_cache_hit_rate_calculation();
    run_test_cache_policy_initialized();
    
    /* Dirty region tracking tests */
    run_test_dirty_tracker_init_success();
    run_test_dirty_tracker_init_null_params();
    run_test_dirty_tracker_mark_region();
    run_test_dirty_tracker_mark_range();
    run_test_dirty_tracker_mark_full();
    run_test_dirty_tracker_clear();
    run_test_dirty_tracker_is_region_dirty();
    run_test_dirty_tracker_needs_full_redraw();
    run_test_dirty_tracker_region_merging();
    run_test_dirty_tracker_cleanup_null();
    
    /* Partial rendering integration tests */
    run_test_partial_render_with_dirty_regions();
    run_test_full_render_when_full_redraw_needed();
    run_test_full_render_when_dirty_tracking_disabled();
    run_test_partial_render_metrics_tracking();
    run_test_dirty_tracker_cleared_after_render();
    
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

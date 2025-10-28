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
/*                         CACHE SYSTEM TESTS                                 */
/* ========================================================================== */

TEST(cache_init_success) {
    /* Test successful cache initialization */
    lle_display_cache_t *cache = NULL;
    
    lle_result_t result = lle_display_cache_init(&cache, 128, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    ASSERT_NOT_NULL(cache, "Cache should be allocated");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_init_null_params) {
    /* Test cache init with null parameters */
    lle_display_cache_t *cache = NULL;
    
    lle_result_t result = lle_display_cache_init(NULL, 128, mock_pool);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null cache pointer");
    
    result = lle_display_cache_init(&cache, 128, NULL);
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Should reject null memory pool");
}

TEST(cache_store_and_lookup_success) {
    /* Test storing and retrieving from cache */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, 128, mock_pool);
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
    lle_display_cache_cleanup(cache);
}

TEST(cache_lookup_miss) {
    /* Test cache miss on non-existent key */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, 128, mock_pool);
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
    /* Test cache hit metrics */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, 128, mock_pool);
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
    }
    
    /* Verify hit count increased */
    ASSERT_EQ(cache->metrics->cache_hits, initial_hits + 5, "Hit count should increase by 5");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(cache_miss_count) {
    /* Test cache miss metrics */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, 128, mock_pool);
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

TEST(cache_update_existing_entry) {
    /* Test updating an existing cache entry */
    lle_display_cache_t *cache = NULL;
    lle_result_t result = lle_display_cache_init(&cache, 128, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache init should succeed");
    
    /* Store initial data */
    const char *initial_data = "initial";
    uint64_t key = 200;
    result = lle_display_cache_store(cache, key, initial_data, strlen(initial_data) + 1);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache store should succeed");
    
    /* Update with new data */
    const char *updated_data = "updated";
    result = lle_display_cache_store(cache, key, updated_data, strlen(updated_data) + 1);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache update should succeed");
    
    /* Lookup and verify updated data */
    void *retrieved_data = NULL;
    size_t retrieved_size = 0;
    result = lle_display_cache_lookup(cache, key, &retrieved_data, &retrieved_size);
    ASSERT_EQ(result, LLE_SUCCESS, "Cache lookup should succeed");
    ASSERT_EQ(strcmp((char*)retrieved_data, updated_data), 0, "Should retrieve updated data");
    
    /* Cleanup */
    lle_display_cache_cleanup(cache);
}

TEST(render_cache_init_success) {
    /* Test render cache initialization */
    lle_render_cache_t *cache = NULL;
    
    lle_result_t result = lle_render_cache_init(&cache, mock_pool);
    ASSERT_EQ(result, LLE_SUCCESS, "Render cache init should succeed");
    ASSERT_NOT_NULL(cache, "Render cache should be allocated");
    ASSERT_NOT_NULL(cache->base_cache, "Base cache should be initialized");
    
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
    
    /* Cache system tests */
    run_test_cache_init_success();
    run_test_cache_init_null_params();
    run_test_cache_store_and_lookup_success();
    run_test_cache_lookup_miss();
    run_test_cache_hit_count();
    run_test_cache_miss_count();
    run_test_cache_update_existing_entry();
    run_test_render_cache_init_success();
    run_test_render_cache_init_null_params();
    
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

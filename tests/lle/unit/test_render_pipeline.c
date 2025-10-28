/**
 * @file test_render_pipeline.c
 * @brief Unit Tests for LLE Render Pipeline
 * 
 * Tests the multi-stage rendering pipeline including:
 * - Pipeline initialization and cleanup
 * - Stage execution (preprocessing, syntax, formatting, composition)
 * - Pipeline metrics tracking
 * - Error handling and parameter validation
 * 
 * IMPLEMENTATION: src/lle/render_pipeline.c
 */

#include "lle/display_integration.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/buffer_management.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ========================================================================== */
/*                         TEST FRAMEWORK                                     */
/* ========================================================================== */

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    __attribute__((unused)) static void run_test_##name(void) { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
    } \
    static void test_##name(void)

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  ✗ ASSERTION FAILED: %s\n", msg); \
            printf("    at %s:%d\n", __FILE__, __LINE__); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)
#define ASSERT_NULL(ptr, msg) ASSERT_TRUE((ptr) == NULL, msg)
#define ASSERT_NOT_NULL(ptr, msg) ASSERT_TRUE((ptr) != NULL, msg)
#define ASSERT_EQ(a, b, msg) ASSERT_TRUE((a) == (b), msg)
#define ASSERT_NEQ(a, b, msg) ASSERT_TRUE((a) != (b), msg)
#define ASSERT_GT(a, b, msg) ASSERT_TRUE((a) > (b), msg)

/* ========================================================================== */
/*                         MOCK OBJECTS                                       */
/* ========================================================================== */

/* Mock memory pool */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t*)&mock_pool_dummy;

/* Stubs for Lusush functions */
lusush_memory_pool_system_t *global_memory_pool = NULL;

void *lusush_pool_alloc(size_t size) {
    return malloc(size);
}

void lusush_pool_free(void *ptr) {
    free(ptr);
}

lusush_pool_config_t lusush_pool_get_default_config(void) {
    lusush_pool_config_t config = {0};
    return config;
}

lusush_pool_error_t lusush_pool_init(const lusush_pool_config_t *config) {
    (void)config;
    return 0;
}

/* Helper to create a mock buffer */
static lle_buffer_t* create_mock_buffer(const char *content) {
    lle_buffer_t *buffer = (lle_buffer_t*)malloc(sizeof(lle_buffer_t));
    memset(buffer, 0, sizeof(lle_buffer_t));
    
    if (content && strlen(content) > 0) {
        buffer->data = strdup(content);
        buffer->length = strlen(content);
        buffer->capacity = buffer->length + 1;
    }
    
    return buffer;
}

static void destroy_mock_buffer(lle_buffer_t *buffer) {
    if (buffer) {
        if (buffer->data) {
            free(buffer->data);
        }
        free(buffer);
    }
}

/* ========================================================================== */
/*                    PIPELINE INITIALIZATION TESTS                           */
/* ========================================================================== */

TEST(pipeline_init_success) {
    lle_render_pipeline_t *pipeline = NULL;
    
    lle_result_t result = lle_render_pipeline_init(&pipeline, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline init should succeed");
    ASSERT_NOT_NULL(pipeline, "Pipeline should be allocated");
    
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_init_null_output) {
    lle_result_t result = lle_render_pipeline_init(NULL, mock_pool);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL output should return error");
}

TEST(pipeline_init_null_pool) {
    lle_render_pipeline_t *pipeline = NULL;
    
    lle_result_t result = lle_render_pipeline_init(&pipeline, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL pool should return error");
}

TEST(pipeline_reinit_after_cleanup) {
    lle_render_pipeline_t *pipeline = NULL;
    
    /* Init, cleanup, then init again */
    lle_render_pipeline_init(&pipeline, mock_pool);
    lle_render_pipeline_cleanup(pipeline);
    
    lle_result_t result = lle_render_pipeline_init(&pipeline, mock_pool);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline should reinitialize after cleanup");
    ASSERT_NOT_NULL(pipeline, "Pipeline should be allocated again");
    
    lle_render_pipeline_cleanup(pipeline);
}

/* ========================================================================== */
/*                    PIPELINE CLEANUP TESTS                                  */
/* ========================================================================== */

TEST(pipeline_cleanup_null) {
    lle_result_t result = lle_render_pipeline_cleanup(NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "Cleanup with NULL should return error");
}

TEST(pipeline_cleanup_success) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_result_t result = lle_render_pipeline_cleanup(pipeline);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Cleanup should succeed");
}

/* ========================================================================== */
/*                    PIPELINE EXECUTION TESTS                                */
/* ========================================================================== */

TEST(pipeline_execute_null_pipeline) {
    lle_buffer_t *buffer = create_mock_buffer("test");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(NULL, &context, &output);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL pipeline should return error");
    
    destroy_mock_buffer(buffer);
}

TEST(pipeline_execute_null_context) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, NULL, &output);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL context should return error");
    
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_execute_null_output) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    lle_buffer_t *buffer = create_mock_buffer("test");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, NULL);
    
    ASSERT_EQ(result, LLE_ERROR_INVALID_PARAMETER, "NULL output should return error");
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_execute_empty_buffer) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_buffer_t *buffer = create_mock_buffer("");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Empty buffer should execute successfully");
    ASSERT_NOT_NULL(output, "Output should be allocated");
    ASSERT_EQ(output->content_length, 0, "Output should be empty");
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_execute_simple_content) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_buffer_t *buffer = create_mock_buffer("Hello, World!");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Simple content should execute successfully");
    ASSERT_NOT_NULL(output, "Output should be allocated");
    ASSERT_GT(output->content_length, 0, "Output should have content");
    ASSERT_NOT_NULL(output->content, "Output content should be allocated");
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_execute_multiline_content) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_buffer_t *buffer = create_mock_buffer("Line 1\nLine 2\nLine 3");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Multiline content should execute successfully");
    ASSERT_NOT_NULL(output, "Output should be allocated");
    ASSERT_GT(output->content_length, 0, "Output should have content");
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

/* ========================================================================== */
/*                    PIPELINE BEHAVIOR TESTS                                 */
/* ========================================================================== */

TEST(pipeline_multiple_executions) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_buffer_t *buffer = create_mock_buffer("test content");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    
    /* Execute pipeline multiple times */
    for (int i = 0; i < 3; i++) {
        lle_render_output_t *output = NULL;
        lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
        
        ASSERT_EQ(result, LLE_SUCCESS, "Each execution should succeed");
        ASSERT_NOT_NULL(output, "Each execution should produce output");
    }
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_different_content_sizes) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    const char *test_strings[] = {
        "",
        "short",
        "This is a medium length string for testing",
        "This is a very long string that contains quite a bit more content to test how the pipeline handles larger buffers and ensures that memory allocation and processing works correctly even with substantial amounts of text data"
    };
    
    for (int i = 0; i < 4; i++) {
        lle_buffer_t *buffer = create_mock_buffer(test_strings[i]);
        lle_render_context_t context = {0};
        context.buffer = buffer;
        lle_render_output_t *output = NULL;
        
        lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
        
        ASSERT_EQ(result, LLE_SUCCESS, "Pipeline should handle various content sizes");
        ASSERT_NOT_NULL(output, "Output should be produced");
        
        destroy_mock_buffer(buffer);
    }
    
    lle_render_pipeline_cleanup(pipeline);
}

/* ========================================================================== */
/*                    PIPELINE STAGE VERIFICATION TESTS                       */
/* ========================================================================== */

TEST(pipeline_handles_special_characters) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_buffer_t *buffer = create_mock_buffer("Special: \t\n\r!@#$%^&*()");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline should handle special characters");
    ASSERT_NOT_NULL(output, "Output should be produced");
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

TEST(pipeline_handles_unicode) {
    lle_render_pipeline_t *pipeline = NULL;
    lle_render_pipeline_init(&pipeline, mock_pool);
    
    lle_buffer_t *buffer = create_mock_buffer("Unicode: 你好世界 🎉");
    lle_render_context_t context = {0};
    context.buffer = buffer;
    lle_render_output_t *output = NULL;
    
    lle_result_t result = lle_render_pipeline_execute(pipeline, &context, &output);
    
    ASSERT_EQ(result, LLE_SUCCESS, "Pipeline should handle Unicode content");
    ASSERT_NOT_NULL(output, "Output should be produced");
    
    destroy_mock_buffer(buffer);
    lle_render_pipeline_cleanup(pipeline);
}

/* ========================================================================== */
/*                         TEST RUNNER                                        */
/* ========================================================================== */

int main(void) {
    printf("=================================================================\n");
    printf("  LLE Render Pipeline Unit Tests\n");
    printf("=================================================================\n\n");
    
    /* Initialization tests */
    run_test_pipeline_init_success();
    run_test_pipeline_init_null_output();
    run_test_pipeline_init_null_pool();
    run_test_pipeline_reinit_after_cleanup();
    
    /* Cleanup tests */
    run_test_pipeline_cleanup_null();
    run_test_pipeline_cleanup_success();
    
    /* Execution tests */
    run_test_pipeline_execute_null_pipeline();
    run_test_pipeline_execute_null_context();
    run_test_pipeline_execute_null_output();
    run_test_pipeline_execute_empty_buffer();
    run_test_pipeline_execute_simple_content();
    run_test_pipeline_execute_multiline_content();
    
    /* Behavior tests */
    run_test_pipeline_multiple_executions();
    run_test_pipeline_different_content_sizes();
    
    /* Special content tests */
    run_test_pipeline_handles_special_characters();
    run_test_pipeline_handles_unicode();
    
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

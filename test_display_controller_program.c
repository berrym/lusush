/*
 * Lusush Shell - Layered Display Architecture
 * Display Controller Test Program - Week 7 Comprehensive Validation
 * 
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ============================================================================
 * 
 * DISPLAY CONTROLLER COMPREHENSIVE TEST PROGRAM
 * 
 * This program provides comprehensive testing of the Week 7 display controller
 * implementation, including all core functionality, performance monitoring,
 * caching, optimization, and integration preparation features.
 * 
 * Test Coverage:
 * - Core API functionality (create, init, display, cleanup, destroy)
 * - Performance monitoring and metrics
 * - Caching system with validation
 * - Configuration management
 * - Error handling and edge cases
 * - Memory safety and resource management
 * - Integration mode preparation
 * - Diagnostic and utility functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

// Mock dependencies for standalone testing
typedef struct {
    bool initialized;
    char name[32];
} layer_event_system_t;

typedef struct {
    bool initialized;
    char prompt_text[256];
} prompt_layer_t;

typedef struct {
    bool initialized;
    char command_text[256];
} command_layer_t;

typedef struct {
    bool initialized;
    char capabilities[64];
} terminal_control_context_t;

typedef enum {
    COMPOSITION_ENGINE_SUCCESS = 0,
    COMPOSITION_ENGINE_ERROR_INVALID_PARAM,
    COMPOSITION_ENGINE_ERROR_NULL_POINTER
} composition_engine_error_t;

typedef struct {
    bool initialized;
    prompt_layer_t *prompt_layer;
    command_layer_t *command_layer;
    layer_event_system_t *event_system;
    char last_output[1024];
} composition_engine_t;

// Mock function implementations
composition_engine_t *composition_engine_create(void) {
    composition_engine_t *engine = malloc(sizeof(composition_engine_t));
    if (engine) {
        memset(engine, 0, sizeof(composition_engine_t));
    }
    return engine;
}

composition_engine_error_t composition_engine_init(
    composition_engine_t *engine,
    prompt_layer_t *prompt_layer,
    command_layer_t *command_layer,
    layer_event_system_t *event_system) {
    
    if (!engine) return COMPOSITION_ENGINE_ERROR_NULL_POINTER;
    
    engine->initialized = true;
    engine->prompt_layer = prompt_layer;
    engine->command_layer = command_layer;
    engine->event_system = event_system;
    
    return COMPOSITION_ENGINE_SUCCESS;
}

composition_engine_error_t composition_engine_compose(composition_engine_t *engine) {
    if (!engine || !engine->initialized) return COMPOSITION_ENGINE_ERROR_INVALID_PARAM;
    
    // Mock composition result
    strcpy(engine->last_output, "Mock composed output: prompt + command");
    return COMPOSITION_ENGINE_SUCCESS;
}

composition_engine_error_t composition_engine_get_output(
    const composition_engine_t *engine,
    char *output,
    size_t output_size) {
    
    if (!engine || !output) return COMPOSITION_ENGINE_ERROR_NULL_POINTER;
    if (output_size < strlen(engine->last_output) + 1) return COMPOSITION_ENGINE_ERROR_INVALID_PARAM;
    
    strcpy(output, engine->last_output);
    return COMPOSITION_ENGINE_SUCCESS;
}

composition_engine_error_t composition_engine_cleanup(composition_engine_t *engine) {
    if (!engine) return COMPOSITION_ENGINE_ERROR_NULL_POINTER;
    engine->initialized = false;
    return COMPOSITION_ENGINE_SUCCESS;
}

void composition_engine_destroy(composition_engine_t *engine) {
    if (engine) free(engine);
}

bool composition_engine_is_initialized(const composition_engine_t *engine) {
    return engine && engine->initialized;
}

const char *composition_engine_error_string(composition_engine_error_t error) {
    switch (error) {
        case COMPOSITION_ENGINE_SUCCESS: return "Success";
        case COMPOSITION_ENGINE_ERROR_INVALID_PARAM: return "Invalid parameter";
        case COMPOSITION_ENGINE_ERROR_NULL_POINTER: return "Null pointer";
        default: return "Unknown error";
    }
}

terminal_control_context_t *terminal_control_create(void) {
    terminal_control_context_t *ctx = malloc(sizeof(terminal_control_context_t));
    if (ctx) {
        memset(ctx, 0, sizeof(terminal_control_context_t));
        ctx->initialized = true;
        strcpy(ctx->capabilities, "mock_terminal");
    }
    return ctx;
}

composition_engine_error_t terminal_control_cleanup(terminal_control_context_t *ctx) {
    if (!ctx) return COMPOSITION_ENGINE_ERROR_NULL_POINTER;
    ctx->initialized = false;
    return COMPOSITION_ENGINE_SUCCESS;
}

void terminal_control_destroy(terminal_control_context_t *ctx) {
    if (ctx) free(ctx);
}

prompt_layer_t *prompt_layer_create(void) {
    prompt_layer_t *layer = malloc(sizeof(prompt_layer_t));
    if (layer) {
        memset(layer, 0, sizeof(prompt_layer_t));
        layer->initialized = true;
        strcpy(layer->prompt_text, "$ ");
    }
    return layer;
}

void prompt_layer_destroy(prompt_layer_t *layer) {
    if (layer) free(layer);
}

command_layer_t *command_layer_create(void) {
    command_layer_t *layer = malloc(sizeof(command_layer_t));
    if (layer) {
        memset(layer, 0, sizeof(command_layer_t));
        layer->initialized = true;
        strcpy(layer->command_text, "echo test");
    }
    return layer;
}

void command_layer_destroy(command_layer_t *layer) {
    if (layer) free(layer);
}

// Include the display controller header
#include "display/display_controller.h"

// ============================================================================
// TEST FRAMEWORK
// ============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
        printf("[PASS] %s\n", message); \
        tests_passed++; \
    } else { \
        printf("[FAIL] %s\n", message); \
        tests_failed++; \
    } \
} while(0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

void test_controller_creation_and_destruction(void) {
    TEST_SECTION("Controller Creation and Destruction");
    
    // Test basic creation
    display_controller_t *controller = display_controller_create();
    TEST_ASSERT(controller != NULL, "Controller creation");
    
    // Test destruction with valid controller
    display_controller_destroy(controller);
    TEST_ASSERT(true, "Controller destruction");
    
    // Test destruction with NULL controller (should not crash)
    display_controller_destroy(NULL);
    TEST_ASSERT(true, "Controller destruction with NULL");
}

void test_controller_initialization(void) {
    TEST_SECTION("Controller Initialization");
    
    display_controller_t *controller = display_controller_create();
    TEST_ASSERT(controller != NULL, "Controller creation for init test");
    
    // Test initialization with default config
    layer_event_system_t event_system = {0};
    display_controller_error_t result = display_controller_init(controller, NULL, &event_system);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Controller initialization with default config");
    
    // Test is_initialized function
    bool initialized = display_controller_is_initialized(controller);
    TEST_ASSERT(initialized == true, "Controller is_initialized check");
    
    // Test initialization with NULL controller
    result = display_controller_init(NULL, NULL, &event_system);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NULL_POINTER, "Controller init with NULL controller");
    
    display_controller_destroy(controller);
}

void test_controller_configuration(void) {
    TEST_SECTION("Controller Configuration");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Test default configuration creation
    display_controller_config_t default_config;
    display_controller_error_t result = display_controller_create_default_config(&default_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Default config creation");
    
    // Test getting current configuration
    display_controller_config_t current_config;
    result = display_controller_get_config(controller, &current_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get current configuration");
    
    // Test setting new configuration
    display_controller_config_t new_config = default_config;
    new_config.optimization_level = DISPLAY_OPTIMIZATION_AGGRESSIVE;
    new_config.cache_ttl_ms = 10000;
    
    result = display_controller_set_config(controller, &new_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Set new configuration");
    
    // Verify configuration was applied
    result = display_controller_get_config(controller, &current_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS && 
                current_config.optimization_level == DISPLAY_OPTIMIZATION_AGGRESSIVE,
                "Configuration was applied correctly");
    
    display_controller_destroy(controller);
}

void test_controller_display_operations(void) {
    TEST_SECTION("Controller Display Operations");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Test basic display operation
    char output[1024];
    display_controller_error_t result = display_controller_display(
        controller, "$ ", "echo test", output, sizeof(output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Basic display operation");
    TEST_ASSERT(strlen(output) > 0, "Display operation produces output");
    
    // Test display with NULL prompt
    result = display_controller_display(controller, NULL, "echo test", output, sizeof(output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Display with NULL prompt");
    
    // Test display with NULL command
    result = display_controller_display(controller, "$ ", NULL, output, sizeof(output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Display with NULL command");
    
    // Test display with small buffer
    char small_output[10];
    result = display_controller_display(controller, "$ ", "echo test", small_output, sizeof(small_output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL, "Display with small buffer");
    
    // Test display with zero buffer size
    result = display_controller_display(controller, "$ ", "echo test", output, 0);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_BUFFER_TOO_SMALL, "Display with zero buffer size");
    
    display_controller_destroy(controller);
}

void test_controller_update_operations(void) {
    TEST_SECTION("Controller Update Operations");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Test basic update operation
    char output[1024];
    display_state_diff_t diff_info;
    display_controller_error_t result = display_controller_update(
        controller, "$ ", "echo new", output, sizeof(output), &diff_info);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Basic update operation");
    
    // Test update without diff info
    result = display_controller_update(controller, "$ ", "echo newer", output, sizeof(output), NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Update without diff info");
    
    display_controller_destroy(controller);
}

void test_controller_refresh_operations(void) {
    TEST_SECTION("Controller Refresh Operations");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Perform initial display to populate cache
    char output[1024];
    display_controller_display(controller, "$ ", "echo test", output, sizeof(output));
    
    // Test refresh operation
    display_controller_error_t result = display_controller_refresh(controller, output, sizeof(output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Basic refresh operation");
    
    display_controller_destroy(controller);
}

void test_controller_performance_monitoring(void) {
    TEST_SECTION("Controller Performance Monitoring");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Perform some operations to generate performance data
    char output[1024];
    for (int i = 0; i < 5; i++) {
        display_controller_display(controller, "$ ", "echo test", output, sizeof(output));
    }
    
    // Test getting performance metrics
    display_controller_performance_t performance;
    display_controller_error_t result = display_controller_get_performance(controller, &performance);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get performance metrics");
    TEST_ASSERT(performance.total_display_operations >= 5, "Performance metrics track operations");
    
    // Test performance reset
    result = display_controller_reset_performance_metrics(controller);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Reset performance metrics");
    
    result = display_controller_get_performance(controller, &performance);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS && 
                performance.total_display_operations == 0,
                "Performance metrics were reset");
    
    display_controller_destroy(controller);
}

void test_controller_caching_system(void) {
    TEST_SECTION("Controller Caching System");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    
    // Initialize with caching enabled
    display_controller_config_t config;
    display_controller_create_default_config(&config);
    config.enable_caching = true;
    config.max_cache_entries = 10;
    
    display_controller_init(controller, &config, &event_system);
    
    // Perform operations to populate cache
    char output[1024];
    display_controller_display(controller, "$ ", "echo test1", output, sizeof(output));
    display_controller_display(controller, "$ ", "echo test2", output, sizeof(output));
    display_controller_display(controller, "$ ", "echo test1", output, sizeof(output)); // Should hit cache
    
    // Test cache validation
    size_t valid_entries, expired_entries;
    bool corruption_detected;
    display_controller_error_t result = display_controller_validate_cache(
        controller, &valid_entries, &expired_entries, &corruption_detected);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Cache validation");
    TEST_ASSERT(valid_entries > 0, "Cache has valid entries");
    TEST_ASSERT(!corruption_detected, "No cache corruption detected");
    
    // Test cache optimization
    result = display_controller_optimize_cache(controller);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Cache optimization");
    
    // Test cache clearing
    result = display_controller_clear_cache(controller);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Cache clearing");
    
    display_controller_destroy(controller);
}

void test_controller_optimization_settings(void) {
    TEST_SECTION("Controller Optimization Settings");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Test setting optimization level
    display_controller_error_t result = display_controller_set_optimization_level(
        controller, DISPLAY_OPTIMIZATION_AGGRESSIVE);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Set optimization level");
    
    // Test enabling adaptive optimization
    result = display_controller_set_adaptive_optimization(controller, true);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Enable adaptive optimization");
    
    // Test disabling adaptive optimization
    result = display_controller_set_adaptive_optimization(controller, false);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Disable adaptive optimization");
    
    display_controller_destroy(controller);
}

void test_controller_integration_mode(void) {
    TEST_SECTION("Controller Integration Mode");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Test enabling integration mode
    display_controller_error_t result = display_controller_set_integration_mode(controller, true);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Enable integration mode");
    
    // Test shell integration preparation
    result = display_controller_prepare_shell_integration(controller, NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Prepare shell integration");
    
    // Test getting integration interface
    char interface_buffer[1024];
    result = display_controller_get_integration_interface(
        controller, interface_buffer, sizeof(interface_buffer));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get integration interface");
    
    display_controller_destroy(controller);
}

void test_controller_diagnostic_functions(void) {
    TEST_SECTION("Controller Diagnostic Functions");
    
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    display_controller_init(controller, NULL, &event_system);
    
    // Test version information
    char version_buffer[64];
    display_controller_error_t result = display_controller_get_version(
        controller, version_buffer, sizeof(version_buffer));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get version information");
    TEST_ASSERT(strlen(version_buffer) > 0, "Version string is not empty");
    
    // Test diagnostic report generation
    char report_buffer[2048];
    result = display_controller_generate_diagnostic_report(
        controller, report_buffer, sizeof(report_buffer));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Generate diagnostic report");
    TEST_ASSERT(strlen(report_buffer) > 100, "Diagnostic report has content");
    
    // Test error string function
    const char *error_str = display_controller_error_string(DISPLAY_CONTROLLER_SUCCESS);
    TEST_ASSERT(error_str != NULL && strlen(error_str) > 0, "Error string function");
    
    display_controller_destroy(controller);
}

void test_controller_error_handling(void) {
    TEST_SECTION("Controller Error Handling");
    
    // Test operations with NULL controller
    char output[1024];
    display_controller_error_t result;
    
    result = display_controller_display(NULL, "$ ", "echo test", output, sizeof(output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_INVALID_PARAM, "Display with NULL controller");
    
    result = display_controller_get_performance(NULL, NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_INVALID_PARAM, "Get performance with NULL controller");
    
    result = display_controller_clear_cache(NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NULL_POINTER, "Clear cache with NULL controller");
    
    // Test operations with uninitialized controller
    display_controller_t *controller = display_controller_create();
    
    result = display_controller_display(controller, "$ ", "echo test", output, sizeof(output));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED, "Display with uninitialized controller");
    
    result = display_controller_get_performance(controller, NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED, "Get performance with uninitialized controller");
    
    display_controller_destroy(controller);
}

void test_controller_memory_safety(void) {
    TEST_SECTION("Controller Memory Safety");
    
    // Test multiple create/destroy cycles
    for (int i = 0; i < 10; i++) {
        display_controller_t *controller = display_controller_create();
        TEST_ASSERT(controller != NULL, "Multiple creation cycles");
        
        if (controller) {
            layer_event_system_t event_system = {0};
            display_controller_init(controller, NULL, &event_system);
            
            char output[1024];
            display_controller_display(controller, "$ ", "echo test", output, sizeof(output));
            
            display_controller_destroy(controller);
        }
    }
    
    // Test cleanup and reinitialization
    display_controller_t *controller = display_controller_create();
    layer_event_system_t event_system = {0};
    
    display_controller_init(controller, NULL, &event_system);
    display_controller_cleanup(controller);
    
    display_controller_error_t result = display_controller_init(controller, NULL, &event_system);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Cleanup and reinitialize");
    
    display_controller_destroy(controller);
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(void) {
    printf("Lusush Display Controller Comprehensive Test Program\n");
    printf("Week 7 - Display Controller Implementation Validation\n");
    printf("=====================================================\n");
    
    // Run all test suites
    test_controller_creation_and_destruction();
    test_controller_initialization();
    test_controller_configuration();
    test_controller_display_operations();
    test_controller_update_operations();
    test_controller_refresh_operations();
    test_controller_performance_monitoring();
    test_controller_caching_system();
    test_controller_optimization_settings();
    test_controller_integration_mode();
    test_controller_diagnostic_functions();
    test_controller_error_handling();
    test_controller_memory_safety();
    
    // Print final summary
    printf("\n" "=" * 50 "\n");
    printf("TEST SUMMARY\n");
    printf("=" * 50 "\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed! Display controller implementation is working correctly.\n");
        printf("Week 7 Display Controller is ready for Week 8 integration.\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed. Please review the failures above.\n");
        printf("Display controller implementation needs attention.\n");
        return 1;
    }
}
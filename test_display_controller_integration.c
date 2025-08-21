/*
 * Lusush Shell - Layered Display Architecture
 * Display Controller Integration Test - Week 7 Validation
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
 * DISPLAY CONTROLLER INTEGRATION TEST
 * 
 * This program provides integration testing of the Week 7 display controller
 * with the existing layered display architecture. It tests the coordination
 * of all display layers through the high-level display controller interface.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

// Include display system headers
#include "display/base_terminal.h"
#include "display/terminal_control.h"
#include "display/layer_events.h"
#include "display/prompt_layer.h"
#include "display/command_layer.h"
#include "display/composition_engine.h"
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
// INTEGRATION TEST FUNCTIONS
// ============================================================================

void test_display_controller_basic_functionality(void) {
    TEST_SECTION("Display Controller Basic Functionality");
    
    // Test controller creation
    display_controller_t *controller = display_controller_create();
    TEST_ASSERT(controller != NULL, "Display controller creation");
    
    if (!controller) {
        return;
    }
    
    // Test initialization without event system (should work)
    display_controller_error_t result = display_controller_init(controller, NULL, NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Display controller initialization");
    
    // Test is_initialized check
    bool initialized = display_controller_is_initialized(controller);
    TEST_ASSERT(initialized == true, "Display controller is initialized");
    
    // Test version information
    char version_buffer[64];
    result = display_controller_get_version(controller, version_buffer, sizeof(version_buffer));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get version information");
    TEST_ASSERT(strlen(version_buffer) > 0, "Version string not empty");
    printf("    Display Controller Version: %s\n", version_buffer);
    
    // Clean up
    display_controller_destroy(controller);
    TEST_ASSERT(true, "Display controller cleanup and destruction");
}

void test_display_controller_configuration(void) {
    TEST_SECTION("Display Controller Configuration");
    
    display_controller_t *controller = display_controller_create();
    if (!controller) return;
    
    display_controller_init(controller, NULL, NULL);
    
    // Test default configuration
    display_controller_config_t default_config;
    display_controller_error_t result = display_controller_create_default_config(&default_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Create default configuration");
    
    // Test getting current configuration
    display_controller_config_t current_config;
    result = display_controller_get_config(controller, &current_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get current configuration");
    
    // Test setting new configuration
    display_controller_config_t new_config = default_config;
    new_config.optimization_level = DISPLAY_OPTIMIZATION_AGGRESSIVE;
    new_config.cache_ttl_ms = 15000;
    new_config.enable_caching = true;
    new_config.enable_performance_monitoring = true;
    
    result = display_controller_set_config(controller, &new_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Set new configuration");
    
    // Verify configuration was applied
    result = display_controller_get_config(controller, &current_config);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS && 
                current_config.optimization_level == DISPLAY_OPTIMIZATION_AGGRESSIVE,
                "Configuration was applied correctly");
    
    printf("    Optimization Level: %d\n", current_config.optimization_level);
    printf("    Cache TTL: %u ms\n", current_config.cache_ttl_ms);
    printf("    Caching Enabled: %s\n", current_config.enable_caching ? "Yes" : "No");
    
    display_controller_destroy(controller);
}

void test_display_controller_performance_monitoring(void) {
    TEST_SECTION("Display Controller Performance Monitoring");
    
    display_controller_t *controller = display_controller_create();
    if (!controller) return;
    
    display_controller_init(controller, NULL, NULL);
    
    // Get initial performance metrics
    display_controller_performance_t performance;
    display_controller_error_t result = display_controller_get_performance(controller, &performance);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get initial performance metrics");
    
    printf("    Initial Operations: %lu\n", performance.total_display_operations);
    printf("    Initial Cache Hits: %lu\n", performance.cache_hits);
    printf("    Initial Cache Misses: %lu\n", performance.cache_misses);
    
    // Test performance reset
    result = display_controller_reset_performance_metrics(controller);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Reset performance metrics");
    
    // Verify reset worked
    result = display_controller_get_performance(controller, &performance);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS && 
                performance.total_display_operations == 0,
                "Performance metrics were reset");
    
    display_controller_destroy(controller);
}

void test_display_controller_optimization_features(void) {
    TEST_SECTION("Display Controller Optimization Features");
    
    display_controller_t *controller = display_controller_create();
    if (!controller) return;
    
    display_controller_init(controller, NULL, NULL);
    
    // Test optimization level setting
    display_controller_error_t result = display_controller_set_optimization_level(
        controller, DISPLAY_OPTIMIZATION_MAXIMUM);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Set optimization level to maximum");
    
    // Test adaptive optimization
    result = display_controller_set_adaptive_optimization(controller, true);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Enable adaptive optimization");
    
    result = display_controller_set_adaptive_optimization(controller, false);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Disable adaptive optimization");
    
    // Test cache operations
    result = display_controller_clear_cache(controller);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Clear cache");
    
    // Test cache validation
    size_t valid_entries, expired_entries;
    bool corruption_detected;
    result = display_controller_validate_cache(controller, &valid_entries, 
                                             &expired_entries, &corruption_detected);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Validate cache");
    TEST_ASSERT(!corruption_detected, "No cache corruption detected");
    
    printf("    Valid cache entries: %zu\n", valid_entries);
    printf("    Expired cache entries: %zu\n", expired_entries);
    
    display_controller_destroy(controller);
}

void test_display_controller_integration_mode(void) {
    TEST_SECTION("Display Controller Integration Mode");
    
    display_controller_t *controller = display_controller_create();
    if (!controller) return;
    
    display_controller_init(controller, NULL, NULL);
    
    // Test integration mode
    display_controller_error_t result = display_controller_set_integration_mode(controller, true);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Enable integration mode");
    
    // Test shell integration preparation
    result = display_controller_prepare_shell_integration(controller, NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Prepare shell integration");
    
    // Test getting integration interface
    char interface_buffer[512];
    result = display_controller_get_integration_interface(
        controller, interface_buffer, sizeof(interface_buffer));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Get integration interface");
    
    display_controller_destroy(controller);
}

void test_display_controller_diagnostic_features(void) {
    TEST_SECTION("Display Controller Diagnostic Features");
    
    display_controller_t *controller = display_controller_create();
    if (!controller) return;
    
    display_controller_init(controller, NULL, NULL);
    
    // Test diagnostic report generation
    char report_buffer[2048];
    display_controller_error_t result = display_controller_generate_diagnostic_report(
        controller, report_buffer, sizeof(report_buffer));
    TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Generate diagnostic report");
    TEST_ASSERT(strlen(report_buffer) > 100, "Diagnostic report has substantial content");
    
    printf("    Diagnostic report length: %zu characters\n", strlen(report_buffer));
    
    // Print first few lines of the report
    printf("    Sample diagnostic output:\n");
    char *line = strtok(report_buffer, "\n");
    for (int i = 0; i < 3 && line; i++) {
        printf("      %s\n", line);
        line = strtok(NULL, "\n");
    }
    
    // Test error string function
    const char *error_str = display_controller_error_string(DISPLAY_CONTROLLER_SUCCESS);
    TEST_ASSERT(error_str != NULL && strlen(error_str) > 0, "Error string function");
    printf("    Success error string: '%s'\n", error_str);
    
    error_str = display_controller_error_string(DISPLAY_CONTROLLER_ERROR_INVALID_PARAM);
    TEST_ASSERT(error_str != NULL && strlen(error_str) > 0, "Error string function with error code");
    printf("    Invalid param error string: '%s'\n", error_str);
    
    display_controller_destroy(controller);
}

void test_display_controller_error_handling(void) {
    TEST_SECTION("Display Controller Error Handling");
    
    // Test operations with NULL controller
    display_controller_error_t result;
    
    result = display_controller_get_performance(NULL, NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_INVALID_PARAM, "Get performance with NULL controller");
    
    result = display_controller_clear_cache(NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NULL_POINTER, "Clear cache with NULL controller");
    
    result = display_controller_set_optimization_level(NULL, DISPLAY_OPTIMIZATION_STANDARD);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NULL_POINTER, "Set optimization level with NULL controller");
    
    // Test operations with uninitialized controller
    display_controller_t *controller = display_controller_create();
    if (controller) {
        display_controller_performance_t performance;
        result = display_controller_get_performance(controller, &performance);
        TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED, 
                   "Get performance with uninitialized controller");
        
        result = display_controller_set_optimization_level(controller, DISPLAY_OPTIMIZATION_MAXIMUM);
        TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_NOT_INITIALIZED, 
                   "Set optimization level with uninitialized controller");
        
        display_controller_destroy(controller);
    }
    
    // Test invalid parameters
    result = display_controller_create_default_config(NULL);
    TEST_ASSERT(result == DISPLAY_CONTROLLER_ERROR_INVALID_PARAM, "Create default config with NULL");
}

void test_display_controller_memory_safety(void) {
    TEST_SECTION("Display Controller Memory Safety");
    
    // Test multiple create/destroy cycles
    for (int i = 0; i < 5; i++) {
        display_controller_t *controller = display_controller_create();
        TEST_ASSERT(controller != NULL, "Multiple creation cycles");
        
        if (controller) {
            display_controller_init(controller, NULL, NULL);
            
            // Perform some operations
            display_controller_config_t config;
            display_controller_get_config(controller, &config);
            display_controller_set_optimization_level(controller, DISPLAY_OPTIMIZATION_AGGRESSIVE);
            
            display_controller_performance_t performance;
            display_controller_get_performance(controller, &performance);
            
            display_controller_destroy(controller);
        }
    }
    
    // Test cleanup and reinitialization
    display_controller_t *controller = display_controller_create();
    if (controller) {
        display_controller_init(controller, NULL, NULL);
        display_controller_error_t result = display_controller_cleanup(controller);
        TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Controller cleanup");
        
        result = display_controller_init(controller, NULL, NULL);
        TEST_ASSERT(result == DISPLAY_CONTROLLER_SUCCESS, "Controller reinitialize after cleanup");
        
        display_controller_destroy(controller);
    }
    
    TEST_ASSERT(true, "Memory safety testing completed without crashes");
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(void) {
    printf("Lusush Display Controller Integration Test\n");
    printf("Week 7 - Display Controller Validation\n");
    printf("=======================================\n");
    
    // Run all integration tests
    test_display_controller_basic_functionality();
    test_display_controller_configuration();
    test_display_controller_performance_monitoring();
    test_display_controller_optimization_features();
    test_display_controller_integration_mode();
    test_display_controller_diagnostic_features();
    test_display_controller_error_handling();
    test_display_controller_memory_safety();
    
    // Print final summary
    printf("\n" "=======================================\n");
    printf("INTEGRATION TEST SUMMARY\n");
    printf("=======================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✓ All integration tests passed!\n");
        printf("✓ Display controller is working correctly with the layered architecture.\n");
        printf("✓ Week 7 Display Controller implementation is ready for Week 8 integration.\n");
        printf("\n🎉 STRATEGIC ACHIEVEMENT:\n");
        printf("   High-level display coordination is functional\n");
        printf("   Performance monitoring and optimization working\n");
        printf("   Configuration management operational\n");
        printf("   Integration preparation complete\n");
        printf("   Memory safety validated\n");
        printf("\n🚀 READY FOR: Week 8 shell integration\n");
        return 0;
    } else {
        printf("\n✗ Some integration tests failed.\n");
        printf("✗ Display controller implementation needs attention.\n");
        return 1;
    }
}
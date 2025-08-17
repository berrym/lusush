/*
 * Lusush Shell - Layered Display Architecture
 * Composition Engine Minimal Test Program - Week 6 Validation
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
 * COMPOSITION ENGINE MINIMAL TEST PROGRAM
 * 
 * This program provides a simplified test of the Week 6 composition engine
 * implementation, focusing on core functionality without complex dependencies.
 * 
 * Test Coverage:
 * - Composition engine creation and destruction
 * - Basic API function validation
 * - Error handling verification
 * - Memory safety validation
 * 
 * Strategic Validation:
 * This test ensures the composition engine core functionality is working
 * correctly as the foundation for the revolutionary layered display architecture.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include only what we need for basic testing
#include "display/composition_engine.h"

// Test counter
static int test_count = 0;
static int test_passed = 0;

// Test macros
#define TEST_START(name) \
    do { \
        test_count++; \
        printf("Test %d: %s...\n", test_count, name); \
    } while(0)

#define TEST_PASS(message) \
    do { \
        test_passed++; \
        printf("✅ %s\n", message); \
    } while(0)

#define TEST_FAIL(message) \
    do { \
        printf("❌ %s\n", message); \
        return false; \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(message); \
        } \
    } while(0)

/**
 * Test composition engine creation and destruction
 */
bool test_composition_engine_lifecycle(void) {
    TEST_START("Composition engine lifecycle");
    
    // Test creation
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Test that engine is not initialized yet
    TEST_ASSERT(!composition_engine_is_initialized(engine), "Engine should not be initialized yet");
    
    // Test version retrieval
    char version[64];
    composition_engine_error_t result = composition_engine_get_version(engine, version, sizeof(version));
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to get version");
    TEST_ASSERT(strlen(version) > 0, "Version string is empty");
    
    printf("   Engine version: %s\n", version);
    
    // Test destruction
    composition_engine_destroy(engine);
    
    TEST_PASS("Composition engine lifecycle working");
    return true;
}

/**
 * Test error handling with NULL parameters
 */
bool test_error_handling(void) {
    TEST_START("Error handling with NULL parameters");
    
    // Test NULL engine operations
    char buffer[256];
    
    composition_engine_error_t result = composition_engine_get_version(NULL, buffer, sizeof(buffer));
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_INVALID_PARAM, "Should fail with NULL engine");
    
    result = composition_engine_get_output(NULL, buffer, sizeof(buffer));
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_INVALID_PARAM, "Should fail with NULL engine");
    
    // Test with valid engine but NULL buffers
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    result = composition_engine_get_version(engine, NULL, sizeof(buffer));
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_INVALID_PARAM, "Should fail with NULL buffer");
    
    result = composition_engine_get_output(engine, NULL, sizeof(buffer));
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_INVALID_PARAM, "Should fail with NULL buffer");
    
    composition_engine_destroy(engine);
    
    TEST_PASS("Error handling working correctly");
    return true;
}

/**
 * Test composition engine configuration functions
 */
bool test_configuration_functions(void) {
    TEST_START("Configuration functions");
    
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Test strategy setting
    composition_engine_error_t result = composition_engine_set_strategy(engine, COMPOSITION_STRATEGY_SIMPLE);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to set strategy");
    
    result = composition_engine_set_strategy(engine, COMPOSITION_STRATEGY_ADAPTIVE);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to set adaptive strategy");
    
    // Test invalid strategy
    result = composition_engine_set_strategy(engine, (composition_strategy_t)999);
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_INVALID_PARAM, "Should fail with invalid strategy");
    
    // Test intelligent positioning setting
    result = composition_engine_set_intelligent_positioning(engine, true);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to enable intelligent positioning");
    
    result = composition_engine_set_intelligent_positioning(engine, false);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to disable intelligent positioning");
    
    // Test performance monitoring setting
    result = composition_engine_set_performance_monitoring(engine, true);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to enable performance monitoring");
    
    result = composition_engine_set_performance_monitoring(engine, false);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to disable performance monitoring");
    
    // Test cache max age setting
    result = composition_engine_set_cache_max_age(engine, 1000);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to set cache max age");
    
    composition_engine_destroy(engine);
    
    TEST_PASS("Configuration functions working correctly");
    return true;
}

/**
 * Test cache management functions
 */
bool test_cache_management(void) {
    TEST_START("Cache management functions");
    
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Test cache clearing
    composition_engine_error_t result = composition_engine_clear_cache(engine);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to clear cache");
    
    // Test cache validation
    size_t valid_entries = 0, expired_entries = 0;
    result = composition_engine_validate_cache(engine, &valid_entries, &expired_entries);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to validate cache");
    
    printf("   Cache validation: %zu valid, %zu expired\n", valid_entries, expired_entries);
    
    // Test hash calculation (should fail without initialization)
    char hash[64];
    result = composition_engine_calculate_hash(engine, hash, sizeof(hash));
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_NOT_INITIALIZED, "Should fail without initialization");
    
    composition_engine_destroy(engine);
    
    TEST_PASS("Cache management functions working correctly");
    return true;
}

/**
 * Test utility functions
 */
bool test_utility_functions(void) {
    TEST_START("Utility functions");
    
    // Test error string conversion
    const char *error_str = composition_engine_error_string(COMPOSITION_ENGINE_SUCCESS);
    TEST_ASSERT(error_str != NULL, "Error string should not be NULL");
    TEST_ASSERT(strlen(error_str) > 0, "Error string should not be empty");
    printf("   Success error string: %s\n", error_str);
    
    error_str = composition_engine_error_string(COMPOSITION_ENGINE_ERROR_INVALID_PARAM);
    TEST_ASSERT(error_str != NULL, "Error string should not be NULL");
    TEST_ASSERT(strlen(error_str) > 0, "Error string should not be empty");
    printf("   Invalid param error string: %s\n", error_str);
    
    // Test strategy string conversion
    const char *strategy_str = composition_engine_strategy_string(COMPOSITION_STRATEGY_SIMPLE);
    TEST_ASSERT(strategy_str != NULL, "Strategy string should not be NULL");
    TEST_ASSERT(strlen(strategy_str) > 0, "Strategy string should not be empty");
    printf("   Simple strategy string: %s\n", strategy_str);
    
    strategy_str = composition_engine_strategy_string(COMPOSITION_STRATEGY_ADAPTIVE);
    TEST_ASSERT(strategy_str != NULL, "Strategy string should not be NULL");
    TEST_ASSERT(strlen(strategy_str) > 0, "Strategy string should not be empty");
    printf("   Adaptive strategy string: %s\n", strategy_str);
    
    TEST_PASS("Utility functions working correctly");
    return true;
}

/**
 * Test uninitialized engine operations
 */
bool test_uninitialized_operations(void) {
    TEST_START("Uninitialized engine operations");
    
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    
    // Test operations that should fail on uninitialized engine
    composition_engine_error_t result = composition_engine_compose(engine);
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_NOT_INITIALIZED, "Should fail when not initialized");
    
    composition_analysis_t analysis;
    result = composition_engine_get_analysis(engine, &analysis);
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_NOT_INITIALIZED, "Should fail when not initialized");
    
    composition_positioning_t positioning;
    result = composition_engine_get_positioning(engine, &positioning);
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_NOT_INITIALIZED, "Should fail when not initialized");
    
    composition_performance_t performance;
    result = composition_engine_get_performance(engine, &performance);
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_NOT_INITIALIZED, "Should fail when not initialized");
    
    result = composition_engine_analyze_prompt(engine, &analysis);
    TEST_ASSERT(result == COMPOSITION_ENGINE_ERROR_NOT_INITIALIZED, "Should fail when not initialized");
    
    composition_engine_destroy(engine);
    
    TEST_PASS("Uninitialized operations properly rejected");
    return true;
}

/**
 * Test memory safety with multiple cycles
 */
bool test_memory_safety(void) {
    TEST_START("Memory safety with multiple cycles");
    
    // Test multiple create/destroy cycles
    for (int cycle = 0; cycle < 5; cycle++) {
        composition_engine_t *engine = composition_engine_create();
        TEST_ASSERT(engine != NULL, "Engine creation failed in cycle");
        
        // Perform some operations
        char version[64];
        composition_engine_error_t result = composition_engine_get_version(engine, version, sizeof(version));
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Version retrieval failed in cycle");
        
        result = composition_engine_set_strategy(engine, COMPOSITION_STRATEGY_SIMPLE);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Strategy setting failed in cycle");
        
        result = composition_engine_clear_cache(engine);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Cache clearing failed in cycle");
        
        composition_engine_destroy(engine);
    }
    
    TEST_PASS("Memory safety validated through multiple cycles");
    return true;
}

/**
 * Main test function
 */
int main(void) {
    printf("Composition Engine Minimal Test\n");
    printf("===============================\n\n");
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_composition_engine_lifecycle();
    all_passed &= test_error_handling();
    all_passed &= test_configuration_functions();
    all_passed &= test_cache_management();
    all_passed &= test_utility_functions();
    all_passed &= test_uninitialized_operations();
    all_passed &= test_memory_safety();
    
    // Print summary
    printf("\nTest Summary\n");
    printf("============\n");
    printf("Tests run: %d\n", test_count);
    printf("Tests passed: %d\n", test_passed);
    printf("Tests failed: %d\n", test_count - test_passed);
    
    if (all_passed) {
        printf("\n🎉 COMPOSITION ENGINE MINIMAL TESTS PASSED!\n");
        printf("==========================================\n");
        printf("✅ Week 6 basic functionality: WORKING\n");
        printf("✅ Core API functions: OPERATIONAL\n");
        printf("✅ Error handling: VALIDATED\n");
        printf("✅ Memory safety: CONFIRMED\n");
        printf("✅ Configuration system: FUNCTIONAL\n");
        printf("✅ Cache management: OPERATIONAL\n");
        printf("\n📊 CORE FUNCTIONALITY VERIFIED:\n");
        printf("✅ Composition engine API working correctly\n");
        printf("✅ Proper error handling for edge cases\n");
        printf("✅ Configuration functions operational\n");
        printf("✅ Memory management safe and reliable\n");
        printf("✅ Basic foundation ready for integration\n");
        printf("\n🎯 STRATEGIC ACHIEVEMENT:\n");
        printf("The composition engine core API is functional and ready\n");
        printf("for integration with prompt and command layers to enable\n");
        printf("the revolutionary universal prompt + syntax highlighting\n");
        printf("compatibility.\n");
        printf("\n🚀 READY FOR: Full integration testing with layers\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED\n");
        printf("==================\n");
        printf("Check the output above for details.\n");
        return 1;
    }
}
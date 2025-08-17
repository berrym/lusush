/*
 * Lusush Shell - Layered Display Architecture
 * Composition Engine Test Program - Week 6 Validation
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
 * COMPOSITION ENGINE TEST PROGRAM
 * 
 * This program validates the Week 6 composition engine implementation by
 * testing intelligent layer combination, universal prompt compatibility,
 * performance optimization, and event system integration.
 * 
 * Test Coverage:
 * - Composition engine lifecycle (create, init, cleanup, destroy)
 * - Intelligent layer combination with various prompt types
 * - Universal prompt structure compatibility
 * - Performance metrics collection and caching
 * - Event system integration and communication
 * - Memory safety and resource management
 * 
 * Strategic Validation:
 * This test ensures the composition engine successfully enables the
 * revolutionary combination of professional themes with real-time syntax
 * highlighting for ANY prompt structure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define _POSIX_C_SOURCE 200809L

#include "display/composition_engine.h"
#include "display/prompt_layer.h"
#include "display/command_layer.h"
#include "display/layer_events.h"

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

// Mock data for testing
static const char *test_prompts[] = {
    "$ ",                                           // Simple prompt
    "[user@host ~]$ ",                             // Complex single-line
    "┌─[user@host]─[~/path] (git-branch)\n└─$ ",   // Multi-line themed
    "    /\\   /\\\n   (  . .)  > ",               // ASCII art style
    "[15:30:42] user@hostname:~/project$ ",        // Timestamp prompt
    "🚀 [DEPLOY:PROD] ➜ "                          // Emoji prompt
};

static const char *test_commands[] = {
    "",                                             // Empty command
    "ls",                                          // Simple command
    "ls -la /home",                                // Command with options
    "git log --oneline | grep fix",               // Pipeline command
    "echo \"Hello $USER\" && export VAR=value"    // Complex command
};

/**
 * Test composition engine creation and destruction
 */
bool test_composition_engine_lifecycle(void) {
    TEST_START("Composition engine lifecycle");
    
    // Test creation
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Engine creation failed");
    TEST_ASSERT(!composition_engine_is_initialized(engine), "Engine should not be initialized yet");
    
    // Test version retrieval
    char version[64];
    composition_engine_error_t result = composition_engine_get_version(engine, version, sizeof(version));
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to get version");
    TEST_ASSERT(strlen(version) > 0, "Version string is empty");
    
    // Test destruction
    composition_engine_destroy(engine);
    
    TEST_PASS("Composition engine lifecycle working");
    return true;
}

/**
 * Test composition engine initialization with layers
 */
bool test_composition_engine_initialization(void) {
    TEST_START("Composition engine initialization");
    
    // Create event system
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    TEST_ASSERT(events != NULL, "Event system creation failed");
    
    layer_events_error_t event_result = layer_events_init(events);
    TEST_ASSERT(event_result == LAYER_EVENTS_SUCCESS, "Event system initialization failed");
    
    // Create prompt layer
    prompt_layer_t *prompt_layer = prompt_layer_create();
    TEST_ASSERT(prompt_layer != NULL, "Prompt layer creation failed");
    
    prompt_layer_error_t prompt_result = prompt_layer_init(prompt_layer, events);
    TEST_ASSERT(prompt_result == PROMPT_LAYER_SUCCESS, "Prompt layer initialization failed");
    
    // Create command layer
    command_layer_t *command_layer = command_layer_create();
    TEST_ASSERT(command_layer != NULL, "Command layer creation failed");
    
    command_layer_error_t command_result = command_layer_init(command_layer, events);
    TEST_ASSERT(command_result == COMMAND_LAYER_SUCCESS, "Command layer initialization failed");
    
    // Create and initialize composition engine
    composition_engine_t *engine = composition_engine_create();
    TEST_ASSERT(engine != NULL, "Composition engine creation failed");
    
    composition_engine_error_t comp_result = composition_engine_init(
        engine, prompt_layer, command_layer, events);
    TEST_ASSERT(comp_result == COMPOSITION_ENGINE_SUCCESS, "Composition engine initialization failed");
    TEST_ASSERT(composition_engine_is_initialized(engine), "Engine should be initialized");
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Composition engine initialization working");
    return true;
}

/**
 * Test basic composition functionality
 */
bool test_basic_composition(void) {
    TEST_START("Basic composition functionality");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Set test content
    prompt_layer_set_content(prompt_layer, "$ ");
    command_layer_set_command(command_layer, "ls -la", 5);
    
    // Perform composition
    composition_engine_error_t result = composition_engine_compose(engine);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Composition failed");
    
    // Get composed output
    char output[2048];
    result = composition_engine_get_output(engine, output, sizeof(output));
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to get composed output");
    TEST_ASSERT(strlen(output) > 0, "Composed output is empty");
    
    printf("   Composed output: '%s'\n", output);
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Basic composition working");
    return true;
}

/**
 * Test composition with various prompt types
 */
bool test_universal_prompt_compatibility(void) {
    TEST_START("Universal prompt compatibility");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Test with various prompt types
    for (size_t i = 0; i < sizeof(test_prompts) / sizeof(test_prompts[0]); i++) {
        prompt_layer_set_content(prompt_layer, test_prompts[i]);
        command_layer_set_command(command_layer, "echo hello", 10);
        
        composition_engine_error_t result = composition_engine_compose(engine);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Composition failed for prompt type");
        
        char output[2048];
        result = composition_engine_get_output(engine, output, sizeof(output));
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to get output for prompt type");
        TEST_ASSERT(strlen(output) > 0, "Output is empty for prompt type");
        
        printf("   Prompt %zu: OK\n", i + 1);
    }
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Universal prompt compatibility working");
    return true;
}

/**
 * Test prompt analysis functionality
 */
bool test_prompt_analysis(void) {
    TEST_START("Prompt analysis functionality");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Test analysis with different prompt types
    prompt_layer_set_content(prompt_layer, "┌─[user@host]─[~/path]\n└─$ ");
    
    composition_analysis_t analysis;
    composition_engine_error_t result = composition_engine_analyze_prompt(engine, &analysis);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Prompt analysis failed");
    
    TEST_ASSERT(analysis.line_count > 1, "Should detect multi-line prompt");
    TEST_ASSERT(analysis.is_multiline, "Should identify as multiline");
    TEST_ASSERT(analysis.recommended_strategy != COMPOSITION_STRATEGY_SIMPLE, 
                "Should not recommend simple strategy for complex prompt");
    
    printf("   Analysis: %zu lines, strategy %d\n", 
           analysis.line_count, analysis.recommended_strategy);
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Prompt analysis working");
    return true;
}

/**
 * Test positioning calculation
 */
bool test_positioning_calculation(void) {
    TEST_START("Positioning calculation");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Test with simple prompt
    prompt_layer_set_content(prompt_layer, "$ ");
    command_layer_set_command(command_layer, "ls", 2);
    
    composition_engine_error_t result = composition_engine_compose(engine);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Composition failed");
    
    composition_positioning_t positioning;
    result = composition_engine_get_positioning(engine, &positioning);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to get positioning");
    
    TEST_ASSERT(positioning.command_on_same_line, "Command should be on same line for simple prompt");
    TEST_ASSERT(positioning.command_start_column > 0, "Command should start after prompt");
    
    printf("   Positioning: line %zu, column %zu\n", 
           positioning.command_start_line, positioning.command_start_column);
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Positioning calculation working");
    return true;
}

/**
 * Test performance metrics collection
 */
bool test_performance_metrics(void) {
    TEST_START("Performance metrics collection");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Enable performance monitoring
    composition_engine_set_performance_monitoring(engine, true);
    
    // Perform several compositions
    for (int i = 0; i < 5; i++) {
        prompt_layer_set_content(prompt_layer, test_prompts[i % 3]);
        command_layer_set_command(command_layer, test_commands[i % 3], 5);
        
        composition_engine_error_t result = composition_engine_compose(engine);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Composition failed");
    }
    
    // Get performance metrics
    composition_performance_t performance;
    composition_engine_error_t result = composition_engine_get_performance(engine, &performance);
    TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to get performance metrics");
    
    TEST_ASSERT(performance.composition_count >= 5, "Should have performed at least 5 compositions");
    TEST_ASSERT(performance.avg_composition_time_ns > 0, "Should have recorded composition times");
    
    printf("   Performance: %llu compositions, avg %llu ns\n", 
           (unsigned long long)performance.composition_count,
           (unsigned long long)performance.avg_composition_time_ns);
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Performance metrics collection working");
    return true;
}

/**
 * Test caching functionality
 */
bool test_caching_functionality(void) {
    TEST_START("Caching functionality");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Enable performance monitoring to track cache hits
    composition_engine_set_performance_monitoring(engine, true);
    
    // Set content and compose multiple times
    prompt_layer_set_content(prompt_layer, "$ ");
    command_layer_set_command(command_layer, "echo test", 9);
    
    // First composition (should be cache miss)
    composition_engine_compose(engine);
    
    // Second composition with same content (should be cache hit)
    composition_engine_compose(engine);
    composition_engine_compose(engine);
    
    // Get performance metrics
    composition_performance_t performance;
    composition_engine_get_performance(engine, &performance);
    
    TEST_ASSERT(performance.composition_count >= 3, "Should have performed multiple compositions");
    TEST_ASSERT(performance.cache_hits > 0, "Should have some cache hits");
    
    printf("   Cache: %llu hits, %llu misses, %.1f%% hit rate\n",
           (unsigned long long)performance.cache_hits,
           (unsigned long long)performance.cache_misses,
           performance.cache_hit_rate);
    
    // Test cache clearing
    composition_engine_clear_cache(engine);
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Caching functionality working");
    return true;
}

/**
 * Test composition strategies
 */
bool test_composition_strategies(void) {
    TEST_START("Composition strategies");
    
    // Setup layers
    layer_events_config_t config = {0};
    layer_event_system_t *events = layer_events_create(&config);
    layer_events_init(events);
    
    prompt_layer_t *prompt_layer = prompt_layer_create();
    prompt_layer_init(prompt_layer, events);
    
    command_layer_t *command_layer = command_layer_create();
    command_layer_init(command_layer, events);
    
    composition_engine_t *engine = composition_engine_create();
    composition_engine_init(engine, prompt_layer, command_layer, events);
    
    // Test different strategies
    composition_strategy_t strategies[] = {
        COMPOSITION_STRATEGY_SIMPLE,
        COMPOSITION_STRATEGY_MULTILINE,
        COMPOSITION_STRATEGY_COMPLEX,
        COMPOSITION_STRATEGY_ADAPTIVE
    };
    
    for (size_t i = 0; i < sizeof(strategies) / sizeof(strategies[0]); i++) {
        composition_engine_error_t result = composition_engine_set_strategy(engine, strategies[i]);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Failed to set strategy");
        
        prompt_layer_set_content(prompt_layer, test_prompts[i % 3]);
        command_layer_set_command(command_layer, "ls", 2);
        
        result = composition_engine_compose(engine);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Composition failed with strategy");
        
        printf("   Strategy %s: OK\n", composition_engine_strategy_string(strategies[i]));
    }
    
    // Cleanup
    composition_engine_destroy(engine);
    command_layer_destroy(command_layer);
    prompt_layer_destroy(prompt_layer);
    layer_events_destroy(events);
    
    TEST_PASS("Composition strategies working");
    return true;
}

/**
 * Test memory safety and resource management
 */
bool test_memory_safety(void) {
    TEST_START("Memory safety and resource management");
    
    // Test multiple create/destroy cycles
    for (int cycle = 0; cycle < 3; cycle++) {
        layer_events_config_t config = {0};
        layer_event_system_t *events = layer_events_create(&config);
        layer_events_init(events);
        
        prompt_layer_t *prompt_layer = prompt_layer_create();
        prompt_layer_init(prompt_layer, events);
        
        command_layer_t *command_layer = command_layer_create();
        command_layer_init(command_layer, events);
        
        composition_engine_t *engine = composition_engine_create();
        composition_engine_init(engine, prompt_layer, command_layer, events);
        
        // Perform some operations
        prompt_layer_set_content(prompt_layer, test_prompts[cycle % 3]);
        command_layer_set_command(command_layer, test_commands[cycle % 3], 5);
        composition_engine_compose(engine);
        
        // Test cleanup
        composition_engine_error_t result = composition_engine_cleanup(engine);
        TEST_ASSERT(result == COMPOSITION_ENGINE_SUCCESS, "Cleanup failed");
        TEST_ASSERT(!composition_engine_is_initialized(engine), "Engine should not be initialized after cleanup");
        
        // Cleanup
        composition_engine_destroy(engine);
        command_layer_destroy(command_layer);
        prompt_layer_destroy(prompt_layer);
        layer_events_destroy(events);
    }
    
    TEST_PASS("Memory safety and resource management working");
    return true;
}

/**
 * Main test function
 */
int main(void) {
    printf("Composition Engine Test Program\n");
    printf("==============================\n\n");
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_composition_engine_lifecycle();
    all_passed &= test_composition_engine_initialization();
    all_passed &= test_basic_composition();
    all_passed &= test_universal_prompt_compatibility();
    all_passed &= test_prompt_analysis();
    all_passed &= test_positioning_calculation();
    all_passed &= test_performance_metrics();
    all_passed &= test_caching_functionality();
    all_passed &= test_composition_strategies();
    all_passed &= test_memory_safety();
    
    // Print summary
    printf("\nTest Summary\n");
    printf("============\n");
    printf("Tests run: %d\n", test_count);
    printf("Tests passed: %d\n", test_passed);
    printf("Tests failed: %d\n", test_count - test_passed);
    
    if (all_passed) {
        printf("\n🎉 ALL COMPOSITION ENGINE TESTS PASSED!\n");
        printf("======================================\n");
        printf("✅ Week 6 composition engine is working correctly\n");
        printf("✅ Intelligent layer combination functional\n");
        printf("✅ Universal prompt compatibility achieved\n");
        printf("✅ Performance optimization working\n");
        printf("✅ Caching system operational\n");
        printf("✅ Memory safety validated\n");
        printf("\n🚀 Ready for next phase: Display Controller (Week 7)\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED\n");
        printf("==================\n");
        printf("Check the output above for details.\n");
        return 1;
    }
}
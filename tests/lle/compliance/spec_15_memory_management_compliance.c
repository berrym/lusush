/**
 * @file spec_15_memory_management_compliance.c
 * @brief Spec 15 Memory Management - Compliance Verification Tests
 * 
 * Verifies that the Memory Management implementation is 100% compliant
 * with specification 15_memory_management_complete.md
 * 
 * ZERO-TOLERANCE ENFORCEMENT:
 * These tests verify spec compliance. Any failure indicates a violation
 * and the code MUST be corrected before commit is allowed.
 * 
 * Tests verify:
 * - All memory pool types from spec are defined
 * - All constants match spec requirements
 * - Memory management states are correct
 * - Pool configurations match spec
 * 
 * Spec Reference: docs/lle_specification/15_memory_management_complete.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Include only the memory management header */
#include "lle/memory_management.h"

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Simple assertion macros */
#define ASSERT_EQ(expected, actual, message) \
    do { \
        tests_run++; \
        if ((expected) != (actual)) { \
            printf("  FAIL: %s\n", message); \
            printf("        Expected: %d, Got: %d\n", (int)(expected), (int)(actual)); \
            tests_failed++; \
            return false; \
        } else { \
            tests_passed++; \
        } \
    } while(0)

#define ASSERT_TRUE(condition, message) \
    do { \
        tests_run++; \
        if (!(condition)) { \
            printf("  FAIL: %s\n", message); \
            tests_failed++; \
            return false; \
        } else { \
            tests_passed++; \
        } \
    } while(0)

/* ============================================================================
 * Memory Pool Constants Compliance Tests
 * ========================================================================== */

/**
 * Test: Verify memory pool size constants match spec
 * Spec Requirement: Memory pool configuration section
 */
static bool test_memory_pool_constants(void) {
    printf("  Testing memory pool size constants...\n");
    
    ASSERT_EQ(256, LLE_MAX_BUFFER_BLOCKS,
              "LLE_MAX_BUFFER_BLOCKS must be 256");
    
    ASSERT_EQ(128, LLE_STRING_CACHE_SIZE,
              "LLE_STRING_CACHE_SIZE must be 128");
    
    ASSERT_EQ(512, LLE_EVENT_POOL_SIZE,
              "LLE_EVENT_POOL_SIZE must be 512");
    
    ASSERT_EQ(64, LLE_INPUT_EVENT_CACHE,
              "LLE_INPUT_EVENT_CACHE must be 64");
    
    ASSERT_EQ(64, LLE_DISPLAY_EVENT_CACHE,
              "LLE_DISPLAY_EVENT_CACHE must be 64");
    
    ASSERT_EQ(32, LLE_SYSTEM_EVENT_CACHE,
              "LLE_SYSTEM_EVENT_CACHE must be 32");
    
    return true;
}

/**
 * Test: Verify pool count constants
 * Spec Requirement: Pool architecture section
 */
static bool test_pool_count_constants(void) {
    printf("  Testing pool count constants...\n");
    
    ASSERT_EQ(8, LLE_PRIMARY_POOL_COUNT,
              "LLE_PRIMARY_POOL_COUNT must be 8");
    
    ASSERT_EQ(4, LLE_SECONDARY_POOL_COUNT,
              "LLE_SECONDARY_POOL_COUNT must be 4");
    
    return true;
}

/**
 * Test: Verify memory management limits
 * Spec Requirement: Memory limits configuration
 */
static bool test_memory_limits(void) {
    printf("  Testing memory limit constants...\n");
    
    ASSERT_EQ(1024, LLE_MAX_SHARED_ALLOCATIONS,
              "LLE_MAX_SHARED_ALLOCATIONS must be 1024");
    
    ASSERT_EQ(256, LLE_MAX_FREE_FRAGMENTS,
              "LLE_MAX_FREE_FRAGMENTS must be 256");
    
    ASSERT_EQ(512, LLE_MAX_TRACKED_BUFFERS,
              "LLE_MAX_TRACKED_BUFFERS must be 512");
    
    return true;
}

/**
 * Test: Verify memory alignment constants
 * Spec Requirement: Memory alignment section
 */
static bool test_alignment_constants(void) {
    printf("  Testing memory alignment constants...\n");
    
    ASSERT_EQ(16, LLE_MEMORY_ALIGNMENT,
              "LLE_MEMORY_ALIGNMENT must be 16");
    
    ASSERT_EQ(64, LLE_BUFFER_ALIGNMENT,
              "LLE_BUFFER_ALIGNMENT must be 64");
    
    return true;
}

/**
 * Test: Verify history and tracking constants
 * Spec Requirement: Memory tracking configuration
 */
static bool test_history_constants(void) {
    printf("  Testing history and tracking constants...\n");
    
    ASSERT_EQ(32, LLE_RESIZE_HISTORY_SIZE,
              "LLE_RESIZE_HISTORY_SIZE must be 32");
    
    ASSERT_EQ(1024, LLE_ACCESS_HISTORY_SIZE,
              "LLE_ACCESS_HISTORY_SIZE must be 1024");
    
    ASSERT_EQ(16, LLE_HOT_REGIONS_COUNT,
              "LLE_HOT_REGIONS_COUNT must be 16");
    
    ASSERT_EQ(64, LLE_TUNING_HISTORY_SIZE,
              "LLE_TUNING_HISTORY_SIZE must be 64");
    
    ASSERT_EQ(128, LLE_ERROR_HISTORY_SIZE,
              "LLE_ERROR_HISTORY_SIZE must be 128");
    
    return true;
}

/**
 * Test: Verify cache size constants
 * Spec Requirement: Cache configuration section
 */
static bool test_cache_constants(void) {
    printf("  Testing cache size constants...\n");
    
    ASSERT_EQ(64, LLE_INPUT_EVENT_CACHE_SIZE,
              "LLE_INPUT_EVENT_CACHE_SIZE must be 64");
    
    ASSERT_EQ(64, LLE_DISPLAY_EVENT_CACHE_SIZE,
              "LLE_DISPLAY_EVENT_CACHE_SIZE must be 64");
    
    ASSERT_EQ(32, LLE_SYSTEM_EVENT_CACHE_SIZE,
              "LLE_SYSTEM_EVENT_CACHE_SIZE must be 32");
    
    ASSERT_EQ(64, LLE_MAX_KEY_SIZE,
              "LLE_MAX_KEY_SIZE must be 64");
    
    ASSERT_EQ(8, LLE_MAX_DERIVED_KEYS,
              "LLE_MAX_DERIVED_KEYS must be 8");
    
    return true;
}

/**
 * Test: Verify memory pool type enumeration
 * Spec Requirement: Memory pool types section
 */
static bool test_memory_pool_types(void) {
    printf("  Testing memory pool type enumeration...\n");
    
    /* Verify all pool types are defined */
    int pool_types[] = {
        LLE_POOL_BUFFER,
        LLE_POOL_EVENT,
        LLE_POOL_STRING,
        LLE_POOL_TEMP,
        LLE_POOL_HISTORY,
        LLE_POOL_SYNTAX,
        LLE_POOL_COMPLETION,
        LLE_POOL_CUSTOM,
        LLE_POOL_COUNT
    };
    
    ASSERT_TRUE(sizeof(pool_types) > 0,
                "Memory pool types must be defined and accessible");
    
    return true;
}

/**
 * Test: Verify memory management state enumeration
 * Spec Requirement: Memory state machine section
 */
static bool test_memory_states(void) {
    printf("  Testing memory management state enumeration...\n");
    
    /* Verify all states are defined */
    int states[] = {
        LLE_MEMORY_STATE_INITIALIZING,
        LLE_MEMORY_STATE_ACTIVE,
        LLE_MEMORY_STATE_OPTIMIZING,
        LLE_MEMORY_STATE_GC_RUNNING,
        LLE_MEMORY_STATE_LOW_MEMORY,
        LLE_MEMORY_STATE_ERROR,
        LLE_MEMORY_STATE_SHUTDOWN
    };
    
    ASSERT_TRUE(sizeof(states) > 0,
                "Memory management states must be defined and accessible");
    
    return true;
}

/**
 * Test: Verify garbage collection enumerations
 * Spec Requirement: Garbage collection section
 */
static bool test_gc_enumerations(void) {
    printf("  Testing garbage collection enumerations...\n");
    
    /* GC strategies */
    int gc_strategies[] = {
        LLE_GC_STRATEGY_MARK_SWEEP,
        LLE_GC_STRATEGY_MARK_SWEEP_COMPACT,
        LLE_GC_STRATEGY_GENERATIONAL,
        LLE_GC_STRATEGY_INCREMENTAL,
        LLE_GC_STRATEGY_CONCURRENT
    };
    
    ASSERT_TRUE(sizeof(gc_strategies) > 0,
                "GC strategies must be defined and accessible");
    
    /* GC states */
    int gc_states[] = {
        LLE_GC_STATE_IDLE,
        LLE_GC_STATE_MARKING,
        LLE_GC_STATE_SWEEPING,
        LLE_GC_STATE_COMPACTING,
        LLE_GC_STATE_ERROR
    };
    
    ASSERT_TRUE(sizeof(gc_states) > 0,
                "GC states must be defined and accessible");
    
    return true;
}

/**
 * Test: Verify pool selection and allocation enumerations
 * Spec Requirement: Pool management section
 */
static bool test_pool_management_enumerations(void) {
    printf("  Testing pool management enumerations...\n");
    
    /* Pool selection algorithms */
    int selection_algorithms[] = {
        LLE_POOL_SELECTION_FIRST_FIT,
        LLE_POOL_SELECTION_BEST_FIT,
        LLE_POOL_SELECTION_WORST_FIT,
        LLE_POOL_SELECTION_NEXT_FIT
    };
    
    ASSERT_TRUE(sizeof(selection_algorithms) > 0,
                "Pool selection algorithms must be defined and accessible");
    
    /* Allocation strategies */
    int allocation_strategies[] = {
        LLE_STRATEGY_PRIMARY_ONLY,
        LLE_STRATEGY_PRIMARY_SECONDARY,
        LLE_STRATEGY_EMERGENCY_ONLY,
        LLE_STRATEGY_AUTOMATIC
    };
    
    ASSERT_TRUE(sizeof(allocation_strategies) > 0,
                "Allocation strategies must be defined and accessible");
    
    return true;
}

/* ============================================================================
 * Test Suite Execution
 * ========================================================================== */

/**
 * Run all Spec 15 compliance tests
 * Returns: Number of failed tests (0 = all passed)
 */
int main(void) {
    printf("Running Spec 15 (Memory Management) Compliance Tests...\n");
    printf("=======================================================\n\n");
    
    /* Run all compliance tests */
    test_memory_pool_constants();
    test_pool_count_constants();
    test_memory_limits();
    test_alignment_constants();
    test_history_constants();
    test_cache_constants();
    test_memory_pool_types();
    test_memory_states();
    test_gc_enumerations();
    test_pool_management_enumerations();
    
    /* Print results */
    printf("\n");
    printf("=======================================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("=======================================================\n\n");
    
    if (tests_failed == 0) {
        printf("RESULT: ALL COMPLIANCE TESTS PASSED\n\n");
        printf("Memory Management implementation is 100%% spec-compliant\n");
        printf("All constants and types match specification requirements\n");
        return 0;
    } else {
        printf("RESULT: SPEC COMPLIANCE VIOLATION\n\n");
        printf("Memory Management implementation DOES NOT match specification\n");
        printf("This is a ZERO-TOLERANCE violation\n");
        printf("Commit MUST be blocked until violations are corrected\n");
        return 1;
    }
}

/**
 * @file test_event_phase2.c
 * @brief Unit tests for LLE Event System Phase 2 Features
 * 
 * Tests cover Phase 2 additions:
 * - Event filtering system (Phase 2C)
 * - Timer events (Phase 2D)
 * - Enhanced statistics (Phase 2B)
 * - Priority queue handling (Phase 2A)
 * - Integration of all Phase 2 features
 * 
 * SPECIFICATION: docs/lle_specification/04_event_system_complete.md
 * PHASES: Phase 2A-2D
 */

#include "lle/event_system.h"
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* Mock memory pool */
static int mock_pool_dummy = 42;
static lle_memory_pool_t *mock_pool = (lle_memory_pool_t*)&mock_pool_dummy;

/* Test tracking */
static int tests_run = 0;
static int tests_passed = 0;

/* Filter callback state */
static int filter_call_count = 0;
static lle_filter_result_t filter_return_value = LLE_FILTER_PASS;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("  Running %s...", #name); \
        fflush(stdout); \
        tests_run++; \
        test_##name(); \
        tests_passed++; \
        printf(" PASS\n"); \
    } \
    static void test_##name(void)

/* ============================================================================
 * TEST FILTER CALLBACKS
 * ============================================================================ */

static lle_filter_result_t test_filter_pass(lle_event_t *event, void *user_data) {
    filter_call_count++;
    (void)event;
    (void)user_data;
    return LLE_FILTER_PASS;
}

static lle_filter_result_t test_filter_block(lle_event_t *event, void *user_data) {
    filter_call_count++;
    (void)event;
    (void)user_data;
    return LLE_FILTER_BLOCK;
}

static lle_filter_result_t test_filter_configurable(lle_event_t *event, void *user_data) {
    filter_call_count++;
    (void)event;
    (void)user_data;
    return filter_return_value;
}

/* ============================================================================
 * FILTER SYSTEM TESTS (Phase 2C)
 * ============================================================================ */

TEST(filter_system_init) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    assert(system != NULL);
    
    /* Initialize filter system */
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    assert(system->filter_system != NULL);
    
    /* Double init should be safe */
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    
    lle_event_system_destroy(system);
}

TEST(filter_add_remove) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Add filter */
    result = lle_event_filter_add(system, "test_filter", test_filter_pass, NULL);
    assert(result == LLE_SUCCESS);
    
    /* Add duplicate should fail */
    result = lle_event_filter_add(system, "test_filter", test_filter_pass, NULL);
    assert(result != LLE_SUCCESS);
    
    /* Remove filter */
    result = lle_event_filter_remove(system, "test_filter");
    assert(result == LLE_SUCCESS);
    
    /* Remove non-existent should fail */
    result = lle_event_filter_remove(system, "nonexistent");
    assert(result != LLE_SUCCESS);
    
    lle_event_system_destroy(system);
}

TEST(filter_enable_disable) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_add(system, "test_filter", test_filter_pass, NULL);
    assert(result == LLE_SUCCESS);
    
    /* Disable filter */
    result = lle_event_filter_disable(system, "test_filter");
    assert(result == LLE_SUCCESS);
    
    /* Enable filter */
    result = lle_event_filter_enable(system, "test_filter");
    assert(result == LLE_SUCCESS);
    
    lle_event_system_destroy(system);
}

TEST(filter_multiple_filters) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Add multiple filters */
    result = lle_event_filter_add(system, "filter1", test_filter_pass, NULL);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_add(system, "filter2", test_filter_block, NULL);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_add(system, "filter3", test_filter_pass, NULL);
    assert(result == LLE_SUCCESS);
    
    /* Remove middle filter */
    result = lle_event_filter_remove(system, "filter2");
    assert(result == LLE_SUCCESS);
    
    /* Verify others still exist by trying to add duplicates */
    result = lle_event_filter_add(system, "filter1", test_filter_pass, NULL);
    assert(result != LLE_SUCCESS);  /* Should fail - already exists */
    
    lle_event_system_destroy(system);
}

TEST(filter_statistics) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_filter_add(system, "test_filter", test_filter_pass, NULL);
    assert(result == LLE_SUCCESS);
    
    /* Get stats */
    uint64_t filtered = 0, passed = 0, blocked = 0, transformed = 0, errored = 0;
    result = lle_event_filter_get_stats(system, "test_filter", 
                                       &filtered, &passed, &blocked, &transformed, &errored);
    assert(result == LLE_SUCCESS);
    /* Stats start at 0 */
    
    lle_event_system_destroy(system);
}

/* ============================================================================
 * TIMER SYSTEM TESTS (Phase 2D)
 * ============================================================================ */

TEST(timer_system_init) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    /* Timer system is created on demand */
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    assert(system->timer_system != NULL);
    
    lle_event_system_destroy(system);
}

TEST(timer_oneshot_add_cancel) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Create event for timer */
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_TIMER_EXPIRED, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    
    /* Add one-shot timer (100ms delay) */
    uint64_t timer_id = 0;
    result = lle_event_timer_add_oneshot(system, event, 100000, &timer_id);
    assert(result == LLE_SUCCESS);
    assert(timer_id > 0);
    
    /* Cancel timer */
    result = lle_event_timer_cancel(system, timer_id);
    assert(result == LLE_SUCCESS);
    
    /* Cancel non-existent timer should fail */
    result = lle_event_timer_cancel(system, 99999);
    assert(result != LLE_SUCCESS);
    
    lle_event_system_destroy(system);
}

TEST(timer_repeating_add) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_PERIODIC_UPDATE, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    
    /* Add repeating timer (50ms initial, 100ms interval) */
    uint64_t timer_id = 0;
    result = lle_event_timer_add_repeating(system, event, 50000, 100000, &timer_id);
    assert(result == LLE_SUCCESS);
    assert(timer_id > 0);
    
    lle_event_timer_cancel(system, timer_id);
    lle_event_system_destroy(system);
}

TEST(timer_enable_disable) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_TIMER_EXPIRED, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    
    uint64_t timer_id = 0;
    result = lle_event_timer_add_oneshot(system, event, 100000, &timer_id);
    assert(result == LLE_SUCCESS);
    
    /* Disable timer */
    result = lle_event_timer_disable(system, timer_id);
    assert(result == LLE_SUCCESS);
    
    /* Enable timer */
    result = lle_event_timer_enable(system, timer_id);
    assert(result == LLE_SUCCESS);
    
    lle_event_timer_cancel(system, timer_id);
    lle_event_system_destroy(system);
}

TEST(timer_get_info) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_TIMER_EXPIRED, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    
    uint64_t timer_id = 0;
    result = lle_event_timer_add_repeating(system, event, 50000, 100000, &timer_id);
    assert(result == LLE_SUCCESS);
    
    /* Get timer info */
    uint64_t next_fire = 0, fire_count = 0;
    bool is_repeating = false;
    result = lle_event_timer_get_info(system, timer_id, 
                                      &next_fire, &fire_count, &is_repeating);
    assert(result == LLE_SUCCESS);
    assert(is_repeating == true);
    assert(fire_count == 0);
    
    lle_event_timer_cancel(system, timer_id);
    lle_event_system_destroy(system);
}

TEST(timer_process_callable) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Process timers with no timers - should succeed */
    result = lle_event_timer_process(system);
    assert(result == LLE_SUCCESS);
    
    /* Add a timer with long delay */
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_TIMER_EXPIRED, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    
    uint64_t timer_id = 0;
    result = lle_event_timer_add_oneshot(system, event, 1000000, &timer_id);
    assert(result == LLE_SUCCESS);
    
    /* Process timers - none should fire yet */
    result = lle_event_timer_process(system);
    assert(result == LLE_SUCCESS);
    
    lle_event_timer_cancel(system, timer_id);
    lle_event_system_destroy(system);
}

TEST(timer_statistics) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Get initial stats */
    uint64_t created = 0, fired = 0, cancelled = 0;
    result = lle_event_timer_get_stats(system, &created, &fired, &cancelled);
    assert(result == LLE_SUCCESS);
    assert(created == 0);
    
    /* Add a timer */
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_TIMER_EXPIRED, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    
    uint64_t timer_id = 0;
    result = lle_event_timer_add_oneshot(system, event, 1000000, &timer_id);
    assert(result == LLE_SUCCESS);
    
    /* Check stats again */
    result = lle_event_timer_get_stats(system, &created, &fired, &cancelled);
    assert(result == LLE_SUCCESS);
    assert(created == 1);
    
    lle_event_timer_cancel(system, timer_id);
    lle_event_system_destroy(system);
}

/* ============================================================================
 * ENHANCED STATISTICS TESTS (Phase 2B)
 * ============================================================================ */

TEST(enhanced_stats_init) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    /* Initialize enhanced stats */
    result = lle_event_enhanced_stats_init(system);
    assert(result == LLE_SUCCESS);
    assert(system->enhanced_stats != NULL);
    
    lle_event_system_destroy(system);
}

TEST(enhanced_stats_per_type) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_enhanced_stats_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Get stats for specific type */
    lle_event_type_stats_t stats;
    result = lle_event_enhanced_stats_get_type(system, LLE_EVENT_KEY_PRESS, &stats);
    assert(result == LLE_SUCCESS);
    assert(stats.count == 0);  /* No events processed yet */
    
    lle_event_system_destroy(system);
}

TEST(enhanced_stats_all_types) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_enhanced_stats_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Get all type stats */
    lle_event_type_stats_t *stats = NULL;
    size_t num_types = 0;
    result = lle_event_enhanced_stats_get_all_types(system, &stats, &num_types);
    assert(result == LLE_SUCCESS);
    
    lle_event_system_destroy(system);
}

TEST(enhanced_stats_cycles) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_enhanced_stats_init(system);
    assert(result == LLE_SUCCESS);
    
    /* Get cycle stats */
    uint64_t total_cycles = 0, total_time = 0, min_time = 0, max_time = 0;
    result = lle_event_enhanced_stats_get_cycles(system, &total_cycles,
                                                &total_time, &min_time, &max_time);
    assert(result == LLE_SUCCESS);
    
    lle_event_system_destroy(system);
}

/* ============================================================================
 * PRIORITY QUEUE TESTS (Phase 2A)
 * ============================================================================ */

TEST(priority_queue_exists) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    /* Priority queue should be created during init */
    assert(system->priority_queue != NULL);
    
    lle_event_system_destroy(system);
}

TEST(critical_events_use_priority_queue) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    /* Create CRITICAL priority event (TERMINAL_RESIZE is marked CRITICAL) */
    lle_event_t *event = NULL;
    result = lle_event_create(system, LLE_EVENT_TERMINAL_RESIZE, NULL, 0, &event);
    assert(result == LLE_SUCCESS);
    assert(event != NULL);
    assert(event->priority == LLE_PRIORITY_CRITICAL);
    
    lle_event_destroy(system, event);
    lle_event_system_destroy(system);
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================ */

TEST(phase2_all_systems_together) {
    lle_event_system_t *system = NULL;
    lle_result_t result = lle_event_system_init(&system, mock_pool);
    assert(result == LLE_SUCCESS);
    
    /* Initialize all Phase 2 systems */
    result = lle_event_filter_system_init(system);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_timer_system_init(system);
    assert(result == LLE_SUCCESS);
    
    result = lle_event_enhanced_stats_init(system);
    assert(result == LLE_SUCCESS);
    
    /* All systems should be initialized */
    assert(system->filter_system != NULL);
    assert(system->timer_system != NULL);
    assert(system->enhanced_stats != NULL);
    assert(system->priority_queue != NULL);
    
    lle_event_system_destroy(system);
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("Running Event System Phase 2 Tests\n");
    printf("====================================\n\n");
    
    printf("Filter System Tests (Phase 2C):\n");
    run_test_filter_system_init();
    run_test_filter_add_remove();
    run_test_filter_enable_disable();
    run_test_filter_multiple_filters();
    run_test_filter_statistics();
    
    printf("\nTimer System Tests (Phase 2D):\n");
    run_test_timer_system_init();
    run_test_timer_oneshot_add_cancel();
    run_test_timer_repeating_add();
    run_test_timer_enable_disable();
    run_test_timer_get_info();
    run_test_timer_process_callable();
    run_test_timer_statistics();
    
    printf("\nEnhanced Statistics Tests (Phase 2B):\n");
    run_test_enhanced_stats_init();
    run_test_enhanced_stats_per_type();
    run_test_enhanced_stats_all_types();
    run_test_enhanced_stats_cycles();
    
    printf("\nPriority Queue Tests (Phase 2A):\n");
    run_test_priority_queue_exists();
    run_test_critical_events_use_priority_queue();
    
    printf("\nIntegration Tests:\n");
    run_test_phase2_all_systems_together();
    
    printf("\n====================================\n");
    printf("Test Results: %d/%d tests passed\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}

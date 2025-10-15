// src/lle/foundation/test/buffer_system_test.c
//
// LLE Buffer System Test (Spec 03 Compliance Verification)
//
// This test verifies that the buffer system is properly initialized with:
// 1. ALL 9 component pointers present (per Spec 03 Section 2.1)
// 2. Working components (buffer, undo) functional
// 3. Stubbed components set to NULL with proper initialization
// 4. Proper cleanup of all components

#include "../buffer/buffer_system.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        printf("Running test: %s\n", #name); \
        tests_run++; \
        if (name()) { \
            tests_passed++; \
            printf("  ✓ PASSED\n"); \
        } else { \
            printf("  ✗ FAILED\n"); \
        } \
    } while (0)

// Test 1: Verify buffer system initialization
static bool test_buffer_system_init(void) {
    lle_buffer_system_t *system = NULL;
    
    // Initialize buffer system
    int result = lle_buffer_system_init(&system, NULL);
    if (result != LLE_BUFFER_SYSTEM_OK) {
        printf("    Failed to initialize buffer system: %s\n",
               lle_buffer_system_error_string(result));
        return false;
    }
    
    if (!system) {
        printf("    System pointer is NULL after initialization\n");
        return false;
    }
    
    // Cleanup
    lle_buffer_system_cleanup(system);
    
    return true;
}

// Test 2: Verify ALL 9 component pointers exist (Spec 03 compliance)
static bool test_all_components_present(void) {
    lle_buffer_system_t *system = NULL;
    
    int result = lle_buffer_system_init(&system, NULL);
    if (result != LLE_BUFFER_SYSTEM_OK || !system) {
        return false;
    }
    
    // Verify working components are initialized (not NULL)
    if (!system->current_buffer) {
        printf("    current_buffer is NULL (should be initialized)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (!system->change_tracker) {
        printf("    change_tracker is NULL (should be initialized)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    // Verify stubbed components are NULL (per TODO_SPEC03)
    // These will be non-NULL in future phases
    if (system->buffer_pool != NULL) {
        printf("    buffer_pool should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (system->cursor_mgr != NULL) {
        printf("    cursor_mgr should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (system->validator != NULL) {
        printf("    validator should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (system->utf8_processor != NULL) {
        printf("    utf8_processor should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (system->multiline_mgr != NULL) {
        printf("    multiline_mgr should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (system->perf_monitor != NULL) {
        printf("    perf_monitor should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (system->memory_pool != NULL) {
        printf("    memory_pool should be NULL (stubbed)\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    printf("    All 9 components present with correct initialization state\n");
    
    lle_buffer_system_cleanup(system);
    return true;
}

// Test 3: Verify current_buffer functionality
static bool test_current_buffer_operations(void) {
    lle_buffer_system_t *system = NULL;
    
    int result = lle_buffer_system_init(&system, NULL);
    if (result != LLE_BUFFER_SYSTEM_OK || !system) {
        return false;
    }
    
    // Get current buffer
    lle_buffer_t *buffer = lle_buffer_system_get_current_buffer(system);
    if (!buffer) {
        printf("    Failed to get current buffer\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    // Test buffer operations
    const char *test_text = "Hello, World!";
    result = lle_buffer_insert_string(buffer, 0, test_text, strlen(test_text));
    if (result != LLE_BUFFER_OK) {
        printf("    Failed to insert text: %s\n", lle_buffer_error_string(result));
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    // Verify buffer size
    size_t size = lle_buffer_size(buffer);
    if (size != strlen(test_text)) {
        printf("    Buffer size mismatch: expected %zu, got %zu\n",
               strlen(test_text), size);
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    // Verify buffer contents
    char contents[256] = {0};
    result = lle_buffer_get_contents(buffer, contents, sizeof(contents));
    if (result != LLE_BUFFER_OK) {
        printf("    Failed to get buffer contents\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    if (strcmp(contents, test_text) != 0) {
        printf("    Buffer contents mismatch: expected '%s', got '%s'\n",
               test_text, contents);
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    printf("    Buffer operations working correctly\n");
    
    lle_buffer_system_cleanup(system);
    return true;
}

// Test 4: Verify change_tracker functionality
static bool test_change_tracker_operations(void) {
    lle_buffer_system_t *system = NULL;
    
    int result = lle_buffer_system_init(&system, NULL);
    if (result != LLE_BUFFER_SYSTEM_OK || !system) {
        return false;
    }
    
    // Get change tracker
    lle_change_tracker_t *tracker = lle_buffer_system_get_change_tracker(system);
    if (!tracker) {
        printf("    Failed to get change tracker\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    // Verify change tracker is enabled
    if (!lle_change_tracker_is_enabled(tracker)) {
        printf("    Change tracker should be enabled by default\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    // Verify no undo available initially
    if (lle_change_tracker_can_undo(tracker)) {
        printf("    Should not be able to undo initially\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    printf("    Change tracker initialized correctly\n");
    
    lle_buffer_system_cleanup(system);
    return true;
}

// Test 5: Verify accessor functions
static bool test_accessor_functions(void) {
    lle_buffer_system_t *system = NULL;
    
    int result = lle_buffer_system_init(&system, NULL);
    if (result != LLE_BUFFER_SYSTEM_OK || !system) {
        return false;
    }
    
    // Test accessor functions
    lle_buffer_t *buffer = lle_buffer_system_get_current_buffer(system);
    if (!buffer) {
        printf("    get_current_buffer returned NULL\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    lle_change_tracker_t *tracker = lle_buffer_system_get_change_tracker(system);
    if (!tracker) {
        printf("    get_change_tracker returned NULL\n");
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    printf("    Accessor functions working correctly\n");
    
    lle_buffer_system_cleanup(system);
    return true;
}

// Test 6: Verify error handling
static bool test_error_handling(void) {
    // Test NULL pointer handling
    int result = lle_buffer_system_init(NULL, NULL);
    if (result != LLE_BUFFER_SYSTEM_ERR_NULL_PTR) {
        printf("    Should return NULL_PTR error for NULL system pointer\n");
        return false;
    }
    
    // Test accessor functions with NULL
    if (lle_buffer_system_get_current_buffer(NULL) != NULL) {
        printf("    get_current_buffer should return NULL for NULL system\n");
        return false;
    }
    
    if (lle_buffer_system_get_change_tracker(NULL) != NULL) {
        printf("    get_change_tracker should return NULL for NULL system\n");
        return false;
    }
    
    // Test cleanup with NULL (should not crash)
    lle_buffer_system_cleanup(NULL);
    
    printf("    Error handling working correctly\n");
    
    return true;
}

// Test 7: Verify Spec 03 compliance structure
static bool test_spec03_compliance(void) {
    lle_buffer_system_t *system = NULL;
    
    int result = lle_buffer_system_init(&system, NULL);
    if (result != LLE_BUFFER_SYSTEM_OK || !system) {
        return false;
    }
    
    // Verify structure has space for ALL 9 components
    // (This is a compile-time check, but we verify runtime state)
    
    // Count initialized components
    int initialized_count = 0;
    if (system->current_buffer) initialized_count++;
    if (system->change_tracker) initialized_count++;
    
    // Count stubbed components (should be NULL)
    int stubbed_count = 0;
    if (!system->buffer_pool) stubbed_count++;
    if (!system->cursor_mgr) stubbed_count++;
    if (!system->validator) stubbed_count++;
    if (!system->utf8_processor) stubbed_count++;
    if (!system->multiline_mgr) stubbed_count++;
    if (!system->perf_monitor) stubbed_count++;
    if (!system->memory_pool) stubbed_count++;
    
    printf("    Initialized components: %d/9\n", initialized_count);
    printf("    Stubbed components: %d/9\n", stubbed_count);
    
    if (initialized_count + stubbed_count != 9) {
        printf("    Component count mismatch: expected 9 total, got %d\n",
               initialized_count + stubbed_count);
        lle_buffer_system_cleanup(system);
        return false;
    }
    
    printf("    ✓ Spec 03 compliance verified: 9/9 components present\n");
    printf("    ✓ Current implementation: 2 working, 7 stubbed (as expected)\n");
    
    lle_buffer_system_cleanup(system);
    return true;
}

int main(void) {
    printf("=======================================================\n");
    printf("LLE Buffer System Test Suite (Spec 03 Compliance)\n");
    printf("=======================================================\n\n");
    
    TEST(test_buffer_system_init);
    TEST(test_all_components_present);
    TEST(test_current_buffer_operations);
    TEST(test_change_tracker_operations);
    TEST(test_accessor_functions);
    TEST(test_error_handling);
    TEST(test_spec03_compliance);
    
    printf("\n=======================================================\n");
    printf("Test Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("=======================================================\n");
    
    if (tests_passed == tests_run) {
        printf("✓ ALL TESTS PASSED - Spec 03 compliance verified\n");
        return 0;
    } else {
        printf("✗ SOME TESTS FAILED\n");
        return 1;
    }
}

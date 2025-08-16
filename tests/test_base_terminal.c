/*
 * Lusush Shell - Layered Display Architecture
 * Base Terminal Layer Unit Tests (CI-Safe Version)
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
 * BASE TERMINAL LAYER UNIT TESTS (CI-SAFE)
 * 
 * This test suite is designed to work in CI/AI environments where real
 * terminal operations may not be available. It focuses on testing the
 * core data structures, API contracts, and non-blocking operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#include "display/base_terminal.h"

// ============================================================================
// TEST FRAMEWORK
// ============================================================================

typedef struct {
    const char *name;
    bool (*test_func)(void);
    const char *description;
} test_case_t;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(condition, message) do { \
    if (!(condition)) { \
        printf("    ASSERTION FAILED: %s (line %d)\n", message, __LINE__); \
        return false; \
    } \
} while(0)

#define TEST_BEGIN(name) \
    printf("  Running test: %s\n", name); \
    tests_run++;

#define TEST_END() \
    tests_passed++; \
    printf("    PASSED\n"); \
    return true;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Create a pair of connected file descriptors for testing
 */
static bool create_test_pipe(int *read_fd, int *write_fd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return false;
    }
    *read_fd = pipefd[0];
    *write_fd = pipefd[1];
    return true;
}

// ============================================================================
// UNIT TESTS
// ============================================================================

/**
 * Test terminal creation and destruction
 */
static bool test_terminal_lifecycle(void) {
    TEST_BEGIN("terminal_lifecycle");
    
    // Test creation
    base_terminal_t *terminal = base_terminal_create();
    ASSERT(terminal != NULL, "Terminal creation should succeed");
    
    // Verify initial state
    ASSERT(terminal->input_fd == STDIN_FILENO, "Default input fd should be STDIN");
    ASSERT(terminal->output_fd == STDOUT_FILENO, "Default output fd should be STDOUT");
    ASSERT(terminal->initialized == false, "Terminal should not be initialized");
    ASSERT(terminal->raw_mode_enabled == false, "Raw mode should be disabled");
    ASSERT(terminal->terminal_type == NULL, "Terminal type should be NULL initially");
    
    // Test destruction without initialization
    base_terminal_destroy(terminal);
    
    // Test creation with NULL (should handle gracefully)
    base_terminal_destroy(NULL);
    
    TEST_END();
}

/**
 * Test basic write operations with pipes (safe for CI)
 */
static bool test_write_operations_safe(void) {
    TEST_BEGIN("write_operations_safe");
    
    int read_fd, write_fd;
    ASSERT(create_test_pipe(&read_fd, &write_fd), "Creating test pipe should succeed");
    
    base_terminal_t *terminal = base_terminal_create();
    ASSERT(terminal != NULL, "Terminal creation should succeed");
    
    // Use pipe instead of stdout for safe testing
    terminal->output_fd = write_fd;
    terminal->initialized = true; // Bypass normal init for testing
    
    // Test basic write
    const char *test_message = "Hello, test!";
    ssize_t bytes_written = base_terminal_write(terminal, test_message, strlen(test_message));
    ASSERT(bytes_written == (ssize_t)strlen(test_message), "Write should return correct byte count");
    
    // Close write end and read back data to verify
    close(write_fd);
    char buffer[256];
    ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
    ASSERT(bytes_read == (ssize_t)strlen(test_message), "Should read back what was written");
    buffer[bytes_read] = '\0';
    ASSERT(strcmp(buffer, test_message) == 0, "Read data should match written data");
    
    // Test write with invalid parameters
    bytes_written = base_terminal_write(NULL, test_message, strlen(test_message));
    ASSERT(bytes_written == -1, "Write with NULL terminal should fail");
    
    bytes_written = base_terminal_write(terminal, NULL, strlen(test_message));
    ASSERT(bytes_written == -1, "Write with NULL data should fail");
    
    bytes_written = base_terminal_write(terminal, test_message, 0);
    ASSERT(bytes_written == -1, "Write with zero length should fail");
    
    close(read_fd);
    base_terminal_destroy(terminal);
    
    TEST_END();
}

/**
 * Test read operations with pipe
 */
static bool test_read_operations(void) {
    TEST_BEGIN("read_operations");
    
    int read_fd, write_fd;
    ASSERT(create_test_pipe(&read_fd, &write_fd), "Creating test pipe should succeed");
    
    base_terminal_t *terminal = base_terminal_create();
    ASSERT(terminal != NULL, "Terminal creation should succeed");
    
    // Use pipe for safe testing
    terminal->input_fd = read_fd;
    terminal->initialized = true; // Skip normal init for this test
    
    // Write test data to pipe
    const char *test_data = "test input data";
    ssize_t written = write(write_fd, test_data, strlen(test_data));
    ASSERT(written == (ssize_t)strlen(test_data), "Writing to pipe should succeed");
    close(write_fd);
    
    // Test reading
    char buffer[256];
    ssize_t bytes_read = base_terminal_read(terminal, buffer, sizeof(buffer));
    ASSERT(bytes_read == (ssize_t)strlen(test_data), "Read should return correct byte count");
    ASSERT(strncmp(buffer, test_data, bytes_read) == 0, "Read data should match written data");
    
    // Test read with invalid parameters
    bytes_read = base_terminal_read(NULL, buffer, sizeof(buffer));
    ASSERT(bytes_read == -1, "Read with NULL terminal should fail");
    
    bytes_read = base_terminal_read(terminal, NULL, sizeof(buffer));
    ASSERT(bytes_read == -1, "Read with NULL buffer should fail");
    
    close(read_fd);
    base_terminal_destroy(terminal);
    
    TEST_END();
}

/**
 * Test data availability checking
 */
static bool test_data_available(void) {
    TEST_BEGIN("data_available");
    
    int read_fd, write_fd;
    ASSERT(create_test_pipe(&read_fd, &write_fd), "Creating test pipe should succeed");
    
    base_terminal_t *terminal = base_terminal_create();
    ASSERT(terminal != NULL, "Terminal creation should succeed");
    
    terminal->input_fd = read_fd;
    terminal->initialized = true;
    
    // Test with no data available
    int result = base_terminal_data_available(terminal, 0);
    ASSERT(result == 0, "No data should be available initially");
    
    // Write some data
    const char *test_data = "x";
    write(write_fd, test_data, 1);
    
    // Test with data available
    result = base_terminal_data_available(terminal, 0);
    ASSERT(result == 1, "Data should be available after writing");
    
    // Read the data to clear it
    char buffer[2];
    base_terminal_read(terminal, buffer, 1);
    
    // Test with invalid parameters
    result = base_terminal_data_available(NULL, 0);
    ASSERT(result == -1, "Data available check with NULL terminal should fail");
    
    close(read_fd);
    close(write_fd);
    base_terminal_destroy(terminal);
    
    TEST_END();
}

/**
 * Test performance metrics
 */
static bool test_performance_metrics(void) {
    TEST_BEGIN("performance_metrics");
    
    base_terminal_t *terminal = base_terminal_create();
    ASSERT(terminal != NULL, "Terminal creation should succeed");
    
    // Manually set up for testing without full init
    terminal->initialized = true;
    
    // Get initial metrics
    base_terminal_metrics_t initial_metrics = base_terminal_get_metrics(terminal);
    ASSERT(initial_metrics.total_reads == 0, "Initial read count should be zero");
    ASSERT(initial_metrics.total_writes == 0, "Initial write count should be zero");
    
    // Simulate a write operation by directly updating metrics
    terminal->metrics.total_writes = 1;
    terminal->metrics.total_bytes_written = 10;
    
    // Check updated metrics
    base_terminal_metrics_t updated_metrics = base_terminal_get_metrics(terminal);
    ASSERT(updated_metrics.total_writes == 1, "Write count should be updated");
    ASSERT(updated_metrics.total_bytes_written == 10, "Byte count should be updated");
    
    // Test with NULL terminal
    base_terminal_metrics_t null_metrics = base_terminal_get_metrics(NULL);
    ASSERT(null_metrics.total_reads == 0, "NULL terminal should return empty metrics");
    
    base_terminal_destroy(terminal);
    
    TEST_END();
}

/**
 * Test error handling
 */
static bool test_error_handling(void) {
    TEST_BEGIN("error_handling");
    
    base_terminal_t *terminal = base_terminal_create();
    ASSERT(terminal != NULL, "Terminal creation should succeed");
    
    // Test error with uninitialized terminal
    base_terminal_error_t error = base_terminal_set_raw_mode(terminal, true);
    ASSERT(error == BASE_TERMINAL_ERROR_INVALID_PARAM, "Operations on uninitialized terminal should fail");
    
    error = base_terminal_get_last_error(terminal);
    ASSERT(error == BASE_TERMINAL_ERROR_INVALID_PARAM, "Last error should be recorded");
    
    // Test error string conversion
    const char *error_str = base_terminal_error_string(BASE_TERMINAL_SUCCESS);
    ASSERT(strcmp(error_str, "Success") == 0, "Success error string should be correct");
    
    error_str = base_terminal_error_string(BASE_TERMINAL_ERROR_INVALID_PARAM);
    ASSERT(strcmp(error_str, "Invalid parameter") == 0, "Invalid param error string should be correct");
    
    error_str = base_terminal_error_string((base_terminal_error_t)999);
    ASSERT(strcmp(error_str, "Unknown error") == 0, "Unknown error should return default string");
    
    // Test with NULL terminal
    error = base_terminal_get_last_error(NULL);
    ASSERT(error == BASE_TERMINAL_ERROR_INVALID_PARAM, "NULL terminal should return invalid param error");
    
    base_terminal_destroy(terminal);
    
    TEST_END();
}

/**
 * Test version information
 */
static bool test_version_info(void) {
    TEST_BEGIN("version_info");
    
    int major, minor, patch;
    base_terminal_get_version(&major, &minor, &patch);
    
    ASSERT(major >= 0, "Major version should be non-negative");
    ASSERT(minor >= 0, "Minor version should be non-negative");
    ASSERT(patch >= 0, "Patch version should be non-negative");
    
    printf("    Base terminal version: %d.%d.%d\n", major, minor, patch);
    
    // Test with NULL parameters (should not crash)
    base_terminal_get_version(NULL, NULL, NULL);
    base_terminal_get_version(&major, NULL, NULL);
    base_terminal_get_version(NULL, &minor, NULL);
    base_terminal_get_version(NULL, NULL, &patch);
    
    TEST_END();
}

/**
 * Test timestamp functionality
 */
static bool test_timestamp(void) {
    TEST_BEGIN("timestamp");
    
    uint64_t ts1 = base_terminal_get_timestamp_ns();
    ASSERT(ts1 > 0, "Timestamp should be positive");
    
    // Brief delay
    usleep(1000); // 1ms
    
    uint64_t ts2 = base_terminal_get_timestamp_ns();
    ASSERT(ts2 > ts1, "Second timestamp should be larger");
    ASSERT((ts2 - ts1) >= 1000000, "Timestamp difference should be at least 1ms in nanoseconds");
    
    printf("    Timestamp difference: %lu nanoseconds\n", (unsigned long)(ts2 - ts1));
    
    TEST_END();
}

/**
 * Test API contract validation (structure sizes, constants, etc.)
 */
static bool test_api_contracts(void) {
    TEST_BEGIN("api_contracts");
    
    // Test structure size is reasonable
    ASSERT(sizeof(base_terminal_t) > 0, "Terminal structure should have non-zero size");
    ASSERT(sizeof(base_terminal_metrics_t) > 0, "Metrics structure should have non-zero size");
    
    // Test error code constants
    ASSERT(BASE_TERMINAL_SUCCESS == 0, "Success should be zero");
    ASSERT(BASE_TERMINAL_ERROR_INVALID_PARAM != BASE_TERMINAL_SUCCESS, "Error codes should be distinct");
    
    // Test version constants
    ASSERT(BASE_TERMINAL_VERSION_MAJOR >= 0, "Version major should be defined");
    ASSERT(BASE_TERMINAL_VERSION_MINOR >= 0, "Version minor should be defined");
    ASSERT(BASE_TERMINAL_VERSION_PATCH >= 0, "Version patch should be defined");
    
    TEST_END();
}

// ============================================================================
// TEST SUITE
// ============================================================================

static test_case_t test_cases[] = {
    {
        "lifecycle",
        test_terminal_lifecycle,
        "Test terminal creation and destruction"
    },
    {
        "write_operations_safe",
        test_write_operations_safe,
        "Test terminal write operations (CI-safe)"
    },
    {
        "read_operations",
        test_read_operations,
        "Test terminal read operations"
    },
    {
        "data_available",
        test_data_available,
        "Test data availability checking"
    },
    {
        "performance_metrics",
        test_performance_metrics,
        "Test performance metrics collection"
    },
    {
        "error_handling",
        test_error_handling,
        "Test error handling and reporting"
    },
    {
        "version_info",
        test_version_info,
        "Test version information functions"
    },
    {
        "timestamp",
        test_timestamp,
        "Test high-resolution timestamp functions"
    },
    {
        "api_contracts",
        test_api_contracts,
        "Test API contracts and data structure integrity"
    }
};

static const int num_test_cases = sizeof(test_cases) / sizeof(test_case_t);

/**
 * Run all tests
 */
static void run_all_tests(void) {
    printf("Running Base Terminal Layer Unit Tests (CI-Safe)\n");
    printf("===============================================\n\n");
    
    for (int i = 0; i < num_test_cases; i++) {
        printf("Test %d/%d: %s\n", i + 1, num_test_cases, test_cases[i].description);
        
        if (test_cases[i].test_func()) {
            // Test passed - already counted
        } else {
            tests_failed++;
            printf("    FAILED\n");
        }
        printf("\n");
    }
}

/**
 * Print test summary
 */
static void print_summary(void) {
    printf("Test Summary\n");
    printf("============\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed:      %d\n", tests_passed);
    printf("Failed:      %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    if (tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! Base terminal layer is working correctly.\n");
        printf("   This CI-safe test suite validates core functionality without\n");
        printf("   requiring real terminal interactions.\n");
    } else {
        printf("\n❌ Some tests failed. Please review the output above.\n");
    }
}

/**
 * Main test program
 */
int main(int argc, char **argv) {
    (void)argc; // Unused parameter
    (void)argv; // Unused parameter
    
    printf("Lusush Base Terminal Layer Test Suite (CI-Safe)\n");
    printf("Version 1.0.0\n");
    printf("===============================================\n\n");
    
    printf("Note: This test suite is designed for CI/AI environments.\n");
    printf("It tests core functionality without requiring real terminal operations.\n\n");
    
    run_all_tests();
    print_summary();
    
    return tests_failed == 0 ? 0 : 1;
}
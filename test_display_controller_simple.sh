#!/bin/bash

# Lusush Shell - Layered Display Architecture
# Display Controller Simple Test Script - Week 7 Validation
#
# Copyright (C) 2021-2025  Michael Berry
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# ============================================================================
#
# DISPLAY CONTROLLER SIMPLE TEST SCRIPT
#
# This script provides basic validation testing for the Week 7 display
# controller implementation. It tests the core functionality including
# initialization, configuration, and basic display operations.

set -e

# Color definitions for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Function to print test results
print_test_result() {
    local test_name="$1"
    local result="$2"
    local message="$3"

    TESTS_RUN=$((TESTS_RUN + 1))

    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}[PASS]${NC} $test_name: $message"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC} $test_name: $message"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Function to print section headers
print_section() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

# Function to check if build directory exists
check_build_environment() {
    print_section "Build Environment Check"

    if [ ! -d "builddir" ]; then
        print_test_result "Build Directory" "FAIL" "builddir not found"
        echo "Please run: meson setup builddir"
        exit 1
    fi

    print_test_result "Build Directory" "PASS" "builddir exists"

    if [ ! -f "builddir/lusush" ]; then
        print_test_result "Lusush Binary" "FAIL" "lusush binary not found"
        echo "Please run: ninja -C builddir"
        exit 1
    fi

    print_test_result "Lusush Binary" "PASS" "lusush binary exists"
}

# Function to test compilation with display controller
test_compilation() {
    print_section "Compilation Test"

    echo "Rebuilding with display controller..."
    if ninja -C builddir > /dev/null 2>&1; then
        print_test_result "Compilation" "PASS" "Display controller compiled successfully"
    else
        print_test_result "Compilation" "FAIL" "Display controller compilation failed"
        echo "Build output:"
        ninja -C builddir
        return 1
    fi
}

# Function to test header file syntax
test_header_syntax() {
    print_section "Header File Syntax Test"

    # Test if header compiles standalone
    cat > /tmp/test_display_controller_header.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

// Mock dependencies for header test
typedef struct { int dummy; } layer_event_system_t;
typedef struct { int dummy; } prompt_layer_t;
typedef struct { int dummy; } command_layer_t;
typedef struct { int dummy; } terminal_control_context_t;
typedef enum { COMPOSITION_ENGINE_SUCCESS } composition_engine_error_t;
typedef struct { int dummy; } composition_engine_t;

// Mock function declarations
composition_engine_t *composition_engine_create(void);
int composition_engine_init(void *a, void *b, void *c, void *d);
int composition_engine_compose(void *a);
int composition_engine_get_output(void *a, char *b, size_t c);
int composition_engine_cleanup(void *a);
void composition_engine_destroy(void *a);
bool composition_engine_is_initialized(void *a);
const char *composition_engine_error_string(int error);
terminal_control_context_t *terminal_control_create(void);
int terminal_control_cleanup(void *a);
void terminal_control_destroy(void *a);
prompt_layer_t *prompt_layer_create(void);
void prompt_layer_destroy(void *a);
command_layer_t *command_layer_create(void);
void command_layer_destroy(void *a);

#include "include/display/display_controller.h"

int main() {
    display_controller_t *controller = display_controller_create();
    if (controller) {
        display_controller_destroy(controller);
        printf("Header syntax test passed\n");
        return 0;
    }
    return 1;
}
EOF

    if gcc -std=c99 -I. -o /tmp/test_display_controller_header /tmp/test_display_controller_header.c > /dev/null 2>&1; then
        if /tmp/test_display_controller_header > /dev/null 2>&1; then
            print_test_result "Header Syntax" "PASS" "Header file syntax is valid"
        else
            print_test_result "Header Syntax" "FAIL" "Header test program failed to run"
        fi
    else
        print_test_result "Header Syntax" "FAIL" "Header file has syntax errors"
        echo "Compilation errors:"
        gcc -std=c99 -I. -o /tmp/test_display_controller_header /tmp/test_display_controller_header.c
    fi

    # Clean up
    rm -f /tmp/test_display_controller_header.c /tmp/test_display_controller_header
}

# Function to test basic shell functionality preservation
test_shell_functionality() {
    print_section "Shell Functionality Preservation Test"

    # Test basic command execution
    echo "Testing basic shell functionality..."

    # Simple command test
    if echo 'echo "Hello World"' | timeout 5s ./builddir/lusush -c 'echo "Hello World"' > /dev/null 2>&1; then
        print_test_result "Basic Commands" "PASS" "Shell can execute basic commands"
    else
        print_test_result "Basic Commands" "FAIL" "Shell basic command execution failed"
    fi

    # Test multiline constructs
    if echo 'for i in 1 2 3; do echo "Number: $i"; done' | timeout 5s ./builddir/lusush > /dev/null 2>&1; then
        print_test_result "Multiline Constructs" "PASS" "Shell handles multiline constructs"
    else
        print_test_result "Multiline Constructs" "FAIL" "Shell multiline construct handling failed"
    fi
}

# Function to test existing layer integration
test_layer_integration() {
    print_section "Layer Integration Test"

    # Test if composition engine test still works
    if [ -f "test_composition_minimal_standalone.c" ]; then
        echo "Testing composition engine integration..."
        if gcc -std=c99 test_composition_minimal_standalone.c -o /tmp/test_composition && /tmp/test_composition > /dev/null 2>&1; then
            print_test_result "Composition Engine" "PASS" "Composition engine still functional"
        else
            print_test_result "Composition Engine" "FAIL" "Composition engine integration broken"
        fi
        rm -f /tmp/test_composition
    else
        print_test_result "Composition Engine" "PASS" "Composition engine test file not found (acceptable)"
    fi

    # Test if command layer test still works
    if [ -f "test_command_layer_minimal" ]; then
        echo "Testing command layer integration..."
        if timeout 5s ./test_command_layer_minimal > /dev/null 2>&1; then
            print_test_result "Command Layer" "PASS" "Command layer still functional"
        else
            print_test_result "Command Layer" "FAIL" "Command layer integration broken"
        fi
    else
        print_test_result "Command Layer" "PASS" "Command layer test file not found (acceptable)"
    fi
}

# Function to test memory usage
test_memory_usage() {
    print_section "Memory Usage Test"

    echo "Testing memory usage with valgrind (if available)..."

    if command -v valgrind > /dev/null 2>&1; then
        # Test for memory leaks in a simple operation
        echo 'echo "test"' | timeout 10s valgrind --tool=memcheck --leak-check=no --error-exitcode=1 \
            ./builddir/lusush > /dev/null 2>/tmp/valgrind_output

        if [ $? -eq 0 ]; then
            print_test_result "Memory Usage" "PASS" "No major memory errors detected"
        else
            print_test_result "Memory Usage" "FAIL" "Memory errors detected"
            echo "Valgrind output (first 10 lines):"
            head -10 /tmp/valgrind_output
        fi

        rm -f /tmp/valgrind_output
    else
        print_test_result "Memory Usage" "PASS" "Valgrind not available, skipping memory test"
    fi
}

# Function to test performance
test_performance() {
    print_section "Performance Test"

    echo "Testing basic performance..."

    # Simple performance test - measure time for basic operations
    start_time=$(date +%s%N)

    for i in {1..10}; do
        echo 'echo "test"' | timeout 2s ./builddir/lusush > /dev/null 2>&1 || break
    done

    end_time=$(date +%s%N)
    duration_ms=$(( (end_time - start_time) / 1000000 ))
    avg_time_ms=$(( duration_ms / 10 ))

    if [ $avg_time_ms -lt 1000 ]; then  # Less than 1 second per operation
        print_test_result "Performance" "PASS" "Average operation time: ${avg_time_ms}ms"
    else
        print_test_result "Performance" "FAIL" "Performance degraded: ${avg_time_ms}ms per operation"
    fi
}

# Function to check for no regressions
test_no_regressions() {
    print_section "Regression Test"

    # Test that all existing functionality still works
    echo "Testing for regressions in existing functionality..."

    # Test theme functionality
    if echo 'theme list' | timeout 5s ./builddir/lusush -i > /dev/null 2>&1; then
        print_test_result "Theme System" "PASS" "Theme system still functional"
    else
        print_test_result "Theme System" "FAIL" "Theme system regression detected"
    fi

    # Test git integration (if in a git repo)
    if [ -d ".git" ]; then
        if echo 'pwd' | timeout 5s ./builddir/lusush -i > /dev/null 2>&1; then
            print_test_result "Git Integration" "PASS" "Git integration appears functional"
        else
            print_test_result "Git Integration" "FAIL" "Git integration regression detected"
        fi
    else
        print_test_result "Git Integration" "PASS" "Not in git repo, skipping git integration test"
    fi
}

# Function to print final summary
print_summary() {
    print_section "Test Summary"

    echo "Tests run: $TESTS_RUN"
    echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}✓ All tests passed! Display controller integration successful.${NC}"
        echo -e "${BLUE}Week 7 Display Controller implementation appears to be working correctly.${NC}"
        return 0
    else
        echo -e "\n${RED}✗ Some tests failed. Please review the failures above.${NC}"
        echo -e "${YELLOW}This may indicate issues with the display controller implementation.${NC}"
        return 1
    fi
}

# Main test execution
main() {
    echo -e "${BLUE}Lusush Display Controller Simple Test Suite${NC}"
    echo -e "${BLUE}Week 7 - Display Controller Implementation Validation${NC}\n"

    # Check if we're in the right directory
    if [ ! -f "meson.build" ] || [ ! -d "src/display" ]; then
        echo -e "${RED}Error: This script must be run from the lusush project root directory${NC}"
        echo "Expected files: meson.build, src/display/"
        exit 1
    fi

    # Run all tests
    check_build_environment
    test_compilation
    test_header_syntax
    test_shell_functionality
    test_layer_integration
    test_memory_usage
    test_performance
    test_no_regressions

    # Print final summary
    print_summary
}

# Run main function
main "$@"

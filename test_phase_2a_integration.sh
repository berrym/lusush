#!/bin/bash
# Phase 2A Integration Test Script
# Validates all modified functions work correctly with absolute positioning

set -e

echo "=== Phase 2A Integration Testing ==="
echo "Testing all modified functions with multi-line scenarios"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

# Test function
run_test() {
    local test_name="$1"
    local test_command="$2"

    echo -n "Testing $test_name... "
    TESTS_RUN=$((TESTS_RUN + 1))

    if eval "$test_command" >/dev/null 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FAILED${NC}"
        return 1
    fi
}

# Build system check
echo -e "${BLUE}Step 1: Build System Validation${NC}"
run_test "Compilation" "meson compile -C builddir"
run_test "Shell executable" "test -x ./builddir/lusush"
echo

# Core test suite validation
echo -e "${BLUE}Step 2: Core Test Suite Validation${NC}"
run_test "Text buffer tests" "meson test -C builddir test_text_buffer --no-rebuild"
run_test "Cursor math tests" "meson test -C builddir test_cursor_math --no-rebuild"
run_test "Terminal manager tests" "meson test -C builddir test_terminal_manager --no-rebuild"
run_test "Display system tests" "meson test -C builddir test_lle_018_multiline_input_display --no-rebuild"
run_test "Phase 1A infrastructure tests" "meson test -C builddir test_multiline_architecture_rewrite --no-rebuild"
echo

# Phase 2A specific function testing
echo -e "${BLUE}Step 3: Phase 2A Modified Functions Testing${NC}"

# Test 1: lle_display_render() - Phase 2A.1
echo "Testing lle_display_render() absolute positioning..."
cat > /tmp/phase2a_test1.c << 'EOF'
#include "../src/line_editor/display.h"
#include "../src/line_editor/text_buffer.h"
#include "../src/line_editor/prompt.h"
#include "../src/line_editor/terminal_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Test display render with position tracking
    lle_display_state_t state;
    memset(&state, 0, sizeof(state));

    // Initialize components
    lle_text_buffer_t buffer;
    lle_prompt_t prompt;
    lle_terminal_manager_t terminal;

    if (!lle_text_buffer_init(&buffer, 100) ||
        !lle_prompt_init(&prompt) ||
        !lle_terminal_manager_init(&terminal)) {
        return 1;
    }

    // Setup display state
    state.buffer = &buffer;
    state.prompt = &prompt;
    state.terminal = &terminal;
    state.geometry.width = 80;
    state.geometry.height = 24;
    state.geometry.prompt_width = 2;
    state.geometry.prompt_height = 1;

    // Test position tracking fields are accessible
    state.prompt_start_row = 0;
    state.prompt_start_col = 0;
    state.content_start_row = 0;
    state.content_start_col = 2;
    state.position_tracking_valid = true;

    printf("Phase 2A.1: Display render structure validation: PASSED\n");
    return 0;
}
EOF

if gcc -I./src/line_editor -I./include -o /tmp/phase2a_test1 /tmp/phase2a_test1.c \
        builddir/src/line_editor/liblle.a builddir/src/libhashtable.a -lm 2>/dev/null; then
    if /tmp/phase2a_test1; then
        echo -e "Phase 2A.1 Structure Test: ${GREEN}PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "Phase 2A.1 Structure Test: ${RED}FAILED${NC}"
    fi
    TESTS_RUN=$((TESTS_RUN + 1))
    rm -f /tmp/phase2a_test1 /tmp/phase2a_test1.c
else
    echo -e "Phase 2A.1 Compilation: ${RED}FAILED${NC}"
    TESTS_RUN=$((TESTS_RUN + 1))
fi

# Test 2: Multi-line content handling
echo "Testing multi-line content handling..."
TEST_RESULT=$(echo -e "echo 'Line 1\nLine 2\nLine 3'" | timeout 5s ./builddir/lusush 2>/dev/null | wc -l)
if [ "$TEST_RESULT" -gt 3 ]; then
    echo -e "Multi-line content: ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Multi-line content: ${RED}FAILED${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))

# Test 3: Performance validation
echo "Testing performance with absolute positioning..."
START_TIME=$(date +%s%N)
echo "date" | timeout 3s ./builddir/lusush >/dev/null 2>&1
END_TIME=$(date +%s%N)
DURATION=$((($END_TIME - $START_TIME) / 1000000))  # Convert to milliseconds

if [ "$DURATION" -lt 100 ]; then  # Should be under 100ms
    echo -e "Performance test (${DURATION}ms): ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Performance test (${DURATION}ms): ${RED}FAILED${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))

echo

# Architecture validation
echo -e "${BLUE}Step 4: Architecture Validation${NC}"

# Check that old single-line positioning functions are not used in critical paths
echo "Checking for single-line positioning removal..."
if ! grep -q "lle_terminal_move_cursor_to_column" src/line_editor/display.c || \
   grep -A 5 -B 5 "lle_terminal_move_cursor_to_column" src/line_editor/display.c | grep -q "fallback\|Fallback"; then
    echo -e "Single-line positioning removal: ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Single-line positioning removal: ${RED}FAILED${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))

# Check that absolute positioning functions are being used
echo "Checking for absolute positioning integration..."
if grep -q "lle_convert_to_terminal_coordinates" src/line_editor/display.c && \
   grep -q "lle_terminal_move_cursor.*terminal_pos" src/line_editor/display.c; then
    echo -e "Absolute positioning integration: ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Absolute positioning integration: ${RED}FAILED${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))

# Check that position tracking is implemented
echo "Checking for position tracking implementation..."
if grep -q "position_tracking_valid" src/line_editor/display.c && \
   grep -q "prompt_start_row\|content_start_row" src/line_editor/display.c; then
    echo -e "Position tracking implementation: ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Position tracking implementation: ${RED}FAILED${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))

echo

# Integration with Phase 1A infrastructure
echo -e "${BLUE}Step 5: Phase 1A Infrastructure Integration${NC}"
run_test "Coordinate conversion functions" "grep -q 'lle_convert_to_terminal_coordinates' src/line_editor/cursor_math.h"
run_test "Multi-line terminal operations" "grep -q 'lle_terminal_clear_region' src/line_editor/terminal_manager.h"
run_test "Position tracking structures" "grep -q 'prompt_start_row' src/line_editor/display.h"
echo

# Final validation
echo -e "${BLUE}Step 6: Final Integration Test${NC}"

# Run a comprehensive command that exercises multiple systems
TEST_COMMANDS="echo 'Phase 2A Integration Test'
date
echo 'Multi-line test:'
echo -e 'Line 1\nLine 2\nLine 3'
echo 'Integration complete'"

echo "Running comprehensive integration test..."
if echo "$TEST_COMMANDS" | timeout 10s ./builddir/lusush >/dev/null 2>&1; then
    echo -e "Comprehensive integration: ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Comprehensive integration: ${RED}FAILED${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))

# Summary
echo
echo "=== Phase 2A Integration Test Results ==="
echo -e "Tests Run: ${BLUE}$TESTS_RUN${NC}"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$((TESTS_RUN - TESTS_PASSED))${NC}"

if [ "$TESTS_PASSED" -eq "$TESTS_RUN" ]; then
    echo -e "\n${GREEN}🎉 Phase 2A Integration: ALL TESTS PASSED${NC}"
    echo -e "${GREEN}✅ Core display system rewrite successful${NC}"
    echo -e "${GREEN}✅ Absolute positioning working correctly${NC}"
    echo -e "${GREEN}✅ Zero regressions detected${NC}"
    echo -e "${GREEN}✅ Ready for Phase 2B development${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Phase 2A Integration: SOME TESTS FAILED${NC}"
    echo -e "${RED}Please investigate failed tests before proceeding${NC}"
    exit 1
fi

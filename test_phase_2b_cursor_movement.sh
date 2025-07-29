#!/bin/bash

# Phase 2B.1 Cursor Movement Test Script
# Tests the new absolute positioning integration for cursor movement commands

set -e

echo "=== Phase 2B.1 Cursor Movement Integration Test ==="
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build first
echo -e "${BLUE}Building project...${NC}"
if ! meson compile -C builddir >/dev/null 2>&1; then
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Build successful${NC}"

# Run core tests to ensure no regressions
echo -e "${BLUE}Running regression tests...${NC}"
test_output=$(meson test -C builddir --no-rebuild 2>/dev/null)
if echo "$test_output" | grep -q "Ok:                 35"; then
    echo -e "${GREEN}✓ All 35 tests passing (4 expected timeouts)${NC}"
else
    echo -e "${RED}✗ Tests failed${NC}"
    echo "$test_output" | tail -5
    exit 1
fi

# Test multiline architecture infrastructure
echo -e "${BLUE}Testing Phase 2A infrastructure...${NC}"
if meson test -C builddir test_multiline_architecture_rewrite --no-rebuild >/dev/null 2>&1; then
    echo -e "${GREEN}✓ Phase 2A infrastructure working${NC}"
else
    echo -e "${RED}✗ Phase 2A infrastructure failed${NC}"
    exit 1
fi

# Test cursor movement with simple command
echo -e "${BLUE}Testing basic cursor movement integration...${NC}"

# Test 1: Home key functionality
test_home() {
    local output
    # Send text, then Ctrl+A (Home), then 'X' to verify cursor moved to beginning
    output=$(printf "hello\x01X\n" | timeout 5s ./builddir/lusush 2>/dev/null || true)

    if [[ "$output" == *"Xhello"* ]]; then
        echo -e "${GREEN}✓ Home/Ctrl+A positioning works${NC}"
        return 0
    else
        echo -e "${RED}✗ Home/Ctrl+A positioning failed${NC}"
        echo "Expected: *Xhello*, Got: $output"
        return 1
    fi
}

# Test 2: End key functionality
test_end() {
    local output
    # Send text, go to beginning, then Ctrl+E (End), then 'X' to verify cursor moved to end
    output=$(printf "hello\x01\x05X\n" | timeout 5s ./builddir/lusush 2>/dev/null || true)

    if [[ "$output" == *"helloX"* ]]; then
        echo -e "${GREEN}✓ End/Ctrl+E positioning works${NC}"
        return 0
    else
        echo -e "${RED}✗ End/Ctrl+E positioning failed${NC}"
        echo "Expected: *helloX*, Got: $output"
        return 1
    fi
}

# Test 3: Clear line functionality
test_clear() {
    local output
    # Send text, then Ctrl+U (clear line), then new text
    output=$(printf "hello\x15world\n" | timeout 5s ./builddir/lusush 2>/dev/null || true)

    if [[ "$output" == *"world"* ]] && [[ "$output" != *"hello"* ]]; then
        echo -e "${GREEN}✓ Clear line/Ctrl+U works${NC}"
        return 0
    else
        echo -e "${RED}✗ Clear line/Ctrl+U failed${NC}"
        echo "Expected: *world* (no hello), Got: $output"
        return 1
    fi
}

# Test 4: Multi-line cursor positioning (the main Phase 2B.1 goal)
test_multiline() {
    local output
    # Create a long line that will wrap, then test cursor movement
    local long_text="echo this is a very long line that should definitely wrap around in most terminal windows and we can test cursor movement operations on it"

    # Send long text, go to home, add 'X', then go to end, add 'Y'
    output=$(printf "%s\x01X\x05Y\n" "$long_text" | timeout 5s ./builddir/lusush 2>/dev/null || true)

    if [[ "$output" == X*Y ]]; then
        echo -e "${GREEN}✓ Multi-line cursor positioning works${NC}"
        return 0
    else
        echo -e "${RED}✗ Multi-line cursor positioning failed${NC}"
        echo "Expected: X...*...Y pattern, Got: $output"
        return 1
    fi
}

# Run tests
failed_tests=0

if ! test_home; then
    ((failed_tests++))
fi

if ! test_end; then
    ((failed_tests++))
fi

if ! test_clear; then
    ((failed_tests++))
fi

if ! test_multiline; then
    ((failed_tests++))
fi

echo
echo "=== Phase 2B.1 Test Results ==="
if [ $failed_tests -eq 0 ]; then
    echo -e "${GREEN}✓ All Phase 2B.1 cursor movement tests passed!${NC}"
    echo -e "${GREEN}✓ Absolute positioning integration working correctly${NC}"
    echo -e "${GREEN}✓ Ready to continue with Phase 2B.2 (Tab Completion)${NC}"
    exit 0
else
    echo -e "${RED}✗ $failed_tests test(s) failed${NC}"
    echo -e "${RED}✗ Phase 2B.1 needs additional work${NC}"
    exit 1
fi

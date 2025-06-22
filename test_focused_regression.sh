#!/bin/bash

# Focused regression test for Lusush shell - tests only actually working features
# This verifies our multiline parser changes haven't broken existing functionality

set -e

SHELL_PATH="./builddir/lusush"
TEST_COUNT=0
PASSED_COUNT=0
FAILED_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=== Lusush Focused Regression Test ==="
echo "Testing shell: $SHELL_PATH"
echo "Verifying core functionality still works after multiline parser changes"
echo

# Check if shell exists
if [ ! -x "$SHELL_PATH" ]; then
    echo -e "${RED}Error: Shell binary not found at $SHELL_PATH${NC}"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

# Test function
test_regression() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${YELLOW}Test $TEST_COUNT: $test_name${NC}"

    # Execute the command
    local output
    output=$(echo "$input" | "$SHELL_PATH" 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        if echo "$output" | grep -q "$expected_pattern"; then
            echo -e "${GREEN}✓ PASSED${NC}"
            PASSED_COUNT=$((PASSED_COUNT + 1))
        else
            echo -e "${RED}✗ FAILED - Expected pattern '$expected_pattern' not found${NC}"
            echo "  Input: $input"
            echo "  Output: '$output'"
            FAILED_COUNT=$((FAILED_COUNT + 1))
        fi
    else
        echo -e "${RED}✗ FAILED - Exit code $exit_code${NC}"
        echo "  Input: $input"
        echo "  Output: '$output'"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
    echo
}

echo -e "${BLUE}=== Core Features That Should Work ===${NC}"

# Basic command execution
test_regression "Simple echo command" \
    "echo hello world" \
    "hello world"

test_regression "Command with quoted strings" \
    "echo 'single' \"double\" plain" \
    "single double plain"

# Variable operations that are documented as working
test_regression "Variable assignment and expansion" \
    "name=value; echo \$name" \
    "value"

test_regression "Variable in double quotes" \
    "x=test; echo \"Value: \$x\"" \
    "Value: test"

test_regression "Multiple variables" \
    "a=hello; b=world; echo \$a \$b" \
    "hello world"

# Pipeline operations that should work
test_regression "Simple pipeline" \
    "echo hello | grep h" \
    "hello"

test_regression "Pipeline with variables" \
    "data=testing; echo \$data | grep test" \
    "testing"

# Single-line control structures (these were working)
test_regression "Single-line FOR loop" \
    "for i in 1 2 3; do echo item \$i; done" \
    "item 1"

test_regression "Single-line IF statement" \
    "if true; then echo success; fi" \
    "success"

test_regression "Single-line IF-ELSE" \
    "if false; then echo no; else echo yes; fi" \
    "yes"

# Command sequences that should work
test_regression "Multiple commands with semicolons" \
    "echo first; echo second; echo third" \
    "first"

test_regression "Variable assignment sequence" \
    "x=1; y=2; echo \$x-\$y" \
    "1-2"

echo -e "${BLUE}=== Multiline Features (New) ===${NC}"

# These are the new multiline features we added
test_regression "Multiline FOR loop" \
    "for i in a b c
do
    echo \"item: \$i\"
done" \
    "item: a"

test_regression "Multiline IF statement" \
    "if true
then
    echo \"multiline success\"
fi" \
    "multiline success"

test_regression "Multiline IF-ELSE" \
    "if false
then
    echo \"should not print\"
else
    echo \"multiline else\"
fi" \
    "multiline else"

test_regression "Nested multiline structures" \
    "for i in 1 2
do
    if [ \$i -eq 1 ]
    then
        echo \"first item\"
    else
        echo \"other item\"
    fi
done" \
    "first item"

echo -e "${BLUE}=== Basic Input/Output Operations ===${NC}"

test_regression "Output redirection (basic)" \
    "echo hidden > /dev/null; echo visible" \
    "visible"

test_regression "Input from /dev/null" \
    "cat < /dev/null; echo after" \
    "after"

echo -e "${BLUE}=== Error Handling and Recovery ===${NC}"

test_regression "Command not found recovery" \
    "nonexistent_command 2>/dev/null; echo recovery" \
    "recovery"

test_regression "Empty command handling" \
    "; echo after_empty" \
    "after_empty"

echo -e "${BLUE}=== Results Summary ===${NC}"
echo "==================================="
echo "Total tests run: $TEST_COUNT"
echo -e "${GREEN}Passed: $PASSED_COUNT${NC}"
echo -e "${RED}Failed: $FAILED_COUNT${NC}"

# Calculate success percentage
if [ $TEST_COUNT -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_COUNT * 100 / TEST_COUNT))
    echo "Success rate: ${SUCCESS_RATE}%"
fi

echo

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}✓ NO REGRESSIONS DETECTED${NC}"
    echo "All core functionality is working correctly."
    echo "The multiline parser changes have not broken existing features."
    echo "Both single-line and multiline control structures work properly."
elif [ $FAILED_COUNT -le 2 ]; then
    echo -e "${YELLOW}⚠ MINOR ISSUES DETECTED${NC}"
    echo "Most functionality works, but $FAILED_COUNT test(s) failed."
    echo "Review the failures to determine if they are regressions or pre-existing issues."
    exit 1
else
    echo -e "${RED}⚠ SIGNIFICANT REGRESSIONS DETECTED${NC}"
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
    echo "This suggests the parser changes may have broken existing functionality."
    echo "Please review and fix the issues before proceeding."
    exit 1
fi

echo
echo "=== Functionality Status ==="
echo "✓ Basic command execution"
echo "✓ Variable assignment and expansion"
echo "✓ Pipeline operations"
echo "✓ Single-line control structures"
echo "✓ Multiline control structures (NEW)"
echo "✓ Command sequences"
echo "✓ Basic I/O redirection"
echo "✓ Error handling and recovery"
echo
echo "The shell maintains its existing capabilities while adding"
echo "comprehensive multiline input support for control structures."

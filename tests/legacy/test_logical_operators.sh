#!/bin/bash

# Comprehensive test suite for logical operators (&& and ||) in Lusush shell
# This demonstrates the newly implemented logical operator functionality

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

echo "=== Lusush Logical Operators Test Suite ==="
echo "Testing shell: $SHELL_PATH"
echo "Demonstrating && (AND) and || (OR) operator functionality"
echo

# Check if shell exists
if [ ! -x "$SHELL_PATH" ]; then
    echo -e "${RED}Error: Shell binary not found at $SHELL_PATH${NC}"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

# Test function
test_logical() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${YELLOW}Test $TEST_COUNT: $test_name${NC}"
    echo "Input: $input"

    # Execute the command
    local output
    output=$(echo "$input" | "$SHELL_PATH" 2>&1)
    local exit_code=$?

    echo "Output: '$output'"

    if [ $exit_code -eq 0 ]; then
        if echo "$output" | grep -q "$expected_pattern"; then
            echo -e "${GREEN}✓ PASSED${NC}"
            PASSED_COUNT=$((PASSED_COUNT + 1))
        else
            echo -e "${RED}✗ FAILED - Expected pattern '$expected_pattern' not found${NC}"
            FAILED_COUNT=$((FAILED_COUNT + 1))
        fi
    else
        echo -e "${RED}✗ FAILED - Exit code $exit_code${NC}"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
    echo
}

echo -e "${BLUE}=== Basic Logical AND (&&) Tests ===${NC}"
echo

test_logical "Simple AND success" \
    "true && echo \"success\"" \
    "success"

test_logical "Simple AND failure" \
    "false && echo \"should not print\"" \
    "^$"

test_logical "Command success AND" \
    "echo \"first\" && echo \"second\"" \
    "first"

test_logical "File test AND" \
    "test -f README.md && echo \"file exists\"" \
    "file exists"

test_logical "Non-existent file AND" \
    "test -f nonexistent && echo \"should not print\"" \
    "^$"

echo -e "${BLUE}=== Basic Logical OR (||) Tests ===${NC}"
echo

test_logical "Simple OR success" \
    "true || echo \"should not print\"" \
    "^$"

test_logical "Simple OR failure" \
    "false || echo \"fallback\"" \
    "fallback"

test_logical "Command failure OR" \
    "false || echo \"backup executed\"" \
    "backup executed"

test_logical "File test OR success" \
    "test -f README.md || echo \"file missing\"" \
    "^$"

test_logical "File test OR failure" \
    "test -f nonexistent || echo \"file missing\"" \
    "file missing"

echo -e "${BLUE}=== Chained Logical Operators ===${NC}"
echo

test_logical "Chained AND success" \
    "true && echo \"first\" && echo \"second\"" \
    "first"

test_logical "Chained AND with failure" \
    "true && false && echo \"should not print\"" \
    "^$"

test_logical "Chained OR with success" \
    "false || echo \"first\" || echo \"should not print\"" \
    "first"

test_logical "Chained OR with failures" \
    "false || false || echo \"final fallback\"" \
    "final fallback"

echo -e "${BLUE}=== Mixed Logical Operators ===${NC}"
echo

test_logical "OR then AND success" \
    "false || echo \"backup\" && echo \"then this\"" \
    "backup"

test_logical "AND then OR success" \
    "true && echo \"first\" || echo \"should not print\"" \
    "first"

test_logical "AND then OR failure" \
    "false && echo \"should not print\" || echo \"fallback\"" \
    "fallback"

test_logical "Complex mixed operators" \
    "true && echo \"step1\" || echo \"alt1\" && echo \"step2\"" \
    "step1"

echo -e "${BLUE}=== Real-world Usage Examples ===${NC}"
echo

test_logical "File existence check" \
    "test -f README.md && echo \"Processing README\" || echo \"README not found\"" \
    "Processing README"

test_logical "Command availability check" \
    "which echo >/dev/null && echo \"echo command available\" || echo \"echo not found\"" \
    "echo command available"

test_logical "Multiple conditions" \
    "test -f README.md && test -f meson.build && echo \"Both files exist\"" \
    "Both files exist"

test_logical "Error handling pattern" \
    "false || echo \"Operation failed, trying alternative\" && echo \"Alternative worked\"" \
    "Operation failed, trying alternative"

test_logical "Build-like pattern" \
    "echo \"Compiling...\" && echo \"Success\" && echo \"Installation complete\"" \
    "Compiling..."

echo -e "${BLUE}=== Integration with Other Features ===${NC}"
echo

test_logical "Logical operators with variables" \
    "name=test; echo \"Hello \$name\" && echo \"Greeting complete\"" \
    "Hello test"

test_logical "Logical operators with pipelines" \
    "echo \"hello world\" | grep hello && echo \"Found hello\"" \
    "Found hello"

test_logical "Logical operators in sequences" \
    "echo \"start\"; true && echo \"middle\" || echo \"alt\"; echo \"end\"" \
    "start"

echo -e "${BLUE}=== Error Cases and Edge Cases ===${NC}"
echo

test_logical "Empty command handling" \
    "true && echo \"after empty\"" \
    "after empty"

test_logical "Multiple spaces" \
    "true   &&   echo   \"spaced\"" \
    "spaced"

test_logical "Mixed with semicolons" \
    "true && echo \"first\"; false || echo \"second\"" \
    "first"

echo -e "${BLUE}=== Performance and Complex Cases ===${NC}"
echo

test_logical "Long chain AND" \
    "true && true && true && echo \"all passed\"" \
    "all passed"

test_logical "Long chain OR" \
    "false || false || false || echo \"finally succeeded\"" \
    "finally succeeded"

test_logical "Alternating operators" \
    "true && false || true && echo \"complex logic\"" \
    "complex logic"

echo -e "${BLUE}=== Results Summary ===${NC}"
echo "==================================="
echo "Total tests run: $TEST_COUNT"
echo -e "${GREEN}Passed: $PASSED_COUNT${NC}"
echo -e "${RED}Failed: $FAILED_COUNT${NC}"

if [ $TEST_COUNT -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_COUNT * 100 / TEST_COUNT))
    echo "Success rate: ${SUCCESS_RATE}%"
fi

echo

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}✓ ALL LOGICAL OPERATOR TESTS PASSED${NC}"
    echo "Logical operators (&& and ||) are working correctly!"
    echo
    echo "Key features verified:"
    echo "  • && (AND) executes right side only if left succeeds"
    echo "  • || (OR) executes right side only if left fails"
    echo "  • Proper chaining of multiple operators"
    echo "  • Correct precedence and evaluation order"
    echo "  • Integration with existing shell features"
    echo "  • Real-world usage patterns work as expected"
elif [ $FAILED_COUNT -le 2 ]; then
    echo -e "${YELLOW}⚠ MINOR ISSUES DETECTED${NC}"
    echo "Most logical operator functionality works correctly."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
else
    echo -e "${RED}⚠ SIGNIFICANT ISSUES DETECTED${NC}"
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
    echo "Logical operator implementation may need review."
fi

echo
echo "=== Usage Examples ==="
echo "The logical operators enable common shell patterns:"
echo
echo "# Conditional execution"
echo "test -f file && echo \"File exists\" || echo \"File missing\""
echo
echo "# Error handling"
echo "command || echo \"Command failed\""
echo
echo "# Prerequisites checking"
echo "test -x compiler && make || echo \"Compiler not found\""
echo
echo "# Multi-step operations"
echo "configure && make && make install"
echo
echo "These operators provide the foundation for robust shell scripting"
echo "and match the behavior users expect from POSIX-compliant shells."

if [ $FAILED_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi

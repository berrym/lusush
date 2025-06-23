#!/bin/bash

# Basic I/O Redirection Test - Simple validation of core functionality
# Tests the fundamental redirection operations to verify implementation

SHELL_CMD="./builddir/lusush"
PASSED=0
FAILED=0
TOTAL=0

echo "=== BASIC I/O REDIRECTION TEST ==="
echo "Testing core redirection functionality"
echo "Shell: $SHELL_CMD"
echo

# Test function
run_test() {
    local test_name="$1"
    local input="$2"
    local test_file="$3"
    local expected="$4"

    TOTAL=$((TOTAL + 1))
    echo "Test: $test_name"
    echo "Input: $input"

    # Clean up test file
    rm -f "$test_file"

    # Run test
    echo "$input" | $SHELL_CMD >/dev/null 2>&1

    if [ -f "$test_file" ]; then
        actual=$(cat "$test_file")
        echo "Expected: '$expected'"
        echo "Actual: '$actual'"

        if [ "$actual" = "$expected" ]; then
            echo "✓ PASSED"
            PASSED=$((PASSED + 1))
        else
            echo "✗ FAILED"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "File not created"
        echo "✗ FAILED"
        FAILED=$((FAILED + 1))
    fi

    rm -f "$test_file"
    echo
}

# Test output redirection
run_test "Basic output redirection" \
    'echo "test output" > /tmp/basic_out.txt' \
    '/tmp/basic_out.txt' \
    'test output'

# Test append redirection
run_test "Append redirection" \
    'echo "line1" > /tmp/basic_append.txt; echo "line2" >> /tmp/basic_append.txt' \
    '/tmp/basic_append.txt' \
    'line1
line2'

# Test variable expansion in redirection
run_test "Variable in redirection" \
    'msg="hello world"; echo "$msg" > /tmp/basic_var.txt' \
    '/tmp/basic_var.txt' \
    'hello world'

# Create input file for input redirection test
echo "input content" > /tmp/basic_input.txt

echo "Test: Input redirection"
echo "Input: cat < /tmp/basic_input.txt"
output=$(echo 'cat < /tmp/basic_input.txt' | $SHELL_CMD 2>&1)
expected="input content"
echo "Expected: '$expected'"
echo "Actual: '$output'"

TOTAL=$((TOTAL + 1))
if [ "$output" = "$expected" ]; then
    echo "✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "✗ FAILED"
    FAILED=$((FAILED + 1))
fi
rm -f /tmp/basic_input.txt
echo

# Test error redirection
echo "Test: Error redirection"
echo "Input: nonexistent_cmd 2> /tmp/basic_err.txt"
rm -f /tmp/basic_err.txt
echo 'nonexistent_cmd 2> /tmp/basic_err.txt' | $SHELL_CMD >/dev/null 2>&1

TOTAL=$((TOTAL + 1))
if [ -f /tmp/basic_err.txt ] && [ -s /tmp/basic_err.txt ]; then
    echo "Error file created with content"
    echo "✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "Error file not created or empty"
    echo "✗ FAILED"
    FAILED=$((FAILED + 1))
fi
rm -f /tmp/basic_err.txt
echo

# Test here string
echo "Test: Here string"
echo "Input: cat <<< \"here string test\""
output=$(echo 'cat <<< "here string test"' | $SHELL_CMD 2>&1)
expected="here string test"
echo "Expected: '$expected'"
echo "Actual: '$output'"

TOTAL=$((TOTAL + 1))
if [ "$output" = "$expected" ]; then
    echo "✓ PASSED"
    PASSED=$((PASSED + 1))
else
    echo "✗ FAILED"
    FAILED=$((FAILED + 1))
fi
echo

echo "=== RESULTS ==="
echo "Total tests: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo "SUCCESS: All basic redirection tests passed!"
    exit 0
else
    success_rate=$((PASSED * 100 / TOTAL))
    echo "SUCCESS RATE: ${success_rate}%"

    if [ $success_rate -ge 80 ]; then
        echo "GOOD: Most redirection features are working"
    else
        echo "NEEDS WORK: Some redirection features need fixes"
    fi
    exit $FAILED
fi

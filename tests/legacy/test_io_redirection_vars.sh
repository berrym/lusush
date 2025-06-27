#!/bin/bash

# Test script for I/O redirection variable expansion functionality
# This tests the fix for variables not expanding in redirection target filenames

echo "==============================================================================="
echo "I/O REDIRECTION VARIABLE EXPANSION TEST SUITE"
echo "==============================================================================="

LUSUSH_SHELL="./builddir/lusush"
TEST_COUNT=0
PASS_COUNT=0

# Test function
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo "Test $TEST_COUNT: $test_name"

    # Run the test command in lusush
    actual_output=$(echo "$test_command" | $LUSUSH_SHELL 2>&1)

    if [ "$actual_output" = "$expected_output" ]; then
        echo "✓ PASSED"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "✗ FAILED"
        echo "  Expected: '$expected_output'"
        echo "  Actual:   '$actual_output'"
    fi
    echo
}

# Clean up any existing test files
rm -f /tmp/lusush_test_* 2>/dev/null

echo "Testing I/O redirection variable expansion..."
echo

# Test 1: Simple variable expansion in output redirection
run_test "Simple variable expansion" \
    'TESTFILE="/tmp/lusush_test_simple"; echo "hello" > $TESTFILE; cat $TESTFILE' \
    "hello"

# Test 2: Variable expansion in function context
run_test "Function parameter expansion" \
    'write_to_file() { echo "content" > /tmp/lusush_test_$1; cat /tmp/lusush_test_$1; }; write_to_file func' \
    "content"

# Test 3: Variable expansion in append redirection
run_test "Variable expansion with append" \
    'OUTFILE="/tmp/lusush_test_append"; echo "line1" > $OUTFILE; echo "line2" >> $OUTFILE; cat $OUTFILE' \
    "line1
line2"

# Test 4: Variable expansion with here string
run_test "Variable expansion with here string" \
    'VAR="test content"; cat <<< $VAR' \
    "test content"

# Test 5: Complex variable expansion
run_test "Complex variable expansion" \
    'PREFIX="lusush"; SUFFIX="redir"; echo "data" > /tmp/${PREFIX}_${SUFFIX}_unique; cat /tmp/${PREFIX}_${SUFFIX}_unique' \
    "data"

# Test 6: Function with multiple parameters
run_test "Function with multiple parameters" \
    'create_named_file() { echo "$2" > /tmp/lusush_test_$1; cat /tmp/lusush_test_$1; }; create_named_file multi "multiple params"' \
    "multiple params"

# Test 7: Variable expansion in error redirection
run_test "Variable expansion in error redirection" \
    'ERRFILE="/tmp/lusush_test_error"; echo "error message" >&2 2> $ERRFILE; cat $ERRFILE' \
    "error message"

# Clean up test files
rm -f /tmp/lusush_test_* /tmp/lusush_redir_* 2>/dev/null

echo "==============================================================================="
echo "TEST RESULTS SUMMARY"
echo "==============================================================================="
echo "Total tests: $TEST_COUNT"
echo "Passed: $PASS_COUNT"
echo "Failed: $((TEST_COUNT - PASS_COUNT))"
echo "Success rate: $(( (PASS_COUNT * 100) / TEST_COUNT ))%"

if [ $PASS_COUNT -eq $TEST_COUNT ]; then
    echo "✓ ALL TESTS PASSED - I/O redirection variable expansion is working correctly!"
    exit 0
else
    echo "✗ SOME TESTS FAILED - I/O redirection variable expansion needs more work"
    exit 1
fi

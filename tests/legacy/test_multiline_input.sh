#!/bin/bash

# Comprehensive test suite for multiline input handling in Lusush shell
# This tests all forms of multiline input that should be supported

set -e  # Exit on any error

SHELL_PATH="./builddir/lusush"
TEST_COUNT=0
PASSED_COUNT=0
FAILED_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test result tracking
log_test() {
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${YELLOW}Test $TEST_COUNT: $1${NC}"
}

pass_test() {
    PASSED_COUNT=$((PASSED_COUNT + 1))
    echo -e "${GREEN}✓ PASSED${NC}"
    echo
}

fail_test() {
    FAILED_COUNT=$((FAILED_COUNT + 1))
    echo -e "${RED}✗ FAILED: $1${NC}"
    echo
}

# Test multiline input by feeding commands via stdin
test_multiline() {
    local test_name="$1"
    local input="$2"
    local expected_pattern="$3"

    log_test "$test_name"

    # Feed input to shell and capture output
    local output
    output=$(echo -e "$input" | $SHELL_PATH 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        if [[ "$output" =~ $expected_pattern ]]; then
            pass_test
        else
            fail_test "Output didn't match expected pattern. Got: '$output'"
        fi
    else
        fail_test "Shell exited with error code $exit_code. Output: '$output'"
    fi
}

# Test that shell correctly handles incomplete input (should not execute prematurely)
test_incomplete_input() {
    local test_name="$1"
    local input="$2"

    log_test "$test_name"

    # Use timeout to test that shell waits for complete input
    # If shell is waiting for more input, timeout will kill it
    local output
    output=$(timeout 2s bash -c "echo -e '$input' | $SHELL_PATH" 2>&1)
    local exit_code=$?

    # Timeout should occur (exit code 124) indicating shell is waiting for more input
    if [ $exit_code -eq 124 ]; then
        pass_test
    else
        fail_test "Shell should have waited for more input but didn't. Exit: $exit_code, Output: '$output'"
    fi
}

echo "=== Lusush Multiline Input Test Suite ==="
echo "Testing shell: $SHELL_PATH"
echo

# Ensure the shell binary exists
if [ ! -x "$SHELL_PATH" ]; then
    echo -e "${RED}Error: Shell binary not found at $SHELL_PATH${NC}"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

# Test 1: Basic backslash continuation
test_multiline "Backslash continuation" \
    "echo hello \\\nworld" \
    "hello world"

# Test 2: IF statement multiline
test_multiline "IF statement multiline" \
    "if true\nthen\n    echo success\nfi" \
    "success"

# Test 3: FOR loop multiline
test_multiline "FOR loop multiline" \
    "for i in 1 2 3\ndo\n    echo item \$i\ndone" \
    "item 1.*item 2.*item 3"

# Test 4: WHILE loop multiline
test_multiline "WHILE loop with variable" \
    "x=1\nwhile [ \$x -le 2 ]\ndo\n    echo loop \$x\n    x=\$((x + 1))\ndone" \
    "loop 1.*loop 2"

# Test 5: Nested control structures
test_multiline "Nested IF in FOR loop" \
    "for i in 1 2\ndo\n    if [ \$i -eq 1 ]\n    then\n        echo first\n    else\n        echo second\n    fi\ndone" \
    "first.*second"

# Test 6: Command group with braces
test_multiline "Command group with braces" \
    "{\n    echo line1\n    echo line2\n}" \
    "line1.*line2"

# Test 7: Subshell with parentheses
test_multiline "Subshell with parentheses" \
    "(\n    echo subshell\n    echo test\n)" \
    "subshell.*test"

# Test 8: Pipeline continuation
test_multiline "Pipeline continuation" \
    "echo 'line1\nline2\nline3' |\ngrep line |\nwc -l" \
    "3"

# Test 9: Variable assignment with multiline value
test_multiline "Multiline quoted string" \
    "msg=\"line1\nline2\"\necho \"\$msg\"" \
    "line1.*line2"

# Test 10: Arithmetic expansion multiline
test_multiline "Arithmetic expansion multiline" \
    "result=\$((\n    5 +\n    3\n))\necho \$result" \
    "8"

# Test 11: Command substitution multiline
test_multiline "Command substitution multiline" \
    "files=\$(\n    ls /\n)\necho \"Found files\"" \
    "Found files"

# Test 12: Function definition (if supported)
test_multiline "Function definition" \
    "myfunc() {\n    echo \"in function\"\n    echo \"parameter: \$1\"\n}\nmyfunc test" \
    "in function.*parameter: test"

# Test 13: Case statement (if supported)
test_multiline "Case statement" \
    "value=apple\ncase \$value in\n    apple)\n        echo fruit\n        ;;\n    *)\n        echo unknown\n        ;;\nesac" \
    "fruit"

# Test 14: Here document
test_multiline "Here document" \
    "cat <<EOF\nThis is line 1\nThis is line 2\nEOF" \
    "This is line 1.*This is line 2"

# Test 15: Here document with variable expansion
test_multiline "Here document with variables" \
    "name=world\ncat <<EOF\nHello \$name\nHow are you?\nEOF" \
    "Hello world.*How are you"

# Test 16: Complex nested structure
test_multiline "Complex nested structure" \
    "for i in 1 2\ndo\n    if [ \$i -eq 1 ]\n    then\n        echo \"Processing \$i\" |\n        while read line\n        do\n            echo \"Got: \$line\"\n        done\n    fi\ndone" \
    "Got: Processing 1"

# Test 17: Multiple commands with semicolons and newlines
test_multiline "Mixed semicolons and newlines" \
    "x=1; y=2\nz=\$((x + y))\necho \"Result: \$z\"" \
    "Result: 3"

# Test 18: Escaped quotes in multiline
test_multiline "Escaped quotes multiline" \
    "echo \"This is a \\\"quoted\\\" \\\nstring on multiple lines\"" \
    "This is a \"quoted\".*string on multiple lines"

# Incomplete input tests (these should timeout, indicating shell is waiting)
echo "=== Testing Incomplete Input Handling ==="

# Test 19: Incomplete IF statement
test_incomplete_input "Incomplete IF statement" \
    "if true\nthen\n    echo test"

# Test 20: Incomplete FOR loop
test_incomplete_input "Incomplete FOR loop" \
    "for i in 1 2\ndo\n    echo \$i"

# Test 21: Unclosed brace group
test_incomplete_input "Unclosed brace group" \
    "{\n    echo test\n    echo more"

# Test 22: Unclosed parentheses
test_incomplete_input "Unclosed parentheses" \
    "(\n    echo test\n    echo more"

# Test 23: Unclosed single quote
test_incomplete_input "Unclosed single quote" \
    "echo 'this is\na multiline"

# Test 24: Unclosed double quote
test_incomplete_input "Unclosed double quote" \
    "echo \"this is\na multiline"

# Test 25: Incomplete here document
test_incomplete_input "Incomplete here document" \
    "cat <<EOF\nThis is line 1\nThis is line 2"

# Test 26: Backslash continuation
test_incomplete_input "Backslash continuation" \
    "echo hello \\"

# Final results
echo "=== Test Results ==="
echo -e "Total tests: $TEST_COUNT"
echo -e "${GREEN}Passed: $PASSED_COUNT${NC}"
echo -e "${RED}Failed: $FAILED_COUNT${NC}"

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed! ✗${NC}"
    exit 1
fi

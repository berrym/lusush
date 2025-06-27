#!/bin/bash

# I/O Redirection Test Suite
# Tests basic and advanced I/O redirection functionality

SHELL_CMD="./builddir/lusush"
PASSED=0
FAILED=0
TOTAL=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== I/O REDIRECTION TEST SUITE ==="
echo "Testing basic and advanced I/O redirection functionality"
echo "Shell: $SHELL_CMD"
echo

# Test function
run_test() {
    local test_name="$1"
    local input="$2"
    local expected="$3"
    local cleanup_cmd="$4"

    TOTAL=$((TOTAL + 1))

    echo "Test: $test_name"
    echo "Input: $input"
    echo "Expected: '$expected'"

    # Run the test
    output=$(echo "$input" | $SHELL_CMD 2>&1)
    exit_code=$?

    echo "Output: '$output'"
    echo "Exit code: $exit_code"

    if [ "$output" = "$expected" ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED=$((FAILED + 1))
    fi

    # Run cleanup if provided
    if [ -n "$cleanup_cmd" ]; then
        eval "$cleanup_cmd"
    fi

    echo
}

# File-based test function
run_file_test() {
    local test_name="$1"
    local input="$2"
    local test_file="$3"
    local expected_content="$4"

    TOTAL=$((TOTAL + 1))

    echo "Test: $test_name"
    echo "Input: $input"
    echo "Expected file content: '$expected_content'"

    # Clean up any existing test file
    rm -f "$test_file"

    # Run the test
    echo "$input" | $SHELL_CMD >/dev/null 2>&1
    exit_code=$?

    # Check file content
    if [ -f "$test_file" ]; then
        actual_content=$(cat "$test_file")
        echo "Actual file content: '$actual_content'"

        if [ "$actual_content" = "$expected_content" ]; then
            echo -e "${GREEN}✓ PASSED${NC}"
            PASSED=$((PASSED + 1))
        else
            echo -e "${RED}✗ FAILED${NC}"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "File not created"
        echo -e "${RED}✗ FAILED${NC}"
        FAILED=$((FAILED + 1))
    fi

    # Cleanup
    rm -f "$test_file"
    echo
}

echo "=== BASIC OUTPUT REDIRECTION ==="

run_file_test "Simple output redirection" \
    'echo "hello world" > /tmp/test_output.txt' \
    '/tmp/test_output.txt' \
    'hello world'

run_file_test "Output redirection with variables" \
    'msg="test message"; echo "$msg" > /tmp/test_var.txt' \
    '/tmp/test_var.txt' \
    'test message'

run_file_test "Multiple commands with redirection" \
    'echo "line1" > /tmp/test_multi.txt; echo "line2" >> /tmp/test_multi.txt' \
    '/tmp/test_multi.txt' \
    'line1
line2'

echo "=== APPEND REDIRECTION ==="

run_file_test "Append redirection" \
    'echo "first" > /tmp/test_append.txt; echo "second" >> /tmp/test_append.txt' \
    '/tmp/test_append.txt' \
    'first
second'

run_file_test "Multiple appends" \
    'echo "1" > /tmp/test_multi_append.txt; echo "2" >> /tmp/test_multi_append.txt; echo "3" >> /tmp/test_multi_append.txt' \
    '/tmp/test_multi_append.txt' \
    '1
2
3'

echo "=== INPUT REDIRECTION ==="

# Create test input file
echo "input line 1
input line 2" > /tmp/test_input.txt

run_test "Input redirection" \
    'cat < /tmp/test_input.txt' \
    'input line 1
input line 2' \
    'rm -f /tmp/test_input.txt'

echo "=== ERROR REDIRECTION ==="

run_test "Stderr redirection to /dev/null" \
    'nonexistent_command 2>/dev/null || echo "command failed"' \
    'command failed'

run_file_test "Stderr redirection to file" \
    'nonexistent_command 2>/tmp/test_stderr.txt || true' \
    '/tmp/test_stderr.txt' \
    'nonexistent_command: No such file or directory'

run_file_test "Stderr append redirection" \
    'echo "error1" >&2 2>/tmp/test_stderr_append.txt; echo "error2" >&2 2>>/tmp/test_stderr_append.txt' \
    '/tmp/test_stderr_append.txt' \
    'error1
error2'

echo "=== COMBINED REDIRECTION ==="

run_file_test "Stdout and stderr to same file" \
    'echo "output" > /tmp/test_combined.txt; echo "error" >&2 2>>/tmp/test_combined.txt' \
    '/tmp/test_combined.txt' \
    'output
error'

run_test "Stdout and stderr separation" \
    'echo "output" > /tmp/test_out.txt; echo "error" >&2 2>/tmp/test_err.txt; cat /tmp/test_out.txt; cat /tmp/test_err.txt' \
    'output
error' \
    'rm -f /tmp/test_out.txt /tmp/test_err.txt'

echo "=== HERE DOCUMENTS ==="

run_test "Basic here document" \
    'cat <<EOF
This is a here document
with multiple lines
EOF' \
    'This is a here document
with multiple lines'

run_test "Here document with variables" \
    'name="world"; cat <<EOF
Hello, $name!
This is a test.
EOF' \
    'Hello, world!
This is a test.'

run_test "Here document with quoted delimiter" \
    'cat <<"END"
This $should not expand
END' \
    'This $should not expand'

echo "=== HERE DOCUMENTS WITH TAB STRIPPING ==="

run_test "Here document with tab stripping" \
    'cat <<-EOF
	This line has a leading tab
	So does this one
EOF' \
    'This line has a leading tab
So does this one'

echo "=== HERE STRINGS ==="

run_test "Basic here string" \
    'cat <<<"Hello, here string!"' \
    'Hello, here string!'

run_test "Here string with variables" \
    'msg="test"; cat <<<"Message: $msg"' \
    'Message: test'

echo "=== ADVANCED PATTERNS ==="

run_test "Redirection with pipes" \
    'echo "data" | cat > /tmp/test_pipe.txt; cat /tmp/test_pipe.txt' \
    'data' \
    'rm -f /tmp/test_pipe.txt'

run_test "Multiple redirections" \
    'echo "out" > /tmp/test_multi_redir.txt 2>&1; cat /tmp/test_multi_redir.txt' \
    'out' \
    'rm -f /tmp/test_multi_redir.txt'

run_test "Redirection in functions" \
    'redirect_func() { echo "function output" > /tmp/test_func.txt; }; redirect_func; cat /tmp/test_func.txt' \
    'function output' \
    'rm -f /tmp/test_func.txt'

echo "=== ERROR CASES ==="

run_test "Redirection to non-writable location" \
    'echo "test" > /root/cannot_write 2>/dev/null || echo "permission denied"' \
    'permission denied'

run_test "Input from non-existent file" \
    'cat < /nonexistent/file 2>/dev/null || echo "file not found"' \
    'file not found'

echo "=== RESULTS ==="
echo "Total tests: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    success_rate=100
    echo -e "${GREEN}Success rate: 100%${NC}"
    echo -e "${GREEN}🎉 ALL TESTS PASSED${NC}"
    echo "I/O redirection is working perfectly!"
else
    success_rate=$((PASSED * 100 / TOTAL))
    echo -e "${YELLOW}Success rate: ${success_rate}%${NC}"
    if [ $success_rate -ge 90 ]; then
        echo -e "${GREEN}✅ EXCELLENT FUNCTIONALITY${NC}"
        echo "I/O redirection is working very well."
    elif [ $success_rate -ge 70 ]; then
        echo -e "${YELLOW}⚠ GOOD FUNCTIONALITY${NC}"
        echo "I/O redirection is working well with some issues."
    else
        echo -e "${RED}❌ NEEDS IMPROVEMENT${NC}"
        echo "I/O redirection needs significant work."
    fi
fi

echo
echo "This test validates:"
echo "  • Basic output redirection (>)"
echo "  • Append redirection (>>)"
echo "  • Input redirection (<)"
echo "  • Error redirection (2>)"
echo "  • Combined redirection patterns"
echo "  • Here documents (<<)"
echo "  • Here documents with tab stripping (<<-)"
echo "  • Here strings (<<<)"
echo "  • Advanced redirection patterns"
echo "  • Error handling and edge cases"

exit $FAILED

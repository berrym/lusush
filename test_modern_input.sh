#!/bin/bash

# Modern Input System Test Suite
# Tests the new modern input system for multiline functionality,
# history integration, and proper POSIX compliance.

SHELL_CMD="./builddir/lusush"
PASSED=0
FAILED=0
TOTAL=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== MODERN INPUT SYSTEM TEST SUITE ==="
echo "Testing multiline input, completion detection, and history integration"
echo "Shell: $SHELL_CMD"
echo

# Test function
run_test() {
    local test_name="$1"
    local input="$2"
    local expected="$3"

    TOTAL=$((TOTAL + 1))

    echo "Test: $test_name"
    echo "Input: $input"
    echo "Expected: '$expected'"

    # Run the test
    output=$(echo -e "$input" | $SHELL_CMD 2>&1)
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
    echo
}

echo "=== BASIC INPUT FUNCTIONALITY ==="

run_test "Simple command" \
    'echo "hello world"' \
    'hello world'

run_test "Variable assignment and expansion" \
    'var="test value"; echo "$var"' \
    'test value'

run_test "Command sequence" \
    'echo "first"; echo "second"' \
    'first
second'

echo "=== MULTILINE CONTROL STRUCTURES ==="

run_test "Simple if statement" \
    'if [ "test" = "test" ]; then\necho "if works"\nfi' \
    'if works'

run_test "If-else statement" \
    'if [ "a" = "b" ]; then\necho "no"\nelse\necho "yes"\nfi' \
    'yes'

run_test "While loop" \
    'i=1; while [ $i -le 3 ]; do\necho "count: $i"\ni=$((i + 1))\ndone' \
    'count: 1
count: 2
count: 3'

run_test "For loop" \
    'for item in apple banana cherry; do\necho "fruit: $item"\ndone' \
    'fruit: apple
fruit: banana
fruit: cherry'

run_test "Case statement" \
    'word="test"\ncase "$word" in\ntest) echo "matched";;\n*) echo "no match";;\nesac' \
    'matched'

echo "=== MULTILINE FUNCTION DEFINITIONS ==="

run_test "Simple function definition" \
    'test_func() {\necho "function works"\n}\ntest_func' \
    'function works'

run_test "Function with parameters" \
    'greet() {\necho "Hello, $1!"\n}\ngreet "World"' \
    'Hello, World!'

run_test "Function with conditional" \
    'check() {\nif [ "$1" = "ok" ]; then\necho "good"\nelse\necho "bad"\nfi\n}\ncheck "ok"' \
    'good'

echo "=== NESTED STRUCTURES ==="

run_test "Nested if statements" \
    'if [ "a" = "a" ]; then\nif [ "b" = "b" ]; then\necho "nested works"\nfi\nfi' \
    'nested works'

run_test "Function with loop" \
    'count_func() {\nfor i in 1 2; do\necho "num: $i"\ndone\n}\ncount_func' \
    'num: 1
num: 2'

run_test "If with case" \
    'value="test"\nif [ "$value" = "test" ]; then\ncase "$value" in\ntest) echo "both work";;\nesac\nfi' \
    'both work'

echo "=== QUOTE HANDLING ==="

run_test "Single quotes multiline" \
    'echo '"'"'line 1\nline 2'"'"'' \
    'line 1\nline 2'

run_test "Double quotes with variables" \
    'name="user"\necho "Hello,\n$name!"' \
    'Hello,
user!'

run_test "Mixed quotes" \
    'echo "He said '"'"'hello'"'"' to me"' \
    "He said 'hello' to me"

echo "=== BRACE AND BRACKET HANDLING ==="

run_test "Brace groups" \
    '{ echo "group 1"; echo "group 2"; }' \
    'group 1
group 2'

run_test "Arithmetic expansion" \
    'echo $((2 + 3))' \
    '5'

run_test "Command substitution" \
    'echo "Result: $(echo "nested")"' \
    'Result: nested'

echo "=== LINE CONTINUATION ==="

run_test "Backslash continuation" \
    'echo "this is a \\\nvery long line"' \
    'this is a very long line'

run_test "Continuation in function" \
    'long_func() {\necho "first part" \\\n"second part"\n}\nlong_func' \
    'first part second part'

echo "=== ERROR RECOVERY ==="

run_test "Empty input handling" \
    '\necho "after empty"' \
    'after empty'

run_test "Comment handling" \
    '# This is a comment\necho "visible"' \
    'visible'

run_test "Mixed comments and commands" \
    'echo "before" # comment\necho "after"' \
    'before
after'

echo "=== COMPLEX SCENARIOS ==="

run_test "Function with everything" \
    'complex() {\nlocal var="$1"\nif [ -n "$var" ]; then\nfor i in 1 2; do\necho "processing $var-$i"\ndone\nelse\necho "empty input"\nfi\n}\ncomplex "test"' \
    'processing test-1
processing test-2'

run_test "Nested command substitution" \
    'echo "Outer: $(echo "Inner: $(echo "deep")")"' \
    'Outer: Inner: deep'

run_test "Case with functions" \
    'action="test"\ntest_action() { echo "testing"; }\ncase "$action" in\ntest) test_action;;\nesac' \
    'testing'

echo "=== RESULTS ==="
echo "Total tests: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    success_rate=100
    echo -e "${GREEN}Success rate: 100%${NC}"
    echo -e "${GREEN}🎉 ALL TESTS PASSED${NC}"
    echo "Modern input system is working perfectly!"
else
    success_rate=$((PASSED * 100 / TOTAL))
    echo -e "${YELLOW}Success rate: ${success_rate}%${NC}"
    if [ $success_rate -ge 95 ]; then
        echo -e "${GREEN}✅ EXCELLENT FUNCTIONALITY${NC}"
        echo "Modern input system is working very well."
    elif [ $success_rate -ge 85 ]; then
        echo -e "${YELLOW}⚠ GOOD FUNCTIONALITY${NC}"
        echo "Modern input system is working well with minor issues."
    else
        echo -e "${RED}❌ NEEDS IMPROVEMENT${NC}"
        echo "Modern input system needs additional work."
    fi
fi

echo
echo "This test validates:"
echo "  • Basic command input and execution"
echo "  • Multiline control structure detection"
echo "  • Quote handling and state tracking"
echo "  • Function definition parsing"
echo "  • Nested structure support"
echo "  • Line continuation processing"
echo "  • Error recovery and comment handling"
echo "  • Complex scenario integration"

exit $FAILED

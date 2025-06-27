#!/bin/bash

# Basic Function Test Suite for Lusush Shell
# Tests fundamental function definition and calling functionality

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SHELL="./builddir/lusush"
PASSED_COUNT=0
FAILED_COUNT=0

# Test function
test_function() {
    local description="$1"
    local input="$2"
    local expected="$3"

    echo "Test: $description"
    echo "Input: $input"
    echo "Expected: '$expected'"

    output=$(echo "$input" | $SHELL 2>&1)
    exit_code=$?

    echo "Output: '$output'"
    echo "Exit code: $exit_code"

    if [ "$output" = "$expected" ] && [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        ((PASSED_COUNT++))
    else
        echo -e "${RED}✗ FAILED${NC}"
        ((FAILED_COUNT++))
    fi
    echo
}

echo "=== BASIC FUNCTION TEST SUITE ==="
echo "Testing fundamental function definition and calling"
echo "Shell: $SHELL"
echo

echo "=== FUNCTION DEFINITION SYNTAX ==="
test_function "Simple function definition and call" \
    'simple() { echo "hello"; }; simple' \
    "hello"

test_function "Function keyword syntax" \
    'function greet() { echo "greetings"; }; greet' \
    "greetings"

test_function "Function without body calls" \
    'empty() { }; echo "defined"; empty; echo "done"' \
    "defined
done"

echo "=== FUNCTION ARGUMENTS ==="
test_function "Single argument" \
    'single() { echo "arg: $1"; }; single hello' \
    "arg: hello"

test_function "Multiple arguments" \
    'multi() { echo "$1-$2"; }; multi first second' \
    "first-second"

test_function "Three arguments" \
    'three() { echo "$1 $2 $3"; }; three one two three' \
    "one two three"

test_function "Function with no arguments" \
    'noargs() { echo "no args: $1"; }; noargs' \
    "no args: "

echo "=== FUNCTION BODY COMPLEXITY ==="
test_function "Multiple commands in function" \
    'multi_cmd() { echo "first"; echo "second"; }; multi_cmd' \
    "first
second"

test_function "Function with variable assignment" \
    'assign() { local_var="inside"; echo "$local_var"; }; assign' \
    "inside"

test_function "Function with conditional" \
    'cond() { if [ "$1" = "test" ]; then echo "match"; fi; }; cond test' \
    "match"

echo "=== FUNCTION SCOPE ==="
test_function "Function parameter isolation" \
    'scope1() { echo "func: $1"; }; scope1 arg1; echo "outside: $1"' \
    "func: arg1
outside: "

test_function "Variable assignment in function" \
    'scope2() { var="func_value"; echo "$var"; }; scope2; echo "outside: $var"' \
    "func_value
outside: func_value"

echo "=== FUNCTION OVERRIDING ==="
test_function "Function redefinition" \
    'override() { echo "first"; }; override; override() { echo "second"; }; override' \
    "first
second"

echo "=== ERROR HANDLING ==="
test_function "Calling undefined function" \
    'undefined_func 2>/dev/null || echo "error"' \
    "error"

test_function "Function with complex name" \
    'my_func_123() { echo "complex name"; }; my_func_123' \
    "complex name"

echo "=== RESULTS ==="
echo "Total tests: $((PASSED_COUNT + FAILED_COUNT))"
echo "Passed: $PASSED_COUNT"
echo "Failed: $FAILED_COUNT"
echo "Success rate: $(( PASSED_COUNT * 100 / (PASSED_COUNT + FAILED_COUNT) ))%"

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL FUNCTION TESTS PASSED!${NC}"
    echo "Function implementation is working correctly!"
elif [ $PASSED_COUNT -gt 0 ]; then
    echo -e "${YELLOW}⚠ PARTIAL FUNCTIONALITY${NC}"
    echo "Some function features are working."
    echo "Failed tests indicate areas needing improvement."
else
    echo -e "${RED}❌ FUNCTIONS NOT WORKING${NC}"
    echo "Function implementation needs significant work."
fi

echo
echo "This test helps identify specific issues with:"
echo "  • Function definition syntax (standard and keyword forms)"
echo "  • Function argument passing and parameter access"
echo "  • Function body execution (simple and complex)"
echo "  • Function scope and variable isolation"
echo "  • Function redefinition and error handling"

exit $FAILED_COUNT

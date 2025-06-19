#!/bin/bash

# Comprehensive test script for lusush POSIX options and set builtin
# Usage: ./test_posix_options.sh

LUSUSH="./builddir/lusush"
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Testing lusush POSIX options and set builtin ==="
echo

# Helper functions
run_test() {
    local description="$1"
    local command="$2"
    local expected_output="$3"
    local expected_exit_code="${4:-0}"
    
    TEST_COUNT=$((TEST_COUNT + 1))
    printf "Test %d: %s\n" "$TEST_COUNT" "$description"
    
    # Run the command and capture output and exit code
    output=$(eval "$command" 2>&1)
    exit_code=$?
    
    # Check exit code
    if [ "$exit_code" -ne "$expected_exit_code" ]; then
        printf "${RED}FAIL${NC}: Expected exit code %d, got %d\n" "$expected_exit_code" "$exit_code"
        printf "Command: %s\n" "$command"
        printf "Output: %s\n" "$output"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo
        return 1
    fi
    
    # Check output if provided
    if [ -n "$expected_output" ]; then
        if echo "$output" | grep -q "$expected_output"; then
            printf "${GREEN}PASS${NC}\n"
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            printf "${RED}FAIL${NC}: Output doesn't contain expected string\n"
            printf "Expected: %s\n" "$expected_output"
            printf "Got: %s\n" "$output"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    else
        printf "${GREEN}PASS${NC}\n"
        PASS_COUNT=$((PASS_COUNT + 1))
    fi
    echo
}

# Test 1: Help option (-h)
run_test "Help option (-h)" "$LUSUSH -h" "Usage:"

# Test 2: Version option (-V)
run_test "Version option (-V)" "$LUSUSH -V" "lusush"

# Test 3: Command mode (-c)
run_test "Command mode (-c) with echo" "$LUSUSH -c 'echo Hello World'" "Hello World"

# Test 4: Verbose mode (-v)
run_test "Verbose mode (-v) with command" "$LUSUSH -v -c 'echo test'" "echo test"

# Test 5: Syntax check mode (-n)
run_test "Syntax check mode (-n) with valid syntax" "$LUSUSH -n -c 'echo test'" ""

# Test 6: Syntax check mode (-n) with invalid syntax
run_test "Syntax check mode (-n) with invalid syntax" "$LUSUSH -n -c 'echo test |'" "" 2

# Test 7: Exit on error (-e) with successful command
run_test "Exit on error (-e) with successful command" "$LUSUSH -e -c 'true; echo success'" "success"

# Test 8: Exit on error (-e) with failing command
run_test "Exit on error (-e) with failing command" "$LUSUSH -e -c 'false; echo should not print'" "" 1

# Test 9: Trace execution (-x)
run_test "Trace execution (-x)" "$LUSUSH -x -c 'echo traced'" "+ echo traced"

# Test 10: Unset variable error (-u)
run_test "Unset variable error (-u) with undefined var" "$LUSUSH -u -c 'echo \$UNDEFINED_VAR'" "unset variable" 1

# Test 11: No globbing (-f)
run_test "No globbing (-f)" "$LUSUSH -f -c 'echo *.nonexistent'" "*.nonexistent"

# Test 12: Interactive mode detection (-i)
run_test "Interactive mode forced (-i)" "$LUSUSH -i -c 'echo interactive'" "interactive"

# Test 13: set builtin - display all variables
run_test "set builtin - display variables" "$LUSUSH -c 'set'" ""

# Test 14: set builtin - enable option
run_test "set builtin - enable verbose" "$LUSUSH -c 'set -v; echo test'" "set: enabled -v"

# Test 15: set builtin - disable option
run_test "set builtin - disable verbose" "$LUSUSH -c 'set -v; set +v; echo test'" "test"

# Test 16: set builtin - multiple options
run_test "set builtin - multiple options" "$LUSUSH -c 'set -ev; false'" "" 1

# Test 17: set builtin - invalid option
run_test "set builtin - invalid option" "$LUSUSH -c 'set -z'" "invalid option" 1

# Test 18: Combined options
run_test "Combined options (-ex)" "$LUSUSH -ex -c 'true; echo combined'" "+ true"

# Test 19: Login shell option (-l)
run_test "Login shell option (-l)" "$LUSUSH -l -c 'echo login'" "login"

# Test 20: Hash commands option (-h in set)
run_test "Hash commands via set" "$LUSUSH -c 'set -h; echo hash enabled'" "hash enabled"

# Summary
echo "=== Test Summary ==="
printf "Total tests: %d\n" "$TEST_COUNT"
printf "${GREEN}Passed: %d${NC}\n" "$PASS_COUNT"
printf "${RED}Failed: %d${NC}\n" "$FAIL_COUNT"

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo
    printf "${GREEN}All tests passed!${NC}\n"
    exit 0
else
    echo
    printf "${RED}Some tests failed.${NC}\n"
    exit 1
fi

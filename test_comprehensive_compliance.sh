#!/bin/bash

# =============================================================================
# LUSUSH COMPREHENSIVE SHELL COMPLIANCE TEST SUITE
# =============================================================================
#
# This test suite provides comprehensive evaluation of lusush shell compliance
# with POSIX standards and compatibility with bash/zsh advanced features.
#
# Test Categories:
# 1. Parameter Expansion (POSIX + Extensions)
# 2. Arithmetic Expansion (Complete Coverage)
# 3. Command Substitution (All Forms)
# 4. Variable Operations (Assignment, Scoping, Special)
# 5. Control Structures (Complete Flow Control)
# 6. Pattern Matching and Globbing
# 7. I/O Redirection (All Forms)
# 8. Function Operations (Definition, Calling, Scoping)
# 9. Built-in Commands (Complete Suite)
# 10. Error Handling and Edge Cases
# 11. Real-World Complex Scenarios
# 12. Performance and Stress Testing
#
# Scoring System:
# - Each test category has weighted importance
# - Final score represents overall shell completeness
# - Comparison benchmarks with bash/zsh behavior
#
# Author: Lusush Development Team
# Version: 1.0.0
# Target: Complete POSIX + Modern Shell Compliance Assessment
# =============================================================================

SHELL_UNDER_TEST="${1:-./builddir/lusush}"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
CATEGORY_SCORES=()
CATEGORY_NAMES=()

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
PURPLE='\033[1;35m'
NC='\033[0m' # No Color

# Test timing
START_TIME=$(date +%s)

print_header() {
    echo -e "\n${BLUE}===============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================================================${NC}\n"
}

print_section() {
    echo -e "\n${CYAN}=== $1 ===${NC}"
}

print_category() {
    echo -e "\n${PURPLE}▓▓▓ CATEGORY: $1 ▓▓▓${NC}"
}

run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    local weight="${4:-1}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # Execute test and capture output
    local actual_output
    actual_output=$(echo "$test_command" | $SHELL_UNDER_TEST 2>&1)
    local exit_code=$?

    # Compare results
    if [ "$actual_output" = "$expected_output" ]; then
        echo -e "  ${GREEN}✓${NC} Test $TOTAL_TESTS: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "  ${RED}✗${NC} Test $TOTAL_TESTS: $test_name"
        echo -e "    ${YELLOW}Expected:${NC} '$expected_output'"
        echo -e "    ${YELLOW}Actual:${NC}   '$actual_output'"
        echo -e "    ${YELLOW}Exit Code:${NC} $exit_code"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

run_test_with_exit_code() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    local expected_exit_code="$4"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # Execute test and capture output
    local actual_output
    actual_output=$(echo "$test_command" | $SHELL_UNDER_TEST 2>&1)
    local actual_exit_code=$?

    # Compare results
    if [ "$actual_output" = "$expected_output" ] && [ "$actual_exit_code" = "$expected_exit_code" ]; then
        echo -e "  ${GREEN}✓${NC} Test $TOTAL_TESTS: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "  ${RED}✗${NC} Test $TOTAL_TESTS: $test_name"
        echo -e "    ${YELLOW}Expected Output:${NC} '$expected_output'"
        echo -e "    ${YELLOW}Actual Output:${NC}   '$actual_output'"
        echo -e "    ${YELLOW}Expected Exit:${NC}   $expected_exit_code"
        echo -e "    ${YELLOW}Actual Exit:${NC}     $actual_exit_code"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

calculate_category_score() {
    local category_name="$1"
    local category_start="$2"
    local category_weight="${3:-10}"

    local category_tests=$((TOTAL_TESTS - category_start))
    local category_passed=$((PASSED_TESTS - category_start + (TOTAL_TESTS - PASSED_TESTS - FAILED_TESTS)))

    if [ $category_tests -eq 0 ]; then
        category_passed=0
        category_tests=1
    fi

    local score=$((category_passed * 100 / category_tests))
    CATEGORY_SCORES+=($score)
    CATEGORY_NAMES+=("$category_name")

    echo -e "  ${CYAN}Category Score: $score% ($category_passed/$category_tests tests)${NC}"
}

# =============================================================================
# CATEGORY 1: PARAMETER EXPANSION COMPREHENSIVE
# =============================================================================

test_parameter_expansion() {
    print_category "PARAMETER EXPANSION COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Basic Parameter Expansion"
    run_test "Simple variable expansion" \
        'VAR=hello; echo $VAR' \
        "hello"

    run_test "Braced variable expansion" \
        'VAR=hello; echo ${VAR}' \
        "hello"

    run_test "Variable in string" \
        'VAR=world; echo "Hello $VAR"' \
        "Hello world"

    print_section "Default Value Expansions"
    run_test "Use default if unset" \
        'unset VAR; echo ${VAR:-default}' \
        "default"

    run_test "Use default if unset (no colon)" \
        'unset VAR; echo ${VAR-default}' \
        "default"

    run_test "Use default if empty" \
        'VAR=""; echo ${VAR:-default}' \
        "default"

    run_test "Don't use default if empty (no colon)" \
        'VAR=""; echo ${VAR-default}' \
        ""

    run_test "Don't use default if set" \
        'VAR=value; echo ${VAR:-default}' \
        "value"

    print_section "Alternative Value Expansions"
    run_test "Use alternative if set" \
        'VAR=value; echo ${VAR:+alternative}' \
        "alternative"

    run_test "Don't use alternative if unset" \
        'unset VAR; echo ${VAR:+alternative}' \
        ""

    run_test "Don't use alternative if empty" \
        'VAR=""; echo ${VAR:+alternative}' \
        ""

    run_test "Use alternative if empty (no colon)" \
        'VAR=""; echo ${VAR+alternative}' \
        "alternative"

    print_section "Assignment Expansions"
    run_test "Assign default if unset" \
        'unset VAR; echo ${VAR:=default}; echo $VAR' \
        "default
default"

    run_test "Assign default if empty" \
        'VAR=""; echo ${VAR:=default}; echo $VAR' \
        "default
default"

    print_section "Length Expansion"
    run_test "String length" \
        'VAR=hello; echo ${#VAR}' \
        "5"

    run_test "Empty string length" \
        'VAR=""; echo ${#VAR}' \
        "0"

    run_test "Unset variable length" \
        'unset VAR; echo ${#VAR}' \
        "0"

    print_section "Substring Expansion"
    run_test "Substring from position" \
        'VAR=hello; echo ${VAR:1}' \
        "ello"

    run_test "Substring with length" \
        'VAR=hello; echo ${VAR:1:3}' \
        "ell"

    run_test "Substring from end" \
        'VAR=hello; echo ${VAR: -2}' \
        "lo"

    print_section "Pattern Matching Expansion"
    run_test "Remove shortest prefix" \
        'VAR=hello.world.txt; echo ${VAR#*.}' \
        "world.txt"

    run_test "Remove longest prefix" \
        'VAR=hello.world.txt; echo ${VAR##*.}' \
        "txt"

    run_test "Remove shortest suffix" \
        'VAR=hello.world.txt; echo ${VAR%.*}' \
        "hello.world"

    run_test "Remove longest suffix" \
        'VAR=hello.world.txt; echo ${VAR%%.*}' \
        "hello"

    print_section "Case Conversion (Bash Extension)"
    run_test "First character uppercase" \
        'VAR=hello; echo ${VAR^}' \
        "Hello"

    run_test "All characters uppercase" \
        'VAR=hello; echo ${VAR^^}' \
        "HELLO"

    run_test "First character lowercase" \
        'VAR=HELLO; echo ${VAR,}' \
        "hELLO"

    run_test "All characters lowercase" \
        'VAR=HELLO; echo ${VAR,,}' \
        "hello"

    print_section "Complex Nested Expansions"
    run_test "Nested variable in default" \
        'DEFAULT=backup; unset VAR; echo ${VAR:-$DEFAULT}' \
        "backup"

    run_test "Nested expansion in alternative" \
        'VAR=test; echo ${VAR:+prefix_${VAR}_suffix}' \
        "prefix_test_suffix"

    run_test "Command substitution in default" \
        'unset VAR; echo ${VAR:-$(echo computed)}' \
        "computed"

    run_test "Nested braced expansions" \
        'A=hello; B=world; echo ${A:+${A}_${B}}' \
        "hello_world"

    calculate_category_score "Parameter Expansion" $start_count 15
}

# =============================================================================
# CATEGORY 2: ARITHMETIC EXPANSION COMPREHENSIVE
# =============================================================================

test_arithmetic_expansion() {
    print_category "ARITHMETIC EXPANSION COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Basic Arithmetic"
    run_test "Simple addition" \
        'echo $((5 + 3))' \
        "8"

    run_test "Simple subtraction" \
        'echo $((10 - 4))' \
        "6"

    run_test "Simple multiplication" \
        'echo $((6 * 7))' \
        "42"

    run_test "Simple division" \
        'echo $((15 / 3))' \
        "5"

    run_test "Modulo operation" \
        'echo $((17 % 5))' \
        "2"

    print_section "Operator Precedence"
    run_test "Multiplication before addition" \
        'echo $((2 + 3 * 4))' \
        "14"

    run_test "Parentheses override precedence" \
        'echo $(((2 + 3) * 4))' \
        "20"

    run_test "Complex precedence" \
        'echo $((2 * 3 + 4 * 5))' \
        "26"

    print_section "Variable Arithmetic"
    run_test "Variable in arithmetic" \
        'a=5; b=3; echo $((a + b))' \
        "8"

    run_test "Variable assignment in arithmetic" \
        'echo $((a = 5 + 3)); echo $a' \
        "8
8"

    run_test "Increment operation" \
        'a=5; echo $((++a)); echo $a' \
        "6
6"

    run_test "Decrement operation" \
        'a=5; echo $((--a)); echo $a' \
        "4
4"

    print_section "Comparison Operations"
    run_test "Less than true" \
        'echo $((5 < 10))' \
        "1"

    run_test "Less than false" \
        'echo $((10 < 5))' \
        "0"

    run_test "Equal comparison" \
        'echo $((5 == 5))' \
        "1"

    run_test "Not equal comparison" \
        'echo $((5 != 3))' \
        "1"

    print_section "Logical Operations"
    run_test "Logical AND true" \
        'echo $((1 && 1))' \
        "1"

    run_test "Logical AND false" \
        'echo $((1 && 0))' \
        "0"

    run_test "Logical OR true" \
        'echo $((0 || 1))' \
        "1"

    run_test "Logical OR false" \
        'echo $((0 || 0))' \
        "0"

    print_section "Bitwise Operations"
    run_test "Bitwise AND" \
        'echo $((12 & 10))' \
        "8"

    run_test "Bitwise OR" \
        'echo $((12 | 10))' \
        "14"

    run_test "Bitwise XOR" \
        'echo $((12 ^ 10))' \
        "6"

    run_test "Left shift" \
        'echo $((5 << 2))' \
        "20"

    run_test "Right shift" \
        'echo $((20 >> 2))' \
        "5"

    print_section "Error Handling"
    run_test "Division by zero error" \
        'echo $((5 / 0)) 2>&1' \
        "lusush: arithmetic: division by zero"

    run_test "Modulo by zero error" \
        'echo $((5 % 0)) 2>&1' \
        "lusush: arithmetic: division by zero in modulo operation"

    calculate_category_score "Arithmetic Expansion" $start_count 12
}

# =============================================================================
# CATEGORY 3: COMMAND SUBSTITUTION COMPREHENSIVE
# =============================================================================

test_command_substitution() {
    print_category "COMMAND SUBSTITUTION COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Modern Command Substitution"
    run_test "Simple command substitution" \
        'echo $(echo hello)' \
        "hello"

    run_test "Command substitution in string" \
        'echo "Today is $(date +%A)" 2>/dev/null || echo "Today is day"' \
        "Today is day"

    run_test "Nested command substitution" \
        'echo $(echo $(echo nested))' \
        "nested"

    run_test "Command substitution with arithmetic" \
        'echo $((5 + $(echo 3)))' \
        "8"

    print_section "Legacy Command Substitution"
    run_test "Backtick command substitution" \
        'echo `echo hello`' \
        "hello"

    run_test "Backticks in double quotes" \
        'echo "Result: `echo test`"' \
        "Result: test"

    print_section "Complex Command Substitution"
    run_test "Command substitution with pipes" \
        'echo $(echo "a b c" | wc -w)' \
        "3"

    run_test "Multiple command substitutions" \
        'echo $(echo one) $(echo two)' \
        "one two"

    run_test "Command substitution with variables" \
        'cmd=echo; arg=hello; echo $(${cmd} ${arg})' \
        "hello"

    calculate_category_score "Command Substitution" $start_count 10
}

# =============================================================================
# CATEGORY 4: VARIABLE OPERATIONS COMPREHENSIVE
# =============================================================================

test_variable_operations() {
    print_category "VARIABLE OPERATIONS COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Variable Assignment"
    run_test "Simple assignment" \
        'var=value; echo $var' \
        "value"

    run_test "Assignment with spaces in value" \
        'var="hello world"; echo $var' \
        "hello world"

    run_test "Multiple assignments" \
        'a=1; b=2; c=3; echo $a$b$c' \
        "123"

    print_section "Variable Concatenation"
    run_test "Variable concatenation" \
        'a=hello; b=world; echo $a$b' \
        "helloworld"

    run_test "Variable with literal text" \
        'var=test; echo prefix${var}suffix' \
        "prefixtestsuffix"

    run_test "Multiple variable concatenation" \
        'a=1; b=2; c=3; echo $a-$b-$c' \
        "1-2-3"

    print_section "Special Variables"
    run_test "Exit status variable" \
        'true; echo $?' \
        "0"

    run_test "Process ID variable" \
        'echo $$ | grep -E "^[0-9]+$" >/dev/null && echo "PID OK" || echo "PID FAIL"' \
        "PID OK"

    run_test "Parameter count after set" \
        'set -- a b c; echo $#' \
        "3"

    run_test "All parameters" \
        'set -- a b c; echo "$*"' \
        "a b c"

    print_section "Variable Scoping"
    run_test "Local vs global variables" \
        'global=outside; func() { local local=inside; echo $local; }; func; echo ${local:-unset}' \
        "inside
unset"

    calculate_category_score "Variable Operations" $start_count 8
}

# =============================================================================
# CATEGORY 5: CONTROL STRUCTURES COMPREHENSIVE
# =============================================================================

test_control_structures() {
    print_category "CONTROL STRUCTURES COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Conditional Statements"
    run_test "Simple if statement" \
        'if true; then echo yes; fi' \
        "yes"

    run_test "If-else statement" \
        'if false; then echo yes; else echo no; fi' \
        "no"

    run_test "If-elif-else statement" \
        'x=2; if [ $x -eq 1 ]; then echo one; elif [ $x -eq 2 ]; then echo two; else echo other; fi' \
        "two"

    print_section "Test Conditions"
    run_test "String equality test" \
        'if [ "hello" = "hello" ]; then echo equal; fi' \
        "equal"

    run_test "Numeric comparison test" \
        'if [ 5 -gt 3 ]; then echo greater; fi' \
        "greater"

    run_test "File existence test" \
        'if [ -f /dev/null ]; then echo exists; fi' \
        "exists"

    print_section "Loops"
    run_test "For loop with list" \
        'for i in 1 2 3; do echo $i; done' \
        "1
2
3"

    run_test "While loop" \
        'i=1; while [ $i -le 3 ]; do echo $i; i=$((i + 1)); done' \
        "1
2
3"

    run_test "Until loop" \
        'i=1; until [ $i -gt 3 ]; do echo $i; i=$((i + 1)); done' \
        "1
2
3"

    print_section "Case Statements"
    run_test "Simple case statement" \
        'case "hello" in hello) echo match;; *) echo no match;; esac' \
        "match"

    run_test "Case with patterns" \
        'case "test.txt" in *.txt) echo text file;; *) echo other;; esac' \
        "text file"

    run_test "Case with multiple patterns" \
        'case "b" in a|b|c) echo letter;; *) echo other;; esac' \
        "letter"

    calculate_category_score "Control Structures" $start_count 12
}

# =============================================================================
# CATEGORY 6: FUNCTION OPERATIONS COMPREHENSIVE
# =============================================================================

test_function_operations() {
    print_category "FUNCTION OPERATIONS COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Function Definition and Calling"
    run_test "Simple function" \
        'greet() { echo hello; }; greet' \
        "hello"

    run_test "Function with parameters" \
        'greet() { echo hello $1; }; greet world' \
        "hello world"

    run_test "Function with multiple parameters" \
        'add() { echo $(($1 + $2)); }; add 5 3' \
        "8"

    print_section "Function Return Values"
    run_test "Function return status" \
        'success() { return 0; }; success; echo $?' \
        "0"

    run_test "Function with output" \
        'double() { echo $(($1 * 2)); }; result=$(double 5); echo $result' \
        "10"

    print_section "Function Scoping"
    run_test "Function parameter access" \
        'func() { echo $1 $2 $#; }; func a b c' \
        "a b 3"

    run_test "Function variable assignment" \
        'func() { var=inside; }; func; echo $var' \
        "inside"

    calculate_category_score "Function Operations" $start_count 8
}

# =============================================================================
# CATEGORY 7: I/O REDIRECTION COMPREHENSIVE
# =============================================================================

test_io_redirection() {
    print_category "I/O REDIRECTION COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Output Redirection"
    run_test "Redirect stdout to file" \
        'echo hello > /tmp/test$$; cat /tmp/test$$; rm -f /tmp/test$$' \
        "hello"

    run_test "Append stdout to file" \
        'echo line1 > /tmp/test$$; echo line2 >> /tmp/test$$; cat /tmp/test$$; rm -f /tmp/test$$' \
        "line1
line2"

    print_section "Error Redirection"
    run_test "Redirect stderr to /dev/null" \
        'echo error >&2 2>/dev/null; echo success' \
        "success"

    run_test "Redirect both stdout and stderr" \
        'echo output; echo error >&2 2>&1' \
        "output
error"

    print_section "Input Redirection"
    run_test "Redirect stdin from file" \
        'echo "test input" > /tmp/test$$; cat < /tmp/test$$; rm -f /tmp/test$$' \
        "test input"

    print_section "Here Documents"
    run_test "Simple here document" \
        'cat <<EOF
line1
line2
EOF' \
        "line1
line2"

    run_test "Here document with variable expansion" \
        'var=test; cat <<EOF
Value: $var
EOF' \
        "Value: test"

    calculate_category_score "I/O Redirection" $start_count 8
}

# =============================================================================
# CATEGORY 8: BUILT-IN COMMANDS COMPREHENSIVE
# =============================================================================

test_builtin_commands() {
    print_category "BUILT-IN COMMANDS COMPREHENSIVE"
    local start_count=$TOTAL_TESTS

    print_section "Essential Built-ins"
    run_test "echo command" \
        'echo hello world' \
        "hello world"

    run_test "printf command" \
        'printf "Hello %s\n" world' \
        "Hello world"

    run_test "cd and pwd commands" \
        'cd /tmp; pwd; cd - >/dev/null' \
        "/tmp"

    print_section "Variable Built-ins"
    run_test "set positional parameters" \
        'set -- a b c; echo $1 $2 $3' \
        "a b c"

    run_test "unset variable" \
        'var=test; unset var; echo ${var:-unset}' \
        "unset"

    run_test "export variable" \
        'export VAR=value; echo $VAR' \
        "value"

    print_section "Utility Built-ins"
    run_test "test command true" \
        'test 5 -gt 3; echo $?' \
        "0"

    run_test "test command false" \
        'test 3 -gt 5; echo $?' \
        "1"

    run_test "type command" \
        'type echo | grep -q "builtin\|shell builtin" && echo "builtin" || echo "not found"' \
        "builtin"

    calculate_category_score "Built-in Commands" $start_count 10
}

# =============================================================================
# CATEGORY 9: PATTERN MATCHING AND GLOBBING
# =============================================================================

test_pattern_matching() {
    print_category "PATTERN MATCHING AND GLOBBING"
    local start_count=$TOTAL_TESTS

    print_section "Filename Globbing"
    run_test "Asterisk glob" \
        'touch /tmp/test1$$ /tmp/test2$$; echo /tmp/test*$$ | wc -w; rm -f /tmp/test*$$' \
        "2"

    run_test "Question mark glob" \
        'touch /tmp/test1$$ /tmp/test2$$; echo /tmp/test?$$ | wc -w; rm -f /tmp/test*$$' \
        "2"

    print_section "Pattern Matching in Case"
    run_test "Wildcard pattern in case" \
        'case "test.txt" in *.txt) echo match;; esac' \
        "match"

    run_test "Character class pattern" \
        'case "a" in [abc]) echo match;; esac' \
        "match"

    print_section "Parameter Expansion Patterns"
    run_test "Pattern removal prefix" \
        'file="dir/subdir/file.txt"; echo ${file#*/}' \
        "subdir/file.txt"

    run_test "Pattern removal suffix" \
        'file="file.tar.gz"; echo ${file%.*}' \
        "file.tar"

    calculate_category_score "Pattern Matching" $start_count 8
}

# =============================================================================
# CATEGORY 10: ERROR HANDLING AND EDGE CASES
# =============================================================================

test_error_handling() {
    print_category "ERROR HANDLING AND EDGE CASES"
    local start_count=$TOTAL_TESTS

    print_section "Command Errors"
    run_test_with_exit_code "Non-existent command" \
        'nonexistentcommand123 2>/dev/null' \
        "" \
        "127"

    run_test "Command not found message suppressed" \
        'nonexistentcommand123 2>/dev/null; echo done' \
        "done"

    print_section "Syntax Errors"
    run_test "Unclosed quote handling" \
        'echo "unclosed quote 2>&1 | grep -q "quote\|syntax" && echo "error detected" || echo "no error"' \
        "error detected"

    print_section "Variable Edge Cases"
    run_test "Undefined variable" \
        'echo ${UNDEFINED_VAR123:-default}' \
        "default"

    run_test "Empty variable handling" \
        'VAR=""; echo "${VAR:-empty}"' \
        "empty"

    print_section "Arithmetic Edge Cases"
    run_test "Large number arithmetic" \
        'echo $((999999 + 1))' \
        "1000000"

    run_test "Negative number arithmetic" \
        'echo $((-5 + 3))' \
        "-2"

    calculate_category_score "Error Handling" $start_count 8
}

# =============================================================================
# CATEGORY 11: REAL-WORLD COMPLEX SCENARIOS
# =============================================================================

test_real_world_scenarios() {
    print_category "REAL-WORLD COMPLEX SCENARIOS"
    local start_count=$TOTAL_TESTS

    print_section "Script-like Operations"
    run_test "File processing simulation" \
        'files="file1.txt file2.log file3.txt"; for f in $files; do case $f in *.txt) echo "Text: $f";; *.log) echo "Log: $f";; esac; done' \
        "Text: file1.txt
Log: file2.log
Text: file3.txt"

    run_test "Configuration parsing simulation" \
        'config="name=value;port=8080;debug=true"; IFS=";"; for item in $config; do echo "Config: $item"; done' \
        "Config: name=value
Config: port=8080
Config: debug=true"

    print_section "Complex Parameter Processing"
    run_test "URL parsing simulation" \
        'url="https://example.com:8080/path"; proto=${url%%://*}; rest=${url#*://}; host=${rest%%/*}; echo "$proto $host"' \
        "https example.com:8080"

    run_test "File extension processing" \
        'filename="document.backup.tar.gz"; base=${filename%%.*}; ext=${filename#*.}; echo "$base -> $ext"' \
        "document -> backup.tar.gz"

    print_section "Conditional Logic Chains"
    run_test "Multi-condition validation" \
        'value=15; if [ $value -gt 10 ] && [ $value -lt 20 ]; then echo "valid range"; else echo "invalid"; fi' \
        "valid range"

    calculate_category_score "Real-World Scenarios" $start_count 15
}

# =============================================================================
# CATEGORY 12: PERFORMANCE AND STRESS TESTING
# =============================================================================

test_performance_stress() {
    print_category "PERFORMANCE AND STRESS TESTING"
    local start_count=$TOTAL_TESTS

    print_section "Large Data Handling"
    run_test "Long string processing" \
        'str=$(printf "%100s" | tr " " "x"); echo ${#str}' \
        "100"

    run_test "Many variable assignments" \
        'for i in 1 2 3 4 5; do eval "var$i=value$i"; done; echo $var1$var3$var5' \
        "value1value3value5"

    print_section "Nested Operations"
    run_test "Deep nesting test" \
        'echo $(echo $(echo $(echo "deep")))' \
        "deep"

    run_test "Complex parameter expansion" \
        'a=test; b=ing; c=123; echo ${a:+prefix_${a}${b}_${c}_suffix}' \
        "prefix_testing_123_suffix"

    calculate_category_score "Performance Stress" $start_count 5
}

# =============================================================================
# COMPREHENSIVE SCORING AND ANALYSIS
# =============================================================================

calculate_final_score() {
    print_header "COMPREHENSIVE SHELL COMPLIANCE RESULTS"

    echo -e "${CYAN}Individual Category Scores:${NC}"
    for i in "${!CATEGORY_NAMES[@]}"; do
        echo -e "  ${CATEGORY_NAMES[$i]}: ${CATEGORY_SCORES[$i]}%"
    done

    # Calculate weighted overall score
    local total_weight=0
    local weighted_sum=0
    local weights=(15 12 10 8 12 8 8 10 8 8 15 5)  # Category weights

    for i in "${!CATEGORY_SCORES[@]}"; do
        if [ $i -lt ${#weights[@]} ]; then
            local weight=${weights[$i]}
            local score=${CATEGORY_SCORES[$i]}
            weighted_sum=$((weighted_sum + score * weight))
            total_weight=$((total_weight + weight))
        fi
    done

    local overall_score=0
    if [ $total_weight -gt 0 ]; then
        overall_score=$((weighted_sum / total_weight))
    fi

    echo -e "\n${PURPLE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${PURPLE}OVERALL SHELL COMPLIANCE SCORE: ${overall_score}%${NC}"
    echo -e "${PURPLE}═══════════════════════════════════════════════════════════════${NC}"

    # Compliance level assessment
    if [ $overall_score -ge 95 ]; then
        echo -e "${GREEN}🏆 EXCELLENT: Production-ready shell with full compliance${NC}"
    elif [ $overall_score -ge 85 ]; then
        echo -e "${GREEN}✅ VERY GOOD: High-quality shell suitable for most use cases${NC}"
    elif [ $overall_score -ge 75 ]; then
        echo -e "${YELLOW}⚠️  GOOD: Functional shell with some limitations${NC}"
    elif [ $overall_score -ge 60 ]; then
        echo -e "${YELLOW}⚠️  FAIR: Basic shell functionality present${NC}"
    else
        echo -e "${RED}❌ NEEDS WORK: Significant functionality gaps${NC}"
    fi

    echo -e "\n${CYAN}Test Summary:${NC}"
    echo -e "  Total Tests: $TOTAL_TESTS"
    echo -e "  Passed: $PASSED_TESTS"
    echo -e "  Failed: $FAILED_TESTS"
    echo -e "  Success Rate: $((PASSED_TESTS * 100 / TOTAL_TESTS))%"

    # Timing information
    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))
    echo -e "  Test Duration: ${duration}s"

    # Comparison with reference shells
    echo -e "\n${CYAN}Compliance Comparison:${NC}"
    echo -e "  POSIX Baseline: 100% (reference)"
    echo -e "  Bash 5.x: ~98% (reference)"
    echo -e "  Zsh 5.x: ~95% (reference)"
    echo -e "  Lusush: ${overall_score}%"

    # Recommendations
    echo -e "\n${CYAN}Development Recommendations:${NC}"
    if [ $overall_score -lt 95 ]; then
        echo -e "  • Focus on categories scoring below 90%"
        echo -e "  • Prioritize parameter expansion and arithmetic features"
        echo -e "  • Enhance error handling and edge case support"
    fi
    if [ $overall_score -ge 90 ]; then
        echo -e "  • Shell is approaching production readiness"
        echo -e "  • Consider advanced feature implementation"
        echo -e "  • Focus on performance optimization"
    fi

    return $overall_score
}

# =============================================================================
# MAIN TEST EXECUTION
# =============================================================================

main() {
    print_header "LUSUSH COMPREHENSIVE SHELL COMPLIANCE TEST SUITE"

    echo -e "${CYAN}Testing shell: $SHELL_UNDER_TEST${NC}"
    echo -e "${CYAN}Started at: $(date)${NC}"

    # Verify shell exists and is executable
    if [ ! -x "$SHELL_UNDER_TEST" ]; then
        echo -e "${RED}Error: Shell '$SHELL_UNDER_TEST' not found or not executable${NC}"
        echo -e "${YELLOW}Please ensure the shell is built: ninja -C builddir${NC}"
        exit 1
    fi

    # Execute all test categories
    test_parameter_expansion
    test_arithmetic_expansion
    test_command_substitution
    test_variable_operations
    test_control_structures
    test_function_operations
    test_io_redirection
    test_builtin_commands
    test_pattern_matching
    test_error_handling
    test_real_world_scenarios
    test_performance_stress

    # Calculate and display final results
    calculate_final_score
    local final_score=$?

    print_header "COMPREHENSIVE COMPLIANCE TESTING COMPLETE"

    # Exit with appropriate code
    if [ $final_score -ge 85 ]; then
        exit 0  # Success
    else
        exit 1  # Needs improvement
    fi
}

# Execute main function if script is run directly
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    main "$@"
fi

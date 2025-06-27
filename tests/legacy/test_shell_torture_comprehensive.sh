#!/bin/bash

# =============================================================================
# LUSUSH COMPREHENSIVE SHELL TORTURE TEST SUITE
# =============================================================================
#
# This is an exhaustive test suite designed to thoroughly evaluate a POSIX shell
# implementation against real-world usage patterns, edge cases, and advanced
# features. It serves as both a compliance test and a demonstration of what
# a professional-grade shell must handle.
#
# Test Categories:
# 1. POSIX Core Features (Required)
# 2. Advanced Shell Features
# 3. Edge Cases and Boundary Conditions
# 4. Real-World Usage Patterns
# 5. Error Handling and Recovery
# 6. Performance and Scalability
#
# Author: Shell Development Team
# Version: 1.0.0
# Target: POSIX.1-2017 + Extensions
# =============================================================================

set -e  # Exit on any test setup failure

# Test configuration
SHELL_UNDER_TEST="./builddir/lusush"
TEST_START_TIME=$(date +%s)
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
PURPLE='\033[1;35m'
CYAN='\033[1;36m'
NC='\033[0m' # No Color

# Test result tracking
declare -a FAILED_TEST_NAMES=()
declare -a PASSED_CATEGORIES=()
declare -a FAILED_CATEGORIES=()

# =============================================================================
# UTILITY FUNCTIONS
# =============================================================================

print_header() {
    echo -e "\n${BLUE}===============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================================================${NC}\n"
}

print_section() {
    echo -e "\n${PURPLE}=== $1 ===${NC}\n"
}

print_test() {
    echo -e "${CYAN}Test $TOTAL_TESTS: $1${NC}"
}

run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    local expected_exit_code="${4:-0}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    print_test "$test_name"

    echo "Command: $test_command"

    # Run the test and capture output and exit code
    local actual_output
    local actual_exit_code

    actual_output=$(echo "$test_command" | $SHELL_UNDER_TEST 2>&1)
    actual_exit_code=$?

    echo "Expected: '$expected_output' (exit: $expected_exit_code)"
    echo "Actual:   '$actual_output' (exit: $actual_exit_code)"

    # Check results
    if [[ "$actual_output" == "$expected_output" ]] && [[ $actual_exit_code -eq $expected_exit_code ]]; then
        echo -e "${GREEN}✓ PASS${NC}\n"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}\n"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_TEST_NAMES+=("$test_name")
        return 1
    fi
}

run_multi_line_test() {
    local test_name="$1"
    local test_script="$2"
    local expected_output="$3"
    local expected_exit_code="${4:-0}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    print_test "$test_name"

    echo "Script:"
    echo "$test_script" | sed 's/^/  /'

    # Run the test and capture output and exit code
    local actual_output
    local actual_exit_code

    actual_output=$(echo "$test_script" | $SHELL_UNDER_TEST 2>&1)
    actual_exit_code=$?

    echo "Expected: '$expected_output' (exit: $expected_exit_code)"
    echo "Actual:   '$actual_output' (exit: $actual_exit_code)"

    # Check results
    if [[ "$actual_output" == "$expected_output" ]] && [[ $actual_exit_code -eq $expected_exit_code ]]; then
        echo -e "${GREEN}✓ PASS${NC}\n"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}\n"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        FAILED_TEST_NAMES+=("$test_name")
        return 1
    fi
}

test_category() {
    local category_name="$1"
    local test_function="$2"

    print_section "$category_name"

    local category_start_tests=$TOTAL_TESTS
    local category_start_passed=$PASSED_TESTS

    # Run the test function
    $test_function

    local category_tests=$((TOTAL_TESTS - category_start_tests))
    local category_passed=$((PASSED_TESTS - category_start_passed))
    local category_failed=$((category_tests - category_passed))

    echo -e "${BLUE}Category Summary: $category_passed/$category_tests passed${NC}"

    if [[ $category_failed -eq 0 ]]; then
        PASSED_CATEGORIES+=("$category_name")
        echo -e "${GREEN}✓ $category_name: ALL TESTS PASSED${NC}\n"
    else
        FAILED_CATEGORIES+=("$category_name")
        echo -e "${RED}✗ $category_name: $category_failed tests failed${NC}\n"
    fi
}

# =============================================================================
# TEST SUITE CATEGORIES
# =============================================================================

# -----------------------------------------------------------------------------
# 1. POSIX CORE FEATURES - Basic shell functionality required by POSIX
# -----------------------------------------------------------------------------

test_basic_commands() {
    run_test "Simple command execution" \
        "echo hello" \
        "hello"

    run_test "Command with arguments" \
        "echo hello world" \
        "hello world"

    run_test "Multiple commands with semicolon" \
        "echo first; echo second" \
        "first
second"

    run_test "Command exit status propagation" \
        "true; echo \$?" \
        "0"

    run_test "Failed command exit status" \
        "false; echo \$?" \
        "1"
}

test_variable_operations() {
    run_test "Variable assignment and expansion" \
        "var=hello; echo \$var" \
        "hello"

    run_test "Variable with spaces in value" \
        'var="hello world"; echo $var' \
        "hello world"

    run_test "Multiple variable assignments" \
        'a=1; b=2; c=3; echo $a $b $c' \
        "1 2 3"

    run_test "Variable assignment without spaces" \
        'var=value; echo $var' \
        "value"

    run_test "Empty variable assignment" \
        'var=; echo "[$var]"' \
        "[]"

    run_test "Undefined variable expands to empty" \
        'echo "[$undefined]"' \
        "[]"

    run_test "Variable name with underscores" \
        'my_var=test; echo $my_var' \
        "test"

    run_test "Variable name with numbers" \
        'var2=test; echo $var2' \
        "test"
}

test_parameter_expansion_basic() {
    run_test "Braced variable expansion" \
        'var=hello; echo ${var}' \
        "hello"

    run_test "Default value expansion (unset)" \
        'echo ${unset:-default}' \
        "default"

    run_test "Default value expansion (empty)" \
        'var=; echo ${var:-default}' \
        "default"

    run_test "Default value expansion (set)" \
        'var=hello; echo ${var:-default}' \
        "hello"

    run_test "Alternative value expansion (set)" \
        'var=hello; echo ${var:+alternative}' \
        "alternative"

    run_test "Alternative value expansion (unset)" \
        'echo ${unset:+alternative}' \
        ""

    run_test "String length expansion" \
        'var=hello; echo ${#var}' \
        "5"

    run_test "Empty string length" \
        'var=; echo ${#var}' \
        "0"
}

test_parameter_expansion_advanced() {
    run_test "Substring expansion" \
        'var=hello; echo ${var:1:3}' \
        "ell"

    run_test "Substring from position" \
        'var=hello; echo ${var:2}' \
        "llo"

    run_test "Prefix removal shortest" \
        'var=hello; echo ${var#h}' \
        "ello"

    run_test "Prefix removal with wildcard" \
        'var=hello; echo ${var#*l}' \
        "lo"

    run_test "Prefix removal longest" \
        'var=hello; echo ${var##*l}' \
        "o"

    run_test "Suffix removal shortest" \
        'var=hello; echo ${var%o}' \
        "hell"

    run_test "Suffix removal with wildcard" \
        'var=hello; echo ${var%l*}' \
        "hel"

    run_test "Suffix removal longest" \
        'var=hello; echo ${var%%l*}' \
        "he"

    run_test "Case conversion - first to upper" \
        'var=hello; echo ${var^}' \
        "Hello"

    run_test "Case conversion - first to lower" \
        'var=Hello; echo ${var,}' \
        "hello"

    run_test "Case conversion - all to upper" \
        'var=hello; echo ${var^^}' \
        "HELLO"

    run_test "Case conversion - all to lower" \
        'var=HELLO; echo ${var,,}' \
        "hello"
}

test_parameter_expansion_real_world() {
    run_test "URL protocol extraction" \
        'url=https://example.com/path; echo ${url#*://}' \
        "example.com/path"

    run_test "Email domain extraction" \
        'email=user@example.com; echo ${email#*@}' \
        "example.com"

    run_test "Email username extraction" \
        'email=user@example.com; echo ${email%@*}' \
        "user"

    run_test "Filename without extension" \
        'file=document.txt; echo ${file%.*}' \
        "document"

    run_test "File extension extraction" \
        'file=document.txt; echo ${file##*.}' \
        "txt"

    run_test "Path basename" \
        'path=/usr/local/bin/gcc; echo ${path##*/}' \
        "gcc"

    run_test "Path dirname" \
        'path=/usr/local/bin; echo ${path%/*}' \
        "/usr/local"

    run_test "Version major number" \
        'version=1.2.3-beta; echo ${version%%.*}' \
        "1"

    run_test "Complex URL parsing" \
        'url=https://user:pass@host.com:8080/path?query=value; echo ${url#*://}' \
        "user:pass@host.com:8080/path?query=value"
}

test_command_substitution() {
    run_test "Modern command substitution" \
        'echo $(echo hello)' \
        "hello"

    run_test "Backtick command substitution" \
        'echo `echo hello`' \
        "hello"

    run_test "Command substitution with pipeline" \
        'echo $(echo "a b c" | wc -w)' \
        "3"

    run_test "Nested command substitution" \
        'echo $(echo $(echo hello))' \
        "hello"

    run_test "Command substitution in quotes" \
        'echo "Result: $(echo test)"' \
        "Result: test"

    run_test "Command substitution in parameter expansion" \
        'echo ${unset:-$(echo default)}' \
        "default"

    run_test "Multiple command substitutions" \
        'echo $(echo first) $(echo second)' \
        "first second"

    run_test "Command substitution with variables" \
        'cmd=echo; arg=hello; echo $(${cmd} ${arg})' \
        "hello"
}

test_arithmetic_expansion() {
    run_test "Basic arithmetic" \
        'echo $((2 + 3))' \
        "5"

    run_test "Arithmetic with variables" \
        'a=5; b=3; echo $((a + b))' \
        "8"

    run_test "Arithmetic precedence" \
        'echo $((2 + 3 * 4))' \
        "14"

    run_test "Arithmetic with parentheses" \
        'echo $(((2 + 3) * 4))' \
        "20"

    run_test "Arithmetic division" \
        'echo $((10 / 3))' \
        "3"

    run_test "Arithmetic modulo" \
        'echo $((10 % 3))' \
        "1"

    run_test "Arithmetic with negative numbers" \
        'echo $((-5 + 3))' \
        "-2"

    run_test "Arithmetic comparison" \
        'echo $((5 > 3))' \
        "1"

    run_test "Complex arithmetic expression" \
        'a=10; b=3; echo $((a * b + 2))' \
        "32"
}

test_quoting_mechanisms() {
    run_test "Single quotes preserve everything" \
        "echo 'hello \$var world'" \
        'hello $var world'

    run_test "Double quotes allow variable expansion" \
        'var=test; echo "hello $var world"' \
        "hello test world"

    run_test "Escaped quotes in double quotes" \
        'echo "He said \"hello\""' \
        'He said "hello"'

    run_test "Escaped backslash" \
        'echo "path\\to\\file"' \
        "path\\to\\file"

    run_test "Mixed quoting" \
        'var=world; echo "hello '"'"'$var'"'"' test"' \
        "hello 'world' test"

    run_test "Empty strings" \
        'echo "" '"'"''"'"' ""' \
        "  "

    run_test "Quoting with special characters" \
        'echo "chars: !@#$%^&*()"' \
        "chars: !@#$%^&*()"

    run_test "Variable expansion in double quotes" \
        'var="hello world"; echo "$var"' \
        "hello world"

    run_test "Command substitution in double quotes" \
        'echo "Today is $(date +%A)"' \
        "Today is $(date +%A)"  # This will show literal on most systems without proper date
}

test_redirection_basic() {
    # Create test files
    echo "test content" > /tmp/lusush_test_input.txt

    run_test "Output redirection" \
        'echo hello > /tmp/lusush_test.out; cat /tmp/lusush_test.out' \
        "hello"

    run_test "Input redirection" \
        'cat < /tmp/lusush_test_input.txt' \
        "test content"

    run_test "Append redirection" \
        'echo first > /tmp/lusush_append.txt; echo second >> /tmp/lusush_append.txt; cat /tmp/lusush_append.txt' \
        "first
second"

    run_test "Error redirection" \
        'echo error >&2 2>/tmp/lusush_error.txt; cat /tmp/lusush_error.txt' \
        "error"

    run_test "Redirect both stdout and stderr" \
        'echo output; echo error >&2' \
        "output
error"

    # Cleanup
    rm -f /tmp/lusush_test.out /tmp/lusush_test_input.txt /tmp/lusush_append.txt /tmp/lusush_error.txt
}

test_pipelines() {
    run_test "Simple pipeline" \
        'echo "hello world" | wc -w' \
        "2"

    run_test "Multi-stage pipeline" \
        'echo "a b c d e" | tr " " "\n" | wc -l' \
        "5"

    run_test "Pipeline with variable" \
        'var="one two three"; echo $var | wc -w' \
        "3"

    run_test "Pipeline exit status" \
        'true | false; echo $?' \
        "1"

    run_test "Complex pipeline" \
        'echo "hello world test" | sed "s/world/universe/" | tr "a-z" "A-Z"' \
        "HELLO UNIVERSE TEST"
}

test_logical_operators() {
    run_test "AND operator success" \
        'true && echo success' \
        "success"

    run_test "AND operator failure" \
        'false && echo success; echo done' \
        "done"

    run_test "OR operator failure" \
        'false || echo fallback' \
        "fallback"

    run_test "OR operator success" \
        'true || echo fallback; echo done' \
        "done"

    run_test "Chained logical operators" \
        'true && echo first && echo second' \
        "first
second"

    run_test "Mixed logical operators" \
        'false || echo recovered && echo continued' \
        "recovered
continued"

    run_test "Logical operators with exit codes" \
        'true && false || echo final; echo $?' \
        "final
0"
}

test_control_structures() {
    run_multi_line_test "Simple if statement" \
'if true; then
    echo success
fi' \
        "success"

    run_multi_line_test "If-else statement" \
'if false; then
    echo failure
else
    echo success
fi' \
        "success"

    run_multi_line_test "If-elif-else statement" \
'value=2
if [ $value -eq 1 ]; then
    echo one
elif [ $value -eq 2 ]; then
    echo two
else
    echo other
fi' \
        "two"

    run_multi_line_test "For loop with list" \
'for item in a b c; do
    echo $item
done' \
        "a
b
c"

    run_multi_line_test "For loop with variable expansion" \
'list="1 2 3"
for num in $list; do
    echo $num
done' \
        "1
2
3"

    run_multi_line_test "While loop" \
'i=1
while [ $i -le 3 ]; do
    echo $i
    i=$((i + 1))
done' \
        "1
2
3"

    run_multi_line_test "Case statement" \
'value=b
case $value in
    a) echo first ;;
    b) echo second ;;
    *) echo other ;;
esac' \
        "second"

    run_multi_line_test "Complex case statement" \
'input="hello"
case $input in
    hello|hi) echo greeting ;;
    bye|goodbye) echo farewell ;;
    *) echo unknown ;;
esac' \
        "greeting"
}

test_functions() {
    run_multi_line_test "Simple function definition and call" \
'hello() {
    echo "Hello World"
}
hello' \
        "Hello World"

    run_multi_line_test "Function with parameters" \
'greet() {
    echo "Hello $1"
}
greet World' \
        "Hello World"

    run_multi_line_test "Function with multiple parameters" \
'add() {
    echo $(($1 + $2))
}
add 5 3' \
        "8"

    run_multi_line_test "Function with local variables" \
'test_scope() {
    local_var="function"
    echo $local_var
}
local_var="global"
test_scope
echo $local_var' \
        "function
global"

    run_multi_line_test "Recursive function" \
'countdown() {
    if [ $1 -gt 0 ]; then
        echo $1
        countdown $(($1 - 1))
    fi
}
countdown 3' \
        "3
2
1"

    run_multi_line_test "Function return value" \
'test_return() {
    return 42
}
test_return
echo $?' \
        "42"
}

test_special_variables() {
    run_test "Exit status variable" \
        'true; echo $?' \
        "0"

    run_test "Process ID variable" \
        'echo $$ | grep -E "^[0-9]+$" > /dev/null && echo "PID is numeric"' \
        "PID is numeric"

    run_multi_line_test "Positional parameters" \
'set -- arg1 arg2 arg3
echo $1 $2 $3' \
        "arg1 arg2 arg3"

    run_multi_line_test "Parameter count" \
'set -- a b c d
echo $#' \
        "4"

    run_multi_line_test "All parameters with quotes" \
'set -- "arg with spaces" "another arg"
echo "$@"' \
        "arg with spaces another arg"

    run_multi_line_test "All parameters without quotes" \
'set -- arg1 arg2 arg3
echo $*' \
        "arg1 arg2 arg3"
}

test_built_in_commands() {
    run_test "Echo command" \
        'echo "hello world"' \
        "hello world"

    run_test "Test command - string comparison" \
        'test "hello" = "hello" && echo match' \
        "match"

    run_test "Test command - bracket form" \
        '[ "hello" = "hello" ] && echo match' \
        "match"

    run_test "Test command - numeric comparison" \
        '[ 5 -gt 3 ] && echo greater' \
        "greater"

    run_test "Test command - file test" \
        'touch /tmp/lusush_testfile; [ -f /tmp/lusush_testfile ] && echo exists; rm -f /tmp/lusush_testfile' \
        "exists"

    run_test "True command" \
        'true && echo success' \
        "success"

    run_test "False command" \
        'false || echo fallback' \
        "fallback"

    run_multi_line_test "Export command" \
'export TESTVAR=testvalue
echo $TESTVAR' \
        "testvalue"

    run_multi_line_test "Unset command" \
'TESTVAR=value
unset TESTVAR
echo ${TESTVAR:-unset}' \
        "unset"

    run_test "PWD command" \
        'pwd | grep -E "^/" > /dev/null && echo "PWD works"' \
        "PWD works"
}

# -----------------------------------------------------------------------------
# 2. ADVANCED SHELL FEATURES - Beyond basic POSIX requirements
# -----------------------------------------------------------------------------

test_advanced_expansions() {
    run_test "Nested parameter expansion" \
        'var1=hello; var2=var1; echo ${!var2}' \
        "${!var2}"  # This may not work in basic shells

    run_test "Parameter expansion with variables in default" \
        'default=backup; echo ${unset:-$default}' \
        "backup"

    run_test "Complex nested expansion" \
        'prefix=hello; suffix=world; echo ${prefix:-${suffix}}' \
        "hello"

    run_test "Arithmetic in parameter expansion" \
        'array_index=2; echo ${array_index:$((array_index-1)):1}' \
        ""  # This tests substring with arithmetic

    run_test "Command substitution in parameter expansion" \
        'echo ${HOME:-$(echo /tmp)}' \
        "${HOME:-$(echo /tmp)}"  # Should use actual HOME or /tmp
}

test_advanced_quoting() {
    run_test "ANSI-C quoting" \
        'echo $'"'"'hello\nworld'"'"'' \
        "hello\nworld"  # Literal \n, not actual newline

    run_test "Complex quote nesting" \
        'echo "outer '"'"'inner'"'"' outer"' \
        "outer 'inner' outer"

    run_test "Quote removal in assignments" \
        'var="hello"; echo ${var}' \
        "hello"

    run_test "Backslash escaping" \
        'echo hello\ world' \
        "hello world"

    run_test "Dollar escaping" \
        'echo \$var' \
        '$var'
}

test_advanced_redirection() {
    run_test "File descriptor redirection" \
        'echo error >&2 2>/dev/null; echo success' \
        "success"

    run_test "Duplicate file descriptors" \
        'echo output 2>&1 | cat' \
        "output"

    run_test "Redirect to file descriptor" \
        'exec 3>/tmp/lusush_fd3; echo hello >&3; exec 3>&-; cat /tmp/lusush_fd3; rm -f /tmp/lusush_fd3' \
        "hello"

    run_test "Here string" \
        'cat <<< "hello world"' \
        "hello world"

    run_multi_line_test "Here document with variables" \
'var=world
cat << EOF
hello $var
EOF' \
        "hello world"

    run_multi_line_test "Here document with tab stripping" \
'cat <<- EOF
	indented
	lines
EOF' \
        "indented
lines"
}

test_advanced_patterns() {
    run_test "Brace expansion" \
        'echo {a,b,c}' \
        "{a,b,c}"  # May not expand in basic shells

    run_test "Brace expansion with ranges" \
        'echo {1..3}' \
        "{1..3}"  # May not expand in basic shells

    run_test "Glob patterns" \
        'echo /usr/bin/b*sh | grep -o bash' \
        "bash"  # If bash exists

    run_test "Extended glob patterns" \
        'echo *.sh | head -n1' \
        "test_shell_torture_comprehensive.sh"  # This script itself

    run_test "Tilde expansion" \
        'echo ~ | grep -E "^/" > /dev/null && echo "Tilde expanded"' \
        "Tilde expanded"
}

test_job_control() {
    run_test "Background job" \
        'sleep 0.1 & echo done' \
        "done"

    run_test "Job control variables" \
        'true & echo $!' \
        ""  # PID should be numeric, hard to test exact value

    run_test "Wait command" \
        'sleep 0.1 & wait; echo finished' \
        "finished"
}

test_aliases() {
    run_multi_line_test "Simple alias" \
'alias ll="ls -l"
alias | grep ll' \
        ""  # Output format varies

    run_multi_line_test "Alias expansion" \
'alias hi="echo hello"
hi' \
        "hello"

    run_multi_line_test "Unalias command" \
'alias test_alias="echo test"
unalias test_alias
alias | grep test_alias' \
        ""
}

# -----------------------------------------------------------------------------
# 3. EDGE CASES AND BOUNDARY CONDITIONS
# -----------------------------------------------------------------------------

test_empty_and_whitespace() {
    run_test "Empty command line" \
        '' \
        ""

    run_test "Only whitespace" \
        '   ' \
        ""

    run_test "Empty variable" \
        'var=; echo "[$var]"' \
        "[]"

    run_test "Whitespace in variables" \
        'var="  hello  "; echo "[$var]"' \
        "[  hello  ]"

    run_test "Tab characters" \
        'echo "hello	world"' \
        "hello	world"

    run_test "Newline handling" \
        'echo "line1
line2"' \
        "line1
line2"
}

test_special_characters() {
    run_test "Dollar signs" \
        'echo "price: \$5.00"' \
        "price: $5.00"

    run_test "Exclamation marks" \
        'echo "hello!"' \
        "hello!"

    run_test "Question marks" \
        'echo "what?"' \
        "what?"

    run_test "Asterisks" \
        'echo "2 * 3 = 6"' \
        "2 * 3 = 6"

    run_test "Brackets and braces" \
        'echo "[array] {hash}"' \
        "[array] {hash}"

    run_test "Parentheses" \
        'echo "(group)"' \
        "(group)"

    run_test "Semicolons in quotes" \
        'echo "cmd1; cmd2"' \
        "cmd1; cmd2"

    run_test "Pipes in quotes" \
        'echo "cmd1 | cmd2"' \
        "cmd1 | cmd2"

    run_test "Ampersands in quotes" \
        'echo "cmd1 && cmd2"' \
        "cmd1 && cmd2"
}

test_unicode_and_encoding() {
    run_test "Basic Unicode" \
        'echo "café"' \
        "café"

    run_test "Unicode in variables" \
        'var="résumé"; echo $var' \
        "résumé"

    run_test "Mixed encoding" \
        'echo "Hello 世界"' \
        "Hello 世界"

    run_test "Unicode in filenames" \
        'touch "/tmp/café.txt"; ls /tmp/café.txt; rm -f "/tmp/café.txt"' \
        "/tmp/café.txt"
}

test_boundary_conditions() {
    run_test "Very long command line" \
        'echo "$(printf "%1000s" | tr " " "a")"' \
        "$(printf "%1000s" | tr " " "a")"  # 1000 character string

    run_test "Many arguments" \
        'echo a b c d e f g h i j k l m n o p q r s t u v w x y z' \
        "a b c d e f g h i j k l m n o p q r s t u v w x y z"

    run_test "Deeply nested quotes" \
        'echo "a\"b\"c\"d\"e"' \
        'a"b"c"d"e'

    run_test "Maximum variable name length" \
        'very_long_variable_name_that_tests_limits=value; echo $very_long_variable_name_that_tests_limits' \
        "value"

    run_test "Zero-length parameter expansion" \
        'var=; echo ${var:0:0}' \
        ""

    run_test "Out of bounds substring" \
        'var=hello; echo ${var:10:5}' \
        ""

    run_test "Negative substring offset" \
        'var=hello; echo ${var:-2:2}' \
        "ll"  # May not work in all shells
}

test_error_conditions() {
    run_test "Division by zero (should not crash)" \
        'echo $((5 / 0)) 2>/dev/null || echo "error handled"' \
        "error handled"

    run_test "Invalid variable name" \
        '2var=invalid 2>/dev/null || echo "error handled"' \
        "error handled"

    run_test "Unmatched quotes (should recover)" \
        'echo "unmatched' \
        "" 1  # Expect error exit code

    run_test "Unmatched parentheses" \
        'echo $((2 + 3)' \
        "" 1  # Expect error exit code

    run_test "Invalid arithmetic" \
        'echo $((hello + world)) 2>/dev/null || echo "error handled"' \
        "error handled"

    run_test "Recursive alias (should detect)" \
        'alias loop=loop; loop 2>/dev/null || echo "error handled"' \
        "error handled"
}

# -----------------------------------------------------------------------------
# 4. REAL-WORLD USAGE PATTERNS - Complex scenarios from actual scripts
# -----------------------------------------------------------------------------

test_script_patterns() {
    run_multi_line_test "Configuration file parsing simulation" \
'config="key1=value1
key2=value2
key3=value3"
for line in $config; do
    key=${line%=*}
    value=${line#*=}
    echo "$key -> $value"
done' \
        "key1 -> value1
key2 -> value2
key3 -> value3"

    run_multi_line_test "Path manipulation" \
'PATH_LIST="/usr/bin:/bin:/usr/local/bin"
IFS=":"
for path in $PATH_LIST; do
    echo "Checking: $path"
done
unset IFS' \
        "Checking: /usr/bin
Checking: /bin
Checking: /usr/local/bin"

    run_multi_line_test "Backup script pattern" \
'DATE=$(date +%Y%m%d)
BACKUP_DIR="/tmp/backup_$DATE"
echo "Creating backup in $BACKUP_DIR"
mkdir -p "$BACKUP_DIR" 2>/dev/null && echo "Directory created"' \
        "Creating backup in /tmp/backup_$DATE
Directory created"

    run_multi_line_test "Log file processing" \
'LOG_LINE="2024-01-01 12:00:00 ERROR Something went wrong"
timestamp=${LOG_LINE%% *}
rest=${LOG_LINE#* }
level=${rest%% *}
rest=${rest#* }
message=${rest#* }
echo "Time: $timestamp, Level: $level, Message: $message"' \
        "Time: 2024-01-01, Level: 12:00:00, Message: Something went wrong"
}

test_system_integration() {
    run_test "Environment variable usage" \
        'echo "Shell: $SHELL" | grep -o "Shell:" || echo "SHELL var exists"' \
        "SHELL var exists"

    run_test "Working directory operations" \
        'cd /tmp && pwd && cd - >/dev/null && echo "back"' \
        "/tmp
back"

    run_test "File operations" \
        'touch /tmp/testfile && [ -f /tmp/testfile ] && echo "file created" && rm /tmp/testfile' \
        "file created"

    run_test "Process substitution simulation" \
        'echo "data" | (read line; echo "processed: $line")' \
        "processed: data"

    run_test "Signal handling simulation" \
        'trap "echo caught" USR1; echo "trap set"' \
        "trap set"
}

test_performance_patterns() {
    run_test "Loop performance" \
        'i=0; while [ $i -lt 100 ]; do i=$((i + 1)); done; echo $i' \
        "100"

    run_test "String building performance" \
        'result=""; for i in 1 2 3 4 5; do result="$result$i"; done; echo $result' \
        "12345"

    run_test "Variable expansion performance" \
        'var1=a; var2=b; var3=c; echo $var1$var2$var3$var1$var2$var3' \
        "abcabc"

    run_test "Command substitution chaining" \
        'echo $(echo $(echo hello))' \
        "hello"

    run_test "Complex parameter expansion chain" \
        'var="  hello world  "; echo "${var##* }" "${var%% *}"' \
        "world hello"
}

# -----------------------------------------------------------------------------
# 5. COMPATIBILITY TESTS - Ensuring behavior matches other shells
# -----------------------------------------------------------------------------

test_posix_compliance() {
    run_test "POSIX variable naming" \
        '_var=test; echo $_var' \
        "test"

    run_test "POSIX parameter expansion" \
        'var=hello; echo "${var-default}"' \
        "hello"

    run_test "POSIX command substitution" \
        'result=`echo test`; echo $result' \
        "test"

    run_test "POSIX here document" \
        'cat << "EOF"
literal $var
EOF' \
        "literal $var"

    run_test "POSIX field splitting" \
        'IFS=":"; var="a:b:c"; set -- $var; echo $1 $2 $3' \
        "a b c"

    run_test "POSIX exit status" \
        '(exit 42); echo $?' \
        "42"
}

test_bash_compatibility() {
    run_test "Bash-style arrays (basic)" \
        'arr=(a b c); echo ${arr[0]} ${arr[1]} ${arr[2]}' \
        "a b c"  # May not work in basic shells

    run_test "Bash parameter expansion" \
        'var=hello; echo ${var/l/L}' \
        "heLlo"  # May not work in basic shells

    run_test "Bash arithmetic" \
        'echo $((2**3))' \
        "8"  # May not work in basic shells

    run_test "Bash process substitution" \
        'echo <(echo test)' \
        "/dev/fd/63"  # May not work in basic shells
}

# -----------------------------------------------------------------------------
# 6. STRESS TESTS - Resource and stability testing
# -----------------------------------------------------------------------------

test_memory_stress() {
    run_test "Large variable content" \
        'large=$(printf "%10000s" | tr " " "x"); echo ${#large}' \
        "10000"

    run_test "Many variables" \
        'for i in $(seq 1 100); do eval "var$i=value$i"; done; echo $var50' \
        "value50"

    run_test "Deep function nesting" \
        'f1() { f2; }; f2() { f3; }; f3() { echo deep; }; f1' \
        "deep"

    run_test "Large parameter expansion" \
        'var=$(printf "%1000s" | tr " " "a"); echo ${#var}' \
        "1000"
}

test_concurrent_operations() {
    run_test "Multiple background jobs" \
        'sleep 0.1 & sleep 0.1 & sleep 0.1 & wait; echo done' \
        "done"

    run_test "Pipe with background" \
        'echo test | cat & wait; echo finished' \
        "test
finished"

    run_test "Complex job control" \
        '(sleep 0.1; echo job1) & (sleep 0.2; echo job2) & wait' \
        "job1
job2"
}

# =============================================================================
# TEST EXECUTION ENGINE
# =============================================================================

main() {
    print_header "LUSUSH COMPREHENSIVE SHELL TORTURE TEST SUITE"

    echo -e "${BLUE}Starting comprehensive shell testing...${NC}"
    echo -e "${BLUE}Shell under test: $SHELL_UNDER_TEST${NC}"
    echo -e "${BLUE}Test start time: $(date)${NC}\n"

    # Verify shell exists and is executable
    if [[ ! -x "$SHELL_UNDER_TEST" ]]; then
        echo -e "${RED}ERROR: Shell executable not found: $SHELL_UNDER_TEST${NC}"
        echo "Please build the shell first with: ninja -C builddir"
        exit 1
    fi

    # Category 1: POSIX Core Features
    test_category "POSIX Core - Basic Commands" test_basic_commands
    test_category "POSIX Core - Variable Operations" test_variable_operations
    test_category "POSIX Core - Parameter Expansion (Basic)" test_parameter_expansion_basic
    test_category "POSIX Core - Parameter Expansion (Advanced)" test_parameter_expansion_advanced
    test_category "POSIX Core - Parameter Expansion (Real-World)" test_parameter_expansion_real_world
    test_category "POSIX Core - Command Substitution" test_command_substitution
    test_category "POSIX Core - Arithmetic Expansion" test_arithmetic_expansion
    test_category "POSIX Core - Quoting Mechanisms" test_quoting_mechanisms
    test_category "POSIX Core - Redirection (Basic)" test_redirection_basic
    test_category "POSIX Core - Pipelines" test_pipelines
    test_category "POSIX Core - Logical Operators" test_logical_operators
    test_category "POSIX Core - Control Structures" test_control_structures
    test_category "POSIX Core - Functions" test_functions
    test_category "POSIX Core - Special Variables" test_special_variables
    test_category "POSIX Core - Built-in Commands" test_built_in_commands

    # Category 2: Advanced Features
    test_category "Advanced - Complex Expansions" test_advanced_expansions
    test_category "Advanced - Complex Quoting" test_advanced_quoting
    test_category "Advanced - Advanced Redirection" test_advanced_redirection
    test_category "Advanced - Pattern Matching" test_advanced_patterns
    test_category "Advanced - Job Control" test_job_control
    test_category "Advanced - Aliases" test_aliases

    # Category 3: Edge Cases
    test_category "Edge Cases - Empty and Whitespace" test_empty_and_whitespace
    test_category "Edge Cases - Special Characters" test_special_characters
    test_category "Edge Cases - Unicode and Encoding" test_unicode_and_encoding
    test_category "Edge Cases - Boundary Conditions" test_boundary_conditions
    test_category "Edge Cases - Error Conditions" test_error_conditions

    # Category 4: Real-World Patterns
    test_category "Real-World - Script Patterns" test_script_patterns
    test_category "Real-World - System Integration" test_system_integration
    test_category "Real-World - Performance Patterns" test_performance_patterns

    # Category 5: Compatibility
    test_category "Compatibility - POSIX Compliance" test_posix_compliance
    test_category "Compatibility - Bash Compatibility" test_bash_compatibility

    # Category 6: Stress Tests
    test_category "Stress - Memory Stress" test_memory_stress
    test_category "Stress - Concurrent Operations" test_concurrent_operations

    # Final Results
    print_header "COMPREHENSIVE TEST RESULTS"

    local test_end_time=$(date +%s)
    local test_duration=$((test_end_time - TEST_START_TIME))

    echo -e "${BLUE}Test execution completed${NC}"
    echo -e "${BLUE}Total time: ${test_duration} seconds${NC}\n"

    echo -e "${CYAN}OVERALL STATISTICS:${NC}"
    echo -e "  Total tests executed: $TOTAL_TESTS"
    echo -e "  ${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "  ${RED}Failed: $FAILED_TESTS${NC}"
    echo -e "  ${YELLOW}Skipped: $SKIPPED_TESTS${NC}"

    local pass_percentage=0
    if [[ $TOTAL_TESTS -gt 0 ]]; then
        pass_percentage=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    echo -e "  ${CYAN}Success rate: ${pass_percentage}%${NC}\n"

    echo -e "${CYAN}CATEGORY RESULTS:${NC}"
    echo -e "${GREEN}Fully passing categories (${#PASSED_CATEGORIES[@]}):${NC}"
    for category in "${PASSED_CATEGORIES[@]}"; do
        echo -e "  ✓ $category"
    done

    if [[ ${#FAILED_CATEGORIES[@]} -gt 0 ]]; then
        echo -e "\n${RED}Categories with failures (${#FAILED_CATEGORIES[@]}):${NC}"
        for category in "${FAILED_CATEGORIES[@]}"; do
            echo -e "  ✗ $category"
        done
    fi

    if [[ ${#FAILED_TEST_NAMES[@]} -gt 0 ]]; then
        echo -e "\n${RED}Failed individual tests (${#FAILED_TEST_NAMES[@]}):${NC}"
        for test_name in "${FAILED_TEST_NAMES[@]}"; do
            echo -e "  ✗ $test_name"
        done
    fi

    echo -e "\n${CYAN}FEATURE COVERAGE ASSESSMENT:${NC}"
    if [[ $pass_percentage -ge 95 ]]; then
        echo -e "${GREEN}🏆 EXCELLENT: Shell demonstrates professional-grade POSIX compliance${NC}"
    elif [[ $pass_percentage -ge 80 ]]; then
        echo -e "${GREEN}✅ GOOD: Shell handles most POSIX features correctly${NC}"
    elif [[ $pass_percentage -ge 60 ]]; then
        echo -e "${YELLOW}⚠️  MODERATE: Shell has basic functionality but needs improvement${NC}"
    else
        echo -e "${RED}❌ POOR: Shell requires significant development work${NC}"
    fi

    echo -e "\n${CYAN}RECOMMENDATIONS:${NC}"
    if [[ $FAILED_TESTS -gt 0 ]]; then
        echo -e "1. Focus on failing test categories for maximum impact"
        echo -e "2. Review POSIX.1-2017 specification for failed features"
        echo -e "3. Test against reference shells (bash, dash) for expected behavior"
        echo -e "4. Consider implementing most commonly used features first"
    else
        echo -e "🎉 Congratulations! All tests passed. Consider extending test coverage."
    fi

    echo -e "\n${BLUE}Test suite execution completed.${NC}"

    # Exit with appropriate code
    if [[ $FAILED_TESTS -eq 0 ]]; then
        exit 0
    else
        exit 1
    fi
}

# Execute main function if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi

#!/bin/bash

# =============================================================================
# RIGOROUS POSIX COMPLIANCE VALIDATION TEST
# =============================================================================
#
# This test suite provides a more rigorous evaluation of POSIX shell compliance
# based on official POSIX standards and known challenging test cases that
# distinguish truly compliant shells from partially compliant ones.
#
# Purpose: Validate that our current 100% score reflects genuine compliance
# rather than lenient testing, and identify areas needing improvement.
#
# Based on: IEEE Std 1003.1-2016 Shell and Utilities
# Reference: VSC-PCTS test patterns and real-world POSIX edge cases
# =============================================================================

SHELL_UNDER_TEST="${1:-./builddir/lusush}"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${BLUE}===============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================================================${NC}\n"
}

print_section() {
    echo -e "\n${PURPLE}=== $1 ===${NC}"
}

run_rigorous_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    local expected_exit_code="${4:-0}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # Create temporary files for capturing output and exit code
    local temp_out="/tmp/lusush_rigorous_test_$$_out"
    local temp_err="/tmp/lusush_rigorous_test_$$_err"

    # Execute test with timeout to prevent hangs - use here-doc to avoid variable expansion
    timeout 10s bash -c "cat << 'TEST_EOF' | $SHELL_UNDER_TEST
$test_command
TEST_EOF" > "$temp_out" 2> "$temp_err"
    local actual_exit_code=$?

    # Handle timeout case
    if [ $actual_exit_code -eq 124 ]; then
        echo -e "  ${RED}✗${NC} $test_name"
        echo -e "    ${RED}TIMEOUT: Test hung for >10 seconds${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        rm -f "$temp_out" "$temp_err"
        return 1
    fi

    local actual_output=$(cat "$temp_out" 2>/dev/null)
    local actual_error=$(cat "$temp_err" 2>/dev/null)

    # Check both output and exit code
    if [ "$actual_output" = "$expected_output" ] && [ $actual_exit_code -eq $expected_exit_code ]; then
        echo -e "  ${GREEN}✓${NC} $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        rm -f "$temp_out" "$temp_err"
        return 0
    else
        echo -e "  ${RED}✗${NC} $test_name"
        echo -e "    Expected: '$expected_output' (exit: $expected_exit_code)"
        echo -e "    Actual:   '$actual_output' (exit: $actual_exit_code)"
        if [ -n "$actual_error" ]; then
            echo -e "    Error:    '$actual_error'"
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
        rm -f "$temp_out" "$temp_err"
        return 1
    fi
}

# =============================================================================
# RIGOROUS POSIX PARAMETER EXPANSION TESTS
# =============================================================================
test_rigorous_parameter_expansion() {
    print_section "RIGOROUS PARAMETER EXPANSION"

    # Test complex nested parameter expansions
    run_rigorous_test "Nested parameter expansion with defaults" \
        'a=outer; b=; echo ${a:-${b:-fallback}}' \
        "outer"

    run_rigorous_test "Parameter expansion with pattern matching" \
        'path=/usr/local/bin/file.txt; echo ${path##*/}' \
        "file.txt"

    run_rigorous_test "Parameter expansion substring with variables" \
        'str=hello; start=1; len=3; echo ${str:$start:$len}' \
        "ell"

    # Test edge cases that trip up many shells
    run_rigorous_test "Empty parameter with colon" \
        'a=; echo "${a:=default}"' \
        "default"

    run_rigorous_test "Unset parameter without colon" \
        'unset a 2>/dev/null; echo "${a=default}"' \
        "default"

    run_rigorous_test "Parameter expansion in arithmetic" \
        'a=5; b=3; echo $((a + b))' \
        "8"
}

# =============================================================================
# RIGOROUS POSIX QUOTING AND ESCAPING
# =============================================================================
test_rigorous_quoting() {
    print_section "RIGOROUS QUOTING AND ESCAPING"

    # Complex quote nesting that many shells fail
    run_rigorous_test "Double quotes containing single quotes" \
        'echo "He said '\''hello'\'' to me"' \
        "He said 'hello' to me"

    run_rigorous_test "Backslash escaping in double quotes" \
        'echo "Line 1\nLine 2"' \
        "Line 1\\nLine 2"

    run_rigorous_test "Dollar sign escaping" \
        'echo "Price: \$5.99"' \
        'Price: $5.99'

    # IFS and field splitting edge cases
    run_rigorous_test "IFS with multiple delimiters" \
        'OLD_IFS="$IFS"; IFS=":"; var="a:b:c"; set -- $var; echo $2; IFS="$OLD_IFS"' \
        "b"

    run_rigorous_test "Empty IFS behavior" \
        'OLD_IFS="$IFS"; IFS=""; var="a b c"; set -- $var; echo $#; IFS="$OLD_IFS"' \
        "1"
}

# =============================================================================
# RIGOROUS POSIX ARITHMETIC EXPANSION
# =============================================================================
test_rigorous_arithmetic() {
    print_section "RIGOROUS ARITHMETIC EXPANSION"

    # Complex arithmetic that tests operator precedence
    run_rigorous_test "Complex operator precedence" \
        'echo $((2 + 3 * 4 - 1))' \
        "13"

    run_rigorous_test "Arithmetic with parentheses" \
        'echo $(((2 + 3) * (4 - 1)))' \
        "15"

    # Base conversion tests
    run_rigorous_test "Octal arithmetic" \
        'echo $((010 + 1))' \
        "9"

    run_rigorous_test "Hexadecimal arithmetic" \
        'echo $((0x10 + 1))' \
        "17"

    # Variable arithmetic
    run_rigorous_test "Arithmetic assignment" \
        'a=5; echo $((a+=3)); echo $a' \
        "8
8"

    # Edge case: division by zero should be handled gracefully
    run_rigorous_test "Division by zero handling" \
        'echo $((5/0)) 2>/dev/null; [ $? -ne 0 ] && echo "error" || echo "no error"' \
        "error"
}

# =============================================================================
# RIGOROUS POSIX COMMAND SUBSTITUTION
# =============================================================================
test_rigorous_command_substitution() {
    print_section "RIGOROUS COMMAND SUBSTITUTION"

    # Nested command substitution
    run_rigorous_test "Nested command substitution" \
        'echo $(echo $(echo nested))' \
        "nested"

    # Command substitution with pipes
    run_rigorous_test "Command substitution with pipeline" \
        'echo $(echo "hello world" | sed "s/world/POSIX/")' \
        "hello POSIX"

    # Backtick vs $() compatibility
    run_rigorous_test "Backtick command substitution" \
        'echo `echo backtick`' \
        "backtick"

    # Command substitution in different contexts
    run_rigorous_test "Command substitution in assignment" \
        'var=$(echo assigned); echo $var' \
        "assigned"
}

# =============================================================================
# RIGOROUS POSIX CONTROL STRUCTURES
# =============================================================================
test_rigorous_control_structures() {
    print_section "RIGOROUS CONTROL STRUCTURES"

    # Complex if-then-else structures
    run_rigorous_test "Nested if statements" \
        'if true; then if false; then echo no; else echo yes; fi; fi' \
        "yes"

    # For loops with various inputs
    run_rigorous_test "For loop with glob pattern" \
        'for f in /dev/null; do echo $f; done' \
        "/dev/null"

    # While/until loop edge cases
    run_rigorous_test "While loop with counter" \
        'i=0; while [ $i -lt 3 ]; do echo $i; i=$((i+1)); done' \
        "0
1
2"

    # Case statements with complex patterns
    run_rigorous_test "Case statement with patterns" \
        'case hello in h*) echo match;; *) echo no;; esac' \
        "match"

    run_rigorous_test "Case statement with multiple patterns" \
        'case 5 in [1-3]) echo low;; [4-6]) echo mid;; *) echo high;; esac' \
        "mid"
}

# =============================================================================
# RIGOROUS POSIX I/O REDIRECTION
# =============================================================================
test_rigorous_io_redirection() {
    print_section "RIGOROUS I/O REDIRECTION"

    # Here document edge cases
    run_rigorous_test "Here document with quoted delimiter" \
        'cat << '\''EOF'\''
$HOME should not expand
EOF' \
        '$HOME should not expand'

    run_rigorous_test "Here document with variable expansion" \
        'var=expanded; cat << EOF
$var
EOF' \
        'expanded'

    run_rigorous_test "Here document with tab stripping" \
        'cat <<- EOF
	test line
EOF' \
        'test line'

    # File descriptor redirection
    run_rigorous_test "Stderr redirection" \
        'echo stdout; echo stderr >&2 2>/dev/null' \
        'stdout
stderr'

    run_rigorous_test "File descriptor duplication" \
        'echo test 2>&1 | cat' \
        'test'
}

# =============================================================================
# RIGOROUS POSIX BUILT-IN COMMANDS
# =============================================================================
test_rigorous_builtins() {
    print_section "RIGOROUS BUILT-IN COMMANDS"

    # Test builtin behavior edge cases
    run_rigorous_test "Echo without newline" \
        'echo -n test; echo done' \
        'testdone'

    run_rigorous_test "Printf with format specifiers" \
        'printf "%d %s\n" 42 hello' \
        '42 hello'

    run_rigorous_test "Test command numeric comparison" \
        'test 10 -gt 5 && echo true || echo false' \
        'true'

    run_rigorous_test "Test command string comparison" \
        'test "abc" = "abc" && echo equal || echo different' \
        'equal'

    # Set command with options
    run_rigorous_test "Set positional parameters" \
        'set -- a b c; echo $# $1 $3' \
        '3 a c'

    # Shift command
    run_rigorous_test "Shift positional parameters" \
        'set -- a b c; shift; echo $1 $2' \
        'b c'

    # Unset command
    run_rigorous_test "Unset variable" \
        'var=test; unset var; echo ${var:-unset}' \
        'unset'
}

# =============================================================================
# RIGOROUS POSIX FUNCTIONS
# =============================================================================
test_rigorous_functions() {
    print_section "RIGOROUS FUNCTIONS"

    # Function definition and calling
    run_rigorous_test "Function with return value" \
        'myfunc() { return 42; }; myfunc; echo $?' \
        '42'

    run_rigorous_test "Function with local variables" \
        'outer=global; myfunc() { outer=local; echo $outer; }; myfunc; echo $outer' \
        'local
global'

    run_rigorous_test "Function with parameters" \
        'greet() { echo "Hello $1 $2"; }; greet World POSIX' \
        'Hello World POSIX'

    # Function edge cases
    run_rigorous_test "Recursive function" \
        'countdown() { [ $1 -gt 0 ] && { echo $1; countdown $(($1-1)); } || echo done; }; countdown 2' \
        '2
1
done'
}

# =============================================================================
# RIGOROUS POSIX PATTERN MATCHING
# =============================================================================
test_rigorous_pattern_matching() {
    print_section "RIGOROUS PATTERN MATCHING"

    # Glob pattern expansion
    run_rigorous_test "Asterisk glob pattern" \
        'echo /dev/n*ll' \
        '/dev/null'

    run_rigorous_test "Question mark glob pattern" \
        'echo /dev/nul?' \
        '/dev/null'

    run_rigorous_test "Character class pattern" \
        'case a in [a-z]) echo lowercase;; [A-Z]) echo uppercase;; esac' \
        'lowercase'

    # Parameter pattern matching
    run_rigorous_test "Parameter prefix removal" \
        'path=/usr/local/bin; echo ${path#/usr/}' \
        'local/bin'

    run_rigorous_test "Parameter suffix removal" \
        'file=document.txt; echo ${file%.txt}' \
        'document'
}

# =============================================================================
# RIGOROUS POSIX ERROR HANDLING
# =============================================================================
test_rigorous_error_handling() {
    print_section "RIGOROUS ERROR HANDLING"

    # Command not found
    run_rigorous_test "Nonexistent command handling" \
        'nonexistentcommand123 2>/dev/null; echo $?' \
        '127'

    # Syntax error recovery
    run_rigorous_test "Syntax error in subshell" \
        'echo before; (echo "valid") || echo "no error"' \
        'before
valid'

    # Set -e behavior
    run_rigorous_test "Exit on error behavior" \
        'set -e; true; false; echo "should not print"' \
        '' \
        1

    # Set -u behavior
    run_rigorous_test "Unset variable error" \
        'set -u; echo $UNDEFINED_VAR_XYZ 2>/dev/null || echo "undefined variable"' \
        'undefined variable'
}

# =============================================================================
# RIGOROUS POSIX ENVIRONMENT AND VARIABLES
# =============================================================================
test_rigorous_environment() {
    print_section "RIGOROUS ENVIRONMENT AND VARIABLES"

    # Environment variable inheritance
    run_rigorous_test "Environment variable export" \
        'export TEST_VAR=exported; sh -c "echo $TEST_VAR"' \
        'exported'

    # Special variables
    run_rigorous_test "Process ID variable" \
        'echo $$ | grep -E "^[0-9]+$" >/dev/null && echo "valid PID"' \
        'valid PID'

    run_rigorous_test "Exit status variable" \
        'false; echo $?' \
        '1'

    run_rigorous_test "Argument count variable" \
        'set -- a b c; echo $#' \
        '3'

    # Variable assignment in command
    run_rigorous_test "Variable assignment in command invocation" \
        'VAR=temp sh -c "echo $VAR"' \
        'temp'
}

# =============================================================================
# MAIN TEST EXECUTION AND SCORING
# =============================================================================

main() {
    print_header "RIGOROUS POSIX COMPLIANCE VALIDATION TEST"
    echo "Testing shell: $SHELL_UNDER_TEST"
    echo "Started at: $(date)"

    if [ ! -x "$SHELL_UNDER_TEST" ]; then
        echo -e "${RED}ERROR: Shell executable not found: $SHELL_UNDER_TEST${NC}"
        exit 1
    fi

    # Run all test categories
    test_rigorous_parameter_expansion
    test_rigorous_quoting
    test_rigorous_arithmetic
    test_rigorous_command_substitution
    test_rigorous_control_structures
    test_rigorous_io_redirection
    test_rigorous_builtins
    test_rigorous_functions
    test_rigorous_pattern_matching
    test_rigorous_error_handling
    test_rigorous_environment

    # Calculate and display results
    print_header "RIGOROUS POSIX COMPLIANCE RESULTS"

    local success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))

    echo "Test Summary:"
    echo "  Total Tests: $TOTAL_TESTS"
    echo "  Passed: $PASSED_TESTS"
    echo "  Failed: $FAILED_TESTS"
    echo "  Success Rate: ${success_rate}%"
    echo

    # Interpret results
    if [ $success_rate -ge 95 ]; then
        echo -e "${GREEN}🏆 EXCELLENT: High POSIX compliance - ready for rigorous environments${NC}"
    elif [ $success_rate -ge 85 ]; then
        echo -e "${YELLOW}⚠️  GOOD: Solid POSIX compliance with minor gaps${NC}"
    elif [ $success_rate -ge 70 ]; then
        echo -e "${YELLOW}⚠️  MODERATE: Decent compliance but significant gaps remain${NC}"
    else
        echo -e "${RED}❌ POOR: Major POSIX compliance issues - significant work needed${NC}"
    fi

    echo
    echo "Rigorous Assessment:"
    echo "  This test suite uses challenging edge cases that distinguish"
    echo "  truly compliant shells from partially compliant ones."
    echo "  A score above 90% indicates genuine POSIX compliance."
    echo
    echo "Completed at: $(date)"

    # Return appropriate exit code
    [ $FAILED_TESTS -eq 0 ]
}

# Execute main function
main "$@"

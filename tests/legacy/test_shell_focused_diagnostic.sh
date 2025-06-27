#!/bin/bash

# =============================================================================
# LUSUSH FOCUSED SHELL DIAGNOSTIC TEST
# =============================================================================
#
# This is a focused diagnostic test designed to quickly identify the most
# critical issues in the lusush shell implementation. It targets core
# functionality that must work for basic shell operation.
#
# Focus Areas:
# 1. Exit status propagation
# 2. Special variables
# 3. Parameter expansion
# 4. Command execution
# 5. Variable assignment
# 6. Basic control structures
#
# Author: Shell Development Team
# Version: 1.0.0
# Target: Core POSIX functionality validation
# =============================================================================

# Test configuration
SHELL_UNDER_TEST="./builddir/lusush"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
NC='\033[0m' # No Color

print_header() {
    echo -e "\n${BLUE}===============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================================================${NC}\n"
}

print_section() {
    echo -e "\n${CYAN}=== $1 ===${NC}"
}

run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    local expected_exit_code="${4:-0}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo -e "${YELLOW}Test $TOTAL_TESTS: $test_name${NC}"
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
        return 1
    fi
}

# =============================================================================
# CRITICAL FUNCTIONALITY TESTS
# =============================================================================

test_exit_status() {
    print_section "Exit Status Propagation (CRITICAL)"

    run_test "True command exit status" \
        'true; echo $?' \
        "0"

    run_test "False command exit status" \
        'false; echo $?' \
        "1"

    run_test "Command sequence exit status" \
        'true; false; echo $?' \
        "1"

    run_test "Success after failure" \
        'false; true; echo $?' \
        "0"

    run_test "External command exit status" \
        'echo test; echo $?' \
        "test
0"
}

test_special_variables() {
    print_section "Special Variables (CRITICAL)"

    run_test "Exit status variable" \
        'echo $?' \
        "0"

    run_test "Process ID variable" \
        'echo $$ | grep -E "^[0-9]+$" > /dev/null && echo "PID_OK"' \
        "PID_OK"

    run_test "Parameter count (no args)" \
        'echo $#' \
        "0"

    run_test "Script name" \
        'echo $0 | grep -o lusush > /dev/null && echo "SCRIPT_OK"' \
        "SCRIPT_OK"
}

test_variable_operations() {
    print_section "Variable Operations (CORE)"

    run_test "Simple assignment" \
        'var=hello; echo $var' \
        "hello"

    run_test "Assignment with spaces" \
        'var="hello world"; echo $var' \
        "hello world"

    run_test "Empty assignment" \
        'var=; echo "[$var]"' \
        "[]"

    run_test "Multiple assignments" \
        'a=1; b=2; echo $a$b' \
        "12"

    run_test "Variable with special chars" \
        'var=hello@world.com; echo $var' \
        "hello@world.com"
}

test_parameter_expansion_core() {
    print_section "Parameter Expansion (CORE)"

    run_test "Braced expansion" \
        'var=hello; echo ${var}' \
        "hello"

    run_test "Default value (unset)" \
        'echo ${unset:-default}' \
        "default"

    run_test "Default value (empty)" \
        'var=; echo ${var:-default}' \
        "default"

    run_test "Length expansion" \
        'var=hello; echo ${#var}' \
        "5"

    run_test "Substring expansion" \
        'var=hello; echo ${var:1:3}' \
        "ell"
}

test_pattern_matching() {
    print_section "Pattern Matching (ADVANCED)"

    run_test "Prefix removal simple" \
        'var=hello; echo ${var#h}' \
        "ello"

    run_test "Prefix removal wildcard" \
        'var=hello; echo ${var#*l}' \
        "lo"

    run_test "URL protocol removal" \
        'url=https://example.com; echo ${url#*://}' \
        "example.com"

    run_test "Email domain extraction" \
        'email=user@domain.com; echo ${email#*@}' \
        "domain.com"

    run_test "File extension removal" \
        'file=test.txt; echo ${file%.*}' \
        "test"
}

test_command_substitution() {
    print_section "Command Substitution (CORE)"

    run_test "Modern command substitution" \
        'echo $(echo hello)' \
        "hello"

    run_test "Backtick command substitution" \
        'echo `echo hello`' \
        "hello"

    run_test "Command substitution in quotes" \
        'echo "Result: $(echo test)"' \
        "Result: test"

    run_test "Command substitution in defaults" \
        'echo ${unset:-$(echo default)}' \
        "default"
}

test_quoting() {
    print_section "Quoting Mechanisms (CORE)"

    run_test "Single quotes preserve all" \
        "echo 'hello \$var world'" \
        'hello $var world'

    run_test "Double quotes expand variables" \
        'var=test; echo "hello $var world"' \
        "hello test world"

    run_test "Escaped quotes" \
        'echo "He said \"hello\""' \
        'He said "hello"'

    run_test "Mixed quoting" \
        'var=world; echo "hello '"'"'$var'"'"'"' \
        "hello 'world'"
}

test_basic_control() {
    print_section "Basic Control Structures (CORE)"

    run_test "Simple if-then" \
        'if true; then echo success; fi' \
        "success"

    run_test "If-else with false" \
        'if false; then echo fail; else echo success; fi' \
        "success"

    run_test "Test command string comparison" \
        'test "hello" = "hello" && echo match' \
        "match"

    run_test "Test command bracket form" \
        '[ "hello" = "hello" ] && echo match' \
        "match"

    run_test "Logical AND success" \
        'true && echo success' \
        "success"

    run_test "Logical OR fallback" \
        'false || echo fallback' \
        "fallback"
}

test_pipelines() {
    print_section "Pipelines and Redirection (CORE)"

    run_test "Simple pipeline" \
        'echo "hello world" | wc -w' \
        "2"

    run_test "Pipeline with variables" \
        'var="one two three"; echo $var | wc -w' \
        "3"

    run_test "Output redirection" \
        'echo hello > /tmp/lusush_test.out; cat /tmp/lusush_test.out; rm -f /tmp/lusush_test.out' \
        "hello"
}

test_edge_cases() {
    print_section "Critical Edge Cases (RELIABILITY)"

    run_test "Empty command" \
        '' \
        ""

    run_test "Whitespace only" \
        '   ' \
        ""

    run_test "Unmatched quotes (error handling)" \
        'echo "unmatched' \
        "" 1

    run_test "Invalid variable name" \
        '2var=invalid 2>/dev/null || echo "error handled"' \
        "error handled"

    run_test "Division by zero" \
        'echo $((5 / 0)) 2>/dev/null || echo "error handled"' \
        "error handled"
}

# =============================================================================
# REAL WORLD PATTERNS
# =============================================================================

test_real_world() {
    print_section "Real-World Usage Patterns (PRACTICAL)"

    run_test "Configuration parsing" \
        'config="key=value"; key=${config%=*}; value=${config#*=}; echo "$key -> $value"' \
        "key -> value"

    run_test "Path manipulation" \
        'path=/usr/local/bin/gcc; echo ${path##*/}' \
        "gcc"

    run_test "File extension check" \
        'file=script.sh; ext=${file##*.}; test "$ext" = "sh" && echo "shell script"' \
        "shell script"

    run_test "URL parsing" \
        'url=https://user@host.com:8080/path; host=${url#*://}; host=${host%%/*}; echo $host' \
        "user@host.com:8080"

    run_test "Environment variable fallback" \
        'echo ${HOME:-/tmp}' \
        "$HOME"  # This will show actual HOME or /tmp
}

# =============================================================================
# MAIN EXECUTION
# =============================================================================

main() {
    print_header "LUSUSH FOCUSED SHELL DIAGNOSTIC TEST"

    echo -e "${BLUE}Shell under test: $SHELL_UNDER_TEST${NC}"
    echo -e "${BLUE}Test start time: $(date)${NC}\n"

    # Verify shell exists
    if [[ ! -x "$SHELL_UNDER_TEST" ]]; then
        echo -e "${RED}ERROR: Shell executable not found: $SHELL_UNDER_TEST${NC}"
        echo "Please build the shell first with: ninja -C builddir"
        exit 1
    fi

    # Run test categories in order of importance
    test_exit_status
    test_special_variables
    test_variable_operations
    test_parameter_expansion_core
    test_pattern_matching
    test_command_substitution
    test_quoting
    test_basic_control
    test_pipelines
    test_edge_cases
    test_real_world

    # Final results
    print_header "DIAGNOSTIC RESULTS"

    echo -e "${CYAN}SUMMARY:${NC}"
    echo -e "  Total tests: $TOTAL_TESTS"
    echo -e "  ${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "  ${RED}Failed: $FAILED_TESTS${NC}"

    local pass_percentage=0
    if [[ $TOTAL_TESTS -gt 0 ]]; then
        pass_percentage=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    echo -e "  ${CYAN}Success rate: ${pass_percentage}%${NC}\n"

    echo -e "${CYAN}ASSESSMENT:${NC}"
    if [[ $pass_percentage -ge 95 ]]; then
        echo -e "${GREEN}🏆 EXCELLENT: Shell is production-ready${NC}"
    elif [[ $pass_percentage -ge 85 ]]; then
        echo -e "${GREEN}✅ VERY GOOD: Shell is highly functional${NC}"
    elif [[ $pass_percentage -ge 75 ]]; then
        echo -e "${YELLOW}⚠️  GOOD: Shell is functional with minor issues${NC}"
    elif [[ $pass_percentage -ge 60 ]]; then
        echo -e "${YELLOW}⚠️  MODERATE: Shell needs improvement${NC}"
    else
        echo -e "${RED}❌ POOR: Shell requires significant work${NC}"
    fi

    echo -e "\n${CYAN}PRIORITY FIXES:${NC}"
    if [[ $FAILED_TESTS -gt 0 ]]; then
        echo -e "1. Focus on exit status propagation if failing"
        echo -e "2. Ensure special variables work correctly"
        echo -e "3. Verify parameter expansion completeness"
        echo -e "4. Test real-world usage patterns"
    else
        echo -e "🎉 All critical functionality working!"
    fi

    echo -e "\n${BLUE}Diagnostic completed.${NC}"

    # Exit with appropriate code
    if [[ $FAILED_TESTS -eq 0 ]]; then
        exit 0
    else
        exit 1
    fi
}

# Execute main function
main "$@"

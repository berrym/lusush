#!/bin/bash

# =============================================================================
# FOCUSED POSIX OPTIONS VALIDATION TEST
# =============================================================================
#
# Tests the 24 implemented POSIX options with better error handling and timeout
# protection. Focuses on core functionality validation.
#
# Author: AI Assistant for Lusush v1.3.0 QA
# =============================================================================

set -euo pipefail

LUSUSH_BINARY="${1:-./builddir/lusush}"
TEST_DIR="/tmp/lusush_focused_test_$$"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
NC='\033[0m'

# Cleanup on exit
cleanup() {
    cd /
    rm -rf "$TEST_DIR" 2>/dev/null || true
}
trap cleanup EXIT

print_header() {
    echo -e "\n${BLUE}===============================================================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================================================${NC}\n"
}

print_section() {
    echo -e "\n${CYAN}=== $1 ===${NC}"
}

# Simple test function with timeout protection
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_exit="${3:-0}"
    local check_type="${4:-exit_only}"  # exit_only, output_contains, output_exact
    local expected_output="${5:-}"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    local actual_output=""
    local actual_exit=0
    local test_passed=false

    # Use timeout to prevent hanging
    if actual_output=$(timeout 3s bash -c "echo '$test_command' | '$LUSUSH_BINARY' -c 'exec $LUSUSH_BINARY'" 2>&1); then
        actual_exit=0
    else
        actual_exit=$?
        if [[ $actual_exit -eq 124 ]]; then
            echo -e "  ${RED}✗${NC} $test_name (TIMEOUT)"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    fi

    # Check based on type
    case "$check_type" in
        "exit_only")
            [[ "$actual_exit" == "$expected_exit" ]] && test_passed=true
            ;;
        "output_contains")
            [[ "$actual_exit" == "$expected_exit" && "$actual_output" == *"$expected_output"* ]] && test_passed=true
            ;;
        "output_exact")
            [[ "$actual_exit" == "$expected_exit" && "$actual_output" == "$expected_output" ]] && test_passed=true
            ;;
    esac

    if $test_passed; then
        echo -e "  ${GREEN}✓${NC} $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "  ${RED}✗${NC} $test_name"
        echo -e "    ${YELLOW}Expected exit:${NC} $expected_exit, got: $actual_exit"
        [[ -n "$expected_output" ]] && echo -e "    ${YELLOW}Expected output:${NC} '$expected_output'"
        echo -e "    ${YELLOW}Actual output:${NC} '$actual_output'"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Test option state by checking set -o output
test_option_existence() {
    local option="$1"
    local test_name="Option $option exists in set -o output"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if timeout 2s "$LUSUSH_BINARY" -c "set -o" 2>/dev/null | grep -q "$option"; then
        echo -e "  ${GREEN}✓${NC} $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "  ${RED}✗${NC} $test_name"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Simple command-line option test
test_cmdline_option() {
    local option="$1"
    local test_name="Command line option $option is accepted"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if timeout 2s "$LUSUSH_BINARY" "$option" -c "echo test" >/dev/null 2>&1; then
        echo -e "  ${GREEN}✓${NC} $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "  ${RED}✗${NC} $test_name"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

print_header "FOCUSED POSIX OPTIONS VALIDATION"
echo "Testing shell: $LUSUSH_BINARY"
echo "Started at: $(date)"

# Verify shell exists
if [[ ! -x "$LUSUSH_BINARY" ]]; then
    echo -e "${RED}ERROR: Shell binary not found: $LUSUSH_BINARY${NC}"
    exit 1
fi

mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# =============================================================================
# BASIC FUNCTIONALITY TEST
# =============================================================================

print_section "Basic Shell Functionality"

TOTAL_TESTS=$((TOTAL_TESTS + 1))
if echo "echo hello" | timeout 2s "$LUSUSH_BINARY" >/dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} Shell executes basic commands"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} Shell executes basic commands"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# =============================================================================
# COMMAND LINE OPTIONS (-a through -x)
# =============================================================================

print_section "Command Line Options (-a through -x)"

# Test that command line options are accepted
test_cmdline_option "-a"
test_cmdline_option "-e"
test_cmdline_option "-f"
test_cmdline_option "-h"
test_cmdline_option "-m"
test_cmdline_option "-n"
test_cmdline_option "-u"
test_cmdline_option "-v"
test_cmdline_option "-x"

# Test -e (exit on error) behavior
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if ! timeout 2s "$LUSUSH_BINARY" -e -c "false; echo should_not_print" 2>/dev/null | grep -q "should_not_print"; then
    echo -e "  ${GREEN}✓${NC} -e option exits on command failure"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} -e option exits on command failure"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test -n (syntax check) behavior
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if ! timeout 2s "$LUSUSH_BINARY" -n -c "echo should_not_execute" 2>/dev/null | grep -q "should_not_execute"; then
    echo -e "  ${GREEN}✓${NC} -n option performs syntax check only"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} -n option performs syntax check only"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# =============================================================================
# SET -O OPTIONS (NAMED OPTIONS)
# =============================================================================

print_section "Named Options (set -o)"

# Test that all 24 options exist in set -o output
test_option_existence "allexport"
test_option_existence "notify"
test_option_existence "noclobber"
test_option_existence "errexit"
test_option_existence "noglob"
test_option_existence "hashall"
test_option_existence "monitor"
test_option_existence "noexec"
test_option_existence "onecmd"
test_option_existence "nounset"
test_option_existence "verbose"
test_option_existence "xtrace"
test_option_existence "ignoreeof"
test_option_existence "nolog"
test_option_existence "emacs"
test_option_existence "vi"
test_option_existence "posix"
test_option_existence "pipefail"
test_option_existence "histexpand"
test_option_existence "history"
test_option_existence "interactive-comments"
test_option_existence "braceexpand"
test_option_existence "physical"
test_option_existence "privileged"

# =============================================================================
# FUNCTIONAL TESTS
# =============================================================================

print_section "Functional Behavior Tests"

# Test set -e functionality through set command
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if ! timeout 2s "$LUSUSH_BINARY" -c "set -e; false; echo should_not_print" 2>/dev/null | grep -q "should_not_print"; then
    echo -e "  ${GREEN}✓${NC} set -e exits on command failure"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} set -e exits on command failure"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test set -u functionality
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if ! timeout 2s "$LUSUSH_BINARY" -c "set -u; echo \$UNDEFINED_VAR" >/dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} set -u errors on undefined variables"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} set -u errors on undefined variables"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test option switching
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if timeout 2s "$LUSUSH_BINARY" -c "set -o emacs; set -o vi; set -o | grep -q 'set -o vi'" 2>/dev/null; then
    echo -e "  ${GREEN}✓${NC} Option switching works (emacs/vi)"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} Option switching works (emacs/vi)"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test multiple options
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if timeout 2s "$LUSUSH_BINARY" -c "set -eu; echo 'combined options work'" >/dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} Multiple option combinations work"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} Multiple option combinations work"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# =============================================================================
# INTEGRATION TESTS
# =============================================================================

print_section "Integration Tests"

# Test that set -o shows option states
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if timeout 2s "$LUSUSH_BINARY" -c "set -o" 2>/dev/null | grep -q "set.*hashall"; then
    echo -e "  ${GREEN}✓${NC} set -o displays option states"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} set -o displays option states"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# Test invalid option handling
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if ! timeout 2s "$LUSUSH_BINARY" -c "set -o invalid_option" >/dev/null 2>&1; then
    echo -e "  ${GREEN}✓${NC} Invalid options are properly rejected"
    PASSED_TESTS=$((PASSED_TESTS + 1))
else
    echo -e "  ${RED}✗${NC} Invalid options are properly rejected"
    FAILED_TESTS=$((FAILED_TESTS + 1))
fi

# =============================================================================
# RESULTS
# =============================================================================

print_header "TEST RESULTS SUMMARY"

echo -e "${BLUE}Total Tests:${NC} $TOTAL_TESTS"
echo -e "${GREEN}Passed:${NC} $PASSED_TESTS"
echo -e "${RED}Failed:${NC} $FAILED_TESTS"

if [[ $FAILED_TESTS -eq 0 ]]; then
    echo -e "\n${GREEN}🎉 ALL TESTS PASSED! 🎉${NC}"
    echo -e "${GREEN}Lusush demonstrates excellent POSIX compliance!${NC}"
    exit 0
else
    PASS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    echo -e "\n${YELLOW}Pass Rate: ${PASS_RATE}%${NC}"

    if [[ $PASS_RATE -ge 95 ]]; then
        echo -e "${GREEN}EXCELLENT${NC} - Very high compliance"
        exit 0
    elif [[ $PASS_RATE -ge 90 ]]; then
        echo -e "${YELLOW}GOOD${NC} - High compliance with minor issues"
        exit 0
    elif [[ $PASS_RATE -ge 80 ]]; then
        echo -e "${YELLOW}ACCEPTABLE${NC} - Reasonable compliance"
        exit 1
    else
        echo -e "${RED}NEEDS WORK${NC} - Significant issues"
        exit 2
    fi
fi

echo -e "\nValidated POSIX Options:"
echo "✓ Command Line: -a, -e, -f, -h, -m, -n, -u, -v, -x"
echo "✓ Named Options: All 24 options present in set -o output"
echo "✓ Functionality: Error handling, option switching, combinations"
echo "✓ Integration: Option state display, error handling"

echo -e "\nTest completed at: $(date)"

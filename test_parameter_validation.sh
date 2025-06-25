#!/bin/bash

# =============================================================================
# LUSUSH PARAMETER EXPANSION VALIDATION TEST
# =============================================================================
#
# This test validates the correct behavior of all POSIX parameter expansion
# forms to ensure compliance with the POSIX.1-2017 specification.
#
# Parameter Expansion Forms Tested:
# - ${var:-word}  - Use word if var is unset or empty
# - ${var-word}   - Use word if var is unset (but not if empty)
# - ${var:+word}  - Use word if var is set and non-empty
# - ${var+word}   - Use word if var is set (even if empty)
# - ${var:=word}  - Assign word to var if unset or empty, then return it
# - ${var=word}   - Assign word to var if unset, then return it
#
# Author: Shell Development Team
# Version: 1.0.0
# Target: POSIX.1-2017 Parameter Expansion Compliance
# =============================================================================

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

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo -e "${YELLOW}Test $TOTAL_TESTS: $test_name${NC}"
    echo "Command: $test_command"

    # Run the test and capture output
    local actual_output
    actual_output=$(echo "$test_command" | $SHELL_UNDER_TEST 2>&1)

    echo "Expected: '$expected_output'"
    echo "Actual:   '$actual_output'"

    # Check results
    if [[ "$actual_output" == "$expected_output" ]]; then
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
# PARAMETER EXPANSION VALIDATION TESTS
# =============================================================================

test_default_expansion_colon_dash() {
    print_section "Default Value Expansion (\${var:-word})"

    run_test "Unset variable with default" \
        'echo "${UNSET_VAR:-default_value}"' \
        "default_value"

    run_test "Empty variable with default" \
        'EMPTY_VAR=""; echo "${EMPTY_VAR:-default_value}"' \
        "default_value"

    run_test "Set variable ignores default" \
        'SET_VAR="actual"; echo "${SET_VAR:-default_value}"' \
        "actual"

    run_test "Variable with spaces and default" \
        'SPACE_VAR="hello world"; echo "${SPACE_VAR:-default_value}"' \
        "hello world"

    run_test "Complex default with special chars" \
        'echo "${UNSET_VAR:-/usr/bin:/bin}"' \
        "/usr/bin:/bin"
}

test_default_expansion_dash() {
    print_section "Default Value Expansion (\${var-word})"

    run_test "Unset variable with default (no colon)" \
        'echo "${UNSET_VAR-default_value}"' \
        "default_value"

    run_test "Empty variable without default (no colon)" \
        'EMPTY_VAR=""; echo "${EMPTY_VAR-default_value}"' \
        ""

    run_test "Set variable ignores default (no colon)" \
        'SET_VAR="actual"; echo "${SET_VAR-default_value}"' \
        "actual"

    run_test "Empty string is preserved (no colon)" \
        'EMPTY_VAR=""; echo "[${EMPTY_VAR-default_value}]"' \
        "[]"
}

test_alternative_expansion_colon_plus() {
    print_section "Alternative Value Expansion (\${var:+word})"

    run_test "Unset variable with alternative" \
        'echo "${UNSET_VAR:+alternative_value}"' \
        ""

    run_test "Empty variable with alternative" \
        'EMPTY_VAR=""; echo "${EMPTY_VAR:+alternative_value}"' \
        ""

    run_test "Set variable uses alternative" \
        'SET_VAR="actual"; echo "${SET_VAR:+alternative_value}"' \
        "alternative_value"

    run_test "Non-empty variable uses alternative" \
        'NONEMPTY_VAR="hello"; echo "${NONEMPTY_VAR:+alternative_value}"' \
        "alternative_value"

    run_test "Alternative with complex value" \
        'SET_VAR="test"; echo "${SET_VAR:+prefix_${SET_VAR}_suffix}"' \
        "prefix_test_suffix"
}

test_alternative_expansion_plus() {
    print_section "Alternative Value Expansion (\${var+word})"

    run_test "Unset variable with alternative (no colon)" \
        'echo "${UNSET_VAR+alternative_value}"' \
        ""

    run_test "Empty variable with alternative (no colon)" \
        'EMPTY_VAR=""; echo "${EMPTY_VAR+alternative_value}"' \
        "alternative_value"

    run_test "Set variable uses alternative (no colon)" \
        'SET_VAR="actual"; echo "${SET_VAR+alternative_value}"' \
        "alternative_value"

    run_test "Empty but set variable uses alternative" \
        'EMPTY_VAR=""; echo "[${EMPTY_VAR+set}]"' \
        "[set]"
}

test_assignment_expansion_colon_equals() {
    print_section "Assignment Expansion (\${var:=word})"

    run_test "Assign to unset variable" \
        'echo "${ASSIGN_VAR:=assigned_value}"; echo "Verify: $ASSIGN_VAR"' \
        "assigned_value
Verify: assigned_value"

    run_test "Assign to empty variable" \
        'ASSIGN_VAR=""; echo "${ASSIGN_VAR:=assigned_value}"; echo "Verify: $ASSIGN_VAR"' \
        "assigned_value
Verify: assigned_value"

    run_test "No assignment to set variable" \
        'ASSIGN_VAR="original"; echo "${ASSIGN_VAR:=assigned_value}"; echo "Verify: $ASSIGN_VAR"' \
        "original
Verify: original"
}

test_assignment_expansion_equals() {
    print_section "Assignment Expansion (\${var=word})"

    run_test "Assign to unset variable (no colon)" \
        'echo "${ASSIGN_VAR2=assigned_value}"; echo "Verify: $ASSIGN_VAR2"' \
        "assigned_value
Verify: assigned_value"

    run_test "No assignment to empty variable (no colon)" \
        'ASSIGN_VAR2=""; echo "${ASSIGN_VAR2=assigned_value}"; echo "Verify: $ASSIGN_VAR2"' \
        "
Verify: "

    run_test "No assignment to set variable (no colon)" \
        'ASSIGN_VAR2="original"; echo "${ASSIGN_VAR2=assigned_value}"; echo "Verify: $ASSIGN_VAR2"' \
        "original
Verify: original"
}

test_complex_scenarios() {
    print_section "Complex Parameter Expansion Scenarios"

    run_test "Nested variable references in default" \
        'DEFAULT="backup"; echo "${UNSET_VAR:-$DEFAULT}"' \
        "backup"

    run_test "Command substitution in default" \
        'echo "${UNSET_VAR:-$(echo computed_default)}"' \
        "computed_default"

    run_test "Multiple expansions in one command" \
        'A="hello"; B=""; echo "${A:-fallback} ${B:-world}"' \
        "hello world"

    run_test "Parameter expansion with arithmetic" \
        'COUNT=5; echo "${COUNT:+Count is $COUNT}"' \
        "Count is 5"

    run_test "URL parsing with parameter expansion" \
        'URL="https://example.com"; echo "${URL#*://}"' \
        "example.com"

    run_test "File extension with parameter expansion" \
        'FILE="script.sh"; echo "${FILE%.*}"' \
        "script"

    run_test "Environment variable fallback" \
        'echo "${CUSTOM_PATH:-$PATH}" | head -c 10' \
        "/usr/local"
}

test_edge_cases() {
    print_section "Edge Cases and Special Characters"

    run_test "Empty string as default" \
        'echo "[${UNSET_VAR:-}]"' \
        "[]"

    run_test "Whitespace in values" \
        'echo "${UNSET_VAR:-  spaces  }"' \
        "  spaces  "

    run_test "Special characters in default" \
        'echo "${UNSET_VAR:-!@#$%^&*()}"' \
        "!@#$%^&*()"

    run_test "Colon in default value" \
        'echo "${UNSET_VAR:-http://example.com:8080}"' \
        "http://example.com:8080"

    run_test "Equals sign in default value" \
        'echo "${UNSET_VAR:-key=value}"' \
        "key=value"

    run_test "Braces in default value" \
        'echo "${UNSET_VAR:-{json: value}}"' \
        "{json: value}"

    run_test "Single quotes in default" \
        'echo "${UNSET_VAR:-'"'"'quoted'"'"'}"' \
        "'quoted'"

    run_test "Double quotes in alternative" \
        'SET="value"; echo "${SET:+\"quoted\"}"' \
        '"quoted"'
}

test_posix_compliance() {
    print_section "POSIX Compliance Verification"

    run_test "POSIX variable naming" \
        '_valid_var=test; echo "${_valid_var:-default}"' \
        "test"

    run_test "POSIX numeric parameter" \
        'set -- arg1 arg2; echo "${1:-default}"' \
        "arg1"

    run_test "POSIX special parameter $#" \
        'set -- a b c; echo "${#:-0}"' \
        "3"

    run_test "POSIX special parameter $$" \
        'echo "${$:-0}" | grep -E "^[0-9]+$" > /dev/null && echo "VALID_PID"' \
        "VALID_PID"

    run_test "Case sensitivity" \
        'lower=test; UPPER=TEST; echo "${lower:-x} ${UPPER:-x}"' \
        "test TEST"

    run_test "Recursive expansion prevention" \
        'VAR="${VAR:-default}"; echo "$VAR"' \
        "default"
}

# =============================================================================
# MAIN EXECUTION
# =============================================================================

main() {
    print_header "LUSUSH PARAMETER EXPANSION VALIDATION TEST"

    echo -e "${BLUE}Shell under test: $SHELL_UNDER_TEST${NC}"
    echo -e "${BLUE}Test start time: $(date)${NC}\n"

    # Verify shell exists
    if [[ ! -x "$SHELL_UNDER_TEST" ]]; then
        echo -e "${RED}ERROR: Shell executable not found: $SHELL_UNDER_TEST${NC}"
        echo "Please build the shell first with: ninja -C builddir"
        exit 1
    fi

    # Run all test categories
    test_default_expansion_colon_dash
    test_default_expansion_dash
    test_alternative_expansion_colon_plus
    test_alternative_expansion_plus
    test_assignment_expansion_colon_equals
    test_assignment_expansion_equals
    test_complex_scenarios
    test_edge_cases
    test_posix_compliance

    # Final results
    print_header "PARAMETER EXPANSION VALIDATION RESULTS"

    echo -e "${CYAN}SUMMARY:${NC}"
    echo -e "  Total tests: $TOTAL_TESTS"
    echo -e "  ${GREEN}Passed: $PASSED_TESTS${NC}"
    echo -e "  ${RED}Failed: $FAILED_TESTS${NC}"

    local pass_percentage=0
    if [[ $TOTAL_TESTS -gt 0 ]]; then
        pass_percentage=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    echo -e "  ${CYAN}Success rate: ${pass_percentage}%${NC}\n"

    echo -e "${CYAN}PARAMETER EXPANSION COMPLIANCE:${NC}"
    if [[ $pass_percentage -ge 95 ]]; then
        echo -e "${GREEN}🏆 EXCELLENT: Full POSIX parameter expansion compliance${NC}"
    elif [[ $pass_percentage -ge 90 ]]; then
        echo -e "${GREEN}✅ VERY GOOD: Strong parameter expansion support${NC}"
    elif [[ $pass_percentage -ge 80 ]]; then
        echo -e "${YELLOW}⚠️  GOOD: Basic parameter expansion working${NC}"
    else
        echo -e "${RED}❌ POOR: Parameter expansion needs significant work${NC}"
    fi

    if [[ $FAILED_TESTS -gt 0 ]]; then
        echo -e "\n${RED}FAILED TESTS REQUIRE ATTENTION${NC}"
        echo -e "Parameter expansion is fundamental to shell scripting."
        echo -e "All forms should work correctly for POSIX compliance."
    else
        echo -e "\n${GREEN}🎉 ALL PARAMETER EXPANSION TESTS PASSED!${NC}"
        echo -e "Shell demonstrates complete POSIX parameter expansion support."
    fi

    echo -e "\n${BLUE}Parameter expansion validation completed.${NC}"

    # Exit with appropriate code
    if [[ $FAILED_TESTS -eq 0 ]]; then
        exit 0
    else
        exit 1
    fi
}

# Execute main function
main "$@"

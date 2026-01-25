#!/usr/local/opt/bash/bin/bash
#
# test_framework.sh - Core testing utilities for lush compatibility validation
#
# Requires bash 4.0+ for associative arrays (uses Homebrew bash on macOS)
#
# Provides functions for:
# - Running syntax checks across shells
# - Running execution tests with output comparison
# - Managing test state and results
# - Profile-aware testing
#

# Strict mode
set -euo pipefail

# ============================================================================
# Configuration
# ============================================================================

# Shell paths (can be overridden)
BASH_PATH="${BASH_PATH:-bash}"
ZSH_PATH="${ZSH_PATH:-zsh}"
LUSH_PATH="${LUSH_PATH:-}"

# Find lush if not specified
find_lush() {
    if [[ -n "$LUSH_PATH" && -x "$LUSH_PATH" ]]; then
        return 0
    fi

    # Try common locations
    local candidates=(
        "../../build/lush"
        "../../../build/lush"
        "./build/lush"
        "$(dirname "$0")/../../build/lush"
    )

    for path in "${candidates[@]}"; do
        if [[ -x "$path" ]]; then
            LUSH_PATH="$(cd "$(dirname "$path")" && pwd)/$(basename "$path")"
            return 0
        fi
    done

    echo "Error: lush binary not found. Build it or set LUSH_PATH." >&2
    return 1
}

# ============================================================================
# Result Tracking
# ============================================================================

declare -A TEST_RESULTS
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Current test context
CURRENT_TEST=""
CURRENT_FEATURE=""
CURRENT_PROFILES=""

# Test file parsing state
TEST_NAME=""
TEST_FEATURE=""
TEST_PROFILES=()
TEST_INPUT=""
declare -A TEST_EXPECTS

reset_results() {
    TEST_RESULTS=()
    TOTAL_TESTS=0
    PASSED_TESTS=0
    FAILED_TESTS=0
    SKIPPED_TESTS=0
}

record_result() {
    local test_name="$1"
    local shell="$2"
    local result="$3"  # pass, fail, skip
    local detail="${4:-}"

    local key="${test_name}:${shell}"
    TEST_RESULTS["$key"]="$result:$detail"

    # Use || true to prevent exit code 1 when incrementing from 0
    # (bash arithmetic returns 1 when result is 0, which happens when var was 0)
    ((TOTAL_TESTS++)) || true
    case "$result" in
        pass) ((PASSED_TESTS++)) || true ;;
        fail) ((FAILED_TESTS++)) || true ;;
        skip) ((SKIPPED_TESTS++)) || true ;;
    esac
}

# ============================================================================
# Shell Execution
# ============================================================================

# Check if a shell is available
shell_available() {
    local shell="$1"
    case "$shell" in
        bash) command -v "$BASH_PATH" &>/dev/null ;;
        zsh)  command -v "$ZSH_PATH" &>/dev/null ;;
        lush) [[ -n "$LUSH_PATH" && -x "$LUSH_PATH" ]] ;;
        *)    return 1 ;;
    esac
}

# Get shell command for a given shell name and optional profile
get_shell_cmd() {
    local shell="$1"
    local profile="${2:-}"

    case "$shell" in
        bash) echo "$BASH_PATH" ;;
        zsh)  echo "$ZSH_PATH" ;;
        lush)
            if [[ -n "$profile" && "$profile" != "lush" ]]; then
                # Run lush with specific profile
                echo "$LUSH_PATH -c 'set -o $profile; eval \"\$1\"' --"
            else
                echo "$LUSH_PATH"
            fi
            ;;
    esac
}

# Run syntax check (-n flag) on input
# Returns: 0 if parses, 1 if fails
syntax_check() {
    local shell="$1"
    local input="$2"
    local profile="${3:-}"

    case "$shell" in
        bash)
            echo "$input" | "$BASH_PATH" -n 2>/dev/null
            ;;
        zsh)
            echo "$input" | "$ZSH_PATH" -n 2>/dev/null
            ;;
        lush)
            if [[ -n "$profile" && "$profile" != "lush" ]]; then
                # First set profile, then check syntax
                echo "set -o $profile; $input" | "$LUSH_PATH" -n 2>/dev/null
            else
                echo "$input" | "$LUSH_PATH" -n 2>/dev/null
            fi
            ;;
    esac
}

# Run code and capture output
# Sets: EXEC_OUTPUT, EXEC_EXIT_CODE, EXEC_STDERR
run_code() {
    local shell="$1"
    local input="$2"
    local profile="${3:-}"

    local tmpout=$(mktemp)
    local tmperr=$(mktemp)
    trap "rm -f '$tmpout' '$tmperr'" RETURN

    local exit_code=0

    case "$shell" in
        bash)
            echo "$input" | "$BASH_PATH" >"$tmpout" 2>"$tmperr" || exit_code=$?
            ;;
        zsh)
            echo "$input" | "$ZSH_PATH" >"$tmpout" 2>"$tmperr" || exit_code=$?
            ;;
        lush)
            if [[ -n "$profile" && "$profile" != "lush" ]]; then
                echo "set -o $profile
$input" | "$LUSH_PATH" >"$tmpout" 2>"$tmperr" || exit_code=$?
            else
                echo "$input" | "$LUSH_PATH" >"$tmpout" 2>"$tmperr" || exit_code=$?
            fi
            ;;
    esac

    EXEC_OUTPUT="$(cat "$tmpout")"
    EXEC_STDERR="$(cat "$tmperr")"
    EXEC_EXIT_CODE=$exit_code
}

# ============================================================================
# Test Assertions
# ============================================================================

# Assert syntax parses successfully
assert_parses() {
    local shell="$1"
    local input="$2"
    local profile="${3:-}"

    if syntax_check "$shell" "$input" "$profile"; then
        return 0
    else
        return 1
    fi
}

# Assert syntax fails to parse
assert_parse_fails() {
    local shell="$1"
    local input="$2"
    local profile="${3:-}"

    if syntax_check "$shell" "$input" "$profile"; then
        return 1
    else
        return 0
    fi
}

# Assert output matches expected
assert_output() {
    local shell="$1"
    local input="$2"
    local expected="$3"
    local profile="${4:-}"

    run_code "$shell" "$input" "$profile"

    if [[ "$EXEC_OUTPUT" == "$expected" ]]; then
        return 0
    else
        return 1
    fi
}

# Assert exit code matches expected
assert_exit_code() {
    local shell="$1"
    local input="$2"
    local expected="$3"
    local profile="${4:-}"

    run_code "$shell" "$input" "$profile"

    if [[ "$EXEC_EXIT_CODE" -eq "$expected" ]]; then
        return 0
    else
        return 1
    fi
}

# ============================================================================
# Test File Parsing
# ============================================================================

# Parse a .test file and extract components
# Sets global variables for the test
parse_test_file() {
    local file="$1"

    # Reset test variables
    TEST_NAME=""
    TEST_FEATURE=""
    TEST_PROFILES=()
    TEST_INPUT=""
    declare -gA TEST_EXPECTS
    TEST_EXPECTS=()

    local in_input=0
    local input_lines=()

    while IFS= read -r line || [[ -n "$line" ]]; do
        # Skip empty lines and comments outside directives
        if [[ $in_input -eq 0 ]]; then
            [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
        fi

        case "$line" in
            @test\ *)
                TEST_NAME="${line#@test }"
                TEST_NAME="${TEST_NAME#\"}"
                TEST_NAME="${TEST_NAME%\"}"
                ;;
            @feature\ *)
                TEST_FEATURE="${line#@feature }"
                ;;
            @profiles\ *)
                read -ra TEST_PROFILES <<< "${line#@profiles }"
                ;;
            @input)
                in_input=1
                input_lines=()
                ;;
            @end)
                in_input=0
                TEST_INPUT="$(printf '%s\n' "${input_lines[@]}")"
                ;;
            @expect\ *)
                local expect_spec="${line#@expect }"
                local key="${expect_spec%%:*}"
                local rest="${expect_spec#*:}"
                local type="${rest%%:*}"
                local value="${rest#*:}"
                # Remove quotes if present
                value="${value#\"}"
                value="${value%\"}"
                value="${value#\'}"
                value="${value%\'}"
                TEST_EXPECTS["${key}:${type}"]="$value"
                ;;
            *)
                if [[ $in_input -eq 1 ]]; then
                    input_lines+=("$line")
                fi
                ;;
        esac
    done < "$file"
}

# ============================================================================
# Test Execution
# ============================================================================

# Run a single test from a parsed test file
run_test() {
    local test_name="$TEST_NAME"
    local feature="$TEST_FEATURE"
    local input="$TEST_INPUT"

    echo "  Testing: $test_name"

    # Test against each shell/profile combination
    for shell in bash zsh lush; do
        if ! shell_available "$shell"; then
            record_result "$test_name" "$shell" "skip" "shell not available"
            continue
        fi

        # Check for parse expectation
        local parse_key="${shell}:parse"
        if [[ -v "TEST_EXPECTS[$parse_key]" ]]; then
            local expected="${TEST_EXPECTS[$parse_key]}"
            if [[ "$expected" == "ok" ]]; then
                if assert_parses "$shell" "$input"; then
                    record_result "$test_name" "$shell" "pass" "parses"
                else
                    record_result "$test_name" "$shell" "fail" "expected to parse"
                fi
            elif [[ "$expected" == "fail" ]]; then
                if assert_parse_fails "$shell" "$input"; then
                    record_result "$test_name" "$shell" "pass" "fails to parse"
                else
                    record_result "$test_name" "$shell" "fail" "expected parse failure"
                fi
            fi
        fi

        # Check for output expectation
        local output_key="${shell}:output"
        if [[ -v "TEST_EXPECTS[$output_key]" ]]; then
            local expected="${TEST_EXPECTS[$output_key]}"
            run_code "$shell" "$input"
            if [[ "$EXEC_OUTPUT" == "$expected" ]]; then
                record_result "$test_name" "$shell" "pass" "output matches"
            else
                record_result "$test_name" "$shell" "fail" "output mismatch: got '$EXEC_OUTPUT', expected '$expected'"
            fi
        fi

        # Check for exit code expectation
        local exit_key="${shell}:exit"
        if [[ -v "TEST_EXPECTS[$exit_key]" ]]; then
            local expected="${TEST_EXPECTS[$exit_key]}"
            run_code "$shell" "$input"
            if [[ "$EXEC_EXIT_CODE" -eq "$expected" ]]; then
                record_result "$test_name" "$shell" "pass" "exit code matches"
            else
                record_result "$test_name" "$shell" "fail" "exit code mismatch: got $EXEC_EXIT_CODE, expected $expected"
            fi
        fi
    done

    # Test lush with different profiles
    for profile in bash zsh posix; do
        local lush_profile_key="lush:${profile}"

        # Parse expectation with profile
        local parse_key="${lush_profile_key}:parse"
        if [[ -v "TEST_EXPECTS[$parse_key]" ]]; then
            local expected="${TEST_EXPECTS[$parse_key]}"
            if [[ "$expected" == "ok" ]]; then
                if assert_parses "lush" "$input" "$profile"; then
                    record_result "$test_name" "$lush_profile_key" "pass" "parses"
                else
                    record_result "$test_name" "$lush_profile_key" "fail" "expected to parse"
                fi
            fi
        fi

        # Output expectation with profile
        local output_key="${lush_profile_key}:output"
        if [[ -v "TEST_EXPECTS[$output_key]" ]]; then
            local expected="${TEST_EXPECTS[$output_key]}"
            run_code "lush" "$input" "$profile"
            if [[ "$EXEC_OUTPUT" == "$expected" ]]; then
                record_result "$test_name" "$lush_profile_key" "pass" "output matches"
            else
                record_result "$test_name" "$lush_profile_key" "fail" "output mismatch: got '$EXEC_OUTPUT', expected '$expected'"
            fi
        fi
    done
}

# Run all tests in a directory
run_tests_in_dir() {
    local dir="$1"
    local pattern="${2:-*.test}"

    echo "Running tests in: $dir"

    for test_file in "$dir"/$pattern; do
        [[ -f "$test_file" ]] || continue

        parse_test_file "$test_file"

        if [[ -n "$TEST_NAME" ]]; then
            run_test
        fi
    done
}

# ============================================================================
# Reporting
# ============================================================================

print_summary() {
    echo ""
    echo "=================================="
    echo "Test Summary"
    echo "=================================="
    echo "Total:   $TOTAL_TESTS"
    echo "Passed:  $PASSED_TESTS"
    echo "Failed:  $FAILED_TESTS"
    echo "Skipped: $SKIPPED_TESTS"
    echo ""

    if [[ $FAILED_TESTS -gt 0 ]]; then
        echo "Failed tests:"
        for key in "${!TEST_RESULTS[@]}"; do
            local result="${TEST_RESULTS[$key]}"
            if [[ "$result" == fail:* ]]; then
                local detail="${result#fail:}"
                echo "  $key: $detail"
            fi
        done
    fi
}

# ============================================================================
# Initialization
# ============================================================================

init_framework() {
    find_lush || return 1
    reset_results
    echo "Lush Compatibility Validation Framework"
    echo "========================================"
    echo "bash: $BASH_PATH ($(bash --version | head -1))"
    echo "zsh:  $ZSH_PATH ($(zsh --version))"
    echo "lush: $LUSH_PATH ($("$LUSH_PATH" --version 2>&1 | head -1))"
    echo ""
}

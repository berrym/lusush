#!/usr/local/opt/bash/bin/bash
#
# validate.sh - Lush Compatibility Validation Runner
#
# Requires bash 4.0+ for associative arrays (uses Homebrew bash on macOS)
#
# Comprehensive testing of lush compatibility with bash and zsh.
# Tests syntax parsing, feature behavior, and semantic differences
# across shell profiles.
#
# Usage:
#   ./validate.sh [options] [test_path...]
#
# Options:
#   -c, --category CAT   Run tests from category (syntax, features, semantic)
#   -f, --feature FEAT   Run tests for specific feature
#   -p, --profile PROF   Focus on specific profile (bash, zsh, posix, lush)
#   -v, --verbose        Show all test details
#   -q, --quiet          Only show summary
#   --report TYPE        Generate report (gaps, coverage, all)
#   --list-features      List all testable features
#   -h, --help           Show this help
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source the test framework
source "$SCRIPT_DIR/lib/test_framework.sh"

# ============================================================================
# Command Line Options
# ============================================================================

CATEGORY=""
FEATURE=""
PROFILE=""
VERBOSE=0
QUIET=0
REPORT=""
LIST_FEATURES=0
TEST_PATHS=()

usage() {
    sed -n '2,/^$/p' "$0" | grep '^#' | sed 's/^# \?//'
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--category)
            CATEGORY="$2"
            shift 2
            ;;
        -f|--feature)
            FEATURE="$2"
            shift 2
            ;;
        -p|--profile)
            PROFILE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -q|--quiet)
            QUIET=1
            shift
            ;;
        --report)
            REPORT="$2"
            shift 2
            ;;
        --list-features)
            LIST_FEATURES=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        -*)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
        *)
            TEST_PATHS+=("$1")
            shift
            ;;
    esac
done

# ============================================================================
# Feature Listing
# ============================================================================

list_features() {
    echo "Testable Features (from lush feature matrix):"
    echo ""
    echo "Arrays:"
    echo "  FEATURE_INDEXED_ARRAYS      - arr=(a b c), \${arr[0]}"
    echo "  FEATURE_ASSOCIATIVE_ARRAYS  - declare -A, \${arr[key]}"
    echo "  FEATURE_ARRAY_ZERO_INDEXED  - 0-indexed (bash) vs 1-indexed (zsh)"
    echo "  FEATURE_ARRAY_APPEND        - arr+=(value)"
    echo ""
    echo "Arithmetic:"
    echo "  FEATURE_ARITH_COMMAND       - (( expr ))"
    echo "  FEATURE_LET_BUILTIN         - let command"
    echo ""
    echo "Extended Tests:"
    echo "  FEATURE_EXTENDED_TEST       - [[ ]]"
    echo "  FEATURE_REGEX_MATCH         - =~ operator"
    echo "  FEATURE_PATTERN_MATCH       - == pattern in [[ ]]"
    echo ""
    echo "Process Substitution:"
    echo "  FEATURE_PROCESS_SUBSTITUTION - <(cmd), >(cmd)"
    echo "  FEATURE_PIPE_STDERR          - |&"
    echo "  FEATURE_COPROC               - coproc"
    echo ""
    echo "Parameter Expansion:"
    echo "  FEATURE_CASE_MODIFICATION    - \${var^^}, \${var,,}"
    echo "  FEATURE_SUBSTRING_EXPANSION  - \${var:offset:len}"
    echo "  FEATURE_PATTERN_SUBSTITUTION - \${var/pat/repl}"
    echo "  FEATURE_INDIRECT_EXPANSION   - \${!var}"
    echo "  FEATURE_ZSH_PARAM_FLAGS      - \${(U)var}, \${(L)var}"
    echo ""
    echo "Globbing:"
    echo "  FEATURE_EXTENDED_GLOB        - ?(pat), *(pat), etc"
    echo "  FEATURE_NULL_GLOB            - unmatched -> nothing"
    echo "  FEATURE_GLOBSTAR             - ** recursive"
    echo "  FEATURE_GLOB_QUALIFIERS      - *(.) *(/) zsh qualifiers"
    echo ""
    echo "Behavior:"
    echo "  FEATURE_WORD_SPLIT_DEFAULT   - word splitting behavior"
    echo "  FEATURE_BRACE_EXPANSION      - {a,b,c}, {1..10}"
    echo "  FEATURE_ANSI_QUOTING         - \$'...'"
    echo ""
    echo "Control Flow:"
    echo "  FEATURE_CASE_FALLTHROUGH     - ;& and ;;&"
    echo "  FEATURE_SELECT_LOOP          - select"
    echo ""
    exit 0
}

if [[ $LIST_FEATURES -eq 1 ]]; then
    list_features
fi

# ============================================================================
# Test Discovery
# ============================================================================

discover_tests() {
    local base_dir="$SCRIPT_DIR/corpus"
    local tests=()

    if [[ ${#TEST_PATHS[@]} -gt 0 ]]; then
        # Use specified paths
        for path in "${TEST_PATHS[@]}"; do
            if [[ -f "$path" ]]; then
                tests+=("$path")
            elif [[ -d "$path" ]]; then
                while IFS= read -r -d '' f; do
                    tests+=("$f")
                done < <(find "$path" -name "*.test" -print0)
            fi
        done
    elif [[ -n "$CATEGORY" ]]; then
        # Filter by category
        local cat_dir="$base_dir/$CATEGORY"
        if [[ -d "$cat_dir" ]]; then
            while IFS= read -r -d '' f; do
                tests+=("$f")
            done < <(find "$cat_dir" -name "*.test" -print0)
        else
            echo "Unknown category: $CATEGORY" >&2
            exit 1
        fi
    else
        # All tests
        while IFS= read -r -d '' f; do
            tests+=("$f")
        done < <(find "$base_dir" -name "*.test" -print0)
    fi

    printf '%s\n' "${tests[@]}"
}

# ============================================================================
# Test Execution
# ============================================================================

run_all_tests() {
    local tests
    mapfile -t tests < <(discover_tests)

    if [[ ${#tests[@]} -eq 0 ]]; then
        echo "No tests found."
        return 0
    fi

    echo "Found ${#tests[@]} test file(s)"
    echo ""

    for test_file in "${tests[@]}"; do
        if [[ $QUIET -eq 0 ]]; then
            echo "File: $(basename "$test_file")"
        fi

        # Parse and run each test in the file
        # Handle multiple tests per file (separated by ---)
        local current_test=""
        local in_test=0

        while IFS= read -r line || [[ -n "$line" ]]; do
            if [[ "$line" == "---" ]]; then
                # End of current test, run it if we have one
                if [[ $in_test -eq 1 && -n "$TEST_NAME" ]]; then
                    run_test
                fi
                # Reset for next test
                TEST_NAME=""
                TEST_FEATURE=""
                TEST_PROFILES=()
                TEST_INPUT=""
                unset TEST_EXPECTS
                declare -gA TEST_EXPECTS
                in_test=0
                continue
            fi

            # Parse test directives
            case "$line" in
                @test\ *)
                    # If we already have a test, run it first
                    if [[ $in_test -eq 1 && -n "$TEST_NAME" ]]; then
                        run_test
                    fi
                    TEST_NAME="${line#@test }"
                    TEST_NAME="${TEST_NAME#\"}"
                    TEST_NAME="${TEST_NAME%\"}"
                    unset TEST_EXPECTS
                    declare -gA TEST_EXPECTS
                    in_test=1
                    ;;
                @feature\ *)
                    TEST_FEATURE="${line#@feature }"
                    ;;
                @profiles\ *)
                    read -ra TEST_PROFILES <<< "${line#@profiles }"
                    ;;
                @input)
                    TEST_INPUT=""
                    local reading_input=1
                    while IFS= read -r input_line; do
                        if [[ "$input_line" == "@end" ]]; then
                            break
                        fi
                        if [[ -n "$TEST_INPUT" ]]; then
                            TEST_INPUT+=$'\n'
                        fi
                        TEST_INPUT+="$input_line"
                    done
                    ;;
                @expect\ *)
                    local expect_spec="${line#@expect }"
                    # Parse: shell:type value or shell:profile:type value
                    local key value
                    if [[ "$expect_spec" =~ ^([^[:space:]]+)[[:space:]]+(.*) ]]; then
                        key="${BASH_REMATCH[1]}"
                        value="${BASH_REMATCH[2]}"
                        # Remove quotes
                        value="${value#\"}"
                        value="${value%\"}"
                        TEST_EXPECTS["$key"]="$value"
                    fi
                    ;;
            esac
        done < "$test_file"

        # Run final test if any
        if [[ $in_test -eq 1 && -n "$TEST_NAME" ]]; then
            run_test
        fi

        echo ""
    done
}

# ============================================================================
# Report Generation
# ============================================================================

generate_gap_report() {
    echo "# Lush Compatibility Gap Report"
    echo ""
    echo "Generated: $(date)"
    echo ""
    echo "## Summary"
    echo ""
    echo "- Total tests: $TOTAL_TESTS"
    echo "- Passed: $PASSED_TESTS"
    echo "- Failed: $FAILED_TESTS"
    echo "- Skipped: $SKIPPED_TESTS"
    echo ""

    if [[ $FAILED_TESTS -gt 0 ]]; then
        echo "## Failed Tests (Potential Gaps)"
        echo ""
        for key in "${!TEST_RESULTS[@]}"; do
            local result="${TEST_RESULTS[$key]}"
            if [[ "$result" == fail:* ]]; then
                local detail="${result#fail:}"
                echo "- **$key**: $detail"
            fi
        done
        echo ""
    fi

    echo "## Feature Coverage"
    echo ""
    echo "TODO: Implement feature coverage tracking"
}

# ============================================================================
# Main
# ============================================================================

main() {
    init_framework || exit 1

    run_all_tests

    print_summary

    if [[ -n "$REPORT" ]]; then
        case "$REPORT" in
            gaps)
                generate_gap_report > "$SCRIPT_DIR/reports/gaps.md"
                echo "Gap report written to reports/gaps.md"
                ;;
            *)
                echo "Unknown report type: $REPORT"
                ;;
        esac
    fi

    # Return non-zero if there were failures
    [[ $FAILED_TESTS -eq 0 ]]
}

main

#!/bin/bash

# Basic Case Statement Test for Lusush Shell
# Simple debugging test to identify case statement issues

set -e

SHELL_PATH="./builddir/lusush"
TEST_COUNT=0
PASSED_COUNT=0
FAILED_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=== BASIC CASE STATEMENT DEBUG TEST ==="
echo "Testing fundamental case statement functionality"
echo "Shell: $SHELL_PATH"
echo

# Check if shell exists and build if needed
if [ ! -x "$SHELL_PATH" ]; then
    echo "Building lusush shell..."
    meson compile -C builddir > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: Failed to build shell${NC}"
        exit 1
    fi
fi

# Test function
test_case() {
    local test_name="$1"
    local input="$2"
    local expected="$3"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${YELLOW}Test $TEST_COUNT: $test_name${NC}"
    echo "Input: $input"
    echo "Expected: '$expected'"

    # Execute the command
    local output
    output=$(echo "$input" | "$SHELL_PATH" 2>&1 | tail -n 1)
    local exit_code=$?

    echo "Output: '$output'"
    echo "Exit code: $exit_code"

    if [ $exit_code -eq 0 ] && [ "$output" = "$expected" ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "${RED}✗ FAILED${NC}"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
    echo
}

echo -e "${BLUE}=== BASIC CASE SYNTAX ===${NC}"

test_case "Simple exact match" \
    'case hello in hello) echo "match" ;; esac' \
    "match"

test_case "No match case" \
    'case test in hello) echo "no" ;; esac' \
    ""

test_case "Second pattern match" \
    'case test in hello) echo "no" ;; test) echo "yes" ;; esac' \
    "yes"

test_case "Default pattern (*)" \
    'case nomatch in hello) echo "no" ;; *) echo "default" ;; esac' \
    "default"

echo -e "${BLUE}=== WILDCARD PATTERNS ===${NC}"

test_case "Simple wildcard (*)" \
    'case hello in h*) echo "wildcard" ;; esac' \
    "wildcard"

test_case "Question mark (?)" \
    'case test in t?st) echo "question" ;; esac' \
    "question"

test_case "No wildcard match" \
    'case hello in x*) echo "no" ;; *) echo "default" ;; esac' \
    "default"

echo -e "${BLUE}=== MULTIPLE PATTERNS ===${NC}"

test_case "Multiple patterns with |" \
    'case test in hello|test) echo "multiple" ;; esac' \
    "multiple"

test_case "Multiple patterns second match" \
    'case hello in hello|test) echo "multiple" ;; esac' \
    "multiple"

echo -e "${BLUE}=== VARIABLES ===${NC}"

test_case "Variable in test word" \
    'var=hello; case $var in hello) echo "variable" ;; esac' \
    "variable"

test_case "Variable in pattern" \
    'pattern=hello; case hello in $pattern) echo "pattern_var" ;; esac' \
    "pattern_var"

echo -e "${BLUE}=== COMMANDS IN CASE ===${NC}"

test_case "Multiple commands" \
    'case test in test) echo "first"; echo "second" ;; esac' \
    "second"

test_case "Command with semicolon" \
    'case test in test) echo "cmd1"; echo "cmd2" ;; esac' \
    "cmd2"

echo -e "${BLUE}=== RESULTS ===${NC}"
echo "Total tests: $TEST_COUNT"
echo -e "${GREEN}Passed: $PASSED_COUNT${NC}"
echo -e "${RED}Failed: $FAILED_COUNT${NC}"

if [ $TEST_COUNT -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_COUNT * 100 / TEST_COUNT))
    echo "Success rate: ${SUCCESS_RATE}%"
fi

echo
if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL BASIC CASE TESTS PASSED!${NC}"
    echo "Case statement implementation is working correctly!"
elif [ $PASSED_COUNT -gt 0 ]; then
    echo -e "${YELLOW}⚠ PARTIAL FUNCTIONALITY${NC}"
    echo "Some case statement features are working."
    echo "Failed tests indicate areas needing improvement."
else
    echo -e "${RED}❌ CASE STATEMENTS NOT WORKING${NC}"
    echo "Case statement implementation needs significant work."
fi

echo
echo "This test helps identify specific issues with:"
echo "  • Basic case syntax and parsing"
echo "  • Pattern matching (exact, wildcards, multiple)"
echo "  • Variable expansion in test words and patterns"
echo "  • Command execution within case blocks"
echo "  • Proper case termination and flow control"

if [ $FAILED_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi

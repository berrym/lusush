#!/bin/bash

# Modern Parameter Expansion Test Suite for Lusush Shell
# Tests advanced parameter expansion features in the modern codebase

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

echo "=== LUSUSH MODERN PARAMETER EXPANSION TEST SUITE ==="
echo "Testing modern parameter expansion features"
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
test_expansion() {
    local test_name="$1"
    local input="$2"
    local expected="$3"

    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "${YELLOW}Test $TEST_COUNT: $test_name${NC}"
    echo "Input: $input"
    echo "Expected: '$expected'"

    # Execute the command
    local output
    output=$(echo "$input" | "$SHELL_PATH" 2>/dev/null | tail -n 1)
    local exit_code=$?

    echo "Output: '$output'"

    if [ $exit_code -eq 0 ] && [ "$output" = "$expected" ]; then
        echo -e "${GREEN}✓ PASSED${NC}"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "${RED}✗ FAILED${NC}"
        if [ $exit_code -ne 0 ]; then
            echo "  Exit code: $exit_code"
        fi
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
    echo
}

echo -e "${BLUE}=== BASIC VARIABLE EXPANSION ===${NC}"

test_expansion "Simple variable expansion" \
    'name=John; echo $name' \
    "John"

test_expansion "Braced variable expansion" \
    'name=John; echo ${name}' \
    "John"

test_expansion "Undefined variable" \
    'echo $undefined' \
    ""

test_expansion "Empty variable" \
    'empty=; echo $empty' \
    ""

echo -e "${BLUE}=== DEFAULT VALUES (:-) ===${NC}"

test_expansion "Default for unset variable" \
    'echo ${unset:-default}' \
    "default"

test_expansion "Default for empty variable" \
    'empty=; echo ${empty:-default}' \
    "default"

test_expansion "No default for set variable" \
    'name=John; echo ${name:-default}' \
    "John"

test_expansion "Default with spaces" \
    'echo ${unset:-hello world}' \
    "hello world"

echo -e "${BLUE}=== ALTERNATIVE VALUES (:+) ===${NC}"

test_expansion "Alternative for set variable" \
    'name=John; echo ${name:+alternative}' \
    "alternative"

test_expansion "No alternative for unset variable" \
    'echo ${unset:+alternative}' \
    ""

test_expansion "No alternative for empty variable" \
    'empty=; echo ${empty:+alternative}' \
    ""

test_expansion "Alternative with spaces" \
    'name=John; echo ${name:+hello world}' \
    "hello world"

echo -e "${BLUE}=== UNSET vs EMPTY DISTINCTION ===${NC}"

test_expansion "Default (-) for unset only" \
    'echo ${unset-default}' \
    "default"

test_expansion "No default (-) for empty" \
    'empty=; echo ${empty-default}' \
    ""

test_expansion "Alternative (+) for set even if empty" \
    'empty=; echo ${empty+alternative}' \
    "alternative"

test_expansion "Alternative (+) for set variable" \
    'name=John; echo ${name+alternative}' \
    "alternative"

echo -e "${BLUE}=== LENGTH EXPANSION ===${NC}"

test_expansion "Length of string" \
    'name=John; echo ${#name}' \
    "4"

test_expansion "Length of empty string" \
    'empty=; echo ${#empty}' \
    "0"

test_expansion "Length of unset variable" \
    'echo ${#unset}' \
    "0"

test_expansion "Length of longer string" \
    'message="Hello World"; echo ${#message}' \
    "11"

test_expansion "Length with special characters" \
    'special="test@#$"; echo ${#special}' \
    "7"

echo -e "${BLUE}=== SUBSTRING EXPANSION ===${NC}"

test_expansion "Substring from start" \
    'text=Hello; echo ${text:1:3}' \
    "ell"

test_expansion "Substring to end" \
    'text=Hello; echo ${text:2}' \
    "llo"

test_expansion "Substring from offset 0" \
    'text=Hello; echo ${text:0:2}' \
    "He"

test_expansion "Substring beyond length" \
    'text=Hi; echo ${text:1:10}' \
    "i"

test_expansion "Substring with offset equal to length" \
    'text=Hi; echo ${text:2}' \
    ""

test_expansion "Substring with negative length" \
    'text=Hello; echo ${text:1:-1}' \
    "ell"

echo -e "${BLUE}=== COMMAND SUBSTITUTION ===${NC}"

test_expansion "Simple command substitution" \
    'echo $(echo hello)' \
    "hello"

test_expansion "Command substitution with variable" \
    'cmd=echo; text=world; echo $(${cmd} ${text})' \
    "world"

test_expansion "Backtick command substitution" \
    'echo `echo hello`' \
    "hello"

test_expansion "Command substitution in quotes" \
    'echo "Result: $(echo test)"' \
    "Result: test"

echo -e "${BLUE}=== COMPLEX COMBINATIONS ===${NC}"

test_expansion "Default with variable" \
    'default=backup; echo ${unset:-$default}' \
    "backup"

test_expansion "Alternative with variable" \
    'name=John; alt=Mr; echo ${name:+$alt $name}' \
    "Mr John"

test_expansion "Nested parameter expansion" \
    'name=John; echo ${name:+Hello ${name}}' \
    "Hello John"

test_expansion "Length of default value" \
    'echo ${#unset:-default}' \
    "0"

test_expansion "Multiple expansions" \
    'first=A; second=B; echo ${first:-X}${second:-Y}' \
    "AB"

echo -e "${BLUE}=== INTEGRATION WITH EXISTING FEATURES ===${NC}"

test_expansion "Parameter expansion in double quotes" \
    'name=John; echo "Hello ${name:-Guest}"' \
    "Hello John"

test_expansion "Parameter expansion with arithmetic" \
    'num=5; echo ${num:-0}' \
    "5"

test_expansion "Parameter expansion with logical operators" \
    'name=John; test -n "${name}" && echo "Name is ${name}"' \
    "Name is John"

test_expansion "Parameter expansion in FOR loop" \
    'for item in ${items:-one two}; do echo $item; done' \
    "one"

echo -e "${BLUE}=== EDGE CASES ===${NC}"

test_expansion "Empty default" \
    'echo "${unset:-}"' \
    ""

test_expansion "Colon in default value" \
    'echo ${unset:-http://example.com}' \
    "http://example.com"

test_expansion "Braces in default value" \
    'echo ${unset:-{test}}' \
    "{test}"

test_expansion "Dollar sign in default" \
    'echo ${unset:-$100}' \
    "$100"

test_expansion "Multiple colons in substring" \
    'text="a:b:c:d"; echo ${text:2:3}' \
    "b:c"

echo -e "${BLUE}=== ADVANCED PATTERNS ===${NC}"

test_expansion "Substring with variable indices" \
    'text=Hello; start=1; len=3; echo ${text:$start:$len}' \
    "ell"

test_expansion "Default with command substitution" \
    'echo ${unset:-$(echo default)}' \
    "default"

test_expansion "Chained parameter expansions" \
    'first=John; echo ${first:+${first} Doe}' \
    "John Doe"

test_expansion "Parameter expansion in assignment" \
    'name=John; full=${name:+Mr. $name}; echo $full' \
    "Mr. John"

echo -e "${BLUE}=== RESULTS SUMMARY ===${NC}"
echo "========================================"
echo "Total tests run: $TEST_COUNT"
echo -e "${GREEN}Passed: $PASSED_COUNT${NC}"
echo -e "${RED}Failed: $FAILED_COUNT${NC}"

if [ $TEST_COUNT -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_COUNT * 100 / TEST_COUNT))
    echo "Success rate: ${SUCCESS_RATE}%"
fi

echo

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL PARAMETER EXPANSION TESTS PASSED!${NC}"
    echo "Modern parameter expansion is working correctly!"
    echo
    echo "✅ Verified features:"
    echo "  • Basic variable expansion (\$var, \${var})"
    echo "  • Default values (\${var:-default}, \${var-default})"
    echo "  • Alternative values (\${var:+alt}, \${var+alt})"
    echo "  • Length expansion (\${#var})"
    echo "  • Substring expansion (\${var:offset:length})"
    echo "  • Command substitution (\$(cmd), \`cmd\`)"
    echo "  • Complex combinations and edge cases"
    echo "  • Integration with existing shell features"

elif [ $FAILED_COUNT -le 3 ]; then
    echo -e "${YELLOW}⚠ MINOR ISSUES DETECTED${NC}"
    echo "Most parameter expansion functionality works correctly."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed - likely edge cases."

else
    echo -e "${RED}❌ SIGNIFICANT ISSUES DETECTED${NC}"
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
    echo "Parameter expansion implementation needs attention."
fi

echo
echo "=== USAGE EXAMPLES ==="
echo "Modern parameter expansion enables powerful shell patterns:"
echo
echo "# Safe defaults"
echo 'echo "Hello ${USER:-guest}"'
echo
echo "# Conditional values"
echo 'echo ${DEBUG:+--verbose}'
echo
echo "# String manipulation"
echo 'filename=document.txt; echo ${filename:0:8}'
echo
echo "# Length checking"
echo 'if [ ${#password} -lt 8 ]; then echo "Password too short"; fi'
echo
echo "These features provide the foundation for robust shell scripting"
echo "with proper error handling and flexible string manipulation."

if [ $FAILED_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi

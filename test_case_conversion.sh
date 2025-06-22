#!/bin/bash

# Comprehensive Test Suite for Case Conversion Parameter Expansion
# Tests ${var^}, ${var,}, ${var^^}, ${var,,} operators

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

echo "=== LUSUSH CASE CONVERSION PARAMETER EXPANSION TEST SUITE ==="
echo "Testing case conversion: ^, ^^, ,, ,, operators"
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

echo -e "${BLUE}=== FIRST CHARACTER UPPERCASE (^) ===${NC}"
echo "Convert first character to uppercase"
echo

test_case "Lowercase to uppercase (^)" \
    'text=hello; echo ${text^}' \
    "Hello"

test_case "Already uppercase (^)" \
    'text=Hello; echo ${text^}' \
    "Hello"

test_case "All lowercase to first upper (^)" \
    'text=world; echo ${text^}' \
    "World"

test_case "Mixed case first char (^)" \
    'text=hELLO; echo ${text^}' \
    "HELLO"

test_case "Number first character (^)" \
    'text=123abc; echo ${text^}' \
    "123abc"

test_case "Special character first (^)" \
    'text=@hello; echo ${text^}' \
    "@hello"

test_case "Empty string (^)" \
    'empty=; echo ${empty^}' \
    ""

test_case "Single character lowercase (^)" \
    'char=a; echo ${char^}' \
    "A"

test_case "Single character uppercase (^)" \
    'char=A; echo ${char^}' \
    "A"

echo -e "${BLUE}=== FIRST CHARACTER LOWERCASE (,) ===${NC}"
echo "Convert first character to lowercase"
echo

test_case "Uppercase to lowercase (,)" \
    'text=HELLO; echo ${text,}' \
    "hELLO"

test_case "Already lowercase (,)" \
    'text=hello; echo ${text,}' \
    "hello"

test_case "All uppercase to first lower (,)" \
    'text=WORLD; echo ${text,}' \
    "wORLD"

test_case "Mixed case first char (,)" \
    'text=Hello; echo ${text,}' \
    "hello"

test_case "Number first character (,)" \
    'text=123ABC; echo ${text,}' \
    "123ABC"

test_case "Special character first (,)" \
    'text=@HELLO; echo ${text,}' \
    "@HELLO"

test_case "Empty string (,)" \
    'empty=; echo ${empty,}' \
    ""

test_case "Single character uppercase (,)" \
    'char=A; echo ${char,}' \
    "a"

test_case "Single character lowercase (,)" \
    'char=a; echo ${char,}' \
    "a"

echo -e "${BLUE}=== ALL CHARACTERS UPPERCASE (^^) ===${NC}"
echo "Convert all characters to uppercase"
echo

test_case "All lowercase to uppercase (^^)" \
    'text=hello; echo ${text^^}' \
    "HELLO"

test_case "Already all uppercase (^^)" \
    'text=HELLO; echo ${text^^}' \
    "HELLO"

test_case "Mixed case to uppercase (^^)" \
    'text=HeLLo; echo ${text^^}' \
    "HELLO"

test_case "With numbers (^^)" \
    'text=hello123; echo ${text^^}' \
    "HELLO123"

test_case "With special characters (^^)" \
    'text=hello-world; echo ${text^^}' \
    "HELLO-WORLD"

test_case "With spaces (^^)" \
    'text="hello world"; echo ${text^^}' \
    "HELLO WORLD"

test_case "Empty string (^^)" \
    'empty=; echo ${empty^^}' \
    ""

test_case "Numbers only (^^)" \
    'nums=12345; echo ${nums^^}' \
    "12345"

test_case "Special chars only (^^)" \
    'special="@#$%"; echo ${special^^}' \
    "@#$%"

echo -e "${BLUE}=== ALL CHARACTERS LOWERCASE (,,) ===${NC}"
echo "Convert all characters to lowercase"
echo

test_case "All uppercase to lowercase (,,)" \
    'text=HELLO; echo ${text,,}' \
    "hello"

test_case "Already all lowercase (,,)" \
    'text=hello; echo ${text,,}' \
    "hello"

test_case "Mixed case to lowercase (,,)" \
    'text=HeLLo; echo ${text,,}' \
    "hello"

test_case "With numbers (,,)" \
    'text=HELLO123; echo ${text,,}' \
    "hello123"

test_case "With special characters (,,)" \
    'text=HELLO-WORLD; echo ${text,,}' \
    "hello-world"

test_case "With spaces (,,)" \
    'text="HELLO WORLD"; echo ${text,,}' \
    "hello world"

test_case "Empty string (,,)" \
    'empty=; echo ${empty,,}' \
    ""

test_case "Numbers only (,,)" \
    'nums=12345; echo ${nums,,}' \
    "12345"

test_case "Special chars only (,,)" \
    'special="@#$%"; echo ${special,,}' \
    "@#$%"

echo -e "${BLUE}=== REAL-WORLD USE CASES ===${NC}"
echo "Practical applications of case conversion"
echo

test_case "Capitalize name" \
    'name=john; echo ${name^}' \
    "John"

test_case "Convert to constant case" \
    'var=my_variable; echo ${var^^}' \
    "MY_VARIABLE"

test_case "Convert constant to lowercase" \
    'CONST=MAX_SIZE; echo ${CONST,,}' \
    "max_size"

test_case "File extension to uppercase" \
    'file=document.pdf; ext=${file##*.}; echo ${ext^^}' \
    "PDF"

test_case "Normalize email domain" \
    'email=USER@EXAMPLE.COM; domain=${email#*@}; echo ${domain,,}' \
    "example.com"

test_case "Protocol normalization" \
    'url=HTTPS://EXAMPLE.COM; protocol=${url%://*}; echo ${protocol,,}' \
    "https"

test_case "Environment variable style" \
    'setting=debug_mode; echo ${setting^^}' \
    "DEBUG_MODE"

test_case "Camel case start" \
    'word=myFunction; echo ${word,}' \
    "myFunction"

echo -e "${BLUE}=== INTEGRATION WITH OTHER FEATURES ===${NC}"
echo "Case conversion with other parameter expansion"
echo

test_case "Case conversion with defaults" \
    'echo ${UNSET_VAR:-hello}^' \
    "hello^"

test_case "Case conversion after pattern matching" \
    'filename=DOCUMENT.TXT; base=${filename%.*}; echo ${base,,}' \
    "document"

test_case "Case conversion with alternatives" \
    'DEBUG=1; flag=${DEBUG:+verbose}; echo ${flag^^}' \
    "VERBOSE"

test_case "Multiple case conversions" \
    'text=HELLO; lower=${text,,}; echo ${lower^}' \
    "Hello"

test_case "Case conversion in assignment" \
    'name=john; proper=${name^}; echo $proper' \
    "John"

test_case "Case conversion with logical operators" \
    'text=hello; test "${text^^}" = "HELLO" && echo "match"' \
    "match"

echo -e "${BLUE}=== EDGE CASES AND SPECIAL SCENARIOS ===${NC}"
echo "Testing boundary conditions"
echo

test_case "Unicode characters (basic)" \
    'text=café; echo ${text^^}' \
    "CAFé"

test_case "Mixed alphanumeric" \
    'code=abc123XYZ; echo ${code,,}' \
    "abc123xyz"

test_case "Underscores and hyphens" \
    'var=HELLO_WORLD-TEST; echo ${var,,}' \
    "hello_world-test"

test_case "Leading/trailing spaces" \
    'text=" HELLO "; echo ${text,,}' \
    " hello "

test_case "Only spaces" \
    'spaces="   "; echo ${spaces^^}' \
    "   "

test_case "Tabs and newlines" \
    'text="hello	world"; echo ${text^^}' \
    "HELLO	WORLD"

echo -e "${BLUE}=== COMPLEX COMBINATIONS ===${NC}"
echo "Advanced case conversion scenarios"
echo

test_case "Chain conversions" \
    'text=Hello; upper=${text^^}; lower=${upper,,}; echo ${lower^}' \
    "Hello"

test_case "File processing pipeline" \
    'file=MyDocument.PDF; base=${file%.*}; echo ${base,,}.${file##*.}' \
    "mydocument.pdf"

test_case "Name formatting" \
    'first=john; last=DOE; echo ${first^} ${last^}' \
    "John DOE"

test_case "Environment variable conversion" \
    'config=database_host; echo export ${config^^}=localhost' \
    "export DATABASE_HOST=localhost"

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
    echo -e "${GREEN}🎉 ALL CASE CONVERSION TESTS PASSED!${NC}"
    echo "Case conversion parameter expansion is working perfectly!"
    echo
    echo "✅ Verified features:"
    echo "  • First character uppercase: \${var^}"
    echo "  • First character lowercase: \${var,}"
    echo "  • All characters uppercase: \${var^^}"
    echo "  • All characters lowercase: \${var,,}"
    echo "  • Integration with other parameter expansion features"
    echo "  • Real-world use cases: names, files, constants, protocols"
    echo "  • Proper handling of numbers, special characters, and spaces"

elif [ $FAILED_COUNT -le 3 ]; then
    echo -e "${YELLOW}⚠ MINOR ISSUES DETECTED${NC}"
    echo "Most case conversion functionality works correctly."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed - likely edge cases."

else
    echo -e "${RED}❌ SIGNIFICANT ISSUES DETECTED${NC}"
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
    echo "Case conversion implementation needs attention."
fi

echo
echo "=== USAGE EXAMPLES ==="
echo "Case conversion enables powerful text transformation:"
echo
echo "# Name formatting"
echo 'name=john; echo "Hello, ${name^}!"'
echo
echo "# Environment variables"
echo 'setting=debug_mode; export ${setting^^}=true'
echo
echo "# File processing"
echo 'file=MyDocument.PDF; echo "${file,,}"'
echo
echo "# Protocol normalization"
echo 'url=HTTPS://EXAMPLE.COM; echo "${url,,}"'
echo
echo "# Constant conversion"
echo 'const=MAX_BUFFER_SIZE; echo "${const,,}"'
echo
echo "These features provide essential text transformation capabilities"
echo "for modern shell scripting and system administration tasks."

if [ $FAILED_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi

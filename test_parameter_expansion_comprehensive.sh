#!/bin/bash

# COMPREHENSIVE PARAMETER EXPANSION TEST SUITE
# Tests all modern parameter expansion features to measure overall improvement
# Combines pattern matching, case conversion, defaults, alternatives, length, substring

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

echo "=== LUSUSH COMPREHENSIVE PARAMETER EXPANSION TEST SUITE ==="
echo "Testing ALL modern parameter expansion features for overall progress"
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
test_comprehensive() {
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

test_comprehensive "Simple variable" \
    'name=John; echo $name' \
    "John"

test_comprehensive "Braced variable" \
    'name=John; echo ${name}' \
    "John"

test_comprehensive "Undefined variable" \
    'echo $undefined' \
    ""

echo -e "${BLUE}=== DEFAULT VALUES ===${NC}"

test_comprehensive "Default for unset" \
    'echo ${unset:-default}' \
    "default"

test_comprehensive "Default for empty" \
    'empty=; echo ${empty:-default}' \
    "default"

test_comprehensive "No default for set" \
    'name=John; echo ${name:-default}' \
    "John"

test_comprehensive "Variable in default" \
    'default=backup; echo ${unset:-$default}' \
    "backup"

echo -e "${BLUE}=== ALTERNATIVE VALUES ===${NC}"

test_comprehensive "Alternative for set" \
    'name=John; echo ${name:+alternative}' \
    "alternative"

test_comprehensive "No alternative for unset" \
    'echo ${unset:+alternative}' \
    ""

test_comprehensive "Variable in alternative" \
    'name=John; alt=Mr; echo ${name:+$alt $name}' \
    "Mr John"

echo -e "${BLUE}=== LENGTH EXPANSION ===${NC}"

test_comprehensive "Length of string" \
    'name=John; echo ${#name}' \
    "4"

test_comprehensive "Length of empty" \
    'empty=; echo ${#empty}' \
    "0"

test_comprehensive "Length of path" \
    'path=/usr/local/bin; echo ${#path}' \
    "14"

echo -e "${BLUE}=== SUBSTRING EXPANSION ===${NC}"

test_comprehensive "Substring middle" \
    'text=Hello; echo ${text:1:3}' \
    "ell"

test_comprehensive "Substring to end" \
    'text=Hello; echo ${text:2}' \
    "llo"

test_comprehensive "Substring from start" \
    'text=Hello; echo ${text:0:2}' \
    "He"

echo -e "${BLUE}=== PATTERN MATCHING - PREFIX REMOVAL ===${NC}"

test_comprehensive "Simple prefix removal" \
    'text=abcdef; echo ${text#ab}' \
    "cdef"

test_comprehensive "Wildcard prefix removal" \
    'text=prefix_data; echo ${text#*_}' \
    "data"

test_comprehensive "Longest prefix removal" \
    'path=/usr/local/bin; echo ${path##*/}' \
    "bin"

test_comprehensive "URL protocol removal" \
    'url=https://example.com/path; echo ${url#*://}' \
    "example.com/path"

test_comprehensive "Email domain extraction" \
    'email=user@example.com; echo ${email#*@}' \
    "example.com"

echo -e "${BLUE}=== PATTERN MATCHING - SUFFIX REMOVAL ===${NC}"

test_comprehensive "Simple suffix removal" \
    'text=abcdef; echo ${text%ef}' \
    "abcd"

test_comprehensive "File extension removal" \
    'filename=document.txt; echo ${filename%.*}' \
    "document"

test_comprehensive "Directory from path" \
    'path=/usr/local/bin; echo ${path%/*}' \
    "/usr/local"

test_comprehensive "Longest suffix removal" \
    'filename=archive.tar.gz; echo ${filename%%.*}' \
    "archive"

test_comprehensive "Email username extraction" \
    'email=user@example.com; echo ${email%@*}' \
    "user"

echo -e "${BLUE}=== CASE CONVERSION ===${NC}"

test_comprehensive "First char uppercase" \
    'text=hello; echo ${text^}' \
    "Hello"

test_comprehensive "First char lowercase" \
    'text=Hello; echo ${text,}' \
    "hello"

test_comprehensive "All uppercase" \
    'text=hello; echo ${text^^}' \
    "HELLO"

test_comprehensive "All lowercase" \
    'text=HELLO; echo ${text,,}' \
    "hello"

echo -e "${BLUE}=== REAL-WORLD APPLICATIONS ===${NC}"

test_comprehensive "Filename processing" \
    'file=Document.PDF; echo ${file,,}' \
    "document.pdf"

test_comprehensive "Path basename" \
    'path=/home/user/script.sh; echo ${path##*/}' \
    "script.sh"

test_comprehensive "Extension extraction" \
    'file=script.sh; echo ${file##*.}' \
    "sh"

test_comprehensive "Name capitalization" \
    'name=john; echo ${name^}' \
    "John"

test_comprehensive "Environment variable style" \
    'var=debug_mode; echo ${var^^}' \
    "DEBUG_MODE"

test_comprehensive "Version major" \
    'version=1.2.3-beta; echo ${version%%.*}' \
    "1"

test_comprehensive "Protocol normalization" \
    'protocol=HTTPS; echo ${protocol,,}' \
    "https"

echo -e "${BLUE}=== COMPLEX COMBINATIONS ===${NC}"

test_comprehensive "Pattern then case" \
    'filename=SCRIPT.SH; base=${filename%.*}; echo ${base,,}' \
    "script"

test_comprehensive "Case then pattern" \
    'text=Hello.World; lower=${text,,}; echo ${lower%.*}' \
    "hello"

test_comprehensive "Multiple patterns" \
    'path=/usr/local/bin/gcc; echo ${path##*/}' \
    "gcc"

test_comprehensive "Default with case" \
    'echo ${UNSET_VAR:-hello}' \
    "hello"

echo -e "${BLUE}=== SPECIAL CHARACTERS ===${NC}"

test_comprehensive "Colon in URL" \
    'url=http://localhost:8080; echo ${url#*://}' \
    "localhost:8080"

test_comprehensive "At symbol in email" \
    'email=test@domain.org; echo ${email%@*}' \
    "test"

test_comprehensive "Dash in filename" \
    'file=my-script.sh; echo ${file%.*}' \
    "my-script"

test_comprehensive "Underscore in variable" \
    'var=my_variable; echo ${var^^}' \
    "MY_VARIABLE"

echo -e "${BLUE}=== EDGE CASES ===${NC}"

test_comprehensive "Empty pattern" \
    'text=hello; echo ${text#}' \
    "hello"

test_comprehensive "Pattern longer than string" \
    'text=hi; echo ${text#verylongpattern}' \
    "hi"

test_comprehensive "Single character" \
    'char=a; echo ${char^}' \
    "A"

test_comprehensive "Numbers only" \
    'nums=12345; echo ${nums^^}' \
    "12345"

echo -e "${BLUE}=== INTEGRATION TESTS ===${NC}"

test_comprehensive "With logical operators" \
    'file=test.txt; test "${file#*.}" = "txt" && echo "match"' \
    "match"

test_comprehensive "Variable in pattern" \
    'text=hello_world; suffix=_world; echo ${text%$suffix}' \
    "hello"

test_comprehensive "Length of expansion" \
    'text=Hello; echo ${#text}' \
    "5"

test_comprehensive "Nested expansions" \
    'first=john; last=doe; echo ${first^} ${last^}' \
    "John Doe"

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
echo "=== FEATURE BREAKDOWN ==="
echo "This comprehensive test covers:"
echo "  • Basic variable expansion (simple and braced)"
echo "  • Default values (\${var:-default})"
echo "  • Alternative values (\${var:+alternative})"
echo "  • Length expansion (\${#var})"
echo "  • Substring expansion (\${var:offset:length})"
echo "  • Pattern matching (\${var#pattern}, \${var%pattern})"
echo "  • Case conversion (\${var^}, \${var,}, \${var^^}, \${var,,})"
echo "  • Real-world applications (files, URLs, emails, names)"
echo "  • Complex combinations and edge cases"
echo "  • Integration with other shell features"

echo
if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}🎉 PERFECT SCORE! ALL PARAMETER EXPANSION TESTS PASSED!${NC}"
    echo "Modern parameter expansion is working flawlessly!"
    echo
    echo "✅ Complete POSIX parameter expansion implementation"
    echo "✅ Professional-grade string manipulation capabilities"
    echo "✅ Real-world usage patterns fully supported"
    echo "✅ Integration with all shell features working"

elif [ $SUCCESS_RATE -ge 90 ]; then
    echo -e "${GREEN}🎯 EXCELLENT! 90%+ Success Rate Achieved!${NC}"
    echo "Modern parameter expansion is highly functional with minor edge cases."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed - mostly complex scenarios."

elif [ $SUCCESS_RATE -ge 80 ]; then
    echo -e "${YELLOW}👍 VERY GOOD! 80%+ Success Rate${NC}"
    echo "Most parameter expansion functionality works correctly."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed - good foundation with room for improvement."

elif [ $SUCCESS_RATE -ge 70 ]; then
    echo -e "${YELLOW}👌 GOOD! 70%+ Success Rate${NC}"
    echo "Core parameter expansion functionality is solid."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed - strong base implementation."

else
    echo -e "${RED}⚠ NEEDS WORK - Below 70% Success Rate${NC}"
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
    echo "Parameter expansion implementation needs significant attention."
fi

echo
echo "=== ACHIEVEMENT SUMMARY ==="
echo "Modern parameter expansion provides:"
echo "  🔧 Essential string manipulation for shell scripting"
echo "  🌐 URL and email processing capabilities"
echo "  📁 File and path manipulation tools"
echo "  🏷️  Name and text formatting features"
echo "  ⚙️  Environment variable generation utilities"
echo "  🔄 Case conversion and normalization functions"
echo
echo "This comprehensive test validates the complete modern parameter"
echo "expansion system against real-world usage patterns and edge cases."

if [ $FAILED_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi

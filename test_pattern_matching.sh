#!/bin/bash

# Comprehensive Test Suite for Pattern Matching Parameter Expansion
# Tests ${var#pattern}, ${var##pattern}, ${var%pattern}, ${var%%pattern}

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

echo "=== LUSUSH PATTERN MATCHING PARAMETER EXPANSION TEST SUITE ==="
echo "Testing pattern matching: #, ##, %, %% operators"
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
test_pattern() {
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

echo -e "${BLUE}=== PREFIX REMOVAL (# and ##) ===${NC}"
echo "Remove patterns from the beginning of variables"
echo

test_pattern "Simple prefix removal (#)" \
    'text=abcdef; echo ${text#ab}' \
    "cdef"

test_pattern "Prefix removal with no match (#)" \
    'text=abcdef; echo ${text#xy}' \
    "abcdef"

test_pattern "Prefix removal entire string (#)" \
    'text=abc; echo ${text#abc}' \
    ""

test_pattern "Wildcard prefix removal (#)" \
    'text=prefix_data; echo ${text#*_}' \
    "data"

test_pattern "Multiple wildcard prefix (#)" \
    'path=/usr/local/bin; echo ${path#*/}' \
    "usr/local/bin"

test_pattern "Longest prefix removal (##)" \
    'path=/usr/local/bin; echo ${path##*/}' \
    "bin"

test_pattern "Longest wildcard prefix (##)" \
    'text=one_two_three; echo ${text##*_}' \
    "three"

test_pattern "Question mark pattern (#)" \
    'text=a1bcdef; echo ${text#?1}' \
    "bcdef"

echo -e "${BLUE}=== SUFFIX REMOVAL (% and %%) ===${NC}"
echo "Remove patterns from the end of variables"
echo

test_pattern "Simple suffix removal (%)" \
    'text=abcdef; echo ${text%ef}' \
    "abcd"

test_pattern "Suffix removal with no match (%)" \
    'text=abcdef; echo ${text%xy}' \
    "abcdef"

test_pattern "Suffix removal entire string (%)" \
    'text=abc; echo ${text%abc}' \
    ""

test_pattern "File extension removal (%)" \
    'filename=document.txt; echo ${filename%.*}' \
    "document"

test_pattern "Wildcard suffix removal (%)" \
    'text=data_suffix; echo ${text%_*}' \
    "data"

test_pattern "Longest suffix removal (%%) - file extensions" \
    'filename=archive.tar.gz; echo ${filename%%.*}' \
    "archive"

test_pattern "Longest wildcard suffix (%%)" \
    'text=one_two_three; echo ${text%%_*}' \
    "one"

test_pattern "Question mark suffix pattern (%)" \
    'text=abcde1; echo ${text%?1}' \
    "abcd"

echo -e "${BLUE}=== REAL-WORLD FILE PATH EXAMPLES ===${NC}"
echo "Common file and path manipulation patterns"
echo

test_pattern "Get filename from path (##)" \
    'path=/home/user/documents/file.txt; echo ${path##*/}' \
    "file.txt"

test_pattern "Get directory from path (%)" \
    'path=/home/user/documents/file.txt; echo ${path%/*}' \
    "/home/user/documents"

test_pattern "Remove file extension (%)" \
    'file=script.sh; echo ${file%.*}' \
    "script"

test_pattern "Get file extension (##)" \
    'file=document.pdf; echo ${file##*.}' \
    "pdf"

test_pattern "Remove all extensions (%%) - compressed files" \
    'file=backup.tar.gz; echo ${file%%.*}' \
    "backup"

test_pattern "Get first extension only (%)" \
    'file=backup.tar.gz; echo ${file%.*}' \
    "backup.tar"

echo -e "${BLUE}=== URL AND PROTOCOL EXAMPLES ===${NC}"
echo "URL manipulation and protocol handling"
echo

test_pattern "Remove protocol from URL (#)" \
    'url=https://example.com/path; echo ${url#*://}' \
    "example.com/path"

test_pattern "Get protocol from URL (%)" \
    'url=https://example.com/path; echo ${url%://*}' \
    "https"

test_pattern "Get domain from URL (# and %)" \
    'url=https://example.com/path/file; domain=${url#*://}; echo ${domain%%/*}' \
    "example.com"

test_pattern "Remove query string (%)" \
    'url=https://example.com/page?param=value; echo ${url%\?*}' \
    "https://example.com/page"

echo -e "${BLUE}=== COMPLEX PATTERNS ===${NC}"
echo "Advanced pattern matching scenarios"
echo

test_pattern "Multiple wildcards prefix (##)" \
    'text=a1b2c3d4; echo ${text##*4}' \
    ""

test_pattern "Nested pattern matching" \
    'path=/var/log/app.log; base=${path##*/}; echo ${base%.*}' \
    "app"

test_pattern "Email domain extraction" \
    'email=user@example.com; echo ${email#*@}' \
    "example.com"

test_pattern "Email username extraction" \
    'email=user@example.com; echo ${email%@*}' \
    "user"

echo -e "${BLUE}=== VERSION STRING MANIPULATION ===${NC}"
echo "Software version and numbering patterns"
echo

test_pattern "Major version extraction" \
    'version=1.2.3-beta; echo ${version%%.*}' \
    "1"

test_pattern "Remove pre-release suffix" \
    'version=1.2.3-beta; echo ${version%-*}' \
    "1.2.3"

test_pattern "Get pre-release suffix" \
    'version=1.2.3-beta; echo ${version#*-}' \
    "beta"

echo -e "${BLUE}=== EDGE CASES AND ERROR CONDITIONS ===${NC}"
echo "Testing boundary conditions and error handling"
echo

test_pattern "Empty variable prefix removal" \
    'empty=; echo ${empty#pattern}' \
    ""

test_pattern "Empty variable suffix removal" \
    'empty=; echo ${empty%pattern}' \
    ""

test_pattern "Empty pattern prefix removal" \
    'text=hello; echo ${text#}' \
    "hello"

test_pattern "Empty pattern suffix removal" \
    'text=hello; echo ${text%}' \
    "hello"

test_pattern "Pattern longer than string" \
    'text=hi; echo ${text#verylongpattern}' \
    "hi"

test_pattern "Special characters in pattern" \
    'text=test.file; echo ${text#*.}' \
    "file"

echo -e "${BLUE}=== INTEGRATION WITH OTHER FEATURES ===${NC}"
echo "Pattern matching with other shell features"
echo

test_pattern "Pattern matching with defaults" \
    'file="${FILE:-test.txt}"; echo ${file%.*}' \
    "test"

test_pattern "Pattern matching with variables in pattern" \
    'text=hello_world; suffix=_world; echo ${text%$suffix}' \
    "hello"

test_pattern "Pattern matching in assignments" \
    'path=/usr/bin/gcc; binary=${path##*/}; echo $binary' \
    "gcc"

test_pattern "Pattern matching with logical operators" \
    'file=test.txt; test "${file#*.}" = "txt" && echo "text file"' \
    "text file"

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
    echo -e "${GREEN}🎉 ALL PATTERN MATCHING TESTS PASSED!${NC}"
    echo "Pattern matching parameter expansion is working correctly!"
    echo
    echo "✅ Verified features:"
    echo "  • Prefix removal: \${var#pattern} (shortest), \${var##pattern} (longest)"
    echo "  • Suffix removal: \${var%pattern} (shortest), \${var%%pattern} (longest)"
    echo "  • Wildcard patterns: * and ? supported"
    echo "  • Real-world use cases: file paths, URLs, versions"
    echo "  • Integration with other parameter expansion features"
    echo "  • Proper error handling for edge cases"

elif [ $FAILED_COUNT -le 3 ]; then
    echo -e "${YELLOW}⚠ MINOR ISSUES DETECTED${NC}"
    echo "Most pattern matching functionality works correctly."
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed - likely edge cases."

else
    echo -e "${RED}❌ SIGNIFICANT ISSUES DETECTED${NC}"
    echo "$FAILED_COUNT out of $TEST_COUNT tests failed."
    echo "Pattern matching implementation needs attention."
fi

echo
echo "=== USAGE EXAMPLES ==="
echo "Pattern matching enables powerful string manipulation:"
echo
echo "# File operations"
echo 'filename="document.pdf"; echo "Name: ${filename%.*}, Type: ${filename##*.}"'
echo
echo "# Path manipulation"
echo 'path="/usr/local/bin/gcc"; echo "Dir: ${path%/*}, File: ${path##*/}"'
echo
echo "# URL processing"
echo 'url="https://example.com/page"; echo "Domain: ${url#*://}"'
echo
echo "# Version handling"
echo 'version="2.1.3-beta"; echo "Release: ${version%-*}, Pre: ${version#*-}"'
echo
echo "These patterns provide essential string manipulation capabilities"
echo "for robust shell scripting and system administration tasks."

if [ $FAILED_COUNT -eq 0 ]; then
    exit 0
else
    exit 1
fi

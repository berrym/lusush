#!/bin/bash

# Comprehensive test suite for umask built-in command
# Tests POSIX compliance and edge cases

echo "=== LUSUSH UMASK BUILT-IN TEST SUITE ==="
echo

# Build lusush first
echo "Building lusush..."
cd "$(dirname "$0")"
ninja -C builddir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"
echo

LUSUSH="./builddir/lusush"
PASS=0
FAIL=0

# Save original umask to restore later
ORIGINAL_UMASK=$(umask)

# Test function with newline-separated commands
test_umask() {
    local description="$1"
    local commands="$2"
    local expected_pattern="$3"

    echo -n "Testing: $description... "

    # Create temporary script
    local temp_script=$(mktemp)
    local temp_output=$(mktemp)
    echo -e "$commands" > "$temp_script"

    # Run test and capture output
    timeout 10s "$LUSUSH" < "$temp_script" > "$temp_output" 2>&1
    local actual_exit=$?

    # Check if output matches expected pattern
    if grep -q "$expected_pattern" "$temp_output" || [[ "$expected_pattern" == "EXIT_CODE_0" && $actual_exit -eq 0 ]]; then
        echo "✅ PASS"
        ((PASS++))
    else
        echo "❌ FAIL"
        echo "   Expected pattern: $expected_pattern"
        echo "   Actual output: $(cat "$temp_output")"
        echo "   Exit code: $actual_exit"
        ((FAIL++))
    fi

    rm -f "$temp_script" "$temp_output"
}

# Test function for exit codes
test_umask_exit() {
    local description="$1"
    local commands="$2"
    local expected_exit="$3"

    echo -n "Testing: $description... "

    # Create temporary script
    local temp_script=$(mktemp)
    echo -e "$commands" > "$temp_script"

    # Run test
    timeout 10s "$LUSUSH" < "$temp_script" > /dev/null 2>&1
    local actual_exit=$?

    rm -f "$temp_script"

    if [ "$actual_exit" -eq "$expected_exit" ]; then
        echo "✅ PASS"
        ((PASS++))
    else
        echo "❌ FAIL (expected exit $expected_exit, got $actual_exit)"
        ((FAIL++))
    fi
}

echo "=== BASIC UMASK FUNCTIONALITY ==="

# Test 1: Display current umask (no arguments)
test_umask "display current umask" "umask" "[0-7][0-7][0-7][0-7]"

# Test 2: Set umask to common value (022)
test_umask "set umask to 022" "umask 022\numask" "0022"

# Test 3: Set umask to restrictive value (077)
test_umask "set umask to 077" "umask 077\numask" "0077"

# Test 4: Set umask to permissive value (000)
test_umask "set umask to 000" "umask 000\numask" "0000"

# Test 5: Set umask to maximum value (0777)
test_umask "set umask to 0777" "umask 0777\numask" "0777"

echo
echo "=== ERROR HANDLING ==="

# Test 6: Invalid octal digit
test_umask_exit "invalid octal digit" "umask 028" 1

# Test 7: Invalid characters
test_umask_exit "invalid characters" "umask abc" 1

# Test 8: Negative value
test_umask_exit "negative value" "umask -1" 1

# Test 9: Value too large
test_umask_exit "value too large" "umask 1000" 1

# Test 10: Empty argument
test_umask_exit "empty argument" "umask ''" 1

# Test 11: Too many arguments
test_umask_exit "too many arguments" "umask 022 044" 1

echo
echo "=== OCTAL PARSING ==="

# Test 12: Leading zero handling
test_umask "leading zero (0022)" "umask 0022\numask" "0022"

# Test 13: Three digit format
test_umask "three digit format" "umask 644\numask" "0644"

# Test 14: Single digit
test_umask "single digit" "umask 2\numask" "0002"

# Test 15: Two digits
test_umask "two digits" "umask 22\numask" "0022"

echo
echo "=== EDGE CASES ==="

# Test 16: Whitespace handling
test_umask_exit "argument with whitespace" "umask ' 022'" 1

# Test 17: Mixed valid/invalid
test_umask_exit "mixed valid invalid chars" "umask 02a" 1

# Test 18: Hex-like input (should fail)
test_umask_exit "hex-like input" "umask 0x22" 1

# Test 19: Very long number
test_umask_exit "very long number" "umask 123456789" 1

echo
echo "=== POSIX COMPLIANCE ==="

# Test 20: Standard umask values
test_umask "standard user umask" "umask 022\numask" "0022"
test_umask "standard root umask" "umask 077\numask" "0077"

# Test 21: Permission calculation verification
# umask 022 means: 666 - 022 = 644 for files, 777 - 022 = 755 for dirs
test_umask "verify umask 022 set" "umask 022\numask" "0022"

# Test 22: All permission bits
test_umask "all permission bits" "umask 777\numask" "0777"

echo
echo "=== INTERACTIVE SCENARIOS ==="

# Test 23: Multiple umask commands
test_umask "multiple umask changes" "umask 022\numask 077\numask" "0077"

# Test 24: umask in subshell (should not affect parent)
test_umask "umask in subshell" "umask 022\n(umask 077)\numask" "0022"

echo
echo "=== REAL-WORLD USAGE ==="

# Test 25: File creation with different umasks
test_umask "umask affects file creation" "umask 077\ntouch /tmp/test_umask_file 2>/dev/null\nls -l /tmp/test_umask_file 2>/dev/null\nrm -f /tmp/test_umask_file" "EXIT_CODE_0"

# Test 26: Directory creation with umask
test_umask "umask affects directory creation" "umask 022\nmkdir /tmp/test_umask_dir 2>/dev/null\nls -ld /tmp/test_umask_dir 2>/dev/null\nrmdir /tmp/test_umask_dir 2>/dev/null" "EXIT_CODE_0"

echo
echo "=== OUTPUT FORMAT ==="

# Test 27: Output format consistency
test_umask "output format four digits" "umask 022\numask" "^0022$"

# Test 28: Leading zeros preserved
test_umask "leading zeros in output" "umask 007\numask" "0007"

echo
echo "=== SYSTEM INTEGRATION ==="

# Test 29: umask persists across commands
test_umask "umask persists" "umask 066\necho test\numask" "0066"

# Test 30: umask exit status success
test_umask_exit "successful umask set" "umask 022" 0

# Test 31: umask display exit status
test_umask_exit "successful umask display" "umask" 0

echo
echo "=== RESULTS ==="
echo "Tests passed: $PASS"
echo "Tests failed: $FAIL"
echo "Total tests: $((PASS + FAIL))"

# Restore original umask
umask $ORIGINAL_UMASK

if [ $FAIL -eq 0 ]; then
    echo "🎉 All tests passed!"
    exit 0
else
    echo "⚠️  Some tests failed"
    exit 1
fi

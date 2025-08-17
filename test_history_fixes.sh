#!/bin/bash

# Lusush History Fixes Test Script
# Tests all three identified history issues

set -e

LUSUSH_BINARY="./builddir/lusush"
HISTORY_FILE="$HOME/.lusush_history"
TEST_RESULTS_FILE="/tmp/lusush_test_results.log"

echo "=== LUSUSH HISTORY FIXES TEST ===" > "$TEST_RESULTS_FILE"
echo "Test started at: $(date)" >> "$TEST_RESULTS_FILE"
echo "" >> "$TEST_RESULTS_FILE"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=0

# Function to print test results
print_result() {
    local test_name="$1"
    local result="$2"
    local details="$3"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✓ PASS${NC}: $test_name"
        echo "✓ PASS: $test_name" >> "$TEST_RESULTS_FILE"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $test_name"
        echo "✗ FAIL: $test_name" >> "$TEST_RESULTS_FILE"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi

    if [ -n "$details" ]; then
        echo "   $details"
        echo "   $details" >> "$TEST_RESULTS_FILE"
    fi
    echo "" >> "$TEST_RESULTS_FILE"
}

# Function to wait for file to be written (with timeout)
wait_for_file() {
    local file="$1"
    local timeout=5
    local count=0

    while [ $count -lt $timeout ]; do
        if [ -s "$file" ]; then
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done
    return 1
}

echo -e "${YELLOW}=== LUSUSH HISTORY FIXES TEST ===${NC}"
echo ""

# Check if lusush binary exists
if [ ! -f "$LUSUSH_BINARY" ]; then
    echo -e "${RED}ERROR${NC}: Lusush binary not found at $LUSUSH_BINARY"
    echo "Please run: meson setup builddir --wipe && ninja -C builddir"
    exit 1
fi

echo "Testing binary: $LUSUSH_BINARY"
echo "History file: $HISTORY_FILE"
echo ""

# ============================================================================
# TEST 1: History Persistence Between Sessions
# ============================================================================

echo -e "${YELLOW}TEST 1: History Persistence Between Sessions${NC}"

# Clean slate
rm -f "$HISTORY_FILE"

# Run shell with commands and exit
printf 'echo "test command 1"\necho "test command 2"\necho "test command 3"\nexit\n' | timeout 10s "$LUSUSH_BINARY" -i >/dev/null 2>&1

# Wait a moment for file to be written
sleep 2

# Check if history file was created and contains commands
if [ -f "$HISTORY_FILE" ]; then
    if [ -s "$HISTORY_FILE" ]; then
        # Check if expected commands are in history
        if grep -q "test command 1" "$HISTORY_FILE" && grep -q "test command 2" "$HISTORY_FILE" && grep -q "test command 3" "$HISTORY_FILE"; then
            lines=$(wc -l < "$HISTORY_FILE")
            print_result "History saved on exit" "PASS" "History file contains $lines lines with expected commands"
        else
            print_result "History saved on exit" "FAIL" "History file exists but doesn't contain expected commands"
        fi
    else
        print_result "History saved on exit" "FAIL" "History file exists but is empty"
    fi
else
    print_result "History saved on exit" "FAIL" "History file was not created"
fi

# ============================================================================
# TEST 2: History Loading on Startup
# ============================================================================

echo -e "${YELLOW}TEST 2: History Loading on Startup${NC}"

# Ensure we have some history
echo -e "echo 'loaded command 1'\necho 'loaded command 2'\necho 'loaded command 3'" > "$HISTORY_FILE"

# Start shell and immediately check history
history_output=$(echo 'history' | timeout 5s "$LUSUSH_BINARY" -i 2>/dev/null | grep -E "(loaded command|history)")

if echo "$history_output" | grep -q "loaded command"; then
    count=$(echo "$history_output" | grep -c "loaded command" || true)
    print_result "History loaded on startup" "PASS" "Found $count history entries from previous session"
else
    print_result "History loaded on startup" "FAIL" "Previous history not loaded on startup"
fi

# ============================================================================
# TEST 3: History Navigation (Up Arrow Simulation)
# ============================================================================

echo -e "${YELLOW}TEST 3: History Navigation Testing${NC}"

# This test is limited since we can't simulate actual arrow keys in non-interactive mode
# We'll test the underlying functions by checking if history entries are accessible

# Create test history
echo -e "nav test 1\nnav test 2\nnav test 3" > "$HISTORY_FILE"

# Test that history command shows entries
nav_output=$(echo 'history' | timeout 5s "$LUSUSH_BINARY" -i 2>/dev/null)

if echo "$nav_output" | grep -q "nav test"; then
    nav_count=$(echo "$nav_output" | grep -c "nav test" || true)
    print_result "History navigation setup" "PASS" "History shows $nav_count navigation test entries"
else
    print_result "History navigation setup" "FAIL" "Navigation test entries not found in history"
fi

# ============================================================================
# TEST 4: History Deduplication
# ============================================================================

echo -e "${YELLOW}TEST 4: History Deduplication${NC}"

rm -f "$HISTORY_FILE"

# Add duplicate commands
printf 'echo "duplicate test"\necho "unique test"\necho "duplicate test"\nexit\n' | timeout 10s "$LUSUSH_BINARY" -i >/dev/null 2>&1

sleep 2

if [ -f "$HISTORY_FILE" ]; then
    duplicate_count=$(grep -c "duplicate test" "$HISTORY_FILE" || true)
    if [ "$duplicate_count" -eq 1 ]; then
        print_result "History deduplication" "PASS" "Duplicate commands properly filtered"
    elif [ "$duplicate_count" -gt 1 ]; then
        print_result "History deduplication" "FAIL" "Found $duplicate_count instances of duplicate command (expected 1)"
    else
        print_result "History deduplication" "FAIL" "Duplicate command not found in history"
    fi
else
    print_result "History deduplication" "FAIL" "History file not created"
fi

# ============================================================================
# TEST 5: History File Format and Integrity
# ============================================================================

echo -e "${YELLOW}TEST 5: History File Format and Integrity${NC}"

if [ -f "$HISTORY_FILE" ]; then
    # Check if file is readable and has proper format
    if [ -r "$HISTORY_FILE" ]; then
        lines=$(wc -l < "$HISTORY_FILE")
        if [ "$lines" -gt 0 ]; then
            # Check for any corrupted lines (empty lines or control characters)
            clean_lines=$(grep -c '^[[:print:]]*$' "$HISTORY_FILE" || true)
            if [ "$clean_lines" -eq "$lines" ]; then
                print_result "History file format" "PASS" "File format is clean ($lines lines)"
            else
                corrupt_lines=$((lines - clean_lines))
                print_result "History file format" "FAIL" "Found $corrupt_lines corrupted lines"
            fi
        else
            print_result "History file format" "FAIL" "History file is empty"
        fi
    else
        print_result "History file format" "FAIL" "History file is not readable"
    fi
else
    print_result "History file format" "FAIL" "History file does not exist"
fi

# ============================================================================
# TEST 6: Large History Handling
# ============================================================================

echo -e "${YELLOW}TEST 6: Large History Handling${NC}"

# Create a large history file to test size limits
rm -f "$HISTORY_FILE"
for i in $(seq 1 100); do
    echo "large history command $i" >> "$HISTORY_FILE"
done

# Add more commands via shell
printf 'echo "new command 1"\necho "new command 2"\nexit\n' | timeout 10s "$LUSUSH_BINARY" -i >/dev/null 2>&1

sleep 2

if [ -f "$HISTORY_FILE" ]; then
    final_lines=$(wc -l < "$HISTORY_FILE")
    if [ "$final_lines" -gt 100 ]; then
        print_result "Large history handling" "PASS" "History grew from 100 to $final_lines lines"
    elif [ "$final_lines" -eq 100 ]; then
        print_result "Large history handling" "FAIL" "History size didn't increase (may not be saving new commands)"
    else
        print_result "Large history handling" "FAIL" "History size decreased unexpectedly to $final_lines lines"
    fi
else
    print_result "Large history handling" "FAIL" "History file disappeared"
fi

# ============================================================================
# FINAL RESULTS
# ============================================================================

echo ""
echo -e "${YELLOW}=== TEST SUMMARY ===${NC}"
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo "Total Tests:  $TOTAL_TESTS"

# Write summary to log
echo "" >> "$TEST_RESULTS_FILE"
echo "=== TEST SUMMARY ===" >> "$TEST_RESULTS_FILE"
echo "Tests Passed: $TESTS_PASSED" >> "$TEST_RESULTS_FILE"
echo "Tests Failed: $TESTS_FAILED" >> "$TEST_RESULTS_FILE"
echo "Total Tests:  $TOTAL_TESTS" >> "$TEST_RESULTS_FILE"
echo "Test completed at: $(date)" >> "$TEST_RESULTS_FILE"

if [ "$TESTS_FAILED" -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
    echo "🎉 ALL TESTS PASSED!" >> "$TEST_RESULTS_FILE"
    exit 0
else
    echo -e "${RED}❌ $TESTS_FAILED TEST(S) FAILED${NC}"
    echo "❌ $TESTS_FAILED TEST(S) FAILED" >> "$TEST_RESULTS_FILE"
    echo ""
    echo "Detailed results saved to: $TEST_RESULTS_FILE"
    echo "Current history file content:"
    if [ -f "$HISTORY_FILE" ]; then
        echo "--- History file content ---"
        cat "$HISTORY_FILE" | head -20
        echo "--- End of history file ---"
    else
        echo "History file does not exist"
    fi
    exit 1
fi

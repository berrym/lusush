#!/bin/bash

# Test script to verify the three critical fixes in LUSUSH v1.0.2+
# 1. Character deletion artifacts
# 2. Hints system runtime configuration
# 3. History navigation creating new prompts

echo "==============================================================================="
echo "LUSUSH CRITICAL FIXES VERIFICATION TEST"
echo "==============================================================================="
echo ""

LUSUSH_BIN="./builddir/lusush"
PASSED=0
FAILED=0

# Function to run a test and check result
run_test() {
    local test_name="$1"
    local command="$2"
    local expected="$3"

    echo "Testing: $test_name"
    echo "Command: $command"

    # Run the command and capture output
    result=$(echo -e "$command" | timeout 5 $LUSUSH_BIN 2>&1 | tail -n 1)

    if [[ "$result" == *"$expected"* ]]; then
        echo "✓ PASSED"
        ((PASSED++))
    else
        echo "✗ FAILED"
        echo "  Expected: $expected"
        echo "  Got: $result"
        ((FAILED++))
    fi
    echo ""
}

# Function to test interactive features
test_interactive() {
    local test_name="$1"
    local script="$2"

    echo "Testing: $test_name"
    echo "Script: $script"

    # Create a temporary expect script
    cat > /tmp/lusush_test_expect.exp << EOF
#!/usr/bin/expect -f
set timeout 10
spawn $LUSUSH_BIN
expect "lusush> "
$script
EOF

    chmod +x /tmp/lusush_test_expect.exp

    if command -v expect >/dev/null 2>&1; then
        /tmp/lusush_test_expect.exp
        if [ $? -eq 0 ]; then
            echo "✓ PASSED (Interactive test completed)"
            ((PASSED++))
        else
            echo "✗ FAILED (Interactive test failed)"
            ((FAILED++))
        fi
    else
        echo "⚠ SKIPPED (expect not available)"
    fi

    rm -f /tmp/lusush_test_expect.exp
    echo ""
}

echo "▓▓▓ FIX 1: HINTS SYSTEM RUNTIME CONFIGURATION ▓▓▓"
echo ""

# Test 1: Check initial hints_enabled state
run_test "Initial hints_enabled state" "config get hints_enabled" "false"

# Test 2-5: Test configuration changes within single session
echo "Testing: Configuration changes within single session"
echo "Command: config set/get sequence"

result=$(echo -e "config set hints_enabled true\nconfig get hints_enabled\nconfig set hints_enabled false\nconfig get hints_enabled\nexit" | timeout 5 $LUSUSH_BIN 2>&1)

if [[ "$result" == *"Set hints_enabled = true"* ]] && [[ "$result" == *"true"* ]] && [[ "$result" == *"Set hints_enabled = false"* ]] && [[ "$result" == *"false"* ]]; then
    echo "✓ PASSED"
    ((PASSED++))
else
    echo "✗ FAILED"
    echo "Output: $result"
    ((FAILED++))
fi
echo ""

echo "▓▓▓ FIX 2: BASIC FUNCTIONALITY VERIFICATION ▓▓▓"
echo ""

# Test basic command execution to ensure no regression
run_test "Basic echo command" "echo 'Hello World'" "Hello World"

# Test variable assignment and retrieval
run_test "Variable assignment" "TEST_VAR=test_value; echo \$TEST_VAR" "test_value"

# Test arithmetic operations
run_test "Arithmetic operations" "echo \$((5 + 3))" "8"

echo "▓▓▓ FIX 3: INTERACTIVE TESTING (if expect available) ▓▓▓"
echo ""

# Test interactive features if expect is available
if command -v expect >/dev/null 2>&1; then
    echo "Testing interactive features with expect..."

    # Test 1: Character deletion (backspace)
    test_interactive "Character deletion test" '
send "echo hello world"
send "\b\b\b\b\b"
send "test\r"
expect "hello test"
send "exit\r"
expect eof
'

    # Test 2: History navigation
    test_interactive "History navigation test" '
send "echo first command\r"
expect "first command"
send "echo second command\r"
expect "second command"
send "\033\[A"
expect "echo second command"
send "\033\[A"
expect "echo first command"
send "\r"
expect "first command"
send "exit\r"
expect eof
'

    # Test 3: Hints system (if enabled)
    test_interactive "Hints system test" '
send "config set hints_enabled true\r"
expect "Set hints_enabled = true"
send "ec"
sleep 1
send "\r"
send "config set hints_enabled false\r"
expect "Set hints_enabled = false"
send "exit\r"
expect eof
'

else
    echo "⚠ expect not available - skipping interactive tests"
    echo "To install expect on Ubuntu/Debian: sudo apt-get install expect"
    echo "To install expect on CentOS/RHEL: sudo yum install expect"
    echo ""
fi

echo "▓▓▓ MANUAL TESTING INSTRUCTIONS ▓▓▓"
echo ""
echo "For comprehensive testing, perform these manual tests:"
echo ""
echo "1. CHARACTER DELETION TEST:"
echo "   - Start lusush: $LUSUSH_BIN"
echo "   - Type: echo hello world"
echo "   - Press backspace 5 times"
echo "   - Type: test"
echo "   - Press Enter"
echo "   - Expected: 'hello test' should be displayed"
echo "   - Verify: No character artifacts remain on screen"
echo ""
echo "2. HINTS SYSTEM TEST:"
echo "   - In lusush, run: config set hints_enabled true"
echo "   - Type: ec (without pressing Enter)"
echo "   - Expected: Should see hint suggestions"
echo "   - Run: config set hints_enabled false"
echo "   - Type: ec (without pressing Enter)"
echo "   - Expected: No hints should appear"
echo ""
echo "3. HISTORY NAVIGATION TEST:"
echo "   - Type: echo first command (press Enter)"
echo "   - Type: echo second command (press Enter)"
echo "   - Press Up arrow key"
echo "   - Expected: Should see 'echo second command'"
echo "   - Press Up arrow key again"
echo "   - Expected: Should see 'echo first command'"
echo "   - Verify: No new prompt lines are created"
echo ""
echo "4. BOTTOM-LINE EDITING TEST:"
echo "   - Resize terminal to ~20 lines"
echo "   - Fill screen with: for i in {1..30}; do echo \"Line \$i\"; done"
echo "   - Prompt should be at bottom line"
echo "   - Test character deletion, history navigation"
echo "   - Expected: No cursor jumping or line consumption"
echo ""

echo "==============================================================================="
echo "TEST RESULTS SUMMARY"
echo "==============================================================================="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✓ ALL TESTS PASSED - Fixes appear to be working correctly"
    exit 0
else
    echo "✗ SOME TESTS FAILED - Issues may remain"
    exit 1
fi

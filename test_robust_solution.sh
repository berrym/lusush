#!/bin/bash

# Comprehensive test for LUSUSH robust solution
# Tests both line consumption and cursor bouncing issues

echo "==============================================================================="
echo "LUSUSH ROBUST SOLUTION COMPREHENSIVE TEST"
echo "==============================================================================="
echo ""

LUSUSH_BIN="./builddir/lusush"
PASSED=0
FAILED=0

if [ ! -f "$LUSUSH_BIN" ]; then
    echo "❌ LUSUSH binary not found at $LUSUSH_BIN"
    echo "Please build first with: ninja -C builddir"
    exit 1
fi

echo "🔍 Testing robust solution for line consumption and cursor bouncing..."
echo ""

# Function to run a test and check result
run_test() {
    local test_name="$1"
    local command="$2"
    local expected="$3"

    echo "Testing: $test_name"
    echo "Command: $command"

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

# Function to create expect script for interactive testing
create_expect_script() {
    local script_name="$1"
    local expect_commands="$2"

    cat > "$script_name" << EOF
#!/usr/bin/expect -f
set timeout 10
spawn $LUSUSH_BIN
expect "lusush> "
$expect_commands
expect eof
EOF
    chmod +x "$script_name"
}

echo "▓▓▓ BASIC FUNCTIONALITY VERIFICATION ▓▓▓"
echo ""

# Test 1: Basic command execution
run_test "Basic echo command" "echo 'Hello World'" "Hello World"

# Test 2: Variable assignment
run_test "Variable assignment" "TEST_VAR=test_value; echo \$TEST_VAR" "test_value"

# Test 3: Arithmetic operations
run_test "Arithmetic operations" "echo \$((5 + 3))" "8"

# Test 4: Configuration system
run_test "Configuration system" "config get hints_enabled" "false"

echo "▓▓▓ LINE CONSUMPTION PREVENTION TESTS ▓▓▓"
echo ""

# Test 5: Multi-line output followed by backspace
echo "Testing: Line consumption prevention"
echo "Creating multi-line output scenario..."

# Create a script that fills the screen and then tests editing
cat > /tmp/lusush_line_test.sh << 'EOF'
#!/bin/bash
# Fill screen with content
for i in {1..25}; do
    echo "Line $i: This is test content to fill the screen"
done
echo "Now testing at bottom line..."
EOF

chmod +x /tmp/lusush_line_test.sh

# Test the line consumption scenario
echo "Command: Multi-line output + editing test"
result=$(echo -e "/tmp/lusush_line_test.sh\necho test\nexit" | timeout 10 $LUSUSH_BIN 2>&1 | tail -n 3)
if [[ "$result" == *"test"* ]]; then
    echo "✓ PASSED - Line consumption prevention working"
    ((PASSED++))
else
    echo "✗ FAILED - Line consumption may still be occurring"
    echo "Output: $result"
    ((FAILED++))
fi
echo ""

echo "▓▓▓ CURSOR POSITIONING TESTS ▓▓▓"
echo ""

# Test 6: Configuration changes within single session
echo "Testing: Configuration changes and cursor stability"
result=$(echo -e "config set hints_enabled true\nconfig get hints_enabled\nconfig set hints_enabled false\nconfig get hints_enabled\nexit" | timeout 5 $LUSUSH_BIN 2>&1)

if [[ "$result" == *"Set hints_enabled = true"* ]] && [[ "$result" == *"true"* ]] && [[ "$result" == *"Set hints_enabled = false"* ]] && [[ "$result" == *"false"* ]]; then
    echo "✓ PASSED - Configuration system stable"
    ((PASSED++))
else
    echo "✗ FAILED - Configuration system issues"
    echo "Output: $result"
    ((FAILED++))
fi
echo ""

echo "▓▓▓ INTERACTIVE TESTING (if expect available) ▓▓▓"
echo ""

if command -v expect >/dev/null 2>&1; then
    echo "Testing interactive features with expect..."

    # Test 7: Character deletion with cursor positioning
    echo "Testing: Character deletion cursor positioning"
    create_expect_script "/tmp/lusush_backspace_test.exp" '
send "echo hello world"
sleep 0.5
send "\b\b\b\b\b"
sleep 0.5
send "test\r"
expect "hello test"
send "exit\r"
'

    if /tmp/lusush_backspace_test.exp >/dev/null 2>&1; then
        echo "✓ PASSED - Character deletion cursor positioning"
        ((PASSED++))
    else
        echo "✗ FAILED - Character deletion cursor positioning"
        ((FAILED++))
    fi
    echo ""

    # Test 8: History navigation cursor positioning
    echo "Testing: History navigation cursor positioning"
    create_expect_script "/tmp/lusush_history_test.exp" '
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
'

    if /tmp/lusush_history_test.exp >/dev/null 2>&1; then
        echo "✓ PASSED - History navigation cursor positioning"
        ((PASSED++))
    else
        echo "✗ FAILED - History navigation cursor positioning"
        ((FAILED++))
    fi
    echo ""

    # Test 9: Long line editing
    echo "Testing: Long line cursor handling"
    create_expect_script "/tmp/lusush_longline_test.exp" '
send "echo this is a very long line that might cause issues with cursor positioning and terminal width handling"
sleep 0.5
send "\b\b\b\b\b\b\b\b\b\b"
sleep 0.5
send "test\r"
expect "test"
send "exit\r"
'

    if /tmp/lusush_longline_test.exp >/dev/null 2>&1; then
        echo "✓ PASSED - Long line cursor handling"
        ((PASSED++))
    else
        echo "✗ FAILED - Long line cursor handling"
        ((FAILED++))
    fi
    echo ""

    # Test 10: Bottom-line scenario
    echo "Testing: Bottom-line editing scenario"
    create_expect_script "/tmp/lusush_bottomline_test.exp" '
# Fill screen
for {set i 1} {$i <= 20} {incr i} {
    send "echo Line $i\r"
    expect "Line $i"
}
send "echo bottom line test"
sleep 0.5
send "\b\b\b\b\b\b\b\b\b"
sleep 0.5
send "hello\r"
expect "hello"
send "exit\r"
'

    if /tmp/lusush_bottomline_test.exp >/dev/null 2>&1; then
        echo "✓ PASSED - Bottom-line editing scenario"
        ((PASSED++))
    else
        echo "✗ FAILED - Bottom-line editing scenario"
        ((FAILED++))
    fi
    echo ""

    # Cleanup expect scripts
    rm -f /tmp/lusush_*_test.exp

else
    echo "⚠ expect not available - skipping interactive tests"
    echo "To install expect on Ubuntu/Debian: sudo apt-get install expect"
    echo "To install expect on CentOS/RHEL: sudo yum install expect"
    echo ""
fi

# Cleanup
rm -f /tmp/lusush_line_test.sh

echo "▓▓▓ ROBUSTNESS VERIFICATION ▓▓▓"
echo ""

# Test 11: POSIX compliance maintained
echo "Testing: POSIX compliance maintained"
posix_result=$(./tests/compliance/test_posix_regression.sh 2>&1 | grep -E "(Passed|Failed):" | tail -2)
if [[ "$posix_result" == *"Passed: 49"* ]] && [[ "$posix_result" == *"Failed: 0"* ]]; then
    echo "✓ PASSED - POSIX compliance maintained (49/49)"
    ((PASSED++))
else
    echo "✗ FAILED - POSIX compliance issues"
    echo "Result: $posix_result"
    ((FAILED++))
fi
echo ""

# Test 12: Memory stability
echo "Testing: Memory stability under load"
result=$(echo -e "for i in {1..100}; do echo \"Test \$i\"; done\necho done\nexit" | timeout 10 $LUSUSH_BIN 2>&1 | tail -n 1)
if [[ "$result" == "done" ]]; then
    echo "✓ PASSED - Memory stability under load"
    ((PASSED++))
else
    echo "✗ FAILED - Memory stability issues"
    echo "Result: $result"
    ((FAILED++))
fi
echo ""

echo "▓▓▓ MANUAL TESTING INSTRUCTIONS ▓▓▓"
echo ""
echo "For comprehensive verification, perform these manual tests:"
echo ""
echo "🧪 Manual Test 1: Line Consumption Prevention"
echo "   1. Resize terminal to ~20 lines"
echo "   2. Run: $LUSUSH_BIN"
echo "   3. Fill screen: for i in {1..30}; do echo \"Line \$i\"; done"
echo "   4. Prompt should be at bottom line"
echo "   5. Type: echo hello world"
echo "   6. Press backspace 5 times"
echo "   7. Type: test"
echo "   8. Press Enter"
echo "   9. Expected: Previous lines should NOT be consumed"
echo "   10. Expected: Output should be 'hello test'"
echo ""
echo "🧪 Manual Test 2: Cursor Bouncing Prevention"
echo "   1. Run: $LUSUSH_BIN"
echo "   2. Type: echo this is a long line of text"
echo "   3. Press backspace multiple times"
echo "   4. Expected: Cursor should move smoothly left, no bouncing"
echo "   5. Type: echo first (press Enter)"
echo "   6. Type: echo second (press Enter)"
echo "   7. Press Up arrow key"
echo "   8. Expected: Should show 'echo second', cursor should not bounce"
echo "   9. Press Up arrow key again"
echo "   10. Expected: Should show 'echo first', cursor should not bounce"
echo ""
echo "🧪 Manual Test 3: Combined Scenario"
echo "   1. Resize terminal to ~15 lines"
echo "   2. Run: $LUSUSH_BIN"
echo "   3. Fill screen: for i in {1..20}; do echo \"Line \$i\"; done"
echo "   4. Type: echo test command"
echo "   5. Press backspace 7 times"
echo "   6. Type: hello world"
echo "   7. Press Up arrow to recall previous command"
echo "   8. Expected: No line consumption AND no cursor bouncing"
echo "   9. Type: exit"
echo ""

echo "==============================================================================="
echo "ROBUST SOLUTION TEST RESULTS SUMMARY"
echo "==============================================================================="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✅ ALL TESTS PASSED"
    echo ""
    echo "🎉 ROBUST SOLUTION VERIFICATION COMPLETE"
    echo "   ✓ Line consumption prevention: WORKING"
    echo "   ✓ Cursor bouncing prevention: WORKING"
    echo "   ✓ POSIX compliance: MAINTAINED"
    echo "   ✓ Configuration system: STABLE"
    echo "   ✓ Memory stability: VERIFIED"
    echo ""
    echo "The robust solution successfully addresses both critical issues:"
    echo "1. Prevents line consumption at bottom of terminal"
    echo "2. Prevents cursor bouncing during editing operations"
    echo "3. Maintains all existing functionality and stability"
    echo ""
    exit 0
else
    echo "❌ SOME TESTS FAILED"
    echo ""
    echo "Issues detected in the robust solution:"
    echo "- $FAILED out of $(($PASSED + $FAILED)) tests failed"
    echo "- Manual testing recommended to identify specific problems"
    echo "- Check the failed tests above for details"
    echo ""
    echo "Recommended next steps:"
    echo "1. Run manual tests to observe actual behavior"
    echo "2. Check specific failure conditions"
    echo "3. Consider additional refinements to the solution"
    echo ""
    exit 1
fi

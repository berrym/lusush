#!/bin/bash

# Automated expect test for LUSUSH history navigation behavior
# This script uses expect to test history navigation and detect if new prompt lines are created

echo "==============================================================================="
echo "LUSUSH HISTORY NAVIGATION - AUTOMATED EXPECT TEST"
echo "==============================================================================="

LUSUSH_BIN="./builddir/lusush"

if [ ! -f "$LUSUSH_BIN" ]; then
    echo "❌ ERROR: LUSUSH binary not found at $LUSUSH_BIN"
    exit 1
fi

if ! command -v expect >/dev/null 2>&1; then
    echo "❌ ERROR: expect not found. Install with:"
    echo "   Fedora: sudo dnf install expect"
    echo "   Ubuntu: sudo apt install expect"
    exit 1
fi

echo "🔍 Testing history navigation with expect..."

# Create expect script
cat > /tmp/lusush_history_test.exp << 'EOF'
#!/usr/bin/expect -f

set timeout 10
set lusush_bin [lindex $argv 0]

# Start lusush
spawn $lusush_bin
expect "lusush> "

# Create history entries
send "echo first\r"
expect "first"
expect "lusush> "

send "echo second\r"
expect "second"
expect "lusush> "

send "echo third\r"
expect "third"
expect "lusush> "

# Test history navigation
# This is where we check for the new prompt line issue
send "\033\[A"
set history_output $expect_out(buffer)

# Wait a moment to see what happens
sleep 0.5

# Check if we got the history entry on the same line or a new line
expect {
    "lusush> lusush> " {
        puts "\n❌ FAILED: New prompt line created during history navigation"
        puts "Detected multiple 'lusush>' prompts - this indicates the bug"
        exit 1
    }
    "echo third" {
        puts "\n✓ SUCCESS: History entry displayed correctly"
    }
    timeout {
        puts "\n⚠ TIMEOUT: History navigation took too long"
        exit 2
    }
}

# Test more navigation
send "\033\[A"
sleep 0.5

expect {
    "lusush> lusush> " {
        puts "❌ FAILED: Multiple UP arrows create multiple prompt lines"
        exit 1
    }
    "echo second" {
        puts "✓ SUCCESS: Second history entry displayed correctly"
    }
    timeout {
        puts "⚠ TIMEOUT: Second history navigation failed"
        exit 2
    }
}

# Test DOWN arrow
send "\033\[B"
sleep 0.5

expect {
    "lusush> lusush> " {
        puts "❌ FAILED: DOWN arrow creates new prompt line"
        exit 1
    }
    "echo third" {
        puts "✓ SUCCESS: DOWN arrow navigation works correctly"
    }
    timeout {
        puts "⚠ TIMEOUT: DOWN arrow navigation failed"
        exit 2
    }
}

# Test editing on history line
send "\b\b\b\b\b"
send "EDITED"
sleep 0.2

# Check if editing works correctly
expect {
    "lusush> lusush> " {
        puts "❌ FAILED: Editing history creates new prompt line"
        exit 1
    }
    "EDITED" {
        puts "✓ SUCCESS: History line editing works correctly"
    }
    timeout {
        puts "⚠ TIMEOUT: History editing failed"
        exit 2
    }
}

# Exit gracefully
send "\r"
expect "lusush> "
send "exit\r"
expect eof

puts "\n✅ ALL TESTS PASSED: History navigation working correctly"
exit 0
EOF

chmod +x /tmp/lusush_history_test.exp

echo ""
echo "Running automated history navigation test..."
echo ""

# Run the expect test
if /tmp/lusush_history_test.exp "$LUSUSH_BIN"; then
    echo ""
    echo "==============================================================================="
    echo "✅ AUTOMATED TEST RESULT: PASS"
    echo "==============================================================================="
    echo ""
    echo "History navigation is working correctly:"
    echo "• UP/DOWN arrows replace current line content"
    echo "• No extra prompt lines are created"
    echo "• History editing works in-place"
    echo ""
else
    exit_code=$?
    echo ""
    echo "==============================================================================="
    echo "❌ AUTOMATED TEST RESULT: FAIL (Exit code: $exit_code)"
    echo "==============================================================================="
    echo ""
    case $exit_code in
        1)
            echo "ISSUE DETECTED: New prompt lines are being created during history navigation"
            echo ""
            echo "PROBLEM: When using UP/DOWN arrows, lusush creates new 'lusush>' prompt"
            echo "lines instead of replacing the current line content."
            echo ""
            echo "EXPECTED BEHAVIOR:"
            echo "lusush> echo third    ← UP arrow should replace this line"
            echo ""
            echo "ACTUAL BEHAVIOR:"
            echo "lusush> "
            echo "lusush> echo third    ← UP arrow creates new line"
            echo ""
            echo "NEXT STEPS:"
            echo "1. The refresh logic needs to be fixed"
            echo "2. History navigation should use line replacement, not line creation"
            echo "3. The cursor positioning might be incorrect"
            ;;
        2)
            echo "ISSUE DETECTED: History navigation timeouts or unexpected behavior"
            echo ""
            echo "PROBLEM: History navigation commands are not responding as expected"
            echo "This could indicate:"
            echo "• History system not working"
            echo "• Keyboard input processing issues"
            echo "• Terminal compatibility problems"
            ;;
        *)
            echo "ISSUE DETECTED: Unexpected test failure"
            echo "Exit code: $exit_code"
            ;;
    esac
    echo ""
    echo "For manual debugging, run: $LUSUSH_BIN"
    echo "Then test UP/DOWN arrows to see the behavior yourself."
fi

# Cleanup
rm -f /tmp/lusush_history_test.exp

echo ""
echo "==============================================================================="
echo "MANUAL VERIFICATION"
echo "==============================================================================="
echo ""
echo "If the automated test failed, please verify manually:"
echo ""
echo "1. Run: $LUSUSH_BIN"
echo "2. Type: echo test1"
echo "3. Type: echo test2"
echo "4. Press UP arrow"
echo "5. Observe: Does 'echo test2' replace current line or create new line?"
echo ""
echo "Expected: Line replacement"
echo "Bug: New prompt line creation"
echo ""

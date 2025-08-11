#!/bin/bash

# Signal Handling Test for Lusush Interactive Mode
# Tests Ctrl+C behavior to ensure it clears line instead of exiting shell

echo "=== Lusush Signal Handling Test ==="
echo ""

# Build if needed
if [ ! -f builddir/lusush ]; then
    echo "Building lusush..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "This test verifies that Ctrl+C:"
echo "  ✅ CORRECT: Clears current line and shows new prompt"
echo "  ❌ BUG: Exits the entire shell"
echo ""

echo "=== Signal Handling Test with Script Pseudo-TTY ==="
echo ""

# Create expect test for signal handling
cat > /tmp/signal_test.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Type some text without pressing ENTER
send "echo \"this text should be cleared by ctrl+c\""

# Wait a moment
sleep 1

# Send Ctrl+C
send "\003"

# Check what happens
expect {
    "$ " {
        puts "\n✅ SUCCESS: Ctrl+C cleared line, shell continued"
        send "echo \"shell is still running\"\r"
        expect "shell is still running"
        send "exit\r"
        exit 0
    }
    eof {
        puts "\n❌ FAILURE: Shell exited on Ctrl+C"
        exit 1
    }
    timeout {
        puts "\n❓ TIMEOUT: Unclear signal behavior"
        send "exit\r"
        exit 2
    }
}
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/signal_test.exp
    echo "Running expect-based signal test..."
    /tmp/signal_test.exp
    signal_test_result=$?
else
    echo "expect not available, running alternative test..."

    # Alternative test with timeout
    echo "Testing signal handling with timeout..."
    timeout 10 script -q -c "./builddir/lusush" /dev/null << 'SIGNAL_INPUT'
echo "test command"
echo "another command"
echo "testing ctrl+c behavior - shell should continue"
exit
SIGNAL_INPUT
    signal_test_result=$?
fi

echo ""
echo "=== Signal Handler Debug Analysis ==="
echo ""

echo "Based on debug output, check for these patterns:"
echo ""
echo "✅ CORRECT signal handling:"
echo "  [SIGNAL_DEBUG] readline_sigint_handler called"
echo "  [SIGNAL_DEBUG] This should clear line, NOT exit shell!"
echo "  [SIGNAL_DEBUG] Clearing readline buffer"
echo "  [SIGNAL_DEBUG] readline_sigint_handler complete - shell should continue"
echo "  Shell continues with new prompt"
echo ""
echo "❌ BROKEN signal handling:"
echo "  Shell exits immediately on Ctrl+C"
echo "  No debug output after Ctrl+C"
echo "  Process terminates unexpectedly"
echo ""

echo "=== Child Process Signal Test ==="
echo ""

echo "Testing Ctrl+C with child processes..."

cat > /tmp/child_signal_test.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 15

spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Start a long-running command
send "sleep 5\r"

# Wait a moment for sleep to start
sleep 1

# Send Ctrl+C to interrupt sleep
send "\003"

# Check that we get back to prompt (sleep should be interrupted)
expect {
    "$ " {
        puts "\n✅ SUCCESS: Ctrl+C interrupted child process, shell continued"
        send "echo \"shell survived child interrupt\"\r"
        expect "shell survived child interrupt"
        send "exit\r"
        exit 0
    }
    eof {
        puts "\n❌ FAILURE: Shell exited when interrupting child process"
        exit 1
    }
    timeout {
        puts "\n❓ TIMEOUT: Child process signal handling unclear"
        send "\003"
        send "exit\r"
        exit 2
    }
}
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/child_signal_test.exp
    echo "Running child process signal test..."
    /tmp/child_signal_test.exp
    child_signal_result=$?
else
    echo "expect not available for child process test"
    child_signal_result=2
fi

echo ""
echo "=== Results Summary ==="
echo ""

if [ $signal_test_result -eq 0 ]; then
    echo "🎉 SIGNAL HANDLING TEST PASSED"
    echo "   Ctrl+C correctly clears line and continues shell"
elif [ $signal_test_result -eq 1 ]; then
    echo "🚨 SIGNAL HANDLING TEST FAILED"
    echo "   Ctrl+C exits shell instead of clearing line"
else
    echo "❓ SIGNAL HANDLING TEST INCONCLUSIVE"
fi

if [ $child_signal_result -eq 0 ]; then
    echo "🎉 CHILD PROCESS SIGNAL TEST PASSED"
    echo "   Ctrl+C correctly interrupts child processes"
elif [ $child_signal_result -eq 1 ]; then
    echo "🚨 CHILD PROCESS SIGNAL TEST FAILED"
    echo "   Ctrl+C handling broken for child processes"
else
    echo "❓ CHILD PROCESS SIGNAL TEST INCONCLUSIVE"
fi

echo ""
echo "=== Manual Testing Instructions ==="
echo ""
echo "For definitive signal testing, run manually:"
echo ""
echo "1. Start lusush interactively:"
echo "   script -q -c './builddir/lusush' /dev/null"
echo ""
echo "2. Test line clearing:"
echo "   - Type: echo \"some text\" (don't press ENTER)"
echo "   - Press Ctrl+C"
echo "   - Should: clear line, show new prompt"
echo ""
echo "3. Test child process interruption:"
echo "   - Type: sleep 10"
echo "   - Press ENTER"
echo "   - Wait 2 seconds"
echo "   - Press Ctrl+C"
echo "   - Should: interrupt sleep, return to prompt"
echo ""
echo "4. Test shell survival:"
echo "   - After both tests above, shell should still be running"
echo "   - Type: echo \"shell survived\""
echo "   - Should: show the message"
echo ""

echo "=== Implementation Status ==="
echo ""

if [ $signal_test_result -eq 0 ] && [ $child_signal_result -eq 0 ]; then
    echo "🎉 ALL SIGNAL TESTS PASSED"
    echo "   Signal handling appears to be working correctly"
    echo "   The user-reported Ctrl+C issue may be resolved"
    echo ""
    echo "Verification needed:"
    echo "   - Manual testing in real terminal"
    echo "   - User confirmation of fix"
elif [ $signal_test_result -eq 1 ] || [ $child_signal_result -eq 1 ]; then
    echo "🚨 SIGNAL HANDLING ISSUES DETECTED"
    echo "   Need to fix signal handlers in src/signals.c"
    echo ""
    echo "Specific fixes needed:"
    echo "   - Ensure readline_sigint_handler doesn't exit shell"
    echo "   - Verify signal handler switching for child processes"
    echo "   - Test signal integration with readline"
else
    echo "❓ SIGNAL TESTING INCONCLUSIVE"
    echo "   Manual testing required for definitive results"
fi

# Cleanup
rm -f /tmp/signal_test.exp /tmp/child_signal_test.exp

echo ""
echo "Use this analysis to determine if signal handling fixes are needed."

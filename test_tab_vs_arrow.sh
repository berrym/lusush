#!/bin/bash

# TAB vs Arrow Key Behavior Test for Lusush
# This test specifically compares what happens with TAB vs UP arrow
# to isolate the completion vs history navigation issue

echo "=== TAB vs Arrow Key Behavior Test ==="
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

echo "This test will compare the behavior of:"
echo "  TAB key: Should trigger completion"
echo "  UP arrow: Should navigate history (NOT trigger completion)"
echo ""

echo "=== Test 1: TAB Key Behavior (Should Show Completion) ==="
echo ""

# Create test for TAB completion
cat > /tmp/tab_test.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Type partial command and press TAB
send "ec"
send "\t"

# Look for completion behavior
expect {
    "echo" {
        puts "\n✅ TAB completion working correctly"
        send "\r"
        expect "$ "
        send "exit\r"
        exit 0
    }
    "display all" {
        puts "\n❌ TAB showing completion menu (may be OK)"
        send "n\r"
        expect "$ "
        send "exit\r"
        exit 0
    }
    timeout {
        puts "\n❓ TAB behavior timeout"
        send "\003"
        send "exit\r"
        exit 1
    }
}
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/tab_test.exp
    echo "Testing TAB completion behavior..."
    /tmp/tab_test.exp
else
    echo "expect not available - testing with input simulation..."
    # Simulate TAB test
    cat > /tmp/tab_input << 'EOF'
echo "test command for completion"
echo "another command"
echo "third command"
ec
exit
EOF
    echo "Running TAB test with input simulation..."
    script -q -c "./builddir/lusush" /dev/null < /tmp/tab_input
fi

echo ""
echo "=== Test 2: UP Arrow Key Behavior (Should Navigate History) ==="
echo ""

# Create test for UP arrow navigation
cat > /tmp/arrow_test.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Add some commands to history
send "echo \"first command\"\r"
expect "$ "

send "echo \"second command\"\r"
expect "$ "

send "echo \"third command\"\r"
expect "$ "

# Now test UP arrow - this is the critical test
send "\033\[A"

# Check what happens
expect {
    "third command" {
        puts "\n✅ UP arrow navigation working correctly"
        send "\r"
        expect "$ "
        send "exit\r"
        exit 0
    }
    "display all" {
        puts "\n❌ BUG FOUND: UP arrow shows completion menu!"
        send "n\r"
        expect "$ "
        send "exit\r"
        exit 1
    }
    timeout {
        puts "\n❓ UP arrow behavior timeout"
        send "\003"
        send "exit\r"
        exit 1
    }
}
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/arrow_test.exp
    echo "Testing UP arrow history navigation..."
    /tmp/arrow_test.exp
    arrow_test_result=$?
else
    echo "expect not available - using input simulation..."
    # We can't easily simulate arrow keys in automated input
    echo "Manual testing required for arrow keys"
    arrow_test_result=2
fi

echo ""
echo "=== Test 3: Completion Trigger Analysis ==="
echo ""

echo "Testing when lusush_tab_completion function gets called..."

# Test with various input scenarios
echo "Scenario 1: Empty line + TAB (should trigger completion):"
echo -e '\t' | script -q -c "./builddir/lusush" /dev/null

echo ""
echo "Scenario 2: Partial command + TAB (should trigger completion):"
echo -e 'ec\t\nexit' | script -q -c "./builddir/lusush" /dev/null

echo ""
echo "Scenario 3: Command + UP arrow simulation:"
echo "Note: Cannot easily simulate arrow keys in pipe input"

echo ""
echo "=== Test 4: Manual Testing Instructions ==="
echo ""

echo "For definitive testing, run these commands manually:"
echo ""
echo "1. Start interactive lusush:"
echo "   script -q -c './builddir/lusush' /dev/null"
echo ""
echo "2. Test TAB completion:"
echo "   - Type: ec"
echo "   - Press TAB"
echo "   - Should complete to 'echo' or show echo options"
echo ""
echo "3. Test history navigation:"
echo "   - Type: echo \"test 1\""
echo "   - Press ENTER"
echo "   - Type: echo \"test 2\""
echo "   - Press ENTER"
echo "   - Press UP ARROW"
echo "   - Should show: echo \"test 2\""
echo "   - Should NOT show: display all XXXX possibilities"
echo ""
echo "4. Test signal handling:"
echo "   - Type: echo \"hello\" (don't press ENTER)"
echo "   - Press Ctrl+C"
echo "   - Should: clear line, show new prompt"
echo "   - Should NOT: exit the shell"
echo ""

echo "=== Debug Output Analysis ==="
echo ""

echo "Key debug messages to look for:"
echo ""
echo "✅ GOOD signs:"
echo "  [DEBUG] Interactive mode = TRUE"
echo "  [DEBUG] show-all-if-ambiguous: OFF"
echo "  [DEBUG] Completion function set for INTERACTIVE mode"
echo "  [SIGNAL_DEBUG] readline_sigint_handler called"
echo "  [COMPLETION_DEBUG] BLOCKED: Empty context completion"
echo ""
echo "❌ BAD signs:"
echo "  [COMPLETION_DEBUG] tab_completion called (on arrow keys)"
echo "  No [SIGNAL_DEBUG] output after Ctrl+C"
echo "  Shell exits instead of continuing"
echo ""

echo "=== Implementation Status ==="
echo ""

if [ $arrow_test_result -eq 0 ]; then
    echo "🎉 UP ARROW TEST PASSED"
    echo "   History navigation appears to be working correctly"
    echo "   The user-reported issue may be resolved"
elif [ $arrow_test_result -eq 1 ]; then
    echo "🚨 UP ARROW TEST FAILED"
    echo "   Completion menu still appears on UP arrow"
    echo "   Need additional fixes to prevent completion trigger"
else
    echo "❓ UP ARROW TEST INCONCLUSIVE"
    echo "   Manual testing required to verify behavior"
fi

echo ""
echo "Based on testing results:"
echo "1. If TAB completion works but UP arrow shows completion menu:"
echo "   → Need to prevent completion function from running on arrow keys"
echo "2. If both TAB and UP arrow work correctly:"
echo "   → Issue may be resolved, need user verification"
echo "3. If neither works:"
echo "   → More fundamental completion system issue"

# Cleanup
rm -f /tmp/tab_test.exp /tmp/arrow_test.exp /tmp/tab_input

echo ""
echo "Next: Based on these results, implement targeted fixes or verify resolution."

#!/bin/bash

# Focused test for specific issues reported on Fedora:
# 1. Backspace consumes a line again
# 2. History recall first time prints new prompt line
# 3. Later at bottom consumes previous line

echo "==============================================================================="
echo "LUSUSH SPECIFIC ISSUES TEST - FEDORA LINUX"
echo "==============================================================================="
echo ""

LUSUSH_BIN="./builddir/lusush"

if [ ! -f "$LUSUSH_BIN" ]; then
    echo "❌ ERROR: LUSUSH binary not found at $LUSUSH_BIN"
    echo "Please build first with: ninja -C builddir"
    exit 1
fi

echo "🔍 Testing specific issues reported on Fedora Linux..."
echo ""
echo "Issues to test:"
echo "1. Backspace consuming lines"
echo "2. First history recall creating new prompt line"
echo "3. Subsequent history at bottom consuming previous lines"
echo ""

echo "==============================================================================="
echo "AUTOMATED SETUP TEST"
echo "==============================================================================="

# Basic functionality test
echo "📝 Testing basic shell functionality..."
result=$(echo -e "echo test\nexit" | timeout 5 $LUSUSH_BIN 2>&1 | grep "test")
if [[ "$result" == "test" ]]; then
    echo "✓ Basic shell functionality working"
else
    echo "✗ Basic shell functionality failed"
    echo "Output: $result"
    exit 1
fi

echo ""
echo "==============================================================================="
echo "ISSUE 1: BACKSPACE LINE CONSUMPTION TEST"
echo "==============================================================================="
echo ""
echo "This test requires manual verification because backspace is interactive."
echo ""
echo "🎯 TEST PROCEDURE:"
echo ""
echo "1. Fill your terminal with content:"
echo "   for i in {1..20}; do echo \"BackspaceTest \$i: Should not disappear\"; done"
echo ""
echo "2. Start lusush: $LUSUSH_BIN"
echo ""
echo "3. Type: echo hello world"
echo ""
echo "4. Press backspace 5 times (should delete 'world')"
echo ""
echo "5. OBSERVE: Do any 'BackspaceTest' lines disappear?"
echo "   ✅ GOOD: All BackspaceTest lines remain visible"
echo "   ❌ BAD: Some BackspaceTest lines are consumed/disappear"
echo ""
echo "6. Type: test"
echo ""
echo "7. Press Enter (should show 'hello test')"
echo ""
echo "8. Type: exit"
echo ""

echo "==============================================================================="
echo "ISSUE 2: FIRST HISTORY RECALL NEW PROMPT LINE"
echo "==============================================================================="
echo ""
echo "This test checks if the FIRST history recall creates a new prompt line."
echo ""
echo "🎯 TEST PROCEDURE:"
echo ""
echo "1. Start lusush: $LUSUSH_BIN"
echo ""
echo "2. Type: echo first command"
echo "   Press Enter"
echo ""
echo "3. Type: echo second command"
echo "   Press Enter"
echo ""
echo "4. Press UP arrow ONCE (first history recall)"
echo ""
echo "5. OBSERVE the behavior:"
echo "   ✅ GOOD: 'echo second command' replaces current line"
echo "   ❌ BAD: New prompt line is created below current line"
echo ""
echo "6. If bad behavior observed, note exactly what you see"
echo ""
echo "7. Type: exit"
echo ""

echo "==============================================================================="
echo "ISSUE 3: SUBSEQUENT HISTORY AT BOTTOM CONSUMING LINES"
echo "==============================================================================="
echo ""
echo "This test checks if history navigation at bottom consumes previous lines."
echo ""
echo "🎯 TEST PROCEDURE:"
echo ""
echo "1. Fill terminal to push prompt to bottom:"
echo "   for i in {1..25}; do echo \"HistoryTest \$i: Should not be consumed\"; done"
echo ""
echo "2. Start lusush: $LUSUSH_BIN"
echo "   (prompt should now be at or near bottom)"
echo ""
echo "3. Create history entries:"
echo "   Type: echo first"
echo "   Press Enter"
echo "   Type: echo second"
echo "   Press Enter"
echo "   Type: echo third"
echo "   Press Enter"
echo ""
echo "4. Navigate history multiple times:"
echo "   Press UP arrow (should show 'echo third')"
echo "   Press UP arrow (should show 'echo second')"
echo "   Press UP arrow (should show 'echo first')"
echo "   Press DOWN arrow (should show 'echo second')"
echo "   Press DOWN arrow (should show 'echo third')"
echo ""
echo "5. OBSERVE after each arrow key press:"
echo "   ✅ GOOD: All HistoryTest lines remain visible"
echo "   ❌ BAD: HistoryTest lines are consumed/disappear"
echo ""
echo "6. Note which specific operations cause line consumption"
echo ""
echo "7. Type: exit"
echo ""

echo "==============================================================================="
echo "AUTOMATED EXPECT TEST FOR COMPARISON"
echo "==============================================================================="

if command -v expect >/dev/null 2>&1; then
    echo ""
    echo "Running automated expect test to compare with manual observations..."

    cat > /tmp/specific_issues_test.exp << 'EOF'
#!/usr/bin/expect -f

set timeout 5
spawn ./builddir/lusush
expect "$ "

# Test basic history without bottom-line scenario first
send "echo cmd1\r"
expect "cmd1"
expect "$ "

send "echo cmd2\r"
expect "cmd2"
expect "$ "

# Test first history recall
send "\033\[A"
sleep 0.2

# Capture what happens
set output $expect_out(buffer)
if {[string match "*\$ *\$ *" $output]} {
    puts "\n❌ DETECTED: New prompt line created on first history recall"
} else {
    puts "\n✓ First history recall appears normal"
}

send "\r"
expect "$ "
send "exit\r"
expect eof
EOF

    chmod +x /tmp/specific_issues_test.exp
    /tmp/specific_issues_test.exp
    rm -f /tmp/specific_issues_test.exp

else
    echo "expect not available - skipping automated test"
fi

echo ""
echo "==============================================================================="
echo "DIAGNOSTIC COMMANDS"
echo "==============================================================================="
echo ""
echo "Before testing, gather diagnostic information:"
echo ""
echo "🔧 System Information:"
echo "   OS: $(cat /etc/fedora-release 2>/dev/null || echo 'Not Fedora')"
echo "   Kernel: $(uname -r)"
echo "   Terminal: $TERM"
echo "   Terminal Size: $(tput lines 2>/dev/null || echo '?')x$(tput cols 2>/dev/null || echo '?')"
echo "   Terminal Program: $(ps -p $PPID -o comm= 2>/dev/null || echo 'unknown')"
echo "   Display Server: $(echo $XDG_SESSION_TYPE 2>/dev/null || echo 'unknown')"
echo ""
echo "🔧 Fill Terminal Commands:"
echo "   For backspace test: for i in {1..20}; do echo \"BackspaceTest \$i: Should not disappear\"; done"
echo "   For history test: for i in {1..25}; do echo \"HistoryTest \$i: Should not be consumed\"; done"
echo ""

echo "==============================================================================="
echo "REPORTING TEMPLATE"
echo "==============================================================================="
echo ""
echo "After running the tests, please report your findings:"
echo ""
echo "🔍 ISSUE 1 - BACKSPACE LINE CONSUMPTION:"
echo "   Observed: (Yes/No) ____"
echo "   Details: ____"
echo ""
echo "🔍 ISSUE 2 - FIRST HISTORY NEW PROMPT LINE:"
echo "   Observed: (Yes/No) ____"
echo "   Details: ____"
echo ""
echo "🔍 ISSUE 3 - HISTORY AT BOTTOM CONSUMING LINES:"
echo "   Observed: (Yes/No) ____"
echo "   Which operations: ____"
echo "   How many lines consumed: ____"
echo ""
echo "🔍 ADDITIONAL OBSERVATIONS:"
echo "   Cursor behavior: ____"
echo "   Screen flickering: ____"
echo "   Terminal artifacts: ____"
echo "   Other issues: ____"
echo ""
echo "==============================================================================="
echo "READY TO TEST"
echo "==============================================================================="
echo ""
echo "Follow the test procedures above and report your findings."
echo "Each test should be run separately for accurate observation."
echo ""
echo "Press Enter to continue or Ctrl+C to cancel..."
read

echo ""
echo "You can now run the manual tests described above."
echo "Start with filling your terminal, then run: $LUSUSH_BIN"
echo ""

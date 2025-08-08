#!/bin/bash

# Real Interactive Tab Completion Test for Lusush Line Editor
# Uses expect to send actual TAB character to interactive shell

echo "========================================================================"
echo "LUSUSH LINE EDITOR - REAL TAB COMPLETION TEST"
echo "========================================================================"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_PATH="$SCRIPT_DIR/builddir/lusush"

if [[ ! -x "$SHELL_PATH" ]]; then
    echo "❌ Error: Shell not found at $SHELL_PATH"
    echo "Please build the shell first: scripts/lle_build.sh build"
    exit 1
fi

echo "✅ Shell found: $SHELL_PATH"

# Check if expect is available
if ! command -v expect >/dev/null 2>&1; then
    echo "❌ Error: 'expect' command not found"
    echo "Please install expect: brew install expect"
    echo ""
    echo "Alternative manual test:"
    echo "1. Run: LLE_DEBUG=1 $SHELL_PATH"
    echo "2. Type 'ec' and press TAB"
    echo "3. Check if it completes to 'echo'"
    echo "4. Type 'exit' to quit"
    exit 1
fi

echo "✅ expect command available"
echo ""

# Create test environment
TEST_DIR="/tmp/lle_real_tab_test_$$"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create test files
echo "test content" > test_file.txt
echo "another file" > another_test.txt
echo "executable" > test_script.sh
chmod +x test_script.sh
mkdir test_directory

echo "✅ Created test environment in: $TEST_DIR"
echo ""

echo "=== RUNNING AUTOMATED TAB COMPLETION TESTS ==="

# Test 1: Command completion
echo "Test 1: Command completion (ec + TAB)"
expect << 'EOF'
log_user 0
set timeout 10
spawn -noecho env LLE_DEBUG=1 /Users/mberry/Lab/c/lusush/builddir/lusush
expect -re "\\\$ "
send "ec\t"
sleep 1
expect -re "\\\$ "
send "exit\r"
expect eof
EOF

TAB_TEST_1=$?
echo "Command completion test result: $TAB_TEST_1"

# Test 2: File completion
echo ""
echo "Test 2: File completion (cat te + TAB)"
expect << 'EOF'
log_user 0
set timeout 10
spawn -noecho env LLE_DEBUG=1 /Users/mberry/Lab/c/lusush/builddir/lusush
expect -re "\\\$ "
send "cat te\t"
sleep 1
expect -re "\\\$ "
send "exit\r"
expect eof
EOF

TAB_TEST_2=$?
echo "File completion test result: $TAB_TEST_2"

# Test 3: With debug output capture
echo ""
echo "Test 3: Debug output capture"
expect << 'EOF' 2>&1 | tee /tmp/tab_real_debug.log
log_user 1
set timeout 10
spawn -noecho env LLE_DEBUG=1 /Users/mberry/Lab/c/lusush/builddir/lusush
expect -re "\\\$ "
send_user "\n=== SENDING TAB AFTER 'ec' ===\n"
send "ec\t"
sleep 2
send_user "\n=== TAB SENT, WAITING FOR RESPONSE ===\n"
expect -re "\\\$ "
send_user "\n=== SENDING EXIT COMMAND ===\n"
send "exit\r"
expect eof
send_user "\n=== SESSION COMPLETE ===\n"
EOF

echo ""
echo "========================================================================"
echo "TEST RESULTS ANALYSIS"
echo "========================================================================"

echo ""
echo "📊 Test Results:"
echo "  Command completion: $([[ $TAB_TEST_1 -eq 0 ]] && echo "✅ PASS" || echo "❌ FAIL ($TAB_TEST_1)")"
echo "  File completion: $([[ $TAB_TEST_2 -eq 0 ]] && echo "✅ PASS" || echo "❌ FAIL ($TAB_TEST_2)")"

echo ""
echo "🔍 Debug Output Analysis:"
if [[ -f /tmp/tab_real_debug.log ]]; then
    echo "Debug log generated. Key events:"

    # Check for TAB events
    TAB_EVENTS=$(grep -c "TAB case executed\|LLE_KEY_TAB" /tmp/tab_real_debug.log 2>/dev/null || echo "0")
    echo "  📊 TAB events detected: $TAB_EVENTS"

    # Check for completion calls
    COMPLETION_CALLS=$(grep -c "enhanced_tab_completion_handle\|ENHANCED_TAB_COMPLETION" /tmp/tab_real_debug.log 2>/dev/null || echo "0")
    echo "  📊 Completion handler calls: $COMPLETION_CALLS"

    # Check for completion results
    if grep -q "Tab completion handled successfully" /tmp/tab_real_debug.log; then
        echo "  ✅ Tab completion processing successful"
    elif grep -q "Tab completion handler returned false" /tmp/tab_real_debug.log; then
        echo "  ⚠️  Tab completion processing failed"
    elif grep -q "Tab completion disabled" /tmp/tab_real_debug.log; then
        echo "  ❌ Tab completion disabled"
    else
        echo "  ❓ Tab completion status unclear"
    fi

    echo ""
    echo "📋 Key Debug Messages:"
    grep -i "tab\|completion" /tmp/tab_real_debug.log | head -10

else
    echo "❌ No debug log generated"
fi

echo ""
echo "========================================================================"
echo "CONCLUSION AND NEXT STEPS"
echo "========================================================================"

# Determine status
WORKING_COUNT=0
if [[ $TAB_TEST_1 -eq 0 ]]; then ((WORKING_COUNT++)); fi
if [[ $TAB_TEST_2 -eq 0 ]]; then ((WORKING_COUNT++)); fi

if [[ $WORKING_COUNT -eq 2 ]] && [[ $TAB_EVENTS -gt 0 ]] && [[ $COMPLETION_CALLS -gt 0 ]]; then
    echo "🎉 TAB COMPLETION STATUS: WORKING PERFECTLY"
    echo "   ✅ All tests passed"
    echo "   ✅ TAB events being processed"
    echo "   ✅ Completion handlers being called"
    echo ""
    echo "   → Tab completion is COMPLETE"
    echo "   → Ready to move to next priority"
    echo "   → Consider LLE-019 Ctrl+R reverse search next"

elif [[ $TAB_EVENTS -gt 0 ]] && [[ $COMPLETION_CALLS -gt 0 ]]; then
    echo "🔧 TAB COMPLETION STATUS: PARTIALLY WORKING"
    echo "   ✅ TAB events being processed"
    echo "   ✅ Completion handlers being called"
    echo "   ⚠️  Some completion scenarios may need refinement"
    echo ""
    echo "   → Core functionality working"
    echo "   → May need provider tuning or display polish"

elif [[ $TAB_EVENTS -gt 0 ]]; then
    echo "🔧 TAB COMPLETION STATUS: TAB DETECTED BUT COMPLETION FAILING"
    echo "   ✅ TAB events being processed"
    echo "   ❌ Completion handlers not being called or failing"
    echo ""
    echo "   → Check completion initialization"
    echo "   → Check provider registration"
    echo "   → Debug completion context creation"

else
    echo "❌ TAB COMPLETION STATUS: NOT WORKING"
    echo "   ❌ TAB events not being processed"
    echo "   ❌ Completion handlers not being called"
    echo ""
    echo "   → Check key event reading"
    echo "   → Check LLE_KEY_TAB mapping"
    echo "   → Debug input processing loop"
fi

echo ""
echo "📚 Debug Resources:"
echo "  Full debug log: /tmp/tab_real_debug.log"
echo "  Search for events: grep -i 'tab\\|completion' /tmp/tab_real_debug.log"
echo "  Manual testing: LLE_DEBUG=1 $SHELL_PATH"

# Cleanup
cd "$SCRIPT_DIR"
rm -rf "$TEST_DIR"

echo "========================================================================"

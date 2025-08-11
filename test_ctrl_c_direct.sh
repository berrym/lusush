#!/bin/bash

# Direct Ctrl+C Behavior Test for Lusush
# This test specifically checks if Ctrl+C exits the shell or behaves correctly

echo "=== Direct Ctrl+C Behavior Test ==="
echo "Date: $(date)"
echo ""

# Ensure lusush is built
if [ ! -f builddir/lusush ]; then
    echo "❌ lusush binary not found. Building..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "❌ Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "This test specifically addresses the user's critical complaint:"
echo "  ❌ BUG: 'ctrl+c exits the shell it should only exit a child process'"
echo "  ✅ FIX: Ctrl+C should clear line or interrupt child, NOT exit shell"
echo ""

echo "=== BACKGROUND: Signal Handler Analysis ==="
echo ""
echo "Based on code review of src/signals.c:"
echo "  ✅ readline_sigint_handler() - Should clear line, keep shell running"
echo "  ✅ child_sigint_handler() - Should interrupt child process"
echo "  ✅ Handler switching logic - Switch based on child process state"
echo "  ✅ rl_catch_signals = 0 - Shell handles signals, not readline"
echo ""

echo "The implementation LOOKS correct, but user reports Ctrl+C exits shell."
echo "This test will determine if the issue is actually resolved."
echo ""

echo "=== TEST APPROACH ==="
echo ""
echo "We'll test Ctrl+C in two scenarios:"
echo "  1. When typing a command (no child process)"
echo "  2. When running a child process (sleep command)"
echo ""

echo "=== AUTOMATED PRE-TEST ==="
echo ""

# Test basic shell startup
echo "Testing basic shell startup and exit..."
echo 'echo "Basic test"' | script -q -c './builddir/lusush' /dev/null >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✅ Shell starts and exits normally"
else
    echo "❌ Shell has basic startup/exit issues"
    exit 1
fi

echo ""
echo "=== CRITICAL MANUAL TEST REQUIRED ==="
echo ""

echo "🚨 IMPORTANT: The next test requires YOUR manual input!"
echo ""
echo "I will start lusush in interactive mode."
echo "When you see the lusush prompt, please:"
echo ""
echo "TEST 1 - Ctrl+C without child process:"
echo "  1. Type: echo hello world"
echo "  2. DON'T press Enter yet"
echo "  3. Press Ctrl+C"
echo "  4. Observe what happens"
echo ""
echo "TEST 2 - Ctrl+C with child process:"
echo "  1. Type: sleep 5"
echo "  2. Press Enter (let it start)"
echo "  3. Press Ctrl+C"
echo "  4. Observe what happens"
echo ""
echo "TEST 3 - Exit normally:"
echo "  1. Type: exit"
echo "  2. Press Enter"
echo ""

echo "EXPECTED RESULTS:"
echo "  ✅ GOOD: Test 1 - Line clears, new prompt, shell continues"
echo "  ✅ GOOD: Test 2 - Sleep interrupted, new prompt, shell continues"
echo "  ✅ GOOD: Test 3 - Shell exits normally on 'exit' command"
echo ""
echo "  ❌ BAD: Test 1 - Shell exits immediately on Ctrl+C"
echo "  ❌ BAD: Test 2 - Shell exits instead of just interrupting sleep"
echo "  ❌ BAD: You're returned to system shell without typing 'exit'"
echo ""

read -p "Ready to start manual test? Press Enter to continue..."

echo ""
echo "Starting lusush in interactive mode now..."
echo "Remember: Test Ctrl+C behavior as described above"
echo ""

# Start lusush in truly interactive mode with force flag
export LUSUSH_FORCE_INTERACTIVE=1
script -q -c './builddir/lusush' /dev/null

echo ""
echo "=== TEST ANALYSIS ==="
echo ""

echo "Now that you've completed the manual test, please analyze:"
echo ""

echo "QUESTION 1: When you typed 'echo hello world' and pressed Ctrl+C:"
echo "  A) Shell exited immediately (BUG - signal handling broken)"
echo "  B) Line cleared, new prompt appeared, shell continued (FIXED)"
echo "  C) Something else happened"
echo ""

echo "QUESTION 2: When you ran 'sleep 5' and pressed Ctrl+C:"
echo "  A) Shell exited immediately (BUG - signal handling broken)"
echo "  B) Sleep was interrupted, shell continued with new prompt (FIXED)"
echo "  C) Sleep continued running, Ctrl+C had no effect (different bug)"
echo ""

echo "QUESTION 3: Could you exit normally with 'exit' command?"
echo "  A) Yes, 'exit' worked normally (GOOD)"
echo "  B) No, 'exit' didn't work (separate issue)"
echo ""

echo "=== DIAGNOSIS GUIDE ==="
echo ""

echo "If Ctrl+C exits the shell in BOTH tests (Answer A to both):"
echo "  ❌ SIGNAL HANDLING BROKEN"
echo "  🔧 URGENT FIX NEEDED in src/signals.c"
echo "  📋 Issue: Signal handler not properly integrated with readline"
echo "  🎯 Action: Fix readline_sigint_handler implementation"
echo ""

echo "If Ctrl+C works correctly in test 1 but not test 2:"
echo "  ❌ CHILD PROCESS SIGNAL HANDLING BROKEN"
echo "  🔧 FIX NEEDED in child_sigint_handler"
echo "  📋 Issue: Child process interrupt not working correctly"
echo ""

echo "If Ctrl+C works correctly in BOTH tests (Answer B to both):"
echo "  ✅ SIGNAL HANDLING FIXED"
echo "  🎉 CRITICAL ISSUE RESOLVED"
echo "  📋 Status: This major user complaint has been addressed"
echo "  🎯 Next: Verify other interactive mode features"
echo ""

echo "=== TECHNICAL DEBUGGING INFO ==="
echo ""

echo "If signal handling is still broken, check:"
echo ""

echo "1. Signal handler installation:"
echo "   File: src/signals.c, function: init_signal_handlers()"
echo "   Should call: set_signal_handler(SIGINT, readline_sigint_handler)"
echo ""

echo "2. Readline signal configuration:"
echo "   File: src/readline_integration.c, function: setup_readline_config()"
echo "   Should have: rl_catch_signals = 0 (let shell handle signals)"
echo ""

echo "3. Signal handler implementation:"
echo "   File: src/signals.c, function: readline_sigint_handler()"
echo "   Should: Clear line, redisplay prompt, NOT exit shell"
echo ""

echo "4. Interactive mode detection:"
echo "   Should properly detect interactive mode for signal context"
echo "   Check is_interactive_session variable"
echo ""

echo "=== CONCLUSION ==="
echo ""

echo "This test determines the current state of the critical signal handling issue."
echo "Based on your manual testing results, you can:"
echo ""
echo "✅ If working: Move on to other interactive mode features"
echo "❌ If broken: Focus on signal handler debugging and fixes"
echo ""

echo "The code LOOKS correct based on review, but manual testing is definitive."
echo ""

echo "Test completed: $(date)"

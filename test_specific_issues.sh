#!/bin/bash

# Focused Test Script for Lusush Interactive Mode Issues
# Tests the 3 critical issues identified in the handoff documentation

echo "=== Lusush Critical Interactive Issues Test ==="
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

echo "This script will test the 3 critical interactive mode issues:"
echo ""
echo "ISSUE #1: History Navigation"
echo "  Problem: UP arrow shows completion menu instead of navigating history"
echo "  Expected: UP arrow should cycle through previous commands"
echo ""
echo "ISSUE #2: Signal Handling"
echo "  Problem: Ctrl+C exits entire shell"
echo "  Expected: Ctrl+C should clear line and continue shell"
echo ""
echo "ISSUE #3: Interactive Operations"
echo "  Problem: Pipes and redirections don't work in interactive mode"
echo "  Expected: Should work same as non-interactive mode"
echo ""

echo "=== BASELINE: Non-Interactive Mode (Should Work) ==="
echo ""

echo "Testing pipes in non-interactive mode:"
echo 'echo "hello world" | grep hello' | ./builddir/lusush
echo "Result: Should show 'hello world' ✓"
echo ""

echo "Testing redirection in non-interactive mode:"
echo 'echo "test content" > /tmp/lusush_test.txt && cat /tmp/lusush_test.txt' | ./builddir/lusush
echo "Result: Should show 'test content' ✓"
echo ""

echo "Testing command substitution in non-interactive mode:"
echo 'echo "User: $(whoami)"' | ./builddir/lusush
echo "Result: Should show user name ✓"
echo ""

echo "=== INTERACTIVE MODE TESTING ==="
echo ""
echo "Now starting lusush in INTERACTIVE mode..."
echo "Debug output will show [DEBUG] and [SIGNAL_DEBUG] messages"
echo ""

echo "🔍 TEST SEQUENCE TO FOLLOW:"
echo ""
echo "1. HISTORY NAVIGATION TEST:"
echo "   → Type: echo \"command 1\""
echo "   → Press ENTER"
echo "   → Type: echo \"command 2\""
echo "   → Press ENTER"
echo "   → Press UP ARROW"
echo "   ✅ CORRECT: Should show 'echo \"command 2\"'"
echo "   ❌ BUG: Shows 'display all 4418 possibilities (y or n)?'"
echo ""

echo "2. SIGNAL HANDLING TEST:"
echo "   → Type: echo \"test line\" (DON'T press ENTER)"
echo "   → Press Ctrl+C"
echo "   ✅ CORRECT: Line clears, new prompt appears, shell continues"
echo "   ❌ BUG: Entire shell exits"
echo ""

echo "3. INTERACTIVE OPERATIONS TEST:"
echo "   → Type: echo hello | grep h"
echo "   → Press ENTER"
echo "   ✅ CORRECT: Shows 'hello'"
echo "   ❌ BUG: Command fails or behaves incorrectly"
echo ""
echo "   → Type: echo test > /tmp/interactive_test.txt"
echo "   → Press ENTER"
echo "   → Type: cat /tmp/interactive_test.txt"
echo "   → Press ENTER"
echo "   ✅ CORRECT: Shows 'test'"
echo "   ❌ BUG: Redirection or cat fails"
echo ""
echo "   → Type: echo \"User: \$(whoami)\""
echo "   → Press ENTER"
echo "   ✅ CORRECT: Shows 'User: [username]'"
echo "   ❌ BUG: Command substitution fails"
echo ""

echo "4. WHEN FINISHED: Type 'exit' to return to this script"
echo ""

echo "🚨 CRITICAL DEBUG MARKERS TO WATCH FOR:"
echo ""
echo "For HISTORY ISSUE - Look for:"
echo "  [COMPLETION_DEBUG] triggered on UP arrow ← This is the bug!"
echo "  Should only see [COMPLETION_DEBUG] on TAB key"
echo ""
echo "For SIGNAL ISSUE - Look for:"
echo "  [SIGNAL_DEBUG] readline_sigint_handler called ← Should see this"
echo "  [SIGNAL_DEBUG] shell should continue ← Should see this"
echo "  If shell exits, signal handling is broken"
echo ""
echo "For OPERATIONS ISSUE - Look for:"
echo "  Different debug output between interactive and non-interactive execution"
echo "  Execution failures that don't occur in non-interactive mode"
echo ""

echo "🎯 STARTING INTERACTIVE MODE WITH DEBUGGING..."
echo ""
echo "Press ENTER to start lusush interactively with full debugging enabled..."
read

# Start interactive mode with debugging
./builddir/lusush

echo ""
echo "=== INTERACTIVE TEST COMPLETE ==="
echo ""

# Cleanup
rm -f /tmp/lusush_test.txt /tmp/interactive_test.txt

echo "📊 RESULTS ANALYSIS:"
echo ""
echo "Based on what you observed, which issues occurred?"
echo ""
echo "ISSUE #1 - History Navigation:"
echo "  [ ] UP arrow showed completion menu (BUG present)"
echo "  [ ] UP arrow navigated history correctly (BUG fixed)"
echo ""
echo "ISSUE #2 - Signal Handling:"
echo "  [ ] Ctrl+C exited the shell (BUG present)"
echo "  [ ] Ctrl+C cleared line, shell continued (BUG fixed)"
echo ""
echo "ISSUE #3 - Interactive Operations:"
echo "  [ ] Pipes/redirections failed in interactive mode (BUG present)"
echo "  [ ] Pipes/redirections worked in interactive mode (BUG fixed)"
echo ""
echo "📝 DEBUG OUTPUT ANALYSIS:"
echo ""
echo "Key debug messages that indicate the root causes:"
echo ""
echo "1. If you saw [COMPLETION_DEBUG] on UP arrow - that's the history bug"
echo "2. If shell exited on Ctrl+C without [SIGNAL_DEBUG] messages - that's the signal bug"
echo "3. If operations failed differently than non-interactive mode - that's the execution bug"
echo ""
echo "Use this information to implement targeted fixes in:"
echo "  - src/readline_integration.c (for history and completion issues)"
echo "  - src/signals.c (for signal handling issues)"
echo "  - src/executor.c or src/input.c (for interactive execution issues)"

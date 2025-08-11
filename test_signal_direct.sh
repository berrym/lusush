#!/bin/bash

# Direct Signal Handling Test for Lusush
# This test directly verifies Ctrl+C behavior in interactive mode

echo "=== Direct Signal Handling Test for Lusush ==="
echo "Testing the critical Ctrl+C behavior issue"
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

echo "This test checks:"
echo "  ❌ DANGEROUS BUG: Ctrl+C exits entire shell"
echo "  ✅ SAFE BEHAVIOR: Ctrl+C clears line, shell continues"
echo ""

echo "IMPORTANT: This test requires manual interaction!"
echo "You will need to:"
echo "  1. Type some text (don't press Enter)"
echo "  2. Press Ctrl+C"
echo "  3. Check if shell continues or exits"
echo ""

read -p "Press Enter to start interactive test..."

echo ""
echo "Starting lusush in interactive mode..."
echo "When you see the lusush prompt:"
echo "  1. Type: echo hello world"
echo "  2. DON'T press Enter yet"
echo "  3. Press Ctrl+C"
echo "  4. Observe what happens"
echo ""
echo "Expected SAFE behavior: Line clears, new prompt appears, shell continues"
echo "Dangerous BUG behavior: Entire shell exits immediately"
echo ""

# Enable signal debugging
export LUSUSH_DEBUG=1
export LUSUSH_FORCE_INTERACTIVE=1

echo "Starting lusush with signal debugging enabled..."
echo "Look for [SIGNAL_DEBUG] messages to see what's happening"
echo ""

# Start lusush in truly interactive mode
script -q -c './builddir/lusush' /dev/null

echo ""
echo "=== TEST COMPLETED ==="
echo ""

echo "What happened when you pressed Ctrl+C?"
echo ""
echo "If you see this message, it means:"
echo "  ✅ Shell returned control (good sign)"
echo "  ❓ Need to analyze if Ctrl+C behavior was correct"
echo ""

echo "Expected Results:"
echo "  ✅ GOOD: Shell showed new prompt after Ctrl+C"
echo "  ✅ GOOD: You could type more commands after Ctrl+C"
echo "  ✅ GOOD: Shell stayed running"
echo ""
echo "  ❌ BAD: Shell exited immediately on Ctrl+C"
echo "  ❌ BAD: You were returned to your system shell"
echo "  ❌ BAD: Lost your lusush session"
echo ""

echo "=== DIAGNOSIS INSTRUCTIONS ==="
echo ""

echo "If Ctrl+C exited the shell (BAD):"
echo "  Problem: Signal handler in src/signals.c not working correctly"
echo "  Solution: Fix readline_sigint_handler to not exit shell"
echo ""

echo "If Ctrl+C cleared line and continued (GOOD):"
echo "  Result: Signal handling is working correctly"
echo "  Status: This critical issue is resolved"
echo ""

echo "=== MANUAL VERIFICATION CHECKLIST ==="
echo ""

echo "Test these scenarios manually:"
echo ""
echo "1. Type command without executing, press Ctrl+C:"
echo "   lusush\$ echo hello[Ctrl+C]"
echo "   Expected: New prompt, shell continues"
echo ""

echo "2. Start child process, interrupt with Ctrl+C:"
echo "   lusush\$ sleep 10[Ctrl+C]"
echo "   Expected: Sleep interrupted, shell continues"
echo ""

echo "3. Multiple Ctrl+C presses:"
echo "   lusush\$ echo test[Ctrl+C][Ctrl+C][Ctrl+C]"
echo "   Expected: Shell robust, continues after each Ctrl+C"
echo ""

echo "=== DEBUGGING INFORMATION ==="
echo ""

echo "If signal handling is broken, check:"
echo "  1. src/signals.c - readline_sigint_handler function"
echo "  2. Signal handler installation in init_signal_handlers"
echo "  3. Readline signal configuration (rl_catch_signals = 0)"
echo ""

echo "Enable debugging with:"
echo "  LUSUSH_DEBUG=1 ./builddir/lusush"
echo "  Look for [SIGNAL_DEBUG] messages"
echo ""

echo "=== NEXT STEPS ==="
echo ""

echo "Based on your manual test results:"
echo ""
echo "If signal handling is BROKEN:"
echo "  Priority: URGENT - Fix readline_sigint_handler"
echo "  Impact: CRITICAL - Users lose shell sessions"
echo "  Files: src/signals.c, src/readline_integration.c"
echo ""

echo "If signal handling is WORKING:"
echo "  Priority: Investigate other reported issues"
echo "  Status: Major progress made"
echo "  Focus: Verify all interactive operations"
echo ""

echo "Test completed: $(date)"

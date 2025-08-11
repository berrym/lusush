#!/bin/bash

# Comprehensive Interactive Mode Debug Script for Lusush
# This script enables all debugging and provides systematic testing

echo "=== Lusush Interactive Mode Debug Session ==="
echo "This script will enable comprehensive debugging and test each issue systematically."
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

echo "=== DEBUGGING SESSION START ==="
echo ""

echo "This script will:"
echo "1. Enable all debugging output"
echo "2. Test history navigation issue (UP arrow)"
echo "3. Test signal handling issue (Ctrl+C)"
echo "4. Test interactive operations (pipes, redirections)"
echo "5. Compare with non-interactive mode behavior"
echo ""

echo "=== PHASE 1: NON-INTERACTIVE MODE BASELINE ==="
echo "Testing non-interactive mode (should work perfectly):"

echo ""
echo "Test 1: Basic command execution"
echo 'echo "Hello from non-interactive mode"' | ./builddir/lusush

echo ""
echo "Test 2: Pipe operation"
echo 'echo "hello world" | grep hello' | ./builddir/lusush

echo ""
echo "Test 3: Redirection"
echo 'echo "test content" > /tmp/lusush_debug_test.txt && cat /tmp/lusush_debug_test.txt' | ./builddir/lusush

echo ""
echo "Test 4: Command substitution"
echo 'echo "User is: $(whoami)"' | ./builddir/lusush

echo ""
echo "NON-INTERACTIVE MODE RESULTS: All should work perfectly (as confirmed by previous testing)"
echo ""

echo "=== PHASE 2: INTERACTIVE MODE DEBUGGING ==="
echo ""
echo "Now testing INTERACTIVE mode with full debugging enabled..."
echo ""

echo "INSTRUCTIONS FOR INTERACTIVE TESTING:"
echo ""
echo "The shell will start with debugging enabled. You'll see [DEBUG] messages."
echo "Please test the following in order:"
echo ""
echo "1. HISTORY NAVIGATION TEST:"
echo "   - Type: echo \"test 1\""
echo "   - Press ENTER"
echo "   - Type: echo \"test 2\""
echo "   - Press ENTER"
echo "   - Press UP ARROW"
echo "   - EXPECTED: Should show 'echo \"test 2\"'"
echo "   - BUG: Shows 'display all 4418 possibilities (y or n)?'"
echo ""
echo "2. SIGNAL HANDLING TEST:"
echo "   - Type: echo \"hello\" (DON'T press ENTER)"
echo "   - Press Ctrl+C"
echo "   - EXPECTED: Line clears, shell continues"
echo "   - BUG: Shell exits completely"
echo ""
echo "3. INTERACTIVE OPERATIONS TEST:"
echo "   - Test: echo hello | grep h"
echo "   - Test: echo test > /tmp/debug_test.txt"
echo "   - Test: cat /tmp/debug_test.txt"
echo "   - Test: echo \$(whoami)"
echo "   - EXPECTED: All should work"
echo "   - BUG: User reports these don't work in interactive mode"
echo ""
echo "4. When finished testing, type: exit"
echo ""
echo "=== DEBUG OUTPUT INTERPRETATION ==="
echo ""
echo "Watch for these debug messages:"
echo "- [DEBUG] Interactive mode = TRUE/FALSE"
echo "- [DEBUG] readline_with_prompt called in INTERACTIVE/NON-INTERACTIVE mode"
echo "- [COMPLETION_DEBUG] When completion is triggered (should only be on TAB)"
echo "- [SIGNAL_DEBUG] Signal handler behavior"
echo ""
echo "=== STARTING INTERACTIVE MODE WITH DEBUGGING ==="
echo ""

# Set environment variable to enable debug mode
export LUSUSH_DEBUG=1

echo "Press ENTER to start lusush in INTERACTIVE mode with full debugging..."
read

echo "Starting lusush with debugging enabled..."
echo "Follow the test instructions above!"
echo ""

# Run in interactive mode - this is where the real issues occur
./builddir/lusush

echo ""
echo "=== DEBUG SESSION COMPLETE ==="
echo ""
echo "Cleanup:"
rm -f /tmp/lusush_debug_test.txt /tmp/debug_test.txt

echo ""
echo "=== ANALYSIS QUESTIONS ==="
echo ""
echo "Based on the debug output, please analyze:"
echo ""
echo "1. HISTORY NAVIGATION ISSUE:"
echo "   - Did debug show completion being triggered on UP arrow?"
echo "   - Were completion settings correctly set to 'off'?"
echo "   - Did arrow keys get bound to completion functions?"
echo ""
echo "2. SIGNAL HANDLING ISSUE:"
echo "   - Did Ctrl+C call readline_sigint_handler or exit the shell?"
echo "   - Were signal handlers properly initialized for interactive mode?"
echo "   - Did the shell continue after Ctrl+C or terminate?"
echo ""
echo "3. INTERACTIVE OPERATIONS ISSUE:"
echo "   - Did pipe/redirect commands fail in interactive mode?"
echo "   - Were there differences in execution between interactive and non-interactive?"
echo "   - Did debug show different behavior in the two modes?"
echo ""
echo "4. MODE DETECTION:"
echo "   - Did debug correctly identify INTERACTIVE vs NON-INTERACTIVE mode?"
echo "   - Were different code paths taken in each mode?"
echo ""
echo "=== KEY DEBUGGING INSIGHTS ==="
echo ""
echo "Expected debug patterns for WORKING system:"
echo "- [DEBUG] Interactive mode = TRUE (when running interactively)"
echo "- [DEBUG] show-all-if-ambiguous: OFF (prevents UP arrow completion menu)"
echo "- [DEBUG] show-all-if-unmodified: OFF (prevents UP arrow completion menu)"
echo "- [SIGNAL_DEBUG] readline_sigint_handler called (on Ctrl+C)"
echo "- [SIGNAL_DEBUG] shell should continue (no exit)"
echo ""
echo "Expected debug patterns for BROKEN system:"
echo "- [COMPLETION_DEBUG] triggered on UP arrow (should only trigger on TAB)"
echo "- Shell exits on Ctrl+C (no debug output after signal)"
echo "- Different execution paths for interactive vs non-interactive operations"
echo ""
echo "Use this debug output to identify the root causes and implement targeted fixes."

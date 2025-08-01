#!/bin/bash
# Focused test script to trace the double-deletion bug
# This will help identify where and when buffer modifications happen multiple times

set -e

echo "=== Double-Deletion Bug Trace ==="
echo
echo "This test will trace buffer modifications during boundary crossing"
echo "to identify the source of the 'removes two characters instead of one' bug."
echo
echo "Building LLE..."
scripts/lle_build.sh build
echo

echo "=== Test Instructions ==="
echo
echo "1. The shell will start with detailed buffer modification logging"
echo "2. Type this EXACT command (it will wrap to second line):"
echo "   echo test_command_that_wraps_across_terminal_lines_for_testing"
echo "3. Press backspace slowly, ONE AT A TIME"
echo "4. Watch for these debug messages:"
echo "   [BACKSPACE_TRACE] - tracks each backspace command"
echo "   [BUFFER_BACKSPACE] - tracks buffer modification calls"
echo "5. Look for the pattern where ONE backspace deletes TWO characters"
echo
echo "=== Key Debug Messages to Watch ==="
echo
echo "Normal pattern (correct):"
echo "  [BACKSPACE_TRACE] START: cursor_pos=5, length=5"
echo "  [BUFFER_BACKSPACE] EXECUTING: cursor_pos=5->4, length=5->4"
echo "  [BACKSPACE_TRACE] AFTER_DELETE: cursor_pos=4, length=4 (deleted 1 chars)"
echo "  [BACKSPACE_TRACE] END: final cursor_pos=4, length=4"
echo
echo "PROBLEM pattern (double deletion):"
echo "  [BACKSPACE_TRACE] START: cursor_pos=3, length=3"
echo "  [BUFFER_BACKSPACE] EXECUTING: cursor_pos=3->2, length=3->2"
echo "  [BUFFER_BACKSPACE] EXECUTING: cursor_pos=2->1, length=2->1  ← SECOND CALL!"
echo "  [BACKSPACE_TRACE] AFTER_DELETE: cursor_pos=1, length=1 (deleted 2 chars)"
echo
echo "=== What to Report ==="
echo
echo "When you see the double deletion, note:"
echo "1. At what point it happens (crossing line boundary?)"
echo "2. The exact sequence of debug messages"
echo "3. How many characters were actually deleted vs expected"
echo "4. Whether BUFFER_BACKSPACE appears twice for one BACKSPACE_TRACE"
echo

echo "Press Enter to start tracing session..."
read

echo
echo "Starting LLE with buffer modification tracing..."
echo "Type the test command and backspace slowly while watching debug output!"
echo

# Enable debug mode and run with narrow terminal
export LLE_DEBUG=1
export COLUMNS=50  # Force line wrapping

./builddir/lusush

echo
echo "=== Analysis Questions ==="
echo
echo "1. Did you see any [BUFFER_BACKSPACE] messages appearing twice?"
echo "2. At what point did the double deletion occur?"
echo "3. What was the pattern: normal backspace or boundary crossing?"
echo "4. Did the 'deleted X chars' count ever show 2 instead of 1?"
echo "5. Were there any unexpected debug message sequences?"
echo

#!/bin/bash
# Diagnostic script to trace the off-by-one backspace issue
# This script will help identify where the cursor positioning goes wrong

set -e

echo "=== Backspace Off-by-One Diagnostic Tool ==="
echo
echo "This script will help trace the exact source of the cursor positioning issue"
echo "where backspace goes 'one space too far into the prompt boundary'."
echo

# Build with debug logging enabled
echo "Building LLE with debug logging..."
scripts/lle_build.sh build
echo

echo "=== Setting up diagnostic environment ==="
# Enable extensive debug logging
export LLE_DEBUG=1
export COLUMNS=60  # Narrow terminal to force line wrapping

echo "Debug environment:"
echo "- LLE_DEBUG=1 (enabled)"
echo "- COLUMNS=60 (narrow terminal for wrapping)"
echo

echo "=== Test Instructions ==="
echo
echo "1. The shell will start with debug logging enabled"
echo "2. Type this EXACT test command:"
echo "   echo this_is_a_test_command_that_will_wrap_to_second_line"
echo "3. The command should wrap to a second line"
echo "4. Press backspace slowly, one character at a time"
echo "5. Watch the debug output for:"
echo "   - DIAGNOSTIC messages showing cursor_pos and buffer_length"
echo "   - Footprint calculations for boundary detection"
echo "   - When boundary crossing is detected"
echo "   - Final cursor positioning"
echo "6. Continue until backspace stops working"
echo "7. Note the final cursor position vs where it should be"
echo

echo "=== What to Look For ==="
echo
echo "Key diagnostic patterns:"
echo "- '[LLE_CMD_BACKSPACE] DIAGNOSTIC: Starting backspace, cursor_pos=X'"
echo "- '[LLE_CMD_BACKSPACE] DIAGNOSTIC: After backspace, cursor_pos=X'"
echo "- '[LLE_INCREMENTAL] DIAGNOSTIC: buffer->cursor_pos=X, text_length=Y'"
echo "- '[LLE_INCREMENTAL] Crossing boundary: true'"
echo "- '[LLE_INCREMENTAL] DIAGNOSTIC: Final state: buffer_pos=X'"
echo
echo "Critical questions to answer:"
echo "1. Does cursor_pos ever go below 0 or become invalid?"
echo "2. Does the boundary detection trigger correctly?"
echo "3. What's the final cursor_pos when backspace stops?"
echo "4. Is the issue in buffer operations or display logic?"
echo

echo "=== Expected vs Actual Behavior ==="
echo
echo "EXPECTED:"
echo "- Cursor_pos decreases with each backspace: 5->4->3->2->1->0"
echo "- When cursor_pos=0, backspace command returns success but does nothing"
echo "- Final cursor position: right after prompt, ready for new input"
echo
echo "REPORTED ISSUE:"
echo "- Cursor ends up 'one space too far into the prompt boundary'"
echo "- Character artifacts left on first line"
echo "- Cursor appears to be ON the last character of prompt, not after it"
echo

echo "=== Starting Diagnostic Session ==="
echo "Press Enter to start..."
read

echo
echo "Starting LLE with diagnostic logging..."
echo "Remember: Type the test command and backspace slowly while watching debug output!"
echo

# Run the shell with debug logging
./builddir/lusush

echo
echo "=== Diagnostic Session Complete ==="
echo
echo "Analysis questions:"
echo "1. What was the final cursor_pos value in the debug output?"
echo "2. Did any DIAGNOSTIC messages show unexpected values?"
echo "3. When did boundary crossing detection trigger?"
echo "4. Were there any character artifacts in the debug buffer content?"
echo "5. Did the cursor_pos ever go negative or invalid?"
echo
echo "Please review the debug output above and report:"
echo "- The exact sequence of cursor_pos values"
echo "- Any unexpected DIAGNOSTIC messages"
echo "- Where the behavior diverged from expected"
echo

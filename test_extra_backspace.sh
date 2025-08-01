#!/bin/bash
# Focused test to track the extra backspace operation issue
# This test will help identify why one extra backspace is being allowed

set -e

echo "=== Extra Backspace Operation Diagnostic ==="
echo
echo "This test focuses on tracking the specific issue where one extra"
echo "backspace operation is allowed when it should be blocked."
echo

# Build first
echo "Building LLE..."
scripts/lle_build.sh build
echo

echo "=== Issue Description ==="
echo "- Behavior looks correct until the very end"
echo "- One extra backspace operation happens when it shouldn't"
echo "- Visual indication shows cursor should be at boundary"
echo "- But logic allows one more backspace through"
echo

echo "=== Diagnostic Strategy ==="
echo "Track the cursor_pos values and boundary checks:"
echo "1. Enable debug logging to see BACKSPACE_CHECK messages"
echo "2. Type a wrapping command and backspace slowly"
echo "3. Watch for the sequence:"
echo "   - [BACKSPACE_CHECK] cursor_pos=2, buffer_length=2"
echo "   - [BACKSPACE_CHECK] ALLOWING: proceeding with backspace"
echo "   - [BACKSPACE_CHECK] COMPLETED: new cursor_pos=1, new_length=1"
echo "   - [BACKSPACE_CHECK] cursor_pos=1, buffer_length=1"
echo "   - [BACKSPACE_CHECK] ALLOWING: proceeding with backspace"
echo "   - [BACKSPACE_CHECK] COMPLETED: new cursor_pos=0, new_length=0"
echo "   - [BACKSPACE_CHECK] cursor_pos=0, buffer_length=0"
echo "   - [BACKSPACE_CHECK] BLOCKED: cursor at boundary"
echo "4. Look for any extra ALLOWING message after the BLOCKED message"
echo

echo "=== Test Procedure ==="
echo "1. Run with debug: LLE_DEBUG=1 ./builddir/lusush"
echo "2. Type: echo test_wrap_command_that_goes_to_next_line"
echo "3. Press backspace repeatedly and count operations"
echo "4. Pay attention to final few BACKSPACE_CHECK messages"
echo "5. Note if there's an extra ALLOWING after expected BLOCKED"
echo

echo "=== What to Look For ==="
echo "❌ WRONG: Extra ALLOWING message after cursor_pos should be 0"
echo "❌ WRONG: cursor_pos going negative or wrapping around"
echo "❌ WRONG: BLOCKED message not appearing when expected"
echo "✅ CORRECT: Clean sequence ending with BLOCKED at cursor_pos=0"
echo

echo "=== Expected vs Actual ==="
echo "EXPECTED final sequence:"
echo "  [BACKSPACE_CHECK] cursor_pos=1, buffer_length=1"
echo "  [BACKSPACE_CHECK] ALLOWING: proceeding with backspace"
echo "  [BACKSPACE_CHECK] COMPLETED: new cursor_pos=0, new_length=0"
echo "  [BACKSPACE_CHECK] cursor_pos=0, buffer_length=0"
echo "  [BACKSPACE_CHECK] BLOCKED: cursor at boundary"
echo "  (no more backspace operations)"
echo
echo "ACTUAL (problematic):"
echo "  (same sequence as above, but then...)"
echo "  [BACKSPACE_CHECK] cursor_pos=?, buffer_length=?"
echo "  [BACKSPACE_CHECK] ALLOWING: proceeding with backspace (SHOULD NOT HAPPEN)"
echo

echo "Press Enter to start diagnostic session..."
read

echo
echo "Starting LLE with backspace operation tracking..."
echo "Watch the BACKSPACE_CHECK messages carefully!"
echo

# Set debug mode and run
export LLE_DEBUG=1
export COLUMNS=50  # Narrow for easier wrapping

./builddir/lusush

echo
echo "=== Analysis Questions ==="
echo "1. Did you see an extra ALLOWING message after the expected BLOCKED?"
echo "2. What was the cursor_pos value in that extra ALLOWING?"
echo "3. Did cursor_pos ever show unexpected values (negative, wrapped)?"
echo "4. Was there a gap between visual cursor position and cursor_pos value?"
echo "5. Did the BLOCKED message appear at the right time?"
echo

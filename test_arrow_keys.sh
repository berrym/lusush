#!/bin/bash

# Quick Arrow Key Test Script for Lusush LLE
# Tests if UP/DOWN arrow keys are properly detected and trigger history navigation

echo "=== LUSUSH ARROW KEY TEST ==="
echo ""
echo "This script will test arrow key detection in the Lusush shell."
echo "Follow these steps:"
echo ""
echo "1. First, create some history by typing commands:"
echo "   - echo \"first command\""
echo "   - echo \"second command\""
echo "   - ls -la"
echo "   - pwd"
echo ""
echo "2. Then test arrow key navigation:"
echo "   - Press UP arrow - should show 'pwd'"
echo "   - Press UP again - should show 'ls -la'"
echo "   - Press DOWN arrow - should go forward in history"
echo ""
echo "3. Look for these SUCCESS indicators:"
echo "   ✅ Clean line clearing (no artifacts)"
echo "   ✅ Proper history command display"
echo "   ✅ No character scattering or gaps"
echo ""
echo "4. Look for these FAILURE indicators:"
echo "   ❌ Character gaps like: e                    e"
echo "   ❌ Prompt duplication or cascading"
echo "   ❌ Terminal corruption"
echo "   ❌ Arrow keys not working at all"
echo ""

read -p "Press ENTER to start Lusush with arrow key debug logging..."

echo ""
echo "=== STARTING LUSUSH WITH DEBUG LOGGING ==="
echo "Debug output will be saved to /tmp/arrow_debug.log"
echo ""

# Start Lusush with debug mode
LLE_DEBUG=1 LLE_DEBUG_DISPLAY=1 ./builddir/lusush 2>/tmp/arrow_debug.log

echo ""
echo "=== TEST COMPLETE ==="
echo ""
echo "Check the results:"
echo "1. Did arrow keys work correctly?"
echo "2. Was history navigation clean?"
echo "3. Any display corruption?"
echo ""
echo "Debug log saved to: /tmp/arrow_debug.log"
echo "You can examine it with: cat /tmp/arrow_debug.log | grep -E '(ARROW|ESCAPE)'"
echo ""

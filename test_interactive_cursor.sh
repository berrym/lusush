#!/bin/bash

# Interactive Cursor Movement Test Script
# This script guides manual testing of Ctrl+A/E cursor movement in real TTY

echo "🎯 Interactive Cursor Movement Test"
echo "==================================="
echo ""
echo "This test MUST be run in a real terminal (TTY) to work properly."
echo "The line editor only operates in interactive mode."
echo ""

# Check if we're in a TTY
if [ ! -t 0 ]; then
    echo "❌ ERROR: This script must be run in a real terminal, not piped input."
    echo "   Run it directly: ./test_interactive_cursor.sh"
    exit 1
fi

echo "✅ Running in TTY - line editor will be active"
echo ""

# Build first
echo "Building lusush..."
if ! scripts/lle_build.sh build > /dev/null 2>&1; then
    echo "❌ Build failed!"
    exit 1
fi
echo "✅ Build successful"
echo ""

echo "📋 MANUAL TEST PROCEDURE:"
echo "========================"
echo ""
echo "1. I will start lusush in interactive mode"
echo "2. You will see a prompt: 'lusush> '"
echo "3. Follow these steps EXACTLY:"
echo ""
echo "   Step 1: Type 'hello world' (but don't press Enter yet)"
echo "   Step 2: Press Ctrl+A"
echo "           🎯 Expected: Cursor should VISUALLY jump to beginning of line"
echo "   Step 3: Press Ctrl+E"
echo "           🎯 Expected: Cursor should VISUALLY jump to end of line"
echo "   Step 4: Press Ctrl+U"
echo "           🎯 Expected: Line should clear completely"
echo "   Step 5: Type 'test successful' and press Enter"
echo "   Step 6: Type 'exit' and press Enter to quit"
echo ""
echo "🚨 WHAT TO WATCH FOR:"
echo "   ✅ SUCCESS: Cursor moves visually/immediately on Ctrl+A/E"
echo "   ❌ FAILURE: Cursor doesn't move visually (stays in same position)"
echo ""
echo "💡 TIP: If cursor doesn't move visually, the bug still exists"
echo ""

read -p "Ready to start the test? Press Enter to continue..."

echo ""
echo "🚀 Starting lusush in interactive mode..."
echo "   (Remember: Type 'hello world', test Ctrl+A/E, then 'exit')"
echo "=========================================================="

# Start lusush with debug enabled so we can see what's happening
LLE_DEBUG=1 ./builddir/lusush

echo ""
echo "🔍 Test completed. Results:"
echo "=========================="
echo ""
echo "Please report what you observed:"
echo ""
echo "1. Did the cursor move VISUALLY when you pressed Ctrl+A? (y/n)"
read -r ctrl_a_result

echo "2. Did the cursor move VISUALLY when you pressed Ctrl+E? (y/n)"
read -r ctrl_e_result

echo "3. Did the line clear immediately when you pressed Ctrl+U? (y/n)"
read -r ctrl_u_result

echo ""
echo "📊 TEST RESULTS SUMMARY:"
echo "========================"

if [[ "$ctrl_a_result" =~ ^[Yy]$ ]]; then
    echo "✅ Ctrl+A: WORKING - Visual cursor movement to beginning"
else
    echo "❌ Ctrl+A: BROKEN - No visual cursor movement"
fi

if [[ "$ctrl_e_result" =~ ^[Yy]$ ]]; then
    echo "✅ Ctrl+E: WORKING - Visual cursor movement to end"
else
    echo "❌ Ctrl+E: BROKEN - No visual cursor movement"
fi

if [[ "$ctrl_u_result" =~ ^[Yy]$ ]]; then
    echo "✅ Ctrl+U: WORKING - Line clearing"
else
    echo "❌ Ctrl+U: BROKEN - Line not clearing"
fi

echo ""

# Overall assessment
if [[ "$ctrl_a_result" =~ ^[Yy]$ && "$ctrl_e_result" =~ ^[Yy]$ && "$ctrl_u_result" =~ ^[Yy]$ ]]; then
    echo "🎉 OVERALL: SUCCESS! All keybindings are working with visual feedback."
    echo "   The cursor movement and line clearing fixes are complete."
    exit 0
elif [[ "$ctrl_u_result" =~ ^[Yy]$ ]] && [[ ! "$ctrl_a_result" =~ ^[Yy]$ || ! "$ctrl_e_result" =~ ^[Yy]$ ]]; then
    echo "🚧 PARTIAL SUCCESS: Line clearing works, but cursor movement needs more work."
    echo ""
    echo "🔧 NEXT STEPS:"
    echo "   1. The display system integration is working (Ctrl+U clears)"
    echo "   2. The cursor positioning logic needs debugging"
    echo "   3. Check cursor position calculations in display system"
    echo "   4. Verify terminal cursor movement commands are effective"
    exit 1
else
    echo "❌ OVERALL: FAILURE - Core keybinding functionality is broken."
    echo ""
    echo "🔧 DEBUGGING NEEDED:"
    echo "   1. Check if LLE_DEBUG output shows cursor operations"
    echo "   2. Verify display system APIs are being called"
    echo "   3. Check terminal manager cursor movement functions"
    echo "   4. Investigate timing or display conflicts"
    exit 1
fi

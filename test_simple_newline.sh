#!/bin/bash

# Simple test to isolate the newline positioning issue
# This test focuses on the core problem: after Enter key in line editor,
# the cursor should be properly positioned for command output

set -e

echo "=== SIMPLE NEWLINE POSITIONING TEST ==="
echo ""

# Build first
echo "Building..."
scripts/lle_build.sh build >/dev/null 2>&1
echo "✅ Build complete"
echo ""

echo "=== Test 1: Direct command execution (should work) ==="
echo "Running: echo 'direct command'"
./builddir/lusush -c "echo 'direct command'"
echo "✅ Direct execution works as expected"
echo ""

echo "=== Test 2: Single line interactive (should work) ==="
echo "This will test basic Enter key handling without wrapping"
echo "Instructions: Type 'echo hello' and press Enter, then type 'exit'"
echo "Expected: 'hello' appears on new line after Enter"
echo ""
echo "Press any key to start..."
read -n 1
echo ""
LLE_DEBUG=1 ./builddir/lusush 2>/tmp/simple_test.log
echo ""
echo "Debug output:"
grep -E "(Enter key|newline|Line completed)" /tmp/simple_test.log | tail -5
echo ""

echo "=== Test 3: The corruption scenario ==="
echo "This tests the exact scenario that causes corruption"
echo "Instructions:"
echo "  1. Type: echo \"this is a very long line that will wrap\""
echo "  2. Backspace several characters to go across the line wrap"
echo "  3. Type: ll wrap\" (to complete the command)"
echo "  4. Press Enter"
echo "  5. LOOK CAREFULLY: Does 'this is a very long line that will wrap' appear on a NEW line?"
echo "  6. Type 'exit'"
echo ""
echo "Expected result: The echo output should appear on a completely new line"
echo "Bug symptom: The output appears mixed with the prompt or on the wrong line"
echo ""
echo "Press any key to start corruption test..."
read -n 1
echo ""
LLE_DEBUG=1 LLE_INTEGRATION_DEBUG=1 ./builddir/lusush 2>/tmp/corruption_test.log
echo ""

echo "=== ANALYSIS ==="
echo ""
echo "Corruption test debug log saved to: /tmp/corruption_test.log"
echo ""
echo "Key things to check:"
echo "1. Did the echo output appear on a NEW line after pressing Enter?"
echo "2. Was there visual corruption mixing the output with the prompt?"
echo ""
echo "Debug log analysis:"
echo ""
echo "Enter key processing:"
grep -A 10 -B 2 "Enter key pressed" /tmp/corruption_test.log | tail -15
echo ""
echo "Newline and cursor operations:"
grep -E "(newline|Move cursor|terminal_write.*1)" /tmp/corruption_test.log | tail -10
echo ""
echo "Cross-line backspace operations:"
grep -A 3 -B 1 "Cross-line operation" /tmp/corruption_test.log | tail -10
echo ""

echo "=== DIAGNOSIS ==="
echo ""
echo "If the bug is fixed, you should see:"
echo "✅ Echo output appears on a clean new line"
echo "✅ No mixing of command output with prompt"
echo "✅ Debug log shows proper newline writing and cursor positioning"
echo ""
echo "If the bug persists, you will see:"
echo "❌ Echo output appears on the same line as the prompt"
echo "❌ Visual corruption mixing output with prompt text"
echo "❌ Missing or failed newline/cursor positioning in debug log"
echo ""
echo "Next steps if bug persists:"
echo "1. Examine /tmp/corruption_test.log for failed operations"
echo "2. Look for missing newline writes or cursor positioning failures"
echo "3. Focus on the Enter key handling sequence"
echo "4. Verify that stdout positioning is correct when shell takes over"

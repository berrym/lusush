#!/bin/bash

# Test script for the three specific backspace fixes
# Tests: empty buffer block, safer multiline clearing, prevent over-clearing

set -e

echo "=========================================="
echo "LLE Backspace Fixes Test"
echo "=========================================="

# Build first
echo "Building LLE..."
if ! scripts/lle_build.sh build; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build successful!"
echo ""

echo "=========================================="
echo "TEST CASE 1: Empty Buffer Backspace Block"
echo "=========================================="
echo "This test verifies that backspace is blocked when buffer is empty."
echo ""
echo "Instructions:"
echo "1. LLE will start"
echo "2. DON'T type anything - just press backspace immediately"
echo "3. You should see debug message: 'BLOCKED: Backspace at buffer start'"
echo "4. Backspace should have no effect"
echo "5. Type 'test' then press Enter to continue"
echo ""

read -p "Press Enter to start Test 1..."

echo "Running empty buffer backspace test..."
DEBUG_LOG1="/tmp/empty_buffer_test.log"

# This should show blocked backspace attempts
timeout 15s bash -c "LLE_DEBUG=1 ./builddir/lusush 2>$DEBUG_LOG1" || true

echo ""
echo "Analyzing Test 1 results..."
if grep -q "BLOCKED: Backspace at buffer start" "$DEBUG_LOG1"; then
    echo "✅ Test 1 PASSED: Empty buffer backspace correctly blocked"
else
    echo "❌ Test 1 FAILED: Empty buffer backspace not blocked"
fi
echo ""

echo "=========================================="
echo "TEST CASE 2: Safer Multiline Clearing"
echo "=========================================="
echo "This test verifies that multiline content clearing doesn't affect previous output."
echo ""
echo "Instructions:"
echo "1. Type exactly: echo Hello World Testing Long Content"
echo "2. Press backspace several times"
echo "3. Observe: No artifacts should remain on wrapped lines"
echo "4. Debug should show 'safe multiline clear' messages"
echo "5. Previous shell output should remain intact"
echo ""

read -p "Press Enter to start Test 2..."

echo "Running multiline clearing test..."
DEBUG_LOG2="/tmp/multiline_clear_test.log"

timeout 20s bash -c "LLE_DEBUG=1 ./builddir/lusush 2>$DEBUG_LOG2" || true

echo ""
echo "Analyzing Test 2 results..."

if grep -q "Content wraps, using safe multiline clear" "$DEBUG_LOG2"; then
    echo "✅ Test 2a PASSED: Safe multiline clearing detected"
else
    echo "❌ Test 2a FAILED: Safe multiline clearing not detected"
fi

if grep -q "SUCCESS: Cleared forward from content start" "$DEBUG_LOG2"; then
    echo "✅ Test 2b PASSED: Forward clearing used (safer method)"
else
    echo "❌ Test 2b FAILED: Forward clearing not used"
fi

# Check that we're NOT using the problematic move-up method
if ! grep -q "SUCCESS: Moved up one line" "$DEBUG_LOG2"; then
    echo "✅ Test 2c PASSED: No longer using problematic move-up method"
else
    echo "❌ Test 2c FAILED: Still using problematic move-up method"
fi

echo ""

echo "=========================================="
echo "TEST CASE 3: Prevent Over-Clearing"
echo "=========================================="
echo "This test verifies that content clearing doesn't consume previous shell output."
echo ""
echo "Instructions:"
echo "1. Type: ls (press Enter to execute and see output)"
echo "2. Then type: echo Hello Again"
echo "3. Press backspace multiple times"
echo "4. The 'ls' command output should remain visible above"
echo "5. No previous shell content should be affected"
echo ""

read -p "Press Enter to start Test 3..."

echo "Running over-clearing prevention test..."
DEBUG_LOG3="/tmp/over_clear_test.log"

timeout 25s bash -c "LLE_DEBUG=1 ./builddir/lusush 2>$DEBUG_LOG3" || true

echo ""
echo "Analyzing Test 3 results..."

backspace_operations=$(grep -c "VISUAL_DEBUG.*BACKSPACE:" "$DEBUG_LOG3" 2>/dev/null || echo "0")
echo "Backspace operations detected: $backspace_operations"

if [ "$backspace_operations" -gt 0 ]; then
    echo "✅ Test 3a PASSED: Backspace operations occurred"

    if grep -q "Cleared forward from content start" "$DEBUG_LOG3"; then
        echo "✅ Test 3b PASSED: Using forward clearing (safe method)"
    else
        echo "❌ Test 3b FAILED: Not using forward clearing"
    fi
else
    echo "ℹ️  Test 3: No backspace operations detected (may need manual testing)"
fi

echo ""

echo "=========================================="
echo "OVERALL TEST SUMMARY"
echo "=========================================="

echo "Debug logs generated:"
echo "  - Test 1 (Empty buffer): $DEBUG_LOG1"
echo "  - Test 2 (Multiline clear): $DEBUG_LOG2"
echo "  - Test 3 (Over-clearing): $DEBUG_LOG3"
echo ""

echo "Key improvements tested:"
echo "1. ✓ Empty buffer backspace blocking"
echo "2. ✓ Safer multiline clearing (no move-up)"
echo "3. ✓ Forward clearing only (prevents over-clearing)"
echo ""

echo "Expected visual behavior:"
echo "✅ Backspace blocked when no content"
echo "✅ Wrapped content clears cleanly"
echo "✅ Previous shell output preserved"
echo "✅ No artifacts left on wrapped lines"
echo ""

echo "To manually verify all fixes work:"
echo "1. Run: LLE_DEBUG=1 ./builddir/lusush"
echo "2. Try backspace on empty prompt (should be blocked)"
echo "3. Type long wrapped content and backspace (should clear cleanly)"
echo "4. Check that previous commands remain visible above"
echo ""

echo "Analysis commands:"
echo "  grep 'BLOCKED\\|safe multiline\\|Cleared forward' $DEBUG_LOG1 $DEBUG_LOG2 $DEBUG_LOG3"
echo "  grep 'VISUAL_DEBUG' $DEBUG_LOG2 | head -20"
echo ""

echo "Test completed! Check visual behavior during interactive sessions."

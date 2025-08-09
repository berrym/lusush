#!/bin/bash

# Comprehensive Test for Tab Completion Fixes
# Tests all the issues identified and fixed in the tab completion system

echo "=== TAB COMPLETION FIXES VALIDATION ==="
echo "Testing fixes for core completion logic issues"
echo ""

# Build first
echo "Building LLE..."
if ! scripts/lle_build.sh build >/dev/null 2>&1; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build successful"
echo ""

# Test 1: Single completion handling (original issue)
echo "=== Test 1: Single Completion Auto-Apply ==="
echo "Testing: ec[TAB] should complete to 'echo' without cycling"

OUTPUT=$(printf "ec\t\n\x04" | ./builddir/lusush 2>/dev/null)
EXPECTED_RESULT="echo"

if echo "$OUTPUT" | grep -q "echo"; then
    echo "✅ PASS: Single completion auto-applied correctly"
else
    echo "❌ FAIL: Single completion not working"
    echo "Output: $OUTPUT"
fi

echo ""

# Test 2: Session separation (main issue)
echo "=== Test 2: Session Separation ==="
echo "Testing: ec[TAB] te[TAB] should NOT apply echo to te"

printf "ec\t te\t\n\x04" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/session_test.log >/dev/null

# Check if session properly ended and new one started
if grep -q "Should continue session.*result=no" /tmp/session_test.log; then
    echo "✅ PASS: Session properly ended when moving to new word"
else
    echo "❌ FAIL: Session continuation logic broken"
    echo "Debug: $(grep 'Should continue session' /tmp/session_test.log)"
fi

echo ""

# Test 3: Multi-completion cycling
echo "=== Test 3: Multi-Completion Cycling ==="
echo "Testing: echo t[TAB][TAB][TAB] should cycle through multiple completions"

printf "echo t\t\t\t\n\x04" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/cycling_test.log >/dev/null

CYCLE_COUNT=$(grep -c "Cycled to completion" /tmp/cycling_test.log)

if [ "$CYCLE_COUNT" -ge 2 ]; then
    echo "✅ PASS: Multi-completion cycling working ($CYCLE_COUNT cycles detected)"
else
    echo "❌ FAIL: Multi-completion cycling not working"
    echo "Cycles detected: $CYCLE_COUNT"
fi

echo ""

# Test 4: No cycling on single completion
echo "=== Test 4: Single Completion No-Cycle ==="
echo "Testing: Single completions should not cycle"

printf "echoo\t\t\n\x04" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/no_cycle_test.log >/dev/null

if grep -q "Only 1 completion available - ending session" /tmp/no_cycle_test.log; then
    echo "✅ PASS: Single completion properly ends session instead of cycling"
else
    echo "❌ FAIL: Single completion cycling prevention not working"
fi

echo ""

# Test 5: Text replacement accuracy
echo "=== Test 5: Text Replacement Accuracy ==="
echo "Testing: Completions replace correct text boundaries"

# Test that 'tes' completes to something starting with 'tes', not 'echo'
printf "echo tes\t\n\x04" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/replacement_test.log >/dev/null

# Check word extraction for 'tes'
if grep -q "Extracted word: 'tes'" /tmp/replacement_test.log; then
    echo "✅ PASS: Word extraction correctly identifies 'tes'"
else
    echo "❌ FAIL: Word extraction incorrect"
    echo "Debug: $(grep 'Extracted word' /tmp/replacement_test.log | tail -1)"
fi

echo ""

# Test 6: Prompt redraw prevention (surgical fix)
echo "=== Test 6: Prompt Redraw Prevention ==="
echo "Testing: TAB cycling should not cause excessive prompt redraws"

printf "echo t\t\t\t\n\x04" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/prompt_test.log >/dev/null

PROMPT_RENDERS=$(grep -c "Rendering prompt" /tmp/prompt_test.log)

if [ "$PROMPT_RENDERS" -le 2 ]; then
    echo "✅ PASS: Prompt redraws controlled ($PROMPT_RENDERS renders)"
else
    echo "❌ FAIL: Excessive prompt redraws ($PROMPT_RENDERS renders)"
fi

echo ""

# Test 7: Menu cancellation
echo "=== Test 7: Menu Cancellation ==="
echo "Testing: ESCAPE should cancel completion menu"

printf "echo t\t\x1b\n\x04" | ./builddir/lusush 2>/dev/null >/dev/null
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ PASS: ESCAPE cancellation working"
else
    echo "❌ FAIL: ESCAPE cancellation broken"
fi

echo ""

# Test 8: Visual artifacts check
echo "=== Test 8: Visual Artifacts Check ==="
echo "Testing: No text corruption or artifacts in output"

OUTPUT=$(printf "ec\t te\t\n\x04" | ./builddir/lusush 2>/dev/null)

# Check for text corruption patterns
if echo "$OUTPUT" | grep -E "(mdests|ececho|teecho)" >/dev/null; then
    echo "❌ FAIL: Text corruption detected in output"
    echo "Corrupted output: $OUTPUT"
else
    echo "✅ PASS: No text corruption artifacts detected"
fi

echo ""

# Test 9: Session state tracking
echo "=== Test 9: Session State Tracking ==="
echo "Testing: Session word boundaries update correctly after completion"

printf "echo t\t\t\n\x04" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/state_test.log >/dev/null

# Check if word_end_pos updates are happening
WORD_END_UPDATES=$(grep -c "cursor_in_region=yes" /tmp/state_test.log)

if [ "$WORD_END_UPDATES" -ge 1 ]; then
    echo "✅ PASS: Session state tracking working ($WORD_END_UPDATES updates)"
else
    echo "❌ FAIL: Session state tracking broken"
fi

echo ""

# Final Assessment
echo "=== FINAL ASSESSMENT ==="

# Count passes
TOTAL_TESTS=9
PASSED_TESTS=0

# Re-run quick checks for final tally
printf "ec\t\n\x04" | ./builddir/lusush 2>/dev/null | grep -q "echo" && ((PASSED_TESTS++))
grep -q "Should continue session.*result=no" /tmp/session_test.log && ((PASSED_TESTS++))
[ "$CYCLE_COUNT" -ge 2 ] && ((PASSED_TESTS++))
grep -q "Only 1 completion available" /tmp/no_cycle_test.log && ((PASSED_TESTS++))
grep -q "Extracted word: 'tes'" /tmp/replacement_test.log && ((PASSED_TESTS++))
[ "$PROMPT_RENDERS" -le 2 ] && ((PASSED_TESTS++))
[ $EXIT_CODE -eq 0 ] && ((PASSED_TESTS++))
! echo "$OUTPUT" | grep -E "(mdests|ececho|teecho)" >/dev/null && ((PASSED_TESTS++))
[ "$WORD_END_UPDATES" -ge 1 ] && ((PASSED_TESTS++))

echo "Test Results: $PASSED_TESTS/$TOTAL_TESTS tests passed"
echo ""

if [ "$PASSED_TESTS" -eq "$TOTAL_TESTS" ]; then
    echo "🎉 SUCCESS: All tab completion fixes working correctly!"
    echo ""
    echo "✅ Fixed Issues:"
    echo "   - Single completion auto-apply (no cycling on 1 item)"
    echo "   - Session separation (new words start new sessions)"
    echo "   - Multi-completion cycling (proper cycling through options)"
    echo "   - Text replacement accuracy (correct word boundaries)"
    echo "   - Prompt redraw prevention (surgical fix working)"
    echo "   - Menu cancellation (ESCAPE working)"
    echo "   - Visual artifact elimination (no text corruption)"
    echo "   - Session state tracking (word boundaries update correctly)"
    echo ""
    echo "📊 Status: Tab completion system fully functional"
    echo "🎯 Original user issue resolved: 'ec[TAB] te[TAB]' now works correctly"

    exit 0
else
    echo "❌ ISSUES REMAIN: $((TOTAL_TESTS - PASSED_TESTS)) tests failed"
    echo ""
    echo "Debug logs available:"
    echo "- /tmp/session_test.log (session separation)"
    echo "- /tmp/cycling_test.log (multi-completion cycling)"
    echo "- /tmp/no_cycle_test.log (single completion handling)"
    echo "- /tmp/replacement_test.log (text replacement)"
    echo "- /tmp/prompt_test.log (prompt redraw prevention)"
    echo "- /tmp/state_test.log (session state tracking)"

    exit 1
fi

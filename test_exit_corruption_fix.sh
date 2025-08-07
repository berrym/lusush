#!/bin/bash

# Test for exit command corruption fix
# This test verifies that the 'exit' command no longer causes display corruption
# when used after multiline line editor operations

set -e

echo "=== EXIT COMMAND CORRUPTION FIX TEST ==="
echo "Testing that 'exit' command doesn't corrupt display after line editor use"
echo ""

# Build latest changes
echo "Building latest fixes..."
if ! scripts/lle_build.sh build >/dev/null 2>&1; then
    echo "❌ Build failed"
    exit 1
fi
echo "✅ Build successful"
echo ""

echo "=== Test 1: Simple exit command (baseline) ==="
echo "Testing basic exit command without line editor complexity"
echo ""

# Test simple exit
echo "Running: echo 'hello' → exit"
printf "echo 'hello'\nexit\n" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/simple_exit_test.log
echo ""
echo "Simple exit test output above ^^^^"
echo "Expected: 'hello' appears normally, no corruption on exit"
echo ""

echo "=== Test 2: Exit after multiline operations (corruption scenario) ==="
echo "Testing exit command after the exact multiline scenario that caused corruption"
echo ""

# Create input sequence that reproduces the original issue:
# 1. Long line that wraps
# 2. Backspace across line boundary
# 3. Complete command
# 4. Press Enter (this was fixed in previous step)
# 5. Use 'exit' command (this is what we're testing now)

echo "Simulating exact user scenario:"
echo "1. echo \"this is a very long line that will wrap\""
echo "2. Backspace 8 times across line boundary"
echo "3. Type 'll wrap\"' to complete"
echo "4. Press Enter"
echo "5. Type 'exit' command ← TESTING THIS STEP"
echo ""

# Create exact input sequence
TEST_SEQUENCE="echo \"this is a very long line that will wrap\""
TEST_SEQUENCE="${TEST_SEQUENCE}"$'\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f'  # 8 backspaces
TEST_SEQUENCE="${TEST_SEQUENCE}ll wrap\""
TEST_SEQUENCE="${TEST_SEQUENCE}"$'\r'           # Enter key
TEST_SEQUENCE="${TEST_SEQUENCE}exit"            # Exit command
TEST_SEQUENCE="${TEST_SEQUENCE}"$'\r'           # Enter for exit

echo "Running multiline corruption scenario test..."
echo -n "$TEST_SEQUENCE" | LLE_DEBUG=1 LLE_INTEGRATION_DEBUG=1 ./builddir/lusush 2>/tmp/exit_corruption_test.log

echo ""
echo "=== RESULTS ANALYSIS ==="
echo ""
echo "Multiline exit test output above ^^^^"
echo ""

# Analyze what happened
echo "Checking for corruption indicators..."
echo ""

echo "1. Exit command execution:"
if grep -q "bin_exit" /tmp/exit_corruption_test.log 2>/dev/null; then
    echo "✅ Exit command executed"
else
    echo "⚠️  Exit command execution not detected in logs"
fi

echo ""
echo "2. Terminal cleanup operations:"
if grep -q "lle_termcap_cleanup\|termcap_cleanup" /tmp/exit_corruption_test.log 2>/dev/null; then
    echo "✅ Terminal cleanup executed"
    if grep -q "lle_termcap_cleanup" /tmp/exit_corruption_test.log 2>/dev/null; then
        echo "✅ Using LLE termcap cleanup (CORRECT)"
    else
        echo "⚠️  Using old termcap cleanup (potential corruption source)"
    fi
else
    echo "⚠️  Terminal cleanup not detected in logs"
fi

echo ""
echo "3. State synchronization during exit:"
grep -E "(Force sync|state.*failed|Moving cursor.*termcap)" /tmp/exit_corruption_test.log 2>/dev/null | tail -5 || echo "   No state sync operations during exit"

echo ""
echo "4. Cross-line operations (should show fix):"
if grep -q "SKIP.*No clear needed.*line end" /tmp/exit_corruption_test.log 2>/dev/null; then
    echo "✅ CORRUPTION FIX ACTIVE: Unnecessary clear-to-EOL skipped at line end"
else
    echo "ℹ️  Clear-to-EOL skip not triggered (may not have reached scenario)"
fi

echo ""
echo "=== VISUAL CORRUPTION ASSESSMENT ==="
echo ""
echo "To verify the exit corruption is fixed, check the output above:"
echo ""
echo "EXPECTED (bug fixed):"
echo "  [prompt] echo \"this is a very long line that will wrap\""
echo "  this is a very long line that will wrap"
echo "  [prompt] exit"
echo "  [clean shell exit, no corruption]"
echo ""
echo "BUG SYMPTOM (not fixed):"
echo "  [garbled text mixing prompts and output]"
echo "  [corrupted terminal state after exit]"
echo "  [visual artifacts or wrong positioning]"
echo ""

# Compare the two tests
echo "=== COMPARISON ==="
echo ""
echo "Simple exit test debug summary:"
grep -c "Enter key\|Force sync\|termcap_cleanup" /tmp/simple_exit_test.log 2>/dev/null || echo "0 debug events"

echo ""
echo "Multiline exit test debug summary:"
grep -c "Enter key\|Force sync\|Cross-line\|termcap_cleanup" /tmp/exit_corruption_test.log 2>/dev/null || echo "0 debug events"

echo ""
echo "=== DETAILED DEBUG LOGS ==="
echo "Simple exit test: /tmp/simple_exit_test.log"
echo "Multiline exit test: /tmp/exit_corruption_test.log"
echo ""
echo "To examine specific issues:"
echo "  grep -A 5 -B 5 'exit' /tmp/exit_corruption_test.log"
echo "  grep -A 3 -B 3 'cleanup' /tmp/exit_corruption_test.log"
echo "  grep 'corruption\\|failed\\|error' /tmp/exit_corruption_test.log"
echo ""

echo "=== SUMMARY ==="
echo ""
echo "FIXES APPLIED:"
echo "1. ✅ Cross-line backspace: Skip clear-to-EOL when at line end"
echo "2. ✅ Enter key handling: Proper cursor positioning calculations"
echo "3. ✅ State-synchronized cleanup: Use LLE termcap instead of old termcap"
echo "4. ✅ Cursor positioning: Use termcap functions with manual state updates"
echo ""
echo "If corruption still occurs with 'exit' command:"
echo "- Check debug logs for failed operations"
echo "- Verify LLE termcap cleanup is being called"
echo "- Look for conflicts between old and new termcap systems"
echo "- Consider terminal state conflicts during cleanup"
echo ""
echo "EXIT CORRUPTION FIX TEST COMPLETE"

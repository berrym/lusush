#!/bin/bash

# Definitive test for down arrow buffer clearing in lusush
# This test verifies that after down arrow clears a history entry,
# pressing ENTER executes an empty command (does nothing) rather than the previous command

set -e

LUSUSH_BINARY="./builddir/lusush"
HISTORY_FILE="$HOME/.lusush_history"
TEST_LOG="/tmp/lusush_clear_test.log"

echo "=== DEFINITIVE DOWN ARROW CLEAR TEST ==="
echo ""

# Check if lusush binary exists
if [ ! -f "$LUSUSH_BINARY" ]; then
    echo "ERROR: Lusush binary not found at $LUSUSH_BINARY"
    echo "Please run: meson setup builddir --wipe && ninja -C builddir"
    exit 1
fi

# Clean up previous test files
rm -f "$TEST_LOG"

# Setup test history with a distinctive command that we can detect
echo "Setting up test history..."
rm -f "$HISTORY_FILE"
cat > "$HISTORY_FILE" << 'EOF'
echo "SHOULD_NOT_EXECUTE"
ls /nonexistent
exit
EOF

echo "Test history created:"
cat "$HISTORY_FILE" | nl
echo ""

echo "=== AUTOMATED TEST ==="
echo ""
echo "Testing the sequence: UP (show exit) → DOWN (clear) → ENTER (should be empty)"
echo ""

# Critical test:
# 1. Up arrow to show "exit"
# 2. Down arrow to clear it
# 3. ENTER - if buffer is truly cleared, this should do nothing
# 4. Then type "echo SUCCESS" to confirm shell is still running
# 5. Exit normally

# Use escape sequences for actual arrow keys
# \033[A = Up arrow
# \033[B = Down arrow
test_commands=$(cat << 'EOF'

echo "SUCCESS - buffer was cleared properly"
exit
EOF
)

# The key test: up arrow, down arrow, enter, then our success command
echo "Running test sequence..."
output=$(printf '\033[A\033[B\n%s\n' "$test_commands" | timeout 10s "$LUSUSH_BINARY" -i 2>&1)

echo "=== TEST RESULTS ==="
echo ""
echo "Full output:"
echo "$output"
echo ""

# Analysis
echo "=== ANALYSIS ==="
echo ""

# Check if shell exited immediately (bad - means "exit" was executed)
if echo "$output" | grep -q "SUCCESS - buffer was cleared properly"; then
    echo "✅ PASS: Shell remained running after ENTER - buffer was cleared!"
    echo "   The down arrow successfully cleared both buffer and display."
else
    echo "❌ FAIL: Shell may have exited immediately - buffer not cleared"
    echo "   The ENTER command may have executed 'exit' instead of empty command."
fi

# Check for the success message
if echo "$output" | grep -q "SUCCESS"; then
    echo "✅ PASS: Success message appeared - shell continued running"
else
    echo "❌ FAIL: Success message missing - shell may have terminated early"
fi

# Check if unwanted commands were executed
if echo "$output" | grep -q "SHOULD_NOT_EXECUTE"; then
    echo "❌ FAIL: Unwanted history command was executed"
else
    echo "✅ PASS: No unwanted history commands executed"
fi

# Count number of prompts to detect display issues
prompt_count=$(echo "$output" | grep -c '\$' || true)
echo ""
echo "Prompt count: $prompt_count"
if [ "$prompt_count" -gt 10 ]; then
    echo "⚠️  WARNING: Many prompts detected - possible display redraw issues"
elif [ "$prompt_count" -ge 3 ]; then
    echo "✅ ACCEPTABLE: Normal number of prompts for this test sequence"
else
    echo "⚠️  WARNING: Too few prompts - test may not have run completely"
fi

echo ""
echo "=== MANUAL VERIFICATION INSTRUCTIONS ==="
echo ""
echo "For definitive verification, manually run:"
echo "  $LUSUSH_BINARY -i"
echo ""
echo "Then test this exact sequence:"
echo "1. Press UP arrow     → should show 'exit'"
echo "2. Press DOWN arrow   → 'exit' should disappear from screen"
echo "3. Press ENTER        → should do nothing (new empty prompt)"
echo "4. Type 'hello'       → should execute normally"
echo "5. Type 'exit'        → should exit shell"
echo ""
echo "If step 3 exits the shell instead of creating a new prompt,"
echo "then the buffer clearing is not working correctly."
echo ""

# Final verdict
echo "=== FINAL VERDICT ==="
if echo "$output" | grep -q "SUCCESS" && ! echo "$output" | grep -q "SHOULD_NOT_EXECUTE"; then
    echo "🎉 TEST PASSED: Down arrow buffer clearing appears to be working!"
    exit 0
else
    echo "❌ TEST FAILED: Buffer clearing needs further investigation"
    exit 1
fi

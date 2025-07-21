#!/bin/bash

# Focused test to reproduce and debug line consumption issues
# Tests both default theme wrapped lines and dark theme line consumption

echo "=========================================="
echo "LINE CONSUMPTION DEBUG TEST"
echo "=========================================="
echo ""

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ Error: ./builddir/lusush not found. Run 'ninja' in builddir first."
    exit 1
fi

echo "✅ Binary found: ./builddir/lusush"
echo ""

echo "ISSUE REPRODUCTION:"
echo "=================="
echo ""

echo "This test reproduces the exact line consumption issues:"
echo ""
echo "1. DEFAULT THEME:"
echo "   - Line-wrapped history recall consumes variable lines"
echo "   - Number of consumed lines = number of lines the recalled command spans"
echo ""
echo "2. DARK THEME:"
echo "   - History recall always consumes previous lines"
echo "   - Happens regardless of command length"
echo ""

echo "AUTOMATED REPRODUCTION TESTS:"
echo "============================="
echo ""

# Test 1: Default theme line consumption with wrapped commands
echo "1. Testing default theme line consumption..."

cat > /tmp/test_default_consumption.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn ./builddir/lusush -i
expect "lusush"

# Ensure default theme
send "theme set original\r"
expect "lusush"

# Add some context lines to see consumption
send "echo 'Line 1 - this should stay visible'\r"
expect "lusush"
send "echo 'Line 2 - this should stay visible'\r"
expect "lusush"
send "echo 'Line 3 - this should stay visible'\r"
expect "lusush"

# Add a long command that will wrap across multiple lines
send "echo 'This is an extremely long command that will definitely wrap across multiple terminal lines and cause line consumption when recalled from history'\r"
expect "lusush"

# Add one more line to see what gets consumed
send "echo 'Line after long command - watch for consumption'\r"
expect "lusush"

puts "\n=== TESTING UP ARROW - WATCH FOR LINE CONSUMPTION ==="

# Recall the long command - this should consume lines equal to command length
send "\033\[A"
sleep 2

puts "Look above - did it consume the 'Line after long command' line?"

# Press DOWN to see original state
send "\033\[B"
sleep 1

puts "Now look - is the line restored or permanently consumed?"

send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_default_consumption.exp
    echo "Running default theme test (look for line consumption)..."
    /tmp/test_default_consumption.exp
    rm -f /tmp/test_default_consumption.exp
    echo ""
else
    echo "⚠️ Expect not available"
fi

echo ""

# Test 2: Dark theme line consumption
echo "2. Testing dark theme line consumption..."

cat > /tmp/test_dark_consumption.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn ./builddir/lusush -i
expect "lusush"

# Set dark theme
send "theme set dark\r"
expect "lusush"

# Add context lines to see consumption
send "echo 'Context line 1 - should stay visible'\r"
expect "lusush"
send "echo 'Context line 2 - should stay visible'\r"
expect "lusush"

# Add some commands to history
send "echo 'short'\r"
expect "lusush"
send "echo 'medium length command'\r"
expect "lusush"

# Add one more line to see consumption
send "echo 'Line after commands - watch for consumption'\r"
expect "lusush"

puts "\n=== TESTING UP ARROW - WATCH FOR LINE CONSUMPTION ==="

# Recall commands - should ANY of these consume previous lines?
send "\033\[A"
sleep 2
puts "Did recalling 'Line after commands' consume previous lines?"

send "\033\[A"
sleep 2
puts "Did recalling 'medium length command' consume previous lines?"

send "\033\[A"
sleep 2
puts "Did recalling 'short' consume previous lines?"

# Navigate back down
send "\033\[B\033\[B\033\[B"
sleep 1

send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_dark_consumption.exp
    echo "Running dark theme test (look for line consumption)..."
    /tmp/test_dark_consumption.exp
    rm -f /tmp/test_dark_consumption.exp
    echo ""
else
    echo "⚠️ Expect not available"
fi

echo ""

echo "MANUAL VERIFICATION STEPS:"
echo "=========================="
echo ""

echo "🔍 MANUAL TEST 1 - DEFAULT THEME LINE CONSUMPTION:"
echo "---------------------------------------------------"
echo "1. ./builddir/lusush -i"
echo "2. theme set original"
echo "3. Type: echo 'visible line 1'"
echo "4. Type: echo 'visible line 2'"
echo "5. Type: echo 'This is a very long command that wraps across multiple terminal lines'"
echo "6. Type: echo 'line after long command'"
echo "7. Press UP arrow to recall 'line after long command'"
echo "8. ❌ BUG: Previous lines disappear"
echo "9. Press UP again to recall the long command"
echo "10. ❌ BUG: Even more previous lines disappear"
echo ""

echo "🔍 MANUAL TEST 2 - DARK THEME LINE CONSUMPTION:"
echo "------------------------------------------------"
echo "1. ./builddir/lusush -i"
echo "2. theme set dark"
echo "3. Type: echo 'context line 1'"
echo "4. Type: echo 'context line 2'"
echo "5. Type: echo 'short'"
echo "6. Type: echo 'after short'"
echo "7. Press UP arrow to recall 'after short'"
echo "8. ❌ BUG: Previous lines disappear"
echo "9. Press UP again to recall 'short'"
echo "10. ❌ BUG: More previous lines disappear"
echo ""

echo "TECHNICAL ANALYSIS:"
echo "=================="
echo ""

echo "The line consumption happens because:"
echo ""
echo "1. DEFAULT THEME (single-line mode):"
echo "   - When recalling wrapped commands, the clearing logic"
echo "   - tries to clear multiple lines to handle the wrap"
echo "   - but clears too many, consuming previous terminal content"
echo ""
echo "2. DARK THEME (multiline mode):"
echo "   - The multiline clearing logic is too aggressive"
echo "   - It clears more lines than the actual prompt occupies"
echo "   - This consumes previous terminal output"
echo ""

echo "ROOT CAUSE:"
echo "==========="
echo ""
echo "The issue is in the refresh logic that tries to clear 'old' content"
echo "but doesn't distinguish between:"
echo "- Content that belongs to the current prompt/command (should be cleared)"
echo "- Previous terminal output (should NOT be cleared)"
echo ""
echo "The clearing logic is 'reaching back' and erasing lines that"
echo "were never part of the current prompt session."
echo ""

echo "INVESTIGATION NEEDED:"
echo "===================="
echo ""
echo "1. Check refreshMultiLine() clearing logic"
echo "2. Check refreshSingleLine() wrapped content handling"
echo "3. Verify oldrows calculation and usage"
echo "4. Ensure clearing only affects prompt-owned lines"
echo ""

echo "=========================================="
echo "Run manual tests above to see line consumption"
echo "The fix needs to prevent clearing non-prompt lines"
echo "=========================================="

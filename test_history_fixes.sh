#!/bin/bash

# Focused test for theme-specific history navigation issues
# Tests the specific fixes for prompt stacking and wrapped line handling

echo "=========================================="
echo "HISTORY NAVIGATION FIXES TEST"
echo "=========================================="
echo ""

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ Error: ./builddir/lusush not found. Run 'ninja' in builddir first."
    exit 1
fi

echo "✅ Binary found: ./builddir/lusush"
echo ""

# Clean debug log
rm -f /tmp/lndebug.txt
echo "🧹 Cleaned debug log"
echo ""

echo "AUTOMATED TESTING:"
echo "=================="

# Test 1: Original theme wrapped lines
echo "1. Testing original theme with wrapped lines..."

cat > /tmp/test_original_theme.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn ./builddir/lusush -i
expect "lusush"

# Make sure we're in original theme (default)
send "theme set original\r"
expect "lusush"

# Type a very long command that will wrap
send "echo 'This is an extremely long command that should definitely wrap across multiple lines in most terminal windows and will test the wrapped line history recall behavior'\r"
expect "lusush"

# Test history recall
puts "\n=== Testing UP arrow with wrapped content ==="
send "\033\[A"
sleep 2

# Look for the command in the buffer
expect {
    "This is an extremely long command" {
        puts "✅ Command recalled successfully"
    }
    timeout {
        puts "❌ Command recall failed"
    }
}

# Test navigation
send "\033\[B\033\[A"
sleep 1

puts "✅ Original theme navigation test complete"

send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_original_theme.exp
    /tmp/test_original_theme.exp
    rm -f /tmp/test_original_theme.exp
else
    echo "   ⚠️  Expect not available - manual testing required"
fi

echo ""

# Test 2: Dark theme multiline prompts
echo "2. Testing dark theme multiline prompts..."

cat > /tmp/test_dark_theme.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn ./builddir/lusush -i
expect "lusush"

# Switch to dark multiline theme
send "theme set dark\r"
expect "lusush"

# Add commands to history
send "echo 'first command'\r"
expect "lusush"

send "echo 'second command with more text'\r"
expect "lusush"

send "echo 'third command for testing'\r"
expect "lusush"

puts "\n=== Testing multiline theme navigation ==="

# Test UP navigation
send "\033\[A"
sleep 1
expect {
    "third command for testing" {
        puts "✅ First UP navigation works"
    }
    timeout {
        puts "❌ First UP navigation failed"
    }
}

# Test more navigation
send "\033\[A"
sleep 1
expect {
    "second command with more text" {
        puts "✅ Second UP navigation works"
    }
    timeout {
        puts "❌ Second UP navigation failed"
    }
}

# Test DOWN navigation
send "\033\[B"
sleep 1
expect {
    "third command for testing" {
        puts "✅ DOWN navigation works"
    }
    timeout {
        puts "❌ DOWN navigation failed"
    }
}

# Test rapid navigation
puts "\n=== Testing rapid navigation ==="
send "\033\[A\033\[A\033\[B\033\[B\033\[A"
sleep 2

puts "✅ Dark theme navigation test complete"

send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_dark_theme.exp
    /tmp/test_dark_theme.exp
    rm -f /tmp/test_dark_theme.exp
else
    echo "   ⚠️  Expect not available - manual testing required"
fi

echo ""

# Check debug output
echo "DEBUG OUTPUT ANALYSIS:"
echo "======================"

if [ -f "/tmp/lndebug.txt" ]; then
    DEBUG_SIZE=$(wc -l /tmp/lndebug.txt | awk '{print $1}')
    echo "📊 Debug log entries: $DEBUG_SIZE"

    if [ $DEBUG_SIZE -gt 0 ]; then
        echo "✅ Debug logging active"
        echo ""
        echo "Recent clear operations:"
        grep "clear" /tmp/lndebug.txt | tail -10 | while read line; do
            echo "   $line"
        done

        CLEAR_COUNT=$(grep -c "clear" /tmp/lndebug.txt)
        echo ""
        echo "Total clear operations: $CLEAR_COUNT"
    else
        echo "⚠️  No debug entries"
    fi
else
    echo "⚠️  No debug log found"
fi

echo ""

echo "MANUAL TESTING INSTRUCTIONS:"
echo "============================"
echo ""
echo "To manually verify the fixes:"
echo ""
echo "🔍 TEST ORIGINAL THEME WRAPPED LINES:"
echo "------------------------------------"
echo "1. ./builddir/lusush -i"
echo "2. theme set original"
echo "3. Type: echo 'Very long command that wraps across multiple terminal lines for testing purposes'"
echo "4. Press UP arrow"
echo "5. ✅ EXPECTED: Clean replacement, no new prompt creation"
echo "6. ❌ BUG: Multiple prompts or stacked content"
echo ""

echo "🔍 TEST DARK THEME MULTILINE PROMPTS:"
echo "------------------------------------"
echo "1. ./builddir/lusush -i"
echo "2. theme set dark"
echo "3. Type several commands: echo 'cmd1', echo 'cmd2', echo 'cmd3'"
echo "4. Use UP/DOWN arrows repeatedly"
echo "5. ✅ EXPECTED: Clean prompt replacement, no stacking"
echo "6. ❌ BUG: Multiple prompt boxes visible"
echo ""

echo "VERIFICATION CHECKLIST:"
echo "======================"
echo ""
echo "✅ Original theme wrapped line issues:"
echo "   - History recall replaces content cleanly"
echo "   - No new prompt creation"
echo "   - Proper cursor positioning"
echo ""
echo "✅ Dark theme multiline prompt issues:"
echo "   - No prompt stacking"
echo "   - Clean multiline prompt clearing"
echo "   - Proper box rendering"
echo ""
echo "✅ Performance:"
echo "   - Responsive navigation"
echo "   - No visible artifacts"
echo "   - Smooth user experience"
echo ""

echo "TECHNICAL DETAILS:"
echo "=================="
echo ""
echo "Fixes applied:"
echo "1. Enhanced multiline clearing in refreshMultiLine()"
echo "2. Improved wrapped content clearing in refreshSingleLine()"
echo "3. Proper line calculation for both themes"
echo "4. Consistent clearing logic across refresh operations"
echo ""

echo "=========================================="
echo "Run manual tests to verify all issues fixed"
echo "Check /tmp/lndebug.txt for clearing operations"
echo "=========================================="

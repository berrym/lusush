#!/bin/bash

# Test script to reproduce theme-specific history navigation issues
# Tests both original theme and dark multiline theme for prompt stacking/duplication

echo "=========================================="
echo "THEME-SPECIFIC HISTORY NAVIGATION TEST"
echo "=========================================="
echo ""

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ Error: ./builddir/lusush not found. Run 'ninja' in builddir first."
    exit 1
fi

echo "✅ Binary found: ./builddir/lusush"
echo ""

echo "ISSUE REPRODUCTION TESTS:"
echo "========================="
echo ""

echo "This script will help reproduce the reported issues:"
echo "1. Original theme: History recall creates new prompts for wrapped lines"
echo "2. Dark multiline theme: History recall causes prompt stacking"
echo ""

echo "MANUAL TESTING REQUIRED:"
echo "========================"
echo ""

echo "🔍 TEST 1: ORIGINAL THEME WRAPPED LINES"
echo "---------------------------------------"
echo "1. Start lusush: ./builddir/lusush -i"
echo "2. Keep default theme (original)"
echo "3. Type a very long command that wraps:"
echo "   echo 'This is a very long command that should definitely wrap across multiple lines in the terminal window and cause display issues'"
echo "4. Press Enter to execute"
echo "5. Press UP arrow to recall"
echo ""
echo "❌ EXPECTED BUG: Creates new prompt instead of replacing content"
echo "✅ DESIRED: Should replace current line content cleanly"
echo ""

echo "🔍 TEST 2: DARK MULTILINE THEME STACKING"
echo "----------------------------------------"
echo "1. Start lusush: ./builddir/lusush -i"
echo "2. Set multiline theme: theme set dark"
echo "3. Type a few commands:"
echo "   echo 'command 1'"
echo "   echo 'command 2'"
echo "   echo 'command 3'"
echo "4. Use UP/DOWN arrows to navigate history"
echo ""
echo "❌ EXPECTED BUG: Prompt stacking (multiple prompts appear)"
echo "✅ DESIRED: Clean prompt replacement"
echo ""

echo "DEBUG INVESTIGATION:"
echo "==================="
echo ""

echo "To debug these issues, check these areas in the code:"
echo ""
echo "1. Theme-specific refresh logic:"
echo "   grep -n -A 10 -B 5 'theme.*dark\\|mlmode' src/linenoise/linenoise.c"
echo ""
echo "2. History navigation implementation:"
echo "   grep -n -A 20 'linenoiseEditHistoryNext' src/linenoise/linenoise.c"
echo ""
echo "3. Refresh patterns in different modes:"
echo "   grep -n 'refreshLine\\|REFRESH_' src/linenoise/linenoise.c"
echo ""

echo "EXPECT SCRIPT AUTOMATED TEST:"
echo "============================="
echo ""

# Create expect script to automate testing
cat > /tmp/test_theme_issues.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

puts "=== TESTING ORIGINAL THEME ==="
spawn ./builddir/lusush -i
expect "lusush"

# Test original theme with wrapped line
send "echo 'This is a very long command that should definitely wrap across multiple lines in the terminal window and cause display issues with history navigation'\r"
expect "lusush"

puts "Pressing UP arrow to test wrapped line recall..."
send "\033\[A"
sleep 2

# Check for issues
puts "Look for: New prompt creation vs clean replacement"
send "\r"
expect "lusush"

puts "\n=== TESTING DARK MULTILINE THEME ==="
# Switch to dark theme
send "theme set dark\r"
expect "lusush"

# Add some commands
send "echo 'command 1'\r"
expect "lusush"
send "echo 'command 2'\r"
expect "lusush"
send "echo 'command 3'\r"
expect "lusush"

puts "Testing history navigation in dark theme..."
send "\033\[A"
sleep 1
send "\033\[A"
sleep 1
send "\033\[B"
sleep 1

puts "Look for: Prompt stacking issues"

# Exit
send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_theme_issues.exp
    echo "Running automated test..."
    /tmp/test_theme_issues.exp
    rm -f /tmp/test_theme_issues.exp
else
    echo "⚠️  Expect not available for automated testing"
fi

echo ""
echo "CODE ANALYSIS SUGGESTIONS:"
echo "=========================="
echo ""

echo "The issues likely stem from:"
echo ""
echo "1. THEME-DEPENDENT REFRESH LOGIC:"
echo "   - Original theme uses single-line refresh"
echo "   - Dark theme uses multiline refresh"
echo "   - Different code paths may have different bugs"
echo ""
echo "2. WRAPPED LINE HANDLING:"
echo "   - Original theme may not properly calculate wrapped content"
echo "   - History recall doesn't account for line wrapping"
echo "   - Creates new prompt instead of replacing"
echo ""
echo "3. MULTILINE PROMPT STACKING:"
echo "   - Dark theme multiline prompts not properly cleared"
echo "   - History navigation leaves previous prompt visible"
echo "   - Clearing logic insufficient for multiline prompts"
echo ""

echo "POTENTIAL FIXES:"
echo "==============="
echo ""

echo "1. UNIFIED REFRESH APPROACH:"
echo "   - Use same refresh logic regardless of theme"
echo "   - Ensure proper clearing before redrawing"
echo "   - Handle wrapped content consistently"
echo ""
echo "2. PROPER CLEARING LOGIC:"
echo "   - Clear entire prompt area before history recall"
echo "   - Calculate proper number of lines to clear"
echo "   - Handle both single-line and multiline prompts"
echo ""
echo "3. THEME-AWARE LINE CALCULATION:"
echo "   - Account for prompt structure differences"
echo "   - Calculate wrapped lines correctly per theme"
echo "   - Ensure cursor positioning accuracy"
echo ""

echo "INVESTIGATION COMMANDS:"
echo "======================"
echo ""

echo "# Check current refresh implementation"
echo "grep -n -A 10 'refreshLineWithFlags.*REFRESH_ALL' src/linenoise/linenoise.c"
echo ""
echo "# Check theme-specific logic"
echo "grep -n -A 5 -B 5 'mlmode\\|multiline' src/linenoise/linenoise.c"
echo ""
echo "# Check prompt clearing logic"
echo "grep -n -A 15 'refreshMultiLine\\|refreshSingleLine' src/linenoise/linenoise.c"
echo ""

echo "=========================================="
echo "Run manual tests above to reproduce issues"
echo "Then investigate code areas mentioned"
echo "=========================================="

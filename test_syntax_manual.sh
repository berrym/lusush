#!/bin/bash

# Manual test script for Lusush syntax highlighting
# This script provides clear instructions for testing syntax highlighting

echo "=== Lusush Syntax Highlighting Manual Test ==="
echo

# Build the shell
cd "$(dirname "$0")"
echo "Building Lusush..."
ninja -C builddir > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    ninja -C builddir
    exit 1
fi

echo "✅ Build successful!"
echo

echo "🎨 Testing Syntax Highlighting"
echo "==============================="
echo

echo "Starting Lusush shell in interactive mode..."
echo "The shell should show syntax highlighting with these colors:"
echo
echo "  🟢 Commands/Builtins (GREEN):     echo, cd, ls, pwd, export, etc."
echo "  🔵 Keywords (BLUE):              if, then, else, for, while, do, done, etc."
echo "  🟡 Strings (YELLOW):             \"hello world\", 'test', etc."
echo "  🟣 Variables (MAGENTA):          \$HOME, \$USER, \$PATH, etc."
echo "  🔴 Operators (RED):              |, &, ;, <, >, etc."
echo "  ⚫ Comments (GRAY):              # this is a comment"
echo "  🔵 Numbers (CYAN):               123, 45.67, etc."
echo
echo "Test Commands to Try:"
echo "---------------------"
echo "1. echo \"hello world\"              # Should show: echo(GREEN) \"hello world\"(YELLOW)"
echo "2. ls -la | grep test             # Should show: ls(GREEN) | (RED) grep(GREEN)"
echo "3. if true; then echo ok; fi      # Should show: if,then,fi(BLUE) echo(GREEN)"
echo "4. export VAR=\"test\"              # Should show: export(GREEN) \"test\"(YELLOW)"
echo "5. echo \$HOME                     # Should show: echo(GREEN) \$HOME(MAGENTA)"
echo "6. # this is a comment            # Should show: entire line in GRAY"
echo "7. cd /tmp && ls                  # Should show: cd,ls(GREEN) &&(RED)"
echo "8. for i in 1 2 3; do echo \$i; done  # Multiple colors"
echo
echo "What to Look For:"
echo "----------------"
echo "✅ GOOD: Actual colors appear as you type"
echo "❌ BAD:  Literal codes like ^[[1;32m or \\033[1;32m appear"
echo "❌ BAD:  No colors at all (everything appears in default terminal color)"
echo
echo "Special Features to Test:"
echo "------------------------"
echo "• Tab completion should still work (try: ec<TAB>)"
echo "• Arrow keys should navigate history"
echo "• Ctrl+R should work for reverse search"
echo "• Ctrl+L should clear screen"
echo "• Ctrl+C should interrupt gracefully"
echo
echo "Starting interactive shell now..."
echo "Type 'exit' or press Ctrl+D to quit when done testing."
echo "=================================================="
echo

# Start the shell in forced interactive mode
exec ./builddir/lusush -i

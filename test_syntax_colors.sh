#!/bin/bash

# Enhanced test script for Lusush syntax highlighting
# This script captures and displays colored output to verify syntax highlighting

echo "=== Lusush Syntax Highlighting Visual Test ==="
echo

# Build the shell
cd "$(dirname "$0")"
echo "Building Lusush..."
ninja -C builddir > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build successful!"
echo

# Function to test syntax highlighting with specific commands
test_command() {
    local cmd="$1"
    local description="$2"

    echo "Testing: $description"
    echo "Command: $cmd"
    echo "Output:"

    # Use script to capture ANSI color codes
    echo "$cmd" | script -q -c './builddir/lusush' /dev/null 2>/dev/null | head -n 10 | grep -v "^$"

    echo "---"
    echo
}

echo "🎨 Visual Syntax Highlighting Tests"
echo "Colors should appear as follows:"
echo "  • Commands/builtins: GREEN"
echo "  • Keywords (if, for, while): BLUE"
echo "  • Strings: YELLOW"
echo "  • Variables: MAGENTA"
echo "  • Operators (|, &, etc): RED"
echo "  • Comments: GRAY"
echo

# Test various syntax elements
test_command 'echo "hello world"' "Basic command with string"
test_command 'ls -la | grep test' "Command with pipe operator"
test_command 'if test -f /etc/passwd; then echo "exists"; fi' "Conditional with keywords"
test_command 'export MY_VAR="value"' "Variable assignment"
test_command 'echo $HOME' "Variable expansion"
test_command '# This is a comment' "Comment syntax"
test_command 'for i in 1 2 3; do echo $i; done' "Loop with keywords"
test_command 'cd /tmp && ls' "Commands with operators"

echo "🔍 Manual Test Instructions:"
echo "Run the following command to test interactively:"
echo "  ./builddir/lusush"
echo
echo "Then type these test commands and observe colors:"
echo "  echo \"hello world\"     # Should show 'echo' in GREEN, string in YELLOW"
echo "  ls | grep test         # Should show commands in GREEN, '|' in RED"
echo "  if true; then echo ok; fi  # Should show 'if', 'then', 'fi' in BLUE"
echo "  export VAR=test        # Should show 'export' in GREEN"
echo "  echo \$HOME             # Should show 'echo' in GREEN, '\$HOME' in MAGENTA"
echo "  # comment              # Should show comment in GRAY"
echo
echo "✅ If you see actual colors (not ^[[1;32m codes), syntax highlighting works!"
echo "❌ If you see literal escape codes, there's still an issue to fix."
echo
echo "Press Ctrl+C to exit the interactive shell when testing."

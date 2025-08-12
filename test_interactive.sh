#!/bin/bash

# Interactive test script for Lusush syntax highlighting
# This script uses expect to automate typing and capture colored output

echo "=== Interactive Lusush Syntax Highlighting Test ==="
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

# Check if expect is available
if ! command -v expect &> /dev/null; then
    echo "⚠️  'expect' not found. Running manual test instead."
    echo
    echo "Manual Test Instructions:"
    echo "1. Run: ./builddir/lusush"
    echo "2. Type: echo \"hello world\""
    echo "3. Look for colors:"
    echo "   - 'echo' should be GREEN"
    echo "   - \"hello world\" should be YELLOW"
    echo "   - NO literal escape codes like ^[[1;32m"
    echo
    echo "4. Press Ctrl+C to exit"
    echo
    echo "Starting shell now..."
    exec ./builddir/lusush
fi

echo "🤖 Running automated test with expect..."

# Create expect script
expect << 'EOF'
# Set timeout
set timeout 10

# Start the shell
spawn ./builddir/lusush

# Wait for prompt
expect "$ "

# Type a simple command with syntax highlighting
send "echo \"hello world\"\r"

# Wait for command to execute
expect "hello world"
expect "$ "

# Type a command with keywords
send "if true; then echo \"works\"; fi\r"

# Wait for execution
expect "works"
expect "$ "

# Type a command with variables
send "echo \$HOME\r"

# Wait for execution
expect "$ "

# Type a comment
send "# this is a comment\r"

# Wait for prompt
expect "$ "

# Exit the shell
send "exit\r"

# Wait for shell to exit
expect eof
EOF

echo
echo "✅ Automated test completed!"
echo
echo "🔍 If syntax highlighting is working correctly, you should have seen:"
echo "  • Commands like 'echo', 'if', 'then' in GREEN"
echo "  • Strings like \"hello world\" in YELLOW"
echo "  • Variables like \$HOME in MAGENTA"
echo "  • Keywords like 'if', 'then', 'fi' in BLUE"
echo "  • Comments starting with # in GRAY"
echo
echo "❌ If you saw literal codes like ^[[1;32m, syntax highlighting needs fixing"
echo "✅ If you saw actual colors, syntax highlighting is working!"

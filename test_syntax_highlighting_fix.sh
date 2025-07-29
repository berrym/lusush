#!/bin/bash

# Test script to verify syntax highlighting fix for Linux terminals
# This script tests that syntax highlighting doesn't create unwanted newlines

echo "=== Syntax Highlighting Fix Test ==="
echo "Testing that syntax highlighting doesn't cause newlines during basic input"
echo

# Get current terminal size
COLS=$(tput cols)
ROWS=$(tput lines)

echo "Current terminal size: ${COLS}x${ROWS}"
echo "Platform: $(uname -s)"
echo

# Enable debug mode to see what's happening
export LLE_DEBUG=1

echo "Starting lusush with debug output enabled..."
echo
echo "Test Cases to Try:"
echo "1. Type: 'echo test' (should not create newline after 'echo ')"
echo "2. Type: 'ls -la' (should not create newline after 'ls ')"
echo "3. Type: 'echo \"hello world\"' (quotes might trigger rewrite, but should work)"
echo "4. Type: 'cat file | grep pattern' (pipes should work correctly)"
echo
echo "Expected Behavior:"
echo "✅ PASS: Text appears on same line as prompt"
echo "✅ PASS: Syntax highlighting works (different colors for commands/args)"
echo "❌ FAIL: Text jumps to newlines during typing"
echo "❌ FAIL: Strange cursor positioning"
echo

echo "Debug output will show:"
echo "- 'Linux true incremental' for simple character additions"
echo "- 'Linux syntax-sensitive char' only for structural characters"
echo "- Platform detection messages"
echo

# Run lusush
echo "Press Ctrl+C to exit lusush when done testing"
echo "Running lusush..."
./builddir/lusush

echo
echo "=== Test Results Analysis ==="
echo "Review the debug output above:"
echo
echo "✅ SUCCESS INDICATORS:"
echo "  - Most characters show 'Linux true incremental'"
echo "  - Few 'syntax-sensitive char' messages"
echo "  - No unwanted newlines during typing"
echo "  - Syntax highlighting colors appear correctly"
echo
echo "❌ FAILURE INDICATORS:"
echo "  - Many 'syntax-sensitive char' messages for simple spaces"
echo "  - Text jumping to newlines"
echo "  - Missing or broken syntax highlighting"
echo
echo "The fix is working if:"
echo "1. Typing 'echo test' shows incremental updates for most characters"
echo "2. Only the first space after 'echo' triggers syntax highlighting rewrite"
echo "3. Subsequent characters in 'test' use incremental updates"
echo "4. Text stays on the same line as the prompt"

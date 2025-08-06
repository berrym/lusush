#!/bin/bash

# Test script for history navigation debugging on Linux
# Tests the exact backspace replication implementation

echo "=== LLE History Navigation Test (Linux) ==="
echo "Testing exact backspace replication implementation"

# Ensure we have a history file with test commands
mkdir -p ~/.config/lusush 2>/dev/null || true
cat > ~/.lusush_history << 'EOF'
echo hello world
ls -la /tmp
ps aux | grep bash
echo testing multiline command that wraps around the terminal width and goes to multiple lines
pwd
date
whoami
EOF

echo "Created test history with 7 commands"
echo "History contents:"
cat ~/.lusush_history | nl

echo ""
echo "=== Starting Interactive Test ==="
echo "Instructions:"
echo "1. Type a few characters"
echo "2. Press UP arrow to test history navigation"
echo "3. Press UP again to test multiline history"
echo "4. Press DOWN to test navigation back"
echo "5. Type 'exit' to finish"
echo ""
echo "Watch for:"
echo "- Display corruption during history navigation"
echo "- Prompt artifacts or positioning errors"
echo "- Characters appearing in wrong places"
echo ""
echo "Starting lusush with debug enabled..."
echo "Debug output will be saved to /tmp/history_debug.log"

# Run lusush in interactive mode with debug
LLE_DEBUG=1 LLE_CURSOR_DEBUG=1 ./builddir/lusush 2>/tmp/history_debug.log

echo ""
echo "=== Test Complete ==="
echo "Debug output saved to /tmp/history_debug.log"
echo "Use 'cat /tmp/history_debug.log | grep -E \"(HISTORY|INPUT_LOOP)\"' to see history-specific logs"

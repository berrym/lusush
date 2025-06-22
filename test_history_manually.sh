#!/bin/bash

# Manual test script for multiline history functionality
# This script provides instructions and examples for testing history conversion

echo "=== Lusush Multiline History Test ==="
echo "This script helps you manually test multiline history functionality"
echo

SHELL_PATH="./builddir/lusush"

# Check if shell exists
if [ ! -x "$SHELL_PATH" ]; then
    echo "Error: Shell binary not found at $SHELL_PATH"
    echo "Please build the shell first with: meson compile -C builddir"
    exit 1
fi

echo "Shell: $SHELL_PATH"
echo

# Clean up history for fresh test
echo "Cleaning up existing history..."
rm -f ~/.lusushist
echo

echo "=== Manual Testing Instructions ==="
echo
echo "1. Start the interactive shell:"
echo "   $SHELL_PATH"
echo
echo "2. Enter this multiline command (press Enter after each line):"
echo "   for i in 1 2 3"
echo "   do"
echo "       echo \"Item: \$i\""
echo "   done"
echo
echo "3. After the command completes, press the UP ARROW key"
echo
echo "4. You should see the command recalled as a single line:"
echo "   Expected: for i in 1 2 3 do echo \"Item: \$i\" done"
echo
echo "5. Try editing the recalled command and press Enter to execute"
echo
echo "6. Test backslash continuation:"
echo "   echo hello \\"
echo "   world"
echo
echo "7. Press UP ARROW again - should show:"
echo "   Expected: echo hello world"
echo
echo "8. Test with an if statement:"
echo "   if true"
echo "   then"
echo "       echo success"
echo "   fi"
echo
echo "9. Press UP ARROW - should show:"
echo "   Expected: if true then echo success fi"
echo
echo "10. Type 'exit' to quit the shell"
echo

echo "=== Additional Test Cases ==="
echo
echo "Try these multiline commands and verify history recall:"
echo
echo "• While loop:"
echo "  x=1"
echo "  while [ \$x -le 3 ]"
echo "  do"
echo "      echo \"Count: \$x\""
echo "      x=\$((x + 1))"
echo "  done"
echo
echo "• Command group:"
echo "  {"
echo "      echo \"Line 1\""
echo "      echo \"Line 2\""
echo "  }"
echo
echo "• Pipeline with continuation:"
echo "  echo \"hello world\" | \\"
echo "  grep hello | \\"
echo "  wc -l"
echo

echo "=== What to Look For ==="
echo
echo "✓ Good signs:"
echo "  - Multiline commands are recalled as single lines"
echo "  - Single lines are easy to edit"
echo "  - Backslash continuations become simple spaces"
echo "  - No embedded newlines in recalled commands"
echo "  - History navigation works smoothly"
echo
echo "✗ Problems to report:"
echo "  - Commands recalled with embedded newlines"
echo "  - History entries that can't be edited easily"
echo "  - Missing history entries"
echo "  - Crashes or hangs during history recall"
echo

echo "=== Starting Interactive Shell ==="
echo "Press Ctrl+C to cancel, or Enter to start the shell..."
read -r

echo "Starting shell in 3 seconds..."
sleep 1
echo "2..."
sleep 1
echo "1..."
sleep 1
echo "Starting $SHELL_PATH"
echo

# Start the shell
exec "$SHELL_PATH"

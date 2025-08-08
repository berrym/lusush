#!/bin/bash

# Simple Tab Completion Test for Lusush Line Editor
# Tests basic tab completion functionality

echo "========================================================================"
echo "LUSUSH LINE EDITOR - TAB COMPLETION TEST"
echo "========================================================================"

# Get absolute path to lusush binary
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_PATH="$SCRIPT_DIR/builddir/lusush"

if [[ ! -x "$SHELL_PATH" ]]; then
    echo "Error: Shell not found at $SHELL_PATH"
    echo "Please build the shell first: scripts/lle_build.sh build"
    exit 1
fi

echo "Testing shell: $SHELL_PATH"
echo ""

echo "=== TAB COMPLETION TEST INSTRUCTIONS ==="
echo "1. Type 'ec' and press TAB - should complete to 'echo'"
echo "2. Type 'l' and press TAB - should show completions (ls, ll, etc.)"
echo "3. Type 'cat ' and press TAB - should show file completions"
echo "4. Use Ctrl+C to exit when testing is complete"
echo ""

echo "Starting interactive shell for tab completion testing..."
echo "Enable debug mode with: export LLE_DEBUG=1"
echo "========================================================================"

# Start shell in interactive mode for manual tab testing
LLE_DEBUG=1 "$SHELL_PATH"

echo ""
echo "Tab completion testing session ended."

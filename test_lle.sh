#!/bin/bash
# Test LLE with debug output redirected to log file
# Usage: ./test_lle.sh

LOG_FILE="/tmp/lle_debug.log"

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Clear old log
> "$LOG_FILE"

echo "Debug output will be written to: $LOG_FILE"
echo "You can tail it in another terminal with: tail -f $LOG_FILE"
echo ""
echo "Starting lusush..."
echo "IMPORTANT: Run 'config set editor.use_lle true' to enable LLE!"
echo ""

# Run lusush with stderr redirected to log
"$SCRIPT_DIR/builddir/lusush" -i 2>"$LOG_FILE"

echo ""
echo "Session ended. Check $LOG_FILE for debug output."

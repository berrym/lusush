#!/bin/bash
# Test script for multiline ENTER display bug fix

echo "Testing multiline ENTER fix..."
echo ""
echo "Test 1: Simple multiline command"
echo "Input: if true; then"
echo "       echo done"
echo "       fi"
echo ""

# Use heredoc to send multiline input to lusush
builddir/lusush -c 'if true; then
echo done
fi'

echo ""
echo "Exit code: $?"
echo ""
echo "Test passed if 'done' appears on a new line after the command"

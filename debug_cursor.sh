#!/bin/bash
# Debug script to help diagnose cursor positioning

echo "Debug info for cursor positioning:"
echo "1. Type 'e' and press TAB"
echo "2. Note cursor position relative to 'echo'"
echo "3. Press TAB again and note if rows are consumed"
echo ""
echo "Expected behavior:"
echo "- Cursor should be right after 'echo'"
echo "- No rows should be consumed on navigation"
echo "- Menu should stay in place below command"
echo ""
echo "Starting lusush with LLE enabled..."
echo ""

LLE_ENABLED=1 ./builddir/lusush

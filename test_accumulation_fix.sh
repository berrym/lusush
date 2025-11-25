#!/bin/bash
# Test the accumulation bug fix

echo "Testing completion accumulation fix..."
echo ""
echo "Type 'e' and press TAB multiple times, then use arrow keys"
echo "The word should update properly without accumulation"
echo ""

LLE_ENABLED=1 ./builddir/lusush

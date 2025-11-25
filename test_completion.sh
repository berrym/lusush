#!/bin/bash
# Test TAB completion

echo "Testing TAB completion with 'e' prefix..."
./builddir/lusush -c "echo 'Testing echo completion'" 2>&1

echo ""
echo "Testing with simple commands..."
echo -e "ls\nexit" | ./builddir/lusush 2>&1 | head -20

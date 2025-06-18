#!/bin/bash

echo "=== Debugging Variable Assignment Logic ==="

cd /home/mberry/Lab/c/lusush

echo -e "\n=== Test 1: Simple echo (no variables) ==="
echo 'echo hello' | ./builddir/lusush

echo -e "\n=== Test 2: Variable assignment only ==="
echo 'x=hello' | ./builddir/lusush

echo -e "\n=== Test 3: Variable assignment then echo separately ==="
cat << 'EOF' | ./builddir/lusush
x=hello
echo $x
EOF

echo -e "\n=== Test 4: Variable assignment and command on same line ==="
echo 'x=hello echo $x' | ./builddir/lusush

echo -e "\n=== Test 5: Check if echo builtin works ==="
echo 'echo "test message"' | ./builddir/lusush

echo -e "\n=== Test 6: Variable expansion without assignment ==="
echo 'echo $HOME' | ./builddir/lusush

echo -e "\n=== Test completed ==="

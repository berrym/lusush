#!/bin/bash

echo "=== Command Substitution Debug Script ==="
echo "Testing lusush command substitution with nested arithmetic"
echo

# Test individual components
echo "1. Testing arithmetic expansion alone:"
echo "Command: ./builddir/lusush -c 'echo \$((2 + 3))'"
./builddir/lusush -c 'echo $((2 + 3))'
echo

echo "2. Testing simple command substitution:"
echo "Command: ./builddir/lusush -c 'echo \$(echo 5)'"
./builddir/lusush -c 'echo $(echo 5)'
echo

echo "3. Testing nested command substitution (the failing case):"
echo "Command: ./builddir/lusush -c 'echo \$(echo \$((2 + 3)))'"
./builddir/lusush -c 'echo $(echo $((2 + 3)))'
echo

echo "4. Testing with quotes:"
echo "Command: ./builddir/lusush -c 'echo \"\$(echo \$((2 + 3)))\"'"
./builddir/lusush -c 'echo "$(echo $((2 + 3)))"'
echo

echo "5. Testing step by step - what gets passed to inner command:"
echo "Command: ./builddir/lusush -c 'cmd=\"echo \$((2 + 3))\"; echo \"Inner command: \$cmd\"; \$cmd'"
./builddir/lusush -c 'cmd="echo $((2 + 3))"; echo "Inner command: $cmd"; $cmd'
echo

echo "6. Testing with simpler nested case:"
echo "Command: ./builddir/lusush -c 'echo \$(echo \$((1+1)))'"
./builddir/lusush -c 'echo $(echo $((1+1)))'
echo

echo "7. Testing if arithmetic works in command context:"
echo "Command: ./builddir/lusush -c 'echo \$((2 + 3)) | cat'"
./builddir/lusush -c 'echo $((2 + 3)) | cat'
echo

echo "8. Testing bash comparison:"
echo "Command: bash -c 'echo \$(echo \$((2 + 3)))'"
bash -c 'echo $(echo $((2 + 3)))'
echo

echo "=== Debug Complete ==="

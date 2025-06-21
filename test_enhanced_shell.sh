#!/bin/bash

echo "=== TESTING ENHANCED TOKENIZER WITH SHELL ==="

echo "Testing variable assignment..."
echo 'var=value; echo $var' | ./builddir/lusush

echo "Testing while loop with arithmetic..."
echo 'i=1; while [ "$i" -lt 3 ]; do echo "i=$i"; i=$((i+1)); done' | ./builddir/lusush

echo "Testing for loop with command substitution..."
echo 'for i in $(echo 1 2 3); do echo "Number: $i"; done' | ./builddir/lusush

echo "Testing test command with flags..."
echo 'if [ -f /etc/passwd ]; then echo "passwd exists"; fi' | ./builddir/lusush

echo "Testing arithmetic expansion..."
echo 'result=$((5 + 3)); echo "Result: $result"' | ./builddir/lusush

echo "=== TEST COMPLETE ==="

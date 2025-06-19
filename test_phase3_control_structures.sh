#!/bin/bash

# Test script for Phase 3: Control Structure Migration
# This script tests if, while, and for control structures work with the new parser

echo "=== LUSUSH Phase 3 Control Structure Migration Tests ==="
echo

cd "$(dirname "$0")"

# Build if needed
if [ ! -f builddir/lusush ]; then
    echo "Building lusush..."
    meson compile -C builddir
fi

# Set up test environment
export NEW_PARSER_DEBUG=1

echo "Test 1: Simple if statement"
echo 'if echo "condition"; then echo "then body"; fi' | ./builddir/lusush
echo "Exit status: $?"
echo

echo "Test 2: If with else"
echo 'if false; then echo "should not print"; else echo "else body"; fi' | ./builddir/lusush
echo "Exit status: $?"
echo

echo "Test 3: Simple while loop"
echo 'i=0; while [ $i -lt 3 ]; do echo "loop $i"; i=$((i+1)); done' | ./builddir/lusush
echo "Exit status: $?"
echo

echo "Test 4: Simple for loop"
echo 'for i in 1 2 3; do echo "item $i"; done' | ./builddir/lusush
echo "Exit status: $?"
echo

echo "Test 5: For loop without word list (should use positional parameters)"
echo 'set -- a b c; for i; do echo "param $i"; done' | ./builddir/lusush
echo "Exit status: $?"
echo

echo "=== Control Structure Parser Routing Test ==="
echo "Testing that control structures are routed to new parser..."

echo "Test 6: Check if statement parsing"
echo 'if true; then echo "new parser if works"; fi' | ./builddir/lusush
echo

echo "Test 7: Check while statement parsing"
echo 'x=1; while [ $x -eq 1 ]; do echo "new parser while works"; x=2; done' | ./builddir/lusush
echo

echo "Test 8: Check for statement parsing"
echo 'for item in hello world; do echo "new parser for: $item"; done' | ./builddir/lusush
echo

echo "=== Phase 3 Tests Complete ==="

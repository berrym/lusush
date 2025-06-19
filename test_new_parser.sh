#!/bin/bash

# Test script for new POSIX-compliant parser
# Tests basic functionality of the new parser infrastructure

echo "=== Testing New POSIX Parser Infrastructure ==="

# Test basic command parsing
echo "Testing simple commands..."
echo "echo hello" | ~/Lab/c/lusush/builddir/lusush

echo "Testing command with arguments..."
echo "echo hello world" | ~/Lab/c/lusush/builddir/lusush

echo "Testing pipeline..."
echo "echo hello | cat" | ~/Lab/c/lusush/builddir/lusush

echo "Testing logical operators..."
echo "true && echo success" | ~/Lab/c/lusush/builddir/lusush
echo "false || echo fallback" | ~/Lab/c/lusush/builddir/lusush

echo "Testing if statement..."
echo "if true; then echo if works; fi" | ~/Lab/c/lusush/builddir/lusush

echo "Testing while loop..."
echo "while false; do echo never; done" | ~/Lab/c/lusush/builddir/lusush

echo "Testing for loop..."
echo "for i in a b c; do echo \$i; done" | ~/Lab/c/lusush/builddir/lusush

echo "=== Parser Infrastructure Test Complete ==="

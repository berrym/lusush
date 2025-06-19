#!/bin/bash

# Test script for new POSIX-compliant parser
# Tests basic functionality of the new parser infrastructure

echo "=== Testing New POSIX Parser Infrastructure ==="
echo "Using LUSUSH_NEW_PARSER=1 to force new parser usage"

# Test basic command parsing
echo "Testing simple commands..."
LUSUSH_NEW_PARSER=1 echo "echo hello" | ~/Lab/c/lusush/builddir/lusush

echo "Testing command with arguments..."
LUSUSH_NEW_PARSER=1 echo "echo hello world" | ~/Lab/c/lusush/builddir/lusush

echo "Testing pipeline..."
LUSUSH_NEW_PARSER=1 echo "echo hello | cat" | ~/Lab/c/lusush/builddir/lusush

echo "Testing logical operators..."
LUSUSH_NEW_PARSER=1 echo "true && echo success" | ~/Lab/c/lusush/builddir/lusush
LUSUSH_NEW_PARSER=1 echo "false || echo fallback" | ~/Lab/c/lusush/builddir/lusush

echo "Testing if statement..."
LUSUSH_NEW_PARSER=1 echo "if true; then echo if works; fi" | ~/Lab/c/lusush/builddir/lusush

echo "Testing while loop..."
LUSUSH_NEW_PARSER=1 echo "while false; do echo never; done" | ~/Lab/c/lusush/builddir/lusush

echo "Testing for loop..."
LUSUSH_NEW_PARSER=1 echo "for i in a b c; do echo \$i; done" | ~/Lab/c/lusush/builddir/lusush

echo "=== Parser Infrastructure Test Complete ==="

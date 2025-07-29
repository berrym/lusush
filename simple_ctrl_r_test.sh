#!/bin/bash

# Simple Ctrl+R Test for Lusush Line Editor
# Tests basic Ctrl+R functionality without complex terminal manipulation

echo "=== Simple Ctrl+R Test ==="
echo

# Create minimal test setup
mkdir -p simple_test
cd simple_test

# Create simple history
echo "echo hello" > .lusush_history
echo "ls -la" >> .lusush_history
echo "pwd" >> .lusush_history

echo "Created simple history:"
cat .lusush_history
echo

echo "=== Manual Test Instructions ==="
echo "1. Shell will start with 3 commands in history"
echo "2. Press Ctrl+R to enter search mode"
echo "3. Type 'echo' to search"
echo "4. Try Ctrl+A, Ctrl+E, Ctrl+U in search mode"
echo "5. Press Ctrl+G to exit search"
echo "6. Type 'exit' to quit"
echo

# Set environment
export LUSUSH_HISTORY=.lusush_history

echo "Starting lusush..."
../builddir/lusush

# Cleanup
cd ..
rm -rf simple_test
echo "Test completed!"

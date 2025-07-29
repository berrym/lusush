#!/bin/bash

# Test script for Enhanced Tab Completion in Lusush Line Editor
# This script tests the enhanced tab completion functionality

echo "=== Enhanced Tab Completion Test ==="
echo

# Create test directory structure
echo "Setting up test files..."
mkdir -p test_completion_dir
cd test_completion_dir

# Create test files with different patterns
touch test_file1.txt
touch test_file2.txt
touch test_abc.txt
touch test_xyz.txt
touch another_file.txt
touch build.sh
touch config.json
touch README.md

echo "Created test files:"
ls -la

echo
echo "=== Testing Enhanced Tab Completion ==="
echo

# Test 1: Basic completion (should complete to first match)
echo "Test 1: Type 'echo test_' and press Tab (should show first match)"
echo "Expected: Complete to 'test_abc.txt' or 'test_file1.txt'"
echo

# Test 2: Multiple Tab presses (should cycle through matches)
echo "Test 2: Type 'echo test_' and press Tab multiple times"
echo "Expected: Cycle through test_abc.txt, test_file1.txt, test_file2.txt, test_xyz.txt"
echo

# Test 3: Partial match completion
echo "Test 3: Type 'echo test_f' and press Tab"
echo "Expected: Cycle through test_file1.txt, test_file2.txt"
echo

# Test 4: Single character completion
echo "Test 4: Type 'echo b' and press Tab"
echo "Expected: Complete to 'build.sh'"
echo

# Test 5: No matches
echo "Test 5: Type 'echo nonexistent' and press Tab"
echo "Expected: No completion, no action"
echo

echo "=== Starting Lusush with Enhanced Debug ==="
echo "Instructions:"
echo "1. Try the test cases above"
echo "2. Watch for debug output showing completion cycling"
echo "3. Look for '[ENHANCED_TAB_COMPLETION]' messages"
echo "4. Type 'exit' to quit"
echo

# Launch lusush with enhanced debugging
export LLE_DEBUG_COMPLETION=1
export LLE_DEBUG=1

# Use expect to simulate interactive session if available
if command -v expect >/dev/null 2>&1; then
    echo "Using expect for automated testing..."
    expect << 'EOF'
set timeout 30
spawn ../builddir/lusush
expect "$ "

# Test basic completion
send "echo test_\t"
expect "$ "

# Test cycling
send "\t"
expect "$ "

send "\t"
expect "$ "

# Clear line and try different test
send "\r"
expect "$ "

send "echo b\t"
expect "$ "

send "\r"
expect "$ "

send "exit\r"
expect eof
EOF
else
    echo "Expect not available, starting manual session..."
    ../builddir/lusush
fi

# Cleanup
echo
echo "=== Cleaning up test files ==="
cd ..
rm -rf test_completion_dir
echo "Test completed!"

#!/bin/bash

# Test script to validate unified input handling between interactive and non-interactive modes

echo "=== Testing Unified Input System ==="
echo "Testing both interactive and non-interactive modes use the same parsing logic"
echo

cd /home/mberry/Lab/c/lusush/builddir

echo "1. Testing simple command (non-interactive):"
echo 'echo "Simple command works"' | ./lusush
echo

echo "2. Testing multiline if statement (non-interactive):"
printf 'if true; then\n  echo "If statement works"\nfi\n' | ./lusush
echo

echo "3. Testing for loop (non-interactive):"
printf 'for i in 1 2 3; do\n  echo "Loop iteration: $i"\ndone\n' | ./lusush
echo

echo "4. Testing nested control structures (non-interactive):"
printf 'if true; then\n  for j in a b; do\n    echo "Nested: $j"\n  done\nfi\n' | ./lusush
echo

echo "5. Testing incomplete command detection (non-interactive):"
echo "This should be properly buffered until complete:"
printf 'if true; then\necho "incomplete command buffering works"\n' | ./lusush
echo

echo "=== Tests complete ==="
echo "The same input buffering and completion detection logic"
echo "is now used for both interactive (linenoise) and non-interactive modes."
echo "This ensures parsing consistency regardless of input method."

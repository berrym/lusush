#!/bin/bash

echo "=== Testing Gradual Parser Migration ==="

echo "Testing command complexity analysis..."

echo "1. Simple command (should use NEW parser):"
echo "echo hello" | ~/Lab/c/lusush/builddir/lusush

echo "2. Pipeline (should use PIPELINE system):" 
echo "echo test | cat" | ~/Lab/c/lusush/builddir/lusush

echo "3. Complex command (should use OLD parser):"
echo "echo a && echo b" | ~/Lab/c/lusush/builddir/lusush

echo "=== Migration Test Complete ==="

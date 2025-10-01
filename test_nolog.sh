#!/bin/bash

# Test script for nolog functionality
echo "=== Testing nolog option ==="

# Test 1: Default behavior (nolog off)
echo "Test 1: Default behavior (nolog should be OFF)"
echo "set -o | grep nolog; exit" | ./builddir/lusush -i

echo ""
echo "Test 2: Enable nolog option"
echo "set -o nolog; set -o | grep nolog; exit" | ./builddir/lusush -i

echo ""
echo "Test 3: Disable nolog option"
echo "set -o nolog; set +o nolog; set -o | grep nolog; exit" | ./builddir/lusush -i

echo ""
echo "Test 4: Manual interactive test with function definitions"
echo "The following tests require manual verification:"
echo ""
echo "4a. Test nolog OFF (function definitions should appear in history):"
echo "  Run: ./builddir/lusush -i"
echo "  Type: my_func() { echo hello; }"
echo "  Type: history"
echo "  Expected: Function definition should appear in history"
echo "  Type: exit"
echo ""
echo "4b. Test nolog ON (function definitions should NOT appear in history):"
echo "  Run: ./builddir/lusush -i"
echo "  Type: set -o nolog"
echo "  Type: my_func() { echo hello; }"
echo "  Type: echo 'regular command'"
echo "  Type: history"
echo "  Expected: Only 'regular command' should appear, not function definition"
echo "  Type: exit"
echo ""
echo "4c. Test various function definition formats:"
echo "  Run: ./builddir/lusush -i"
echo "  Type: set -o nolog"
echo "  Type: func1() { echo test; }"
echo "  Type: func2 () { echo test; }"
echo "  Type: echo 'between functions'"
echo "  Type: func3(){ echo test; }"
echo "  Type: history"
echo "  Expected: Only 'between functions' should appear in history"
echo "  Type: exit"
echo ""
echo "=== All automated tests completed ==="
echo "Note: nolog prevents function definitions from entering command history"
echo "Function definitions are detected by pattern matching '()' syntax"

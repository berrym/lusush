#!/bin/bash

# Test script for modern execution engine integration
echo "=== MODERN EXECUTION ENGINE INTEGRATION TEST ==="

# Test simple commands
echo "Test 1: Simple echo command"
echo 'echo "Hello from modern executor"' | ./builddir/lusush

echo -e "\nTest 2: Variable assignment and expansion"
echo 'name=world; echo "Hello $name"' | ./builddir/lusush

echo -e "\nTest 3: Arithmetic assignment"
echo 'i=5; result=$((i+1)); echo "Result: $result"' | ./builddir/lusush

echo -e "\nTest 4: Simple if statement"
echo 'if true; then echo "IF works"; fi' | ./builddir/lusush

echo -e "\nTest 5: If-else statement"
echo 'if false; then echo "fail"; else echo "ELSE works"; fi' | ./builddir/lusush

echo -e "\nTest 6: For loop"
echo 'for i in 1 2 3; do echo "Number: $i"; done' | ./builddir/lusush

echo -e "\nTest 7: Simple pipeline"
echo 'echo "pipeline test" | cat' | ./builddir/lusush

echo -e "\nTest 8: Variable persistence"
echo 'x=42; echo "Value: $x"' | ./builddir/lusush

echo -e "\n=== Integration test complete ==="

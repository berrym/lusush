#!/bin/bash

echo "=== Testing Variable Expansion Fix ==="

echo "Test 1: Simple variable assignment"
echo 'myvar=hello' | ./lusush

echo -e "\nTest 2: Variable expansion"
echo -e 'myvar=hello\necho $myvar' | ./lusush

echo -e "\nTest 3: Assignment with command on same line"
echo 'myvar=hello echo "Variable: $myvar"' | ./lusush

echo -e "\nTest 4: Multiple assignments"
echo -e 'var1=first\nvar2=second\necho "$var1 $var2"' | ./lusush

echo -e "\nTest 5: Environment variable expansion"
echo 'echo "Home: $HOME"' | ./lusush

echo -e "\nTest 6: For loop with variable expansion"
echo -e 'for i in a b c; do echo "Item: $i"; done' | ./lusush

echo -e "\n=== Tests completed ==="

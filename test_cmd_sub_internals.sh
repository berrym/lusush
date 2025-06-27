#!/bin/bash

echo "=== Command Substitution Internal Flow Test ==="
echo "Testing the specific issue with nested arithmetic in command substitution"
echo

# Test the exact failing case from the test suite
echo "1. The failing test case:"
echo "Expected: '5'"
echo -n "Actual: '"
./builddir/lusush -c 'echo $(echo $((2 + 3)))' 2>/dev/null
echo "'"
echo

# Test if it's a parsing issue by using a file
echo "2. Testing via script file (bypasses command line parsing):"
echo 'echo $(echo $((2 + 3)))' > /tmp/test_cmd_sub.sh
chmod +x /tmp/test_cmd_sub.sh
echo -n "Result: '"
./builddir/lusush /tmp/test_cmd_sub.sh 2>/dev/null
echo "'"
rm -f /tmp/test_cmd_sub.sh
echo

# Test if it's a stdin issue
echo "3. Testing via stdin:"
echo -n "Result: '"
echo 'echo $(echo $((2 + 3)))' | ./builddir/lusush 2>/dev/null
echo "'"
echo

# Test simpler arithmetic in command substitution
echo "4. Testing simpler arithmetic:"
echo -n "Result: '"
./builddir/lusush -c 'echo $(echo $((5)))' 2>/dev/null
echo "'"
echo

# Test without arithmetic - just variables
echo "5. Testing with simple variable:"
echo -n "Result: '"
./builddir/lusush -c 'x=5; echo $(echo $x)' 2>/dev/null
echo "'"
echo

# Test the components step by step
echo "6. Component test - inner arithmetic:"
echo -n "Inner arithmetic result: '"
./builddir/lusush -c 'echo $((2 + 3))' 2>/dev/null
echo "'"

echo "7. Component test - command substitution with literal:"
echo -n "Command substitution literal: '"
./builddir/lusush -c 'echo $(echo 5)' 2>/dev/null
echo "'"

echo "8. Component test - what the parser should see:"
echo "The command substitution should parse 'echo \$((2 + 3))' and execute it"
echo "Let's see what happens when we execute that exact string:"
echo -n "Direct execution: '"
./builddir/lusush -c 'echo $((2 + 3))' 2>/dev/null
echo "'"
echo

# Test with different arithmetic expressions
echo "9. Testing other arithmetic expressions in command substitution:"
echo -n "$(echo \$((1+1))): '"
./builddir/lusush -c 'echo $(echo $((1+1)))' 2>/dev/null
echo "'"

echo -n "$(echo \$((3*2))): '"
./builddir/lusush -c 'echo $(echo $((3*2)))' 2>/dev/null
echo "'"

echo -n "$(echo \$((10-5))): '"
./builddir/lusush -c 'echo $(echo $((10-5)))' 2>/dev/null
echo "'"
echo

# Test error output
echo "10. Checking for error messages:"
echo "Command: ./builddir/lusush -c 'echo \$(echo \$((2 + 3)))'"
./builddir/lusush -c 'echo $(echo $((2 + 3)))' 2>&1
echo

echo "=== Internal Flow Test Complete ==="

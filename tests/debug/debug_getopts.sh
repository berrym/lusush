#!/bin/bash

# Focused getopts debug test for lusush shell
# This test isolates specific getopts functionality to identify issues

echo "=== GETOPTS FOCUSED DEBUG TEST ==="
echo "Testing specific getopts cases that are failing"
echo "Shell: ./builddir/lusush"
echo

# Test 1: Basic option with argument (separate)
echo "Test 1: Basic option with argument"
echo "Command: getopts \"a:\" opt -a value; echo \"\$opt|\$OPTARG\""
result=$(echo 'getopts "a:" opt -a value; echo "$opt|$OPTARG"' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: 'a|value'"
echo

# Test 2: Basic option with argument (attached)
echo "Test 2: Option with attached argument"
echo "Command: getopts \"f:\" opt -fvalue; echo \"\$opt|\$OPTARG\""
result=$(echo 'getopts "f:" opt -fvalue; echo "$opt|$OPTARG"' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: 'f|value'"
echo

# Test 3: Multiple separate commands
echo "Test 3: Multiple getopts calls"
echo "Command 1: getopts \"ab\" opt1 -a"
result1=$(echo 'getopts "ab" opt1 -a; echo "$opt1"' | ./builddir/lusush 2>/dev/null)
echo "Result 1: '$result1'"
echo "Command 2: getopts \"ab\" opt2 -b"
result2=$(echo 'getopts "ab" opt2 -b; echo "$opt2"' | ./builddir/lusush 2>/dev/null)
echo "Result 2: '$result2'"
echo

# Test 4: OPTIND behavior
echo "Test 4: OPTIND behavior"
echo "Command: echo \$OPTIND; getopts \"a\" opt -a; echo \$OPTIND"
result=$(echo 'echo $OPTIND; getopts "a" opt -a; echo $OPTIND' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: Initially unset/1, then 2"
echo

# Test 5: Silent mode
echo "Test 5: Silent mode with invalid option"
echo "Command: getopts \":abc\" opt -z; echo \"\$opt|\$OPTARG\""
result=$(echo 'getopts ":abc" opt -z; echo "$opt|$OPTARG"' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: '?|z'"
echo

# Test 6: Return codes
echo "Test 6: Return codes"
echo "Command: getopts \"a\" opt -a; echo \$?"
result=$(echo 'getopts "a" opt -a; echo $?' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: '0'"
echo

echo "Command: getopts \"a\" opt; echo \$?"
result=$(echo 'getopts "a" opt; echo $?' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: '1'"
echo

# Test 7: End of options
echo "Test 7: End of options (--)"
echo "Command: getopts \"a\" opt -a -- -b; echo \"\$opt|\$?\""
result=$(echo 'getopts "a" opt -a -- -b; echo "$opt|$?"' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: 'a|0'"
echo

# Test 8: Variable setting mechanism
echo "Test 8: Variable setting check"
echo "Command: getopts \"a\" myvar -a; echo \"myvar=\$myvar\""
result=$(echo 'getopts "a" myvar -a; echo "myvar=$myvar"' | ./builddir/lusush 2>/dev/null)
echo "Result: '$result'"
echo "Expected: 'myvar=a'"
echo

# Test 9: Basic functionality verification
echo "Test 9: Most basic case"
echo "Command: getopts \"a\" opt -a"
echo 'getopts "a" opt -a' | ./builddir/lusush 2>&1
echo "Exit code: $?"
echo

echo "=== DEBUG COMPLETE ==="
echo "This will help identify which specific aspects need fixing"

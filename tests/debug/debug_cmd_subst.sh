#!/bin/bash

echo "=== Command Substitution Debug Script ==="
echo

echo "Test 1: Simple command substitution (should work)"
echo 'echo $(echo hello)' | ./builddir/lusush
echo

echo "Test 2: Command substitution with variables (currently failing)"
echo 'cmd=echo; arg=hello; echo $(${cmd} ${arg})' | ./builddir/lusush
echo

echo "Test 3: Let's see what variables are set"
echo 'cmd=echo; arg=hello; echo "cmd=$cmd arg=$arg"' | ./builddir/lusush
echo

echo "Test 4: Test variable expansion outside command substitution"
echo 'cmd=echo; arg=hello; ${cmd} ${arg}' | ./builddir/lusush
echo

echo "Test 5: Test with simpler variable expansion"
echo 'cmd=echo; arg=hello; echo $cmd $arg' | ./builddir/lusush
echo

echo "Test 6: Test direct command in substitution"
echo 'echo $(echo hello)' | ./builddir/lusush
echo

echo "Test 7: Test with /bin/sh to see expected behavior"
echo "Expected behavior with /bin/sh:"
echo 'cmd=echo; arg=hello; echo $(${cmd} ${arg})' | /bin/sh
echo

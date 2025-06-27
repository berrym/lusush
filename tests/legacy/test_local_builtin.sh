#!/bin/bash

echo "=== Local Builtin Test Script ==="
echo

echo "Test 1: Basic local variable declaration"
echo 'func() { local myvar=test; echo $myvar; }; func; echo ${myvar:-unset}' | ./builddir/lusush
echo

echo "Test 2: Local variable shadows global variable"
echo 'global=outside; func() { local global=inside; echo $global; }; func; echo $global' | ./builddir/lusush
echo

echo "Test 3: Local variable without assignment"
echo 'func() { local myvar; myvar=assigned; echo $myvar; }; func; echo ${myvar:-unset}' | ./builddir/lusush
echo

echo "Test 4: Multiple local variables"
echo 'func() { local a=1 b=2 c=3; echo "$a $b $c"; }; func; echo "${a:-unset} ${b:-unset} ${c:-unset}"' | ./builddir/lusush
echo

echo "Test 5: Local command outside function (should fail)"
echo 'local test=fail 2>&1' | ./builddir/lusush
echo

echo "Test 6: The original Test 79 scenario"
echo 'global=outside; func() { local local=inside; echo $local; }; func; echo ${local:-unset}' | ./builddir/lusush
echo

echo "Test 7: Nested function scopes"
echo 'outer() { local var=outer; inner() { local var=inner; echo "inner: $var"; }; inner; echo "outer: $var"; }; outer; echo "global: ${var:-unset}"' | ./builddir/lusush
echo

echo "Test 8: Local variable with empty value"
echo 'func() { local empty=; echo "empty: ${empty:-default}"; }; func; echo "global: ${empty:-unset}"' | ./builddir/lusush
echo

echo "Test 9: Invalid variable names"
echo 'func() { local 123invalid=test; }; func 2>&1' | ./builddir/lusush
echo

echo "Test 10: Local with no arguments"
echo 'func() { local; echo "local with no args"; }; func' | ./builddir/lusush
echo

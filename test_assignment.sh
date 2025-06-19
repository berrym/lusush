#!/bin/bash

echo "=== VARIABLE ASSIGNMENT TEST ==="
echo

echo "1. Basic assignments:"
simple_var=hello
echo "  simple_var = $simple_var"

quoted_var="hello world"
echo "  quoted_var = $quoted_var"

number_var=42
echo "  number_var = $number_var"

echo
echo "2. Variable expansion:"
name=lusush
echo "  Welcome to $name shell"
echo "  Shell name length: ${#name}"

echo
echo "3. Parameter expansion:"
echo "  Default value: ${unset_var:-default_value}"
echo "  Assignment: ${assign_var:=assigned_value}"
echo "  After assignment: assign_var = $assign_var"

set_var=test_value
echo "  Alternate: ${set_var:+alternate_value}"
echo "  Unset alternate: ${unset_var2:+wont_show}"

echo
echo "4. Command substitution in assignments:"
current_date=$(date +%Y-%m-%d)
echo "  current_date = $current_date"

file_count=$(ls *.md | wc -l)
echo "  markdown files count = $file_count"

echo
echo "5. Assignments in control structures:"

echo "Assignment in if statement:"
if true; then
    if_var=IF_VALUE
    if_num=123
fi
echo "  if_var = $if_var, if_num = $if_num"

echo "Assignment in for loop:"
for item in A B; do
    loop_var=$item
    loop_counter=done
done
echo "  loop_var = $loop_var, loop_counter = $loop_counter"

echo "Assignment in while loop:"
while_count=0
while [ "$while_count" -lt 2 ]; do
    while_var=WHILE_$while_count
    while_count=$((while_count + 1))
done
echo "  while_var = $while_var"

echo
echo "6. Multiple assignments:"
var_a=A; var_b=B; var_c=C
echo "  Multiple vars: $var_a $var_b $var_c"

echo
echo "=== VARIABLE ASSIGNMENT TEST COMPLETE ==="

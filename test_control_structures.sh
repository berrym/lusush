#!/bin/bash

echo "=== CONTROL STRUCTURES COMPREHENSIVE TEST ==="
echo

echo "1. IF/THEN/ELSE statements:"
echo

echo "Basic if statement:"
if true; then
    echo "  Basic if works"
fi

echo "If-else statement:"
if false; then
    echo "  This should not print"
else
    echo "  Else clause works"
fi

echo "If-elif-else statement:"
value=2
if [ "$value" = "1" ]; then
    echo "  Value is 1"
elif [ "$value" = "2" ]; then
    echo "  Value is 2 (elif works)"
else
    echo "  Value is something else"
fi

echo
echo "2. FOR loops:"
echo

echo "Basic for loop:"
for i in 1 2 3; do
    echo "  Iteration: $i"
done

echo "For loop with files:"
for file in *.md; do
    echo "  Found file: $file"
    break  # Just show first one
done

echo
echo "3. WHILE loops:"
echo

echo "Basic while loop:"
counter=1
while [ "$counter" -le 3 ]; do
    echo "  While iteration: $counter"
    counter=$((counter + 1))
done

echo
echo "4. Multi-command bodies:"
echo

echo "IF with multiple commands:"
if true; then
    var1=FIRST
    var2=SECOND
    echo "  Set var1=$var1 and var2=$var2"
fi

echo "FOR with multiple commands:"
for num in 5 6; do
    result=$((num * 2))
    echo "  $num * 2 = $result"
done

echo "WHILE with multiple commands:"
count=1
while [ "$count" -le 2 ]; do
    echo "  Processing count: $count"
    count=$((count + 1))
    status="processed"
done
echo "  Final status: $status"

echo
echo "5. Mixed separators:"
echo

echo "Semicolon separators in if:"
if true; then
    semi_a=A; semi_b=B; echo "  Semi vars: $semi_a $semi_b"
fi

echo "Mixed separators in for:"
for item in single; do
    mixed_var=MIXED; echo "  Mixed: $mixed_var"
    final_step=DONE
done
echo "  Final step: $final_step"

echo
echo "=== CONTROL STRUCTURES TEST COMPLETE ==="

#!/usr/bin/env lusush

echo "Simple Documentation Test"
echo "========================="

# Test 1: Basic for loop
echo "Test 1: Basic for loop"
for i in 1 2 3; do
    echo "Number: $i"
done
echo

# Test 2: Variable persistence in for loop
echo "Test 2: Variable persistence in for loop"
result=0
echo "Initial result: $result"
for i in 1 2 3; do
    result=$((result + i))
    echo "Inside loop: result=$result, i=$i"
done
echo "Final result: $result"
echo

# Test 3: Variable persistence in while loop
echo "Test 3: Variable persistence in while loop"
result=0
i=1
echo "Initial result: $result"
while [ $i -le 3 ]; do
    result=$((result + i))
    echo "Inside loop: result=$result, i=$i"
    i=$((i + 1))
done
echo "Final result: $result"
echo

# Test 4: Parameter expansion
echo "Test 4: Parameter expansion"
filename="/path/to/file.txt"
echo "Directory: ${filename%/*}"
echo "Basename: ${filename##*/}"
echo "Extension: ${filename##*.}"
echo

# Test 5: String length
echo "Test 5: String length"
text="hello"
echo "Length: ${#text}"
echo

echo "Test completed."

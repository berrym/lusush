#!/usr/bin/env lusush

echo "Starting debug test script"

x=5
y=10

echo "Setting variables: x=$x, y=$y"

for i in 1 2 3; do
    echo "Loop iteration $i"
    result=$((x + y + i))
    echo "Result: $result"
done

echo "Finished debug test script"

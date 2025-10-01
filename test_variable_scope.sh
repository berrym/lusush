#!/usr/bin/env lusush
echo "Testing variable scope..."
result=0
echo "Initial result: $result"
for i in 1 2 3; do
    echo "Processing $i"
    result=$(($result + $i))
    echo "Inside loop result: $result"
done
echo "Final result: $result"

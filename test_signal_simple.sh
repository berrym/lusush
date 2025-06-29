#!/bin/bash

echo "Testing SIGINT handling in lusush..."

# Test 1: Send SIGINT to shell with no active process - should not exit
echo "Test 1: SIGINT to shell with no active process"
echo "echo 'Before SIGINT'; sleep 1; echo 'After sleep'" | timeout 10 bash -c '
    ./builddir/lusush &
    SHELL_PID=$!
    sleep 2
    kill -INT $SHELL_PID
    sleep 1
    if kill -0 $SHELL_PID 2>/dev/null; then
        echo "✓ Shell survived SIGINT (correct behavior)"
        kill $SHELL_PID
        exit 0
    else
        echo "✗ Shell exited on SIGINT (incorrect behavior)"
        exit 1
    fi
'

# Test 2: Test that shell can execute commands normally
echo "Test 2: Normal command execution"
RESULT=$(echo "echo 'Hello World'" | ./builddir/lusush)
if [ "$RESULT" = "Hello World" ]; then
    echo "✓ Normal command execution works"
else
    echo "✗ Normal command execution failed"
    exit 1
fi

# Test 3: Test exit command still works
echo "Test 3: Exit command functionality"
echo "exit" | timeout 5 ./builddir/lusush
if [ $? -eq 0 ]; then
    echo "✓ Exit command works correctly"
else
    echo "✗ Exit command failed"
    exit 1
fi

echo "All signal handling tests passed!"

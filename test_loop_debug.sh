#!/bin/bash

# Test script to reproduce the loop debug bug
# This creates the test files and runs the failing case

echo "Creating test loop script..."
cat > simple_loop_test.sh << 'EOF'
#!/usr/bin/env lusush
for i in 1 2 3; do
    echo "Iteration: $i"
done
EOF

echo "Test script created. Now testing the bug..."
echo ""
echo "Expected behavior: Should show 'Iteration: 1', 'Iteration: 2', 'Iteration: 3'"
echo "Actual behavior: Will show empty values and 'DEBUG: Unhandled keyword type 46 (DONE)'"
echo ""

echo "Running test command:"
echo "echo 'debug on; debug break add simple_loop_test.sh 3; source simple_loop_test.sh' | ./builddir/lusush"
echo ""

# Run the failing test case
echo 'debug on; debug break add simple_loop_test.sh 3; source simple_loop_test.sh' | ./builddir/lusush

echo ""
echo "Test completed. If you see the DONE error above, the bug is present."

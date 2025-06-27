#!/bin/bash

# Test script for positional parameters implementation
# This script tests the basic POSIX positional parameter functionality

echo "=== LUSUSH POSITIONAL PARAMETERS TEST ==="
echo ""

# Build the shell
echo "Building lusush..."
cd builddir && ninja && cd ..
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo ""

# Create a test script for positional parameters
cat > test_script.sh << 'EOF'
#!/usr/bin/env ./builddir/lusush

echo "Script name: $0"
echo "First argument: $1"
echo "Second argument: $2"
echo "Third argument: $3"
echo "Number of arguments: $#"
echo "All arguments (\$*): $*"
echo "All arguments (\$@): $@"
echo "Exit status of last command: $?"
echo "Shell PID: $$"
echo "Background PID: $!"
EOF

chmod +x test_script.sh

echo "=== TEST 1: Basic positional parameters ==="
echo "Running: ./test_script.sh hello world test"
./test_script.sh hello world test
echo ""

echo "=== TEST 2: No arguments ==="
echo "Running: ./test_script.sh"
./test_script.sh
echo ""

echo "=== TEST 3: Single argument ==="
echo "Running: ./test_script.sh single"
./test_script.sh single
echo ""

echo "=== TEST 4: Special variables in interactive mode ==="
echo "Testing special variables directly..."
echo 'echo "Exit status: $?"' | ./builddir/lusush
echo 'echo "Shell PID: $$"' | ./builddir/lusush
echo 'echo "Script name: $0"' | ./builddir/lusush
echo 'echo "Arg count: $#"' | ./builddir/lusush
echo ""

echo "=== TEST 5: Background job testing ==="
echo "Testing background PID variable..."
echo 'sleep 1 & echo "Background PID: $!"' | ./builddir/lusush
echo ""

echo "=== TEST 6: Command line with -c option ==="
echo "Testing -c option with positional parameters..."
./builddir/lusush -c 'echo "In -c mode: $0 $# args"'
echo ""

# Cleanup
rm -f test_script.sh

echo "=== POSITIONAL PARAMETERS TEST COMPLETE ==="

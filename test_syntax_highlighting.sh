#!/bin/bash

# Test script for Lusush syntax highlighting
# This script will test the visual syntax highlighting implementation

echo "=== Lusush Syntax Highlighting Test ==="
echo

# Build the shell
echo "Building Lusush..."
cd "$(dirname "$0")"
ninja -C builddir

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo

# Test syntax highlighting with various commands
echo "Testing syntax highlighting..."
echo "You should see colored output for different syntax elements:"
echo "- Commands in GREEN"
echo "- Keywords in BLUE"
echo "- Strings in YELLOW"
echo "- Variables in MAGENTA"
echo "- Operators in RED"
echo "- Comments in GRAY"
echo

# Create a test input file with various syntax elements
cat > test_input.tmp << 'EOF'
echo "hello world"
if test -f /etc/passwd; then echo "file exists"; fi
ls -la | grep "\.txt" | sort
export MY_VAR="test value"
echo $HOME
# This is a comment
for i in 1 2 3; do echo $i; done
cd /tmp && ls
EOF

echo "Running interactive test (press Ctrl+C to exit)..."
echo "Type these commands to see syntax highlighting:"
echo
cat test_input.tmp
echo
echo "Starting Lusush shell..."

# Run the shell interactively
./builddir/lusush

# Clean up
rm -f test_input.tmp

echo
echo "Test completed!"

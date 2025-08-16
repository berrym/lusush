#!/bin/bash

# Lusush Base Terminal Layer Test Script
# Compiles and runs the base terminal unit tests

set -e  # Exit on any error

echo "Lusush Base Terminal Layer Test Script"
echo "======================================"

# Check if we're in the right directory
if [ ! -f "src/display/base_terminal.c" ]; then
    echo "Error: Must be run from the lusush root directory"
    exit 1
fi

# Ensure build directory exists
if [ ! -d "builddir" ]; then
    echo "Setting up build directory..."
    meson setup builddir
fi

# Build the main project first to ensure all dependencies are ready
echo "Building lusush..."
ninja -C builddir

# Create test directory if it doesn't exist
mkdir -p tests/display

# Compile the test program
echo "Compiling base terminal tests..."
gcc -std=c99 -Wall -Wextra -g \
    -I include \
    -I include/display \
    -D_DEFAULT_SOURCE \
    -D_XOPEN_SOURCE=700 \
    -D_XOPEN_SOURCE_EXTENDED \
    tests/test_base_terminal.c \
    src/display/base_terminal.c \
    -o tests/display/test_base_terminal

echo "Compilation successful!"
echo ""

# Run the tests
echo "Running base terminal tests..."
echo "=============================="
./tests/display/test_base_terminal

exit_code=$?

echo ""
if [ $exit_code -eq 0 ]; then
    echo "✅ All tests passed successfully!"
    echo ""
    echo "Base Terminal Layer (Layer 1) is ready for production!"
    echo "Next step: Implement Layer 2 (Terminal Control)"
else
    echo "❌ Some tests failed (exit code: $exit_code)"
    echo "Please review the test output above and fix any issues."
fi

exit $exit_code

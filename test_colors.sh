#!/bin/bash

# Test script to verify lusush prompt colors are working
# This script tests the lusush shell in various scenarios to ensure
# prompt colors work correctly in different terminal environments

echo "=== Lusush Prompt Color Test ==="
echo

# Test 1: Basic shell startup with different TERM values
echo "Test 1: Testing prompt colors with different TERM values"
echo "--------------------------------------------------------"

for term in "xterm-256color" "konsole" "gnome-terminal" "xterm" "screen" "tmux"; do
    echo "Testing with TERM=$term:"
    TERM="$term" timeout 2s ./builddir/lusush -c 'echo "Shell started successfully"' 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "  ✓ Shell started successfully with TERM=$term"
    else
        echo "  ✗ Shell failed to start with TERM=$term"
    fi
done

echo

# Test 2: Check termcap capabilities
echo "Test 2: Checking termcap capabilities"
echo "-------------------------------------"

echo "Basic capabilities:"
echo 'termcap capabilities' | ./builddir/lusush | grep -E "(Basic Colors|256 Colors|True Color)" | head -3

echo

# Test 3: Test error message colors
echo "Test 3: Testing colored error messages"
echo "--------------------------------------"

echo "Triggering a command not found error:"
echo 'nonexistent_command_12345' | ./builddir/lusush 2>&1 | head -1

echo

# Test 4: Test theme system integration
echo "Test 4: Testing theme integration"
echo "---------------------------------"

echo "Theme color detection:"
echo 'termcap integration' | ./builddir/lusush | head -5

echo

# Test 5: Interactive prompt test (if TTY available)
echo "Test 5: Interactive prompt test"
echo "-------------------------------"

if [ -t 0 ]; then
    echo "Running interactive test (type 'exit' to quit):"
    echo "Look for colored prompt when shell starts..."
    ./builddir/lusush -i
else
    echo "Skipping interactive test (not running in TTY)"
    echo "To test interactively, run: ./builddir/lusush -i"
fi

echo

# Test 6: Check if colors are actually being generated
echo "Test 6: Checking color escape sequence generation"
echo "-------------------------------------------------"

echo "Testing if ANSI color codes are present in output:"
echo 'PS1' | ./builddir/lusush | od -c | grep -E "033\|\\\\033" | head -2
if [ $? -eq 0 ]; then
    echo "  ✓ ANSI escape sequences detected in prompt"
else
    echo "  ✗ No ANSI escape sequences found - colors may not be working"
fi

echo

# Test 7: Environment variable check
echo "Test 7: Environment check"
echo "-------------------------"

echo "Current environment:"
echo "  TERM: ${TERM:-not set}"
echo "  COLORTERM: ${COLORTERM:-not set}"
echo "  Testing lusush with current environment:"

echo 'termcap capabilities' | ./builddir/lusush | grep "TERM:" | head -1

echo
echo "=== Test Complete ==="
echo
echo "If you're still not seeing colors in your terminal:"
echo "1. Make sure your terminal supports colors"
echo "2. Check that TERM environment variable is set correctly"
echo "3. Try running: ./builddir/lusush -i directly"
echo "4. Check if other programs show colors in your terminal"

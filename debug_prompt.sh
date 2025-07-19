#!/bin/bash

# Debug script to test lusush prompt generation
# This helps diagnose why prompt colors might not be working

echo "=== Lusush Prompt Debug Test ==="
echo

# Function to test prompt generation
test_prompt() {
    local term_val="$1"
    local test_name="$2"

    echo "Testing: $test_name (TERM=$term_val)"
    echo "----------------------------------------"

    # Test basic shell startup and prompt generation
    export TERM="$term_val"

    # Create a simple test script that shows PS1
    cat > /tmp/lusush_test.sh << 'EOF'
echo "Current PS1: $PS1"
echo "TERM: $TERM"
echo "Colors supported check:"
if [ -n "$PS1" ]; then
    echo "PS1 length: ${#PS1}"
    echo "PS1 contains escape sequences:"
    echo "$PS1" | od -c | head -3
else
    echo "PS1 is empty or not set"
fi
exit
EOF

    # Run the test
    echo "Running lusush with debug script..."
    timeout 5s ./builddir/lusush /tmp/lusush_test.sh 2>&1

    echo
    echo "Direct prompt check:"
    echo 'echo "PS1: $PS1"' | timeout 2s ./builddir/lusush 2>/dev/null

    echo
    echo "----------------------------------------"
    echo
}

# Test with different terminal types
test_prompt "xterm-256color" "Standard xterm with 256 colors"
test_prompt "konsole" "KDE Konsole terminal"
test_prompt "gnome-terminal" "GNOME Terminal"
test_prompt "xterm" "Basic xterm"

# Test current environment
echo "Testing with current environment:"
echo "TERM: ${TERM:-not set}"
echo "COLORTERM: ${COLORTERM:-not set}"
echo

# Test interactive shell detection
echo "=== Interactive Shell Test ==="
echo "Testing if lusush detects as interactive..."

cat > /tmp/lusush_interactive_test.sh << 'EOF'
echo "Interactive shell: $-"
echo "TTY check: $(tty 2>/dev/null || echo 'not a tty')"
echo "PS1 set: ${PS1:+yes}"
if [ -n "$PS1" ]; then
    echo "PS1 preview: $PS1"
    echo "PS1 raw bytes:"
    echo "$PS1" | od -c | head -2
fi
exit
EOF

echo "Running interactive test..."
./builddir/lusush /tmp/lusush_interactive_test.sh

echo
echo "=== Color Capability Test ==="
echo "Testing termcap color detection..."

echo 'termcap capabilities | grep -A 20 "Color Capabilities"' | ./builddir/lusush

echo
echo "=== Theme System Test ==="
echo "Testing theme color support..."

echo 'termcap integration | head -10' | ./builddir/lusush

echo
echo "=== Manual Color Test ==="
echo "Testing manual color output..."

# Test if we can generate colors manually
cat > /tmp/color_test.sh << 'EOF'
echo "Manual ANSI color test:"
echo -e "\033[31mRed text\033[0m"
echo -e "\033[32mGreen text\033[0m"
echo -e "\033[34mBlue text\033[0m"
echo "If you see colors above, your terminal supports them."
EOF

bash /tmp/color_test.sh

echo
echo "=== Debug Summary ==="
echo "1. Check if PS1 is being set with escape sequences"
echo "2. Verify TERM environment variable is correct"
echo "3. Check if termcap reports color support"
echo "4. Test if manual ANSI codes work in your terminal"
echo

# Cleanup
rm -f /tmp/lusush_test.sh /tmp/lusush_interactive_test.sh /tmp/color_test.sh

echo "Debug test complete."
echo
echo "If PS1 contains escape sequences but you don't see colors:"
echo "- Your terminal may not be interpreting ANSI codes"
echo "- Check terminal settings for color support"
echo "- Try: export TERM=xterm-256color before running lusush"

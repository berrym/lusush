#!/bin/bash

# Lusush Interactive Mode Issue Diagnosis Using Script Command
# This uses script to create a pseudo-TTY for true interactive testing

echo "=== Lusush Interactive Mode Issue Diagnosis ==="
echo "Using script command to create pseudo-TTY for proper interactive testing"
echo ""

# Build if needed
if [ ! -f builddir/lusush ]; then
    echo "Building lusush..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "=== PHASE 1: Baseline Non-Interactive Test ==="
echo ""

echo "Testing pipes in non-interactive mode (should work):"
echo 'echo "hello world" | grep hello' | ./builddir/lusush
echo ""

echo "Testing redirection in non-interactive mode (should work):"
echo 'echo "test" > /tmp/test.txt && cat /tmp/test.txt && rm /tmp/test.txt' | ./builddir/lusush
echo ""

echo "=== PHASE 2: Interactive Mode Issues Using Script ==="
echo ""

echo "Now testing each critical issue using script command for pseudo-TTY..."
echo ""

echo "ISSUE #1: History Navigation"
echo "Creating test script for history navigation..."

cat > /tmp/test_history.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

# Start lusush with script for pseudo-TTY
spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Enter first command
send "echo \"command 1\"\r"
expect "$ "

# Enter second command
send "echo \"command 2\"\r"
expect "$ "

# Test UP arrow - this should show "echo \"command 2\"" not completion menu
send "\033\[A"

# Wait a moment to see what happens
sleep 1

# Send Ctrl+C to try to clear/exit
send "\003"

# Wait for response
sleep 1

# Try to exit gracefully
send "exit\r"

# Wait for completion
expect eof
EOF

echo "Running history navigation test..."
if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/test_history.exp
    /tmp/test_history.exp
else
    echo "expect not available, using manual script test..."

    # Create input script for manual testing
    echo "Creating input sequence for manual testing..."
    cat > /tmp/manual_test_input << 'EOF'
echo "command 1"
echo "command 2"
# Now press UP arrow manually - should show "echo command 2", not completion menu
# Press Ctrl+C - should clear line, not exit shell
exit
EOF

    echo "Manual test input created. Running with script..."
    echo "Watch for the debug output to see what happens with arrow keys and Ctrl+C"
    echo ""
    script -q -c "./builddir/lusush" /dev/null < /tmp/manual_test_input
fi

echo ""
echo "ISSUE #2: Signal Handling Direct Test"
echo "Testing Ctrl+C behavior with a simple command..."

cat > /tmp/test_signals.sh << 'EOF'
#!/bin/bash
echo "Testing signal handling in lusush with script pseudo-TTY..."

# Use script to create pseudo-TTY and test signal handling
script -q -c 'echo "Type some text and press Ctrl+C to test signal handling. Shell should NOT exit." && ./builddir/lusush' /dev/null
EOF

chmod +x /tmp/test_signals.sh
echo "Running signal handling test..."
# /tmp/test_signals.sh

echo ""
echo "ISSUE #3: Interactive Operations Test"
echo "Testing pipes and redirections in interactive mode..."

cat > /tmp/test_operations.sh << 'EOF'
#!/bin/bash

echo "Testing interactive operations with script pseudo-TTY..."

# Create a sequence of commands to test
cat > /tmp/commands_to_test << 'COMMANDS'
echo hello | grep h
echo "test content" > /tmp/interactive_test.txt
cat /tmp/interactive_test.txt
echo "User: $(whoami)"
ls | head -3
echo done
exit
COMMANDS

echo "Running operations test..."
script -q -c "./builddir/lusush" /dev/null < /tmp/commands_to_test

rm -f /tmp/commands_to_test /tmp/interactive_test.txt
EOF

chmod +x /tmp/test_operations.sh
/tmp/test_operations.sh

echo ""
echo "=== DETAILED DEBUG ANALYSIS ==="
echo ""

echo "Creating comprehensive debug test..."

cat > /tmp/debug_comprehensive.sh << 'EOF'
#!/bin/bash

echo "=== Comprehensive Debug Test with Script Pseudo-TTY ==="

# Test with full debugging enabled
export LUSUSH_DEBUG=1

echo "Running lusush with comprehensive debugging..."
echo "This will show exactly what happens in true interactive mode:"

# Create comprehensive test sequence
cat > /tmp/debug_commands << 'DEBUGCMD'
echo "=== Debug Test Sequence ==="
echo "First command"
echo "Second command"
echo "Third command"
echo "Now testing UP arrow navigation..."
echo "Type UP arrow after this prompt appears"
exit
DEBUGCMD

echo "Starting debug session with script pseudo-TTY..."
script -q -c "./builddir/lusush" /dev/null < /tmp/debug_commands

rm -f /tmp/debug_commands
EOF

chmod +x /tmp/debug_comprehensive.sh
/tmp/debug_comprehensive.sh

echo ""
echo "=== RESULTS SUMMARY ==="
echo ""

echo "📊 DIAGNOSTIC RESULTS:"
echo ""
echo "1. DEBUG OUTPUT ANALYSIS:"
echo "   - Interactive mode detection: Look for '[DEBUG] Interactive mode = TRUE'"
echo "   - Signal setup: Look for '[SIGNAL_DEBUG] INTERACTIVE mode'"
echo "   - Key bindings: Look for '[DEBUG] Key bindings setup complete'"
echo "   - Completion conflicts: Look for '[DEBUG] WARNING: Completion setup may override'"
echo ""

echo "2. BEHAVIORAL ANALYSIS:"
echo "   - History navigation: Did UP arrow work or show completion menu?"
echo "   - Signal handling: Did Ctrl+C exit shell or clear line?"
echo "   - Operations: Did pipes/redirections work in interactive mode?"
echo ""

echo "3. ROOT CAUSE INDICATORS:"
echo "   - If completion debug triggers on arrow keys: history binding issue"
echo "   - If shell exits on Ctrl+C: signal handling issue"
echo "   - If operations fail differently than non-interactive: execution pipeline issue"
echo ""

echo "📝 NEXT STEPS BASED ON RESULTS:"
echo ""
echo "If history navigation shows completion menu:"
echo "   → Fix: Remove arrow key completion bindings in src/readline_integration.c"
echo ""
echo "If Ctrl+C exits shell:"
echo "   → Fix: Modify signal handlers to clear line instead of exit"
echo ""
echo "If operations fail in interactive mode:"
echo "   → Fix: Debug execution pipeline differences between modes"
echo ""

# Cleanup
rm -f /tmp/test_history.exp /tmp/test_signals.sh /tmp/test_operations.sh /tmp/debug_comprehensive.sh

echo "Use this diagnostic information to implement targeted fixes."

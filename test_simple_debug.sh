#!/bin/bash

# Simple debug test for multiline clearing issues
# Tests the specific prompt stacking problem with debug output

echo "Simple Debug Test for Multiline Clearing"
echo "========================================"

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ Error: ./builddir/lusush not found."
    exit 1
fi

echo "✅ Binary found: ./builddir/lusush"

# Clean debug log
rm -f /tmp/lndebug.txt
echo "🧹 Cleaned debug log"
echo ""

echo "SIMPLE TEST SEQUENCE:"
echo "===================="
echo ""

echo "This will test the dark theme prompt stacking issue:"
echo "1. Start lusush"
echo "2. Set dark theme"
echo "3. Add a few commands"
echo "4. Navigate history with UP/DOWN"
echo "5. Check debug output"
echo ""

# Create simple expect script
cat > /tmp/simple_test.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 5

spawn ./builddir/lusush -i
expect "lusush"

# Set dark theme
send "theme set dark\r"
expect "lusush"

# Add commands
send "echo cmd1\r"
expect "lusush"
send "echo cmd2\r"
expect "lusush"

# Navigate history
puts "Testing history navigation..."
send "\033\[A"
sleep 1
send "\033\[A"
sleep 1
send "\033\[B"
sleep 1

send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/simple_test.exp
    echo "Running simple test..."
    /tmp/simple_test.exp
    rm -f /tmp/simple_test.exp
else
    echo "❌ Expect not available"
    exit 1
fi

echo ""
echo "DEBUG OUTPUT:"
echo "============="

if [ -f "/tmp/lndebug.txt" ]; then
    echo "Debug entries found:"
    cat /tmp/lndebug.txt
    echo ""
    echo "Clear operations:"
    grep -c "clear" /tmp/lndebug.txt || echo "0"
else
    echo "❌ No debug log found"
fi

echo ""
echo "ANALYSIS:"
echo "========="
echo "If prompt stacking occurs, we need to investigate:"
echo "1. Whether oldrows is being set correctly"
echo "2. Whether our clearing logic is being executed"
echo "3. Whether the clearing escape sequences are correct"
echo ""
echo "Manual test: ./builddir/lusush -i"
echo "Then: theme set dark, add commands, use UP/DOWN arrows"

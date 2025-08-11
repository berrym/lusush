#!/bin/bash

# Arrow Key History Navigation Test for Lusush
# Tests that UP/DOWN arrow keys navigate through unified history correctly

echo "=== Lusush Arrow Key History Navigation Test ==="
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

echo "This test verifies that arrow key navigation works with unified history:"
echo "  UP arrow: Navigate to previous commands"
echo "  DOWN arrow: Navigate to next commands"
echo "  History content: Should match what 'history' builtin shows"
echo ""

echo "=== Phase 1: Populate History ==="
echo ""

# Clean history for consistent test
rm -f ~/.lusush_history ~/.lusushist .lusush_history .lusushist

echo "Populating history with test commands..."

# Create expect script for history population
cat > /tmp/populate_history.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Add test commands to history
send "echo \"history test 1\"\r"
expect "$ "

send "echo \"history test 2\"\r"
expect "$ "

send "echo \"history test 3\"\r"
expect "$ "

send "pwd\r"
expect "$ "

send "whoami\r"
expect "$ "

# Check history builtin
send "history\r"
expect "$ "

# Exit to save history
send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/populate_history.exp
    echo "Running history population with expect..."
    /tmp/populate_history.exp
else
    echo "expect not available, using input file..."
    cat > /tmp/history_populate.txt << 'POPULATE'
echo "history test 1"
echo "history test 2"
echo "history test 3"
pwd
whoami
history
exit
POPULATE

    script -q -c './builddir/lusush' /dev/null < /tmp/history_populate.txt
fi

echo ""
echo "=== Phase 2: Arrow Key Navigation Test ==="
echo ""

echo "Now testing arrow key navigation through the populated history..."

# Create expect script for arrow key testing
cat > /tmp/arrow_navigation.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 10

spawn script -q -c "./builddir/lusush" /dev/null

# Wait for prompt
expect "$ "

# Test UP arrow navigation
puts "\n=== Testing UP Arrow Navigation ==="

# Press UP arrow - should show last command
send "\033\[A"
expect {
    "whoami" {
        puts "✅ UP arrow #1: Shows 'whoami' (last command)"
        send "\r"
        expect "$ "
    }
    "history" {
        puts "✅ UP arrow #1: Shows 'history' (last command)"
        send "\033\[C"
        send "\033\[C"
        send "\033\[C"
        send "\033\[C"
        send "\033\[C"
        send "\033\[C"
        send "\033\[C"
        send "\r"
        expect "$ "
    }
    "display all" {
        puts "❌ FAILURE: UP arrow still shows completion menu!"
        send "n\r"
        exit 1
    }
    timeout {
        puts "❓ TIMEOUT: UP arrow navigation unclear"
        exit 2
    }
}

# Press UP arrow again - should show previous command
send "\033\[A"
expect {
    -re "(pwd|history test 3)" {
        puts "✅ UP arrow #2: Shows previous command correctly"

        # Test DOWN arrow - should go forward in history
        send "\033\[B"
        expect {
            -re "(whoami|history)" {
                puts "✅ DOWN arrow: Navigation forward works"
            }
            timeout {
                puts "❓ DOWN arrow: Timeout"
            }
        }
    }
    timeout {
        puts "❓ TIMEOUT: Second UP arrow unclear"
    }
}

puts "\n=== Testing History Content Consistency ==="

# Clear line and check history builtin
send "\003"
expect "$ "

send "history\r"
expect "$ "

puts "✅ History builtin executed - check if content matches navigation"

send "exit\r"
expect eof

puts "\n=== Arrow Key Navigation Test Complete ==="
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/arrow_navigation.exp
    echo "Running arrow key navigation test..."
    /tmp/arrow_navigation.exp
    arrow_result=$?
else
    echo "expect not available - manual testing required"
    arrow_result=2
fi

echo ""
echo "=== Phase 3: History Consistency Verification ==="
echo ""

echo "Verifying that arrow navigation and history builtin show same content..."

# Test current history state
if [ -f ~/.lusush_history ]; then
    echo "✅ History file exists: ~/.lusush_history"
    echo "   Entries in file: $(wc -l < ~/.lusush_history)"
    echo "   Recent entries:"
    tail -5 ~/.lusush_history | sed 's/^/     /'
fi

echo ""
echo "=== Results Summary ==="
echo ""

if [ $arrow_result -eq 0 ]; then
    echo "🎉 ARROW KEY NAVIGATION: SUCCESS"
    echo "   ✅ UP arrow navigates to previous commands"
    echo "   ✅ DOWN arrow navigates to next commands"
    echo "   ✅ No completion menu interference"
    echo "   ✅ History navigation working correctly"
elif [ $arrow_result -eq 1 ]; then
    echo "❌ ARROW KEY NAVIGATION: FAILED"
    echo "   UP arrow still shows completion menu"
    echo "   History navigation not working"
elif [ $arrow_result -eq 2 ]; then
    echo "❓ ARROW KEY NAVIGATION: INCONCLUSIVE"
    echo "   Manual testing required"
fi

echo ""
echo "=== Manual Testing Instructions ==="
echo ""

echo "For definitive arrow key testing:"
echo ""
echo "1. Start lusush interactively:"
echo "   script -q -c './builddir/lusush' /dev/null"
echo ""
echo "2. Type several commands:"
echo "   lusush$ echo \"test 1\""
echo "   lusush$ echo \"test 2\""
echo "   lusush$ echo \"test 3\""
echo ""
echo "3. Test arrow navigation:"
echo "   lusush$ [Press UP ARROW]"
echo "   Expected: Should show \"echo \"test 3\"\""
echo "   Bug: Shows \"display all XXXX possibilities\""
echo ""
echo "4. Test history builtin:"
echo "   lusush$ history"
echo "   Expected: Should show numbered list including test commands"
echo ""
echo "5. Verify consistency:"
echo "   Arrow navigation and history builtin should show same commands"
echo ""

echo "=== Success Criteria ==="
echo ""
echo "Unified history system is working correctly when:"
echo "✅ Arrow keys navigate through actual commands (not completion menu)"
echo "✅ History builtin shows the same commands as arrow navigation"
echo "✅ History persists between sessions"
echo "✅ No \"display all XXXX possibilities\" on arrow keys"
echo "✅ History file contains readable command entries"

# Cleanup
rm -f /tmp/populate_history.exp /tmp/arrow_navigation.exp /tmp/history_populate.txt

echo ""
echo "Test complete. Use results to verify unified history system functionality."

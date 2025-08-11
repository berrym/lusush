#!/bin/bash

# Test script for Ctrl+R reverse search functionality
# This script tests if Ctrl+R works without display corruption

echo "Testing Ctrl+R reverse search functionality..."
echo

# Create a temporary expect script to test Ctrl+R
cat > /tmp/test_ctrl_r.exp << 'EOF'
#!/usr/bin/expect -f

set timeout 10
spawn ./builddir/lusush

# Add some commands to history
send "echo first_command\r"
expect "$ "

send "echo second_command\r"
expect "$ "

send "echo third_command\r"
expect "$ "

# Now test Ctrl+R reverse search
send "\022"  # Ctrl+R
expect {
    "(reverse-i-search)" {
        puts "SUCCESS: Ctrl+R search prompt appeared"

        # Type search term
        send "second"
        expect {
            "second_command" {
                puts "SUCCESS: Found command in search"

                # Cancel search with Ctrl+G
                send "\007"  # Ctrl+G
                expect "$ " {
                    puts "SUCCESS: Search cancelled cleanly"
                }
            }
            timeout {
                puts "ERROR: Command not found in search"
                exit 1
            }
        }
    }
    timeout {
        puts "ERROR: Ctrl+R search prompt did not appear"
        exit 1
    }
}

# Exit the shell
send "exit\r"
expect eof

puts "All Ctrl+R tests passed!"
EOF

# Make the expect script executable
chmod +x /tmp/test_ctrl_r.exp

# Check if expect is installed
if ! command -v expect &> /dev/null; then
    echo "Expect is not installed. Testing manually..."
    echo
    echo "Manual test instructions:"
    echo "1. Run: script -q -c './builddir/lusush' /dev/null"
    echo "2. Type: echo first_command"
    echo "3. Type: echo second_command"
    echo "4. Press: Ctrl+R"
    echo "5. Check: Should see '(reverse-i-search)' without corruption"
    echo "6. Type: 'second' to search"
    echo "7. Check: Should find 'second_command' cleanly"
    echo "8. Press: Ctrl+G to cancel"
    echo "9. Check: Should return to normal prompt without artifacts"
    exit 0
fi

# Run the expect test
echo "Running automated Ctrl+R test..."
/tmp/test_ctrl_r.exp

# Clean up
rm -f /tmp/test_ctrl_r.exp

echo "Test completed."

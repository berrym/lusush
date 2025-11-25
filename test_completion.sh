#!/bin/bash
# Test completion cursor positioning

echo "Testing completion cursor positioning..."
echo "Type 'e' then TAB to see completion menu"
echo "Watch if cursor moves up one row after first completion"
echo ""

# Use expect or send keys programmatically
(
    sleep 1
    echo -n "e"
    sleep 0.5
    # Send TAB character (ASCII 9)
    printf "\t"
    sleep 2
    # Send ESC to clear menu
    printf "\033"
    sleep 1
    echo "exit"
) | ./builddir/lusush 2>&1

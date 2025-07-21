#!/bin/bash

# History Navigation Multiline Test
# Specific test for the history recall bug in multiline mode

echo "History Navigation Multiline Test"
echo "================================="
echo ""
echo "This test specifically targets the history navigation issues:"
echo "1. Multiple prompt lines appearing in a stack"
echo "2. Consuming previously rendered lines per keypress"
echo "3. Cursor positioning after history navigation"
echo ""

# Build first
echo "Building project..."
cd builddir && ninja
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi
cd ..
echo "✅ Build successful"
echo ""

# Create a test script that demonstrates the history issue
cat > /tmp/history_test_script.sh << 'EOF'
#!/bin/bash

echo "History Navigation Test Script"
echo "=============================="
echo ""

# Set a multiline prompt that should wrap
export PS1='\033[1;32muser@very-long-hostname-that-definitely-wraps\033[0m:\033[1;34m~/project\033[0m$ '

echo "Prompt set to a long prompt that should wrap lines."
echo "Current prompt display:"
printf "%b" "$PS1"
echo "cursor_should_be_here"
echo ""

echo "Steps to test:"
echo "1. Type: echo 'first command'"
echo "2. Press Enter to execute"
echo "3. Type: echo 'second command'"
echo "4. Press Enter to execute"
echo "5. Type: echo 'third command'"
echo "6. Press Enter to execute"
echo ""

echo "Now test history navigation:"
echo "7. Press UP arrow - should show 'echo third command'"
echo "8. Press UP arrow - should show 'echo second command'"
echo "9. Press UP arrow - should show 'echo first command'"
echo "10. Press DOWN arrow - should show 'echo second command'"
echo "11. Press DOWN arrow - should show 'echo third command'"
echo "12. Press DOWN arrow - should show empty line"
echo ""

echo "What to watch for:"
echo "❌ BAD: Multiple prompt lines stacking up"
echo "❌ BAD: Previous lines being consumed/eaten"
echo "❌ BAD: Cursor appearing at wrong offset"
echo "❌ BAD: Visual artifacts or jumping"
echo ""
echo "✅ GOOD: Single prompt line, properly positioned"
echo "✅ GOOD: History content replaces cleanly"
echo "✅ GOOD: Cursor appears right after prompt"
echo "✅ GOOD: No visual artifacts"
echo ""

echo "Press Ctrl+D to exit this test shell"
echo ""

# Pre-populate some history for testing
history -c  # Clear current history
echo 'echo "first command"' >> ~/.bash_history
echo 'echo "second command"' >> ~/.bash_history
echo 'echo "third command"' >> ~/.bash_history
history -r  # Reload history

EOF

chmod +x /tmp/history_test_script.sh

echo "Test 1: Basic History Navigation"
echo "==============================="
echo ""
echo "Run the history test script:"
echo "   ./builddir/lusush /tmp/history_test_script.sh"
echo ""
echo "Follow the instructions in the script to test history navigation."
echo "Pay special attention to what happens when you press UP/DOWN arrows."
echo ""

# Create a simplified test for manual verification
cat > /tmp/simple_history_test.sh << 'EOF'
#!/bin/bash

# Simple test case
export PS1='\033[32mLONG_PROMPT_THAT_SHOULD_WRAP_TO_NEXT_LINE\033[0m$ '

echo "Simple History Test"
echo "=================="
echo "1. Type a few commands and press enter after each:"
echo "   echo hello"
echo "   echo world"
echo "   echo test"
echo ""
echo "2. Then use UP/DOWN arrows to navigate history"
echo "3. Watch for:"
echo "   - Prompt stacking (multiple prompts appearing)"
echo "   - Line consumption (previous lines disappearing)"
echo "   - Cursor misalignment"
echo ""
echo "Expected behavior:"
echo "   - Single prompt line"
echo "   - Clean history replacement"
echo "   - Cursor at correct position"
EOF

chmod +x /tmp/simple_history_test.sh

echo "Test 2: Simplified History Test"
echo "==============================="
echo ""
echo "Run the simple test:"
echo "   ./builddir/lusush /tmp/simple_history_test.sh"
echo ""

# Create a debug version that shows what's happening
cat > /tmp/debug_history.c << 'EOF'
#include <stdio.h>
#include <string.h>

// Test the cursor positioning math
void test_cursor_math() {
    printf("Cursor Positioning Math Test\n");
    printf("============================\n\n");

    // Test case: Long prompt that wraps
    const char* prompt = "\033[32mLONG_PROMPT_THAT_SHOULD_WRAP_TO_NEXT_LINE\033[0m$ ";
    size_t raw_length = strlen(prompt);
    size_t display_width = 47; // Approximate visual width (no ANSI codes)
    size_t terminal_width = 80;

    printf("Prompt: %s\n", prompt);
    printf("Raw length: %zu bytes\n", raw_length);
    printf("Display width: %zu columns\n", display_width);
    printf("Terminal width: %zu columns\n\n", terminal_width);

    // Test wrapping calculation
    size_t lines_used = (display_width + terminal_width - 1) / terminal_width;
    size_t last_line_width = display_width % terminal_width;

    printf("Lines used by prompt: %zu\n", lines_used);
    printf("Last line width: %zu\n", last_line_width);
    printf("Cursor should be at column: %zu\n\n", last_line_width);

    // Test with user input
    const char* user_input = "echo hello world";
    size_t input_length = strlen(user_input);
    size_t total_width = display_width + input_length;

    printf("With user input: '%s'\n", user_input);
    printf("Input length: %zu\n", input_length);
    printf("Total width: %zu\n", total_width);

    size_t total_lines = (total_width + terminal_width - 1) / terminal_width;
    size_t cursor_line = total_lines - 1;
    size_t cursor_col = total_width % terminal_width;

    printf("Total lines: %zu\n", total_lines);
    printf("Cursor should be on line: %zu (0-based)\n", cursor_line);
    printf("Cursor should be at column: %zu\n", cursor_col);
}

int main() {
    test_cursor_math();
    return 0;
}
EOF

echo "Test 3: Debug Math Verification"
echo "==============================="
echo ""
echo "Compile and run debug test:"
echo "   gcc -o /tmp/debug_history /tmp/debug_history.c && /tmp/debug_history"
echo ""

# Run the debug test immediately
gcc -o /tmp/debug_history /tmp/debug_history.c && /tmp/debug_history

echo ""
echo "Test 4: Visual Verification Commands"
echo "==================================="
echo ""
echo "Test multiline prompts in different terminals:"
echo ""
echo "# In current terminal:"
echo "./builddir/lusush"
echo "export PS1='\\033[1;32mvery-long-prompt-that-should-wrap-around\\033[0m\\$ '"
echo "# Type a few commands, then use UP/DOWN arrows"
echo ""
echo "# In Konsole:"
echo "konsole -e ./builddir/lusush"
echo ""
echo "# In foot:"
echo "foot ./builddir/lusush"
echo ""
echo "# Test different prompt lengths:"
echo "export PS1='\\033[32mSHORT\\033[0m\\$ '"  # Should work fine
echo "export PS1='\\033[32mMEDIUM_LENGTH_PROMPT\\033[0m\\$ '"  # Moderate test
echo "export PS1='\\033[32mVERY_LONG_PROMPT_THAT_DEFINITELY_EXCEEDS_NORMAL_TERMINAL_WIDTH\\033[0m\\$ '"  # Should wrap
echo ""

echo "Test 5: Regression Verification"
echo "==============================="
echo ""
echo "Verify these still work correctly:"
echo "✓ Single-line prompts (no wrapping)"
echo "✓ Arrow key editing (left/right)"
echo "✓ Tab completion"
echo "✓ Ctrl+C interrupt"
echo "✓ Command execution"
echo ""

echo "Summary of Issues to Check"
echo "========================="
echo ""
echo "Before fix - PROBLEMS:"
echo "❌ History UP/DOWN created prompt stacks"
echo "❌ Previous lines consumed per keypress"
echo "❌ Cursor appeared at wrong offset after newlines"
echo "❌ Visual artifacts and jumping"
echo ""
echo "After fix - EXPECTED:"
echo "✅ Single prompt line maintained"
echo "✅ Clean history content replacement"
echo "✅ Cursor positioned correctly (column 0 after wrap)"
echo "✅ No visual artifacts"
echo "✅ Smooth navigation experience"
echo ""

echo "Manual Test Instructions"
echo "========================"
echo ""
echo "1. Run: ./builddir/lusush /tmp/history_test_script.sh"
echo "2. Follow the step-by-step instructions"
echo "3. Pay attention to prompt rendering during history navigation"
echo "4. Test in different terminals (Konsole, foot, iTerm2)"
echo "5. Report any remaining issues"
echo ""

echo "Files created for testing:"
echo "  /tmp/history_test_script.sh - Comprehensive history test"
echo "  /tmp/simple_history_test.sh - Simplified test case"
echo "  /tmp/debug_history.c - Math verification"
echo ""

echo "Expected Result:"
echo "History navigation should work smoothly with proper cursor"
echo "positioning and no visual artifacts in multiline prompts."

# Cleanup function
cleanup() {
    rm -f /tmp/history_test_script.sh /tmp/simple_history_test.sh /tmp/debug_history.c /tmp/debug_history
}

trap cleanup EXIT

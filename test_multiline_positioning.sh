#!/bin/bash

# Targeted test to isolate multiline prompt rendering issues
# Focuses on understanding how linenoise handles theme-generated multiline prompts

echo "=========================================="
echo "MULTILINE PROMPT POSITIONING TEST"
echo "=========================================="
echo ""

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ Error: ./builddir/lusush not found. Run 'ninja' in builddir first."
    exit 1
fi

echo "✅ Binary found: ./builddir/lusush"
echo ""

echo "MULTILINE PROMPT ANALYSIS:"
echo "=========================="
echo ""

echo "This test isolates the core multiline prompt rendering issue:"
echo ""
echo "1. Theme system generates multiline prompts with \\n sequences"
echo "2. Linenoise multiline mode tries to manage cursor positioning"
echo "3. Conflict between theme prompt structure and linenoise expectations"
echo "4. Results in prompt stacking and positioning errors"
echo ""

echo "STEP-BY-STEP ANALYSIS:"
echo "======================"
echo ""

# Test 1: Examine the dark theme prompt structure
echo "1. Examining dark theme prompt structure..."

DARK_PROMPT=$(echo 'theme set dark; echo "$PS1"' | ./builddir/lusush 2>/dev/null | tail -1)
echo "Dark theme PS1:"
echo "\"$DARK_PROMPT\""
echo ""

# Count newlines in prompt
NEWLINE_COUNT=$(echo "$DARK_PROMPT" | grep -o '\\n' | wc -l)
echo "Newlines in prompt: $NEWLINE_COUNT"

# Check for carriage returns
CR_COUNT=$(echo "$DARK_PROMPT" | grep -o '\\r' | wc -l)
echo "Carriage returns in prompt: $CR_COUNT"
echo ""

# Test 2: Compare with single-line theme
echo "2. Comparing with single-line theme behavior..."

DEFAULT_PROMPT=$(echo 'echo "$PS1"' | ./builddir/lusush 2>/dev/null | tail -1)
echo "Default theme PS1:"
echo "\"$DEFAULT_PROMPT\""
echo ""

DEFAULT_NEWLINES=$(echo "$DEFAULT_PROMPT" | grep -o '\\n' | wc -l)
echo "Newlines in default prompt: $DEFAULT_NEWLINES"
echo ""

# Test 3: Interactive positioning test
echo "3. Interactive positioning test..."

cat > /tmp/position_test.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 5

puts "=== SINGLE COMMAND TEST ==="
spawn ./builddir/lusush -i
expect "lusush"

# Test dark theme
send "theme set dark\r"
expect "lusush"

puts "After setting dark theme, observe prompt structure..."
sleep 1

# Type one simple command
send "echo 'test'\r"
expect "lusush"

puts "After one command, observe positioning..."
sleep 1

# Exit
send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/position_test.exp
    echo "Running positioning test..."
    /tmp/position_test.exp
    rm -f /tmp/position_test.exp
    echo ""
else
    echo "⚠️ Expect not available"
    echo ""
fi

# Test 4: Minimal history navigation test
echo "4. Minimal history navigation test..."

cat > /tmp/minimal_history.exp << 'EOF'
#!/usr/bin/expect -f
set timeout 5

puts "=== MINIMAL HISTORY NAVIGATION ==="
spawn ./builddir/lusush -i
expect "lusush"

# Set dark theme
send "theme set dark\r"
expect "lusush"

# Add one command
send "echo 'first'\r"
expect "lusush"

puts "Before navigation - note current state"
sleep 1

# Try UP arrow once
send "\033\[A"
sleep 2

puts "After UP arrow - look for positioning issues"
sleep 1

# Try DOWN arrow
send "\033\[B"
sleep 1

puts "After DOWN arrow - final state"
sleep 1

send "exit\r"
expect eof
EOF

if command -v expect >/dev/null 2>&1; then
    chmod +x /tmp/minimal_history.exp
    echo "Running minimal history test..."
    /tmp/minimal_history.exp
    rm -f /tmp/minimal_history.exp
    echo ""
else
    echo "⚠️ Expect not available"
    echo ""
fi

echo "TECHNICAL INVESTIGATION:"
echo "========================"
echo ""

echo "Key areas to investigate:"
echo ""
echo "1. PROMPT STRUCTURE PARSING:"
echo "   - How does linenoise parse \\n and \\r in prompts?"
echo "   - Does it correctly calculate prompt dimensions?"
echo "   - Are ANSI escape sequences handled properly?"
echo ""
echo "2. MULTILINE MODE BEHAVIOR:"
echo "   - What triggers multiline mode vs single-line mode?"
echo "   - How does mlmode affect refresh behavior?"
echo "   - Is the cursor positioning logic theme-aware?"
echo ""
echo "3. REFRESH LOGIC INTERACTION:"
echo "   - How does refreshMultiLine() handle theme prompts?"
echo "   - Are oldrows calculations correct for theme prompts?"
echo "   - Does the clearing logic understand prompt boundaries?"
echo ""
echo "4. THEME SYSTEM INTEGRATION:"
echo "   - When does the theme system output the prompt?"
echo "   - How does linenoise receive the themed prompt?"
echo "   - Are there timing issues between theme and linenoise?"
echo ""

echo "DEBUGGING COMMANDS:"
echo "=================="
echo ""
echo "# Check linenoise multiline mode setting"
echo "grep -n 'linenoiseSetMultiLine' src/**/*.c"
echo ""
echo "# Check prompt parsing logic"
echo "grep -n -A 10 'promptTextColumnLen\\|promptnewlines' src/linenoise/linenoise.c"
echo ""
echo "# Check refresh logic for multiline"
echo "grep -n -A 20 'refreshMultiLine' src/linenoise/linenoise.c"
echo ""
echo "# Check theme prompt generation"
echo "grep -n -A 10 'theme_generate_primary_prompt' src/**/*.c"
echo ""

echo "MANUAL DEBUGGING STEPS:"
echo "======================="
echo ""
echo "1. Enable debug mode in linenoise.c (#if 1)"
echo "2. Run: ./builddir/lusush -i"
echo "3. Execute: theme set dark"
echo "4. Type: echo test"
echo "5. Press UP arrow"
echo "6. Check /tmp/lndebug.txt for:"
echo "   - Prompt dimensions calculations"
echo "   - Row/column positioning"
echo "   - Clear operations"
echo ""

echo "HYPOTHESIS:"
echo "==========="
echo ""
echo "The issue is likely that:"
echo "1. Theme system generates \\n\\r multiline prompts"
echo "2. Linenoise calculates prompt dimensions incorrectly"
echo "3. refreshMultiLine() uses wrong oldrows values"
echo "4. Cursor positioning becomes misaligned"
echo "5. Each refresh compounds the positioning error"
echo ""
echo "The fix requires ensuring that:"
echo "- Prompt dimension calculations are theme-aware"
echo "- Multiline refresh respects theme prompt structure"
echo "- Cursor positioning accounts for theme newlines"
echo "- Clearing only affects the actual prompt area"
echo ""

echo "=========================================="
echo "Next: Investigate prompt parsing and multiline logic"
echo "Focus on theme prompt structure vs linenoise expectations"
echo "=========================================="

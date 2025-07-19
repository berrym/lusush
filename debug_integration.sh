#!/bin/bash

# Debug script to test theme and prompt integration in Lusush Shell
# This script helps identify where the integration is failing

echo "=== Lusush Theme-Prompt Integration Debug ==="
echo "============================================="
echo

# Check if lusush executable exists
if [ ! -f "./builddir/lusush" ]; then
    echo "Error: ./builddir/lusush not found. Please build lusush first."
    exit 1
fi

LUSUSH="./builddir/lusush"

echo "1. Testing basic shell functionality..."
echo "---------------------------------------"
BASIC_TEST=$(echo 'echo "Shell is working"' | $LUSUSH 2>&1)
echo "Basic shell test: $BASIC_TEST"
echo

echo "2. Testing theme system..."
echo "-------------------------"
THEME_LIST=$(echo 'theme list' | $LUSUSH 2>&1)
echo "Available themes:"
echo "$THEME_LIST"
echo

THEME_INFO=$(echo 'theme info' | $LUSUSH 2>&1)
echo "Current theme info:"
echo "$THEME_INFO"
echo

echo "3. Testing prompt system variables..."
echo "------------------------------------"
echo "PS1 content:"
PS1_CONTENT=$(echo 'printf "PS1:[%s]\n" "$PS1"' | $LUSUSH 2>&1)
echo "$PS1_CONTENT"

echo "PS2 content:"
PS2_CONTENT=$(echo 'printf "PS2:[%s]\n" "$PS2"' | $LUSUSH 2>&1)
echo "$PS2_CONTENT"
echo

echo "4. Testing termcap integration..."
echo "--------------------------------"
TERMCAP_TEST=$(echo 'termcap capabilities' | $LUSUSH 2>&1)
echo "$TERMCAP_TEST" | head -20
echo

echo "5. Testing environment variables..."
echo "----------------------------------"
echo "TERM: $(echo 'printf "%s\n" "$TERM"' | $LUSUSH)"
echo "COLORTERM: $(echo 'printf "%s\n" "$COLORTERM"' | $LUSUSH)"
echo "HOME: $(echo 'printf "%s\n" "$HOME"' | $LUSUSH)"
echo "PWD: $(echo 'printf "%s\n" "$PWD"' | $LUSUSH)"
echo

echo "6. Testing with different TERM values..."
echo "---------------------------------------"
echo "With TERM=xterm-256color:"
XTERM_PS1=$(TERM=xterm-256color echo 'printf "PS1:[%s]\n" "$PS1"' | $LUSUSH)
echo "$XTERM_PS1"

echo "With TERM=xterm:"
XTERM_BASIC=$(TERM=xterm echo 'printf "PS1:[%s]\n" "$PS1"' | $LUSUSH)
echo "$XTERM_BASIC"

echo "With TERM=dumb:"
DUMB_TERM=$(TERM=dumb echo 'printf "PS1:[%s]\n" "$PS1"' | $LUSUSH)
echo "$DUMB_TERM"
echo

echo "7. Testing theme switching..."
echo "----------------------------"
echo "Switching to minimal theme:"
MINIMAL_RESULT=$(echo 'theme set minimal && printf "Result: PS1=[%s]\n" "$PS1"' | $LUSUSH 2>&1)
echo "$MINIMAL_RESULT"

echo "Switching to professional theme:"
PROF_RESULT=$(echo 'theme set professional && printf "Result: PS1=[%s]\n" "$PS1"' | $LUSUSH 2>&1)
echo "$PROF_RESULT"

echo "Switching to classic theme:"
CLASSIC_RESULT=$(echo 'theme set classic && printf "Result: PS1=[%s]\n" "$PS1"' | $LUSUSH 2>&1)
echo "$CLASSIC_RESULT"
echo

echo "8. Testing manual prompt rebuild..."
echo "----------------------------------"
REBUILD_TEST=$(echo 'rebuild_prompt 2>/dev/null; printf "After rebuild: PS1=[%s]\n" "$PS1"' | $LUSUSH 2>&1)
echo "$REBUILD_TEST"
echo

echo "9. Testing color detection..."
echo "----------------------------"
echo "Testing with forced colors:"
FORCE_COLOR=$(echo 'export FORCE_COLOR=1; printf "PS1:[%s]\n" "$PS1"' | $LUSUSH)
echo "$FORCE_COLOR"

echo "Testing with NO_COLOR:"
NO_COLOR_TEST=$(NO_COLOR=1 echo 'printf "PS1:[%s]\n" "$PS1"' | $LUSUSH)
echo "$NO_COLOR_TEST"
echo

echo "10. Testing Git integration..."
echo "-----------------------------"
if [ -d ".git" ]; then
    echo "In Git repository - testing Git branch detection:"
    GIT_BRANCH=$(git branch --show-current 2>/dev/null || echo "no-branch")
    echo "Actual Git branch: $GIT_BRANCH"

    GIT_PS1_TEST=$(echo 'printf "PS1 with Git:[%s]\n" "$PS1"' | $LUSUSH)
    echo "$GIT_PS1_TEST"
else
    echo "Not in a Git repository - creating temporary Git repo for testing:"
    mkdir -p /tmp/git_test
    cd /tmp/git_test
    git init >/dev/null 2>&1
    echo "test" > test.txt
    git add test.txt >/dev/null 2>&1
    git commit -m "test" >/dev/null 2>&1

    GIT_PS1_TEST=$(echo 'printf "PS1 with Git:[%s]\n" "$PS1"' | $LUSUSH)
    echo "$GIT_PS1_TEST"

    cd - >/dev/null
    rm -rf /tmp/git_test
fi
echo

echo "11. Testing direct command execution..."
echo "-------------------------------------"
echo "Running shell with -c flag:"
DIRECT_TEST=$($LUSUSH -c 'printf "Direct PS1:[%s]\n" "$PS1"' 2>&1)
echo "$DIRECT_TEST"
echo

echo "12. Testing with verbose output..."
echo "--------------------------------"
echo "Running with debug output (if available):"
DEBUG_TEST=$(echo 'printf "Debug PS1:[%s]\n" "$PS1"' | $LUSUSH 2>&1)
echo "$DEBUG_TEST"
echo

echo "=== Debug Summary ==="
echo "===================="

# Check for common issues
if [[ "$PS1_CONTENT" == *"PS1:[$]"* ]]; then
    echo "❌ ISSUE: PS1 is minimal ($) - prompt building is not working"
fi

if [[ "$THEME_INFO" == *"Theme: professional"* ]]; then
    echo "✅ Theme system is working - professional theme is active"
else
    echo "❌ ISSUE: Theme system may not be working properly"
fi

if [[ "$TERMCAP_TEST" == *"Basic Colors: Yes"* ]]; then
    echo "✅ Termcap system is working"
else
    echo "❌ ISSUE: Termcap system may not be working properly"
fi

if [[ "$PS1_CONTENT" == *"\033["* ]]; then
    echo "✅ Color codes are present in PS1"
else
    echo "❌ ISSUE: No color codes detected in PS1"
fi

echo
echo "=== Next Steps ==="
echo "=================="
echo "1. If PS1 is just '$', check prompt building logic"
echo "2. If themes aren't switching, check theme registration"
echo "3. If no colors, check termcap and TTY detection"
echo "4. For Git issues, check Git status detection"
echo "5. Check initialization order in src/init.c"

echo
echo "To run the shell interactively: $LUSUSH -i"
echo "To see raw prompt with escapes: printf '%q\\n' \"\$PS1\""

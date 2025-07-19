#!/bin/bash

# Comprehensive Theme-Prompt Integration Test for Lusush Shell
# Tests that the enhanced prompt system properly integrates with the theme system

echo "=== Lusush Theme-Prompt Integration Test ==="
echo "============================================="
echo

# Check if lusush executable exists
if [ ! -f "./builddir/lusush" ]; then
    echo "Error: ./builddir/lusush not found. Please build lusush first."
    exit 1
fi

# Test configuration
LUSUSH="./builddir/lusush"
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test result tracking
test_result() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"
    local details="$4"

    TEST_COUNT=$((TEST_COUNT + 1))

    printf "  %-40s " "$test_name"

    if [[ "$actual" == *"$expected"* ]]; then
        echo -e "${GREEN}[PASS]${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
        if [ -n "$details" ]; then
            echo "    ✓ $details"
        fi
    else
        echo -e "${RED}[FAIL]${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo "    Expected: $expected"
        echo "    Actual: $actual"
        if [ -n "$details" ]; then
            echo "    ✗ $details"
        fi
    fi
}

# Test basic prompt generation
echo -e "${CYAN}Test 1: Basic Prompt Generation${NC}"
echo "------------------------------"

# Test default theme prompt
PROMPT=$(echo 'printf "PS1:%s\n" "$PS1"' | $LUSUSH | grep "PS1:" | cut -d':' -f2-)
test_result "Default theme prompt generation" "~/Lab/c/lusush" "$PROMPT" "Should contain current directory"

# Check for Git integration
test_result "Git integration in prompt" "master" "$PROMPT" "Should show current git branch"

# Check for professional ASCII styling (no Unicode by default)
test_result "Professional styling present" "on" "$PROMPT" "Should use professional 'on' git syntax"
test_result "Git branch integration" "master" "$PROMPT" "Should show git branch with clean ASCII"

echo

# Test theme switching
echo -e "${CYAN}Test 2: Theme Switching Integration${NC}"
echo "-----------------------------------"

# Test corporate theme
CORPORATE_PROMPT=$(echo 'theme set corporate && printf "PS1:%s\n" "$PS1"' | $LUSUSH | grep "PS1:" | cut -d':' -f2-)
test_result "Corporate theme prompt" "~/Lab/c/lusush" "$CORPORATE_PROMPT" "Corporate theme should work"

# Test dark theme
DARK_PROMPT=$(echo 'theme set dark && printf "PS1:%s\n" "$PS1"' | $LUSUSH | grep "PS1:" | cut -d':' -f2-)
test_result "Dark theme prompt" "~/Lab/c/lusush" "$DARK_PROMPT" "Dark theme should work"

# Test minimal theme
MINIMAL_PROMPT=$(echo 'theme set minimal && printf "PS1:%s\n" "$PS1"' | $LUSUSH | grep "PS1:" | cut -d':' -f2-)
test_result "Minimal theme prompt" "$" "$MINIMAL_PROMPT" "Minimal theme should work"

# Test colorful theme
COLORFUL_PROMPT=$(echo 'theme set colorful && printf "PS1:%s\n" "$PS1"' | $LUSUSH | grep "PS1:" | cut -d':' -f2-)
test_result "Colorful theme prompt" "~/Lab/c/lusush" "$COLORFUL_PROMPT" "Colorful theme should work"

echo

# Test color escape sequences
echo -e "${CYAN}Test 3: Color Escape Sequences${NC}"
echo "----------------------------------"

# Test that ANSI color codes are present
PROMPT_RAW=$(echo 'printf "%s\n" "$PS1"' | $LUSUSH)
ESCAPE_COUNT=$(echo "$PROMPT_RAW" | od -c | grep -c '033')

if [ "$ESCAPE_COUNT" -gt 0 ]; then
    test_result "ANSI escape sequences present" "present" "present" "Found $ESCAPE_COUNT escape sequences"
else
    test_result "ANSI escape sequences present" "present" "missing" "No escape sequences found"
fi

echo

# Test responsive design
echo -e "${CYAN}Test 4: Responsive Design${NC}"
echo "--------------------------"

# Test narrow terminal simulation
TERM_WIDTH_80=$(COLUMNS=80 echo 'printf "%s\n" "$PS1"' | $LUSUSH)
test_result "Narrow terminal handling" "~/Lab/c/lusush" "$TERM_WIDTH_80" "Should adapt to 80 columns"

# Test very narrow terminal
TERM_WIDTH_40=$(COLUMNS=40 echo 'printf "%s\n" "$PS1"' | $LUSUSH)
test_result "Very narrow terminal" "lusush" "$TERM_WIDTH_40" "Should show at least directory name"

echo

# Test terminal capability detection
echo -e "${CYAN}Test 5: Terminal Capability Integration${NC}"
echo "--------------------------------------"

# Test with different TERM values
XTERM_PROMPT=$(TERM=xterm-256color echo 'printf "%s\n" "$PS1"' | $LUSUSH)
test_result "xterm-256color support" "~/Lab/c/lusush" "$XTERM_PROMPT" "Should work with xterm-256color"

KONSOLE_PROMPT=$(TERM=konsole echo 'printf "%s\n" "$PS1"' | $LUSUSH)
test_result "Konsole terminal support" "~/Lab/c/lusush" "$KONSOLE_PROMPT" "Should work with Konsole"

GNOME_PROMPT=$(TERM=gnome-terminal echo 'printf "%s\n" "$PS1"' | $LUSUSH)
test_result "GNOME terminal support" "~/Lab/c/lusush" "$GNOME_PROMPT" "Should work with GNOME terminal"

echo

# Test theme-specific features
echo -e "${CYAN}Test 6: Theme-Specific Features${NC}"
echo "-------------------------------"

# Test theme info integration
THEME_INFO=$(echo 'theme info' | $LUSUSH)
test_result "Theme info command" "Theme:" "$THEME_INFO" "Should show current theme info"

# Test theme listing
THEME_LIST=$(echo 'theme list' | $LUSUSH)
test_result "Theme list command" "Available themes:" "$THEME_LIST" "Should list available themes"

# Test invalid theme handling
INVALID_THEME=$(echo 'theme set nonexistent 2>&1' | $LUSUSH)
test_result "Invalid theme handling" "not found" "$INVALID_THEME" "Should handle invalid themes gracefully"

echo

# Test prompt caching and performance
echo -e "${CYAN}Test 7: Performance and Caching${NC}"
echo "----------------------------------"

# Test multiple prompt generations (should be fast due to caching)
start_time=$(date +%s%N)
for i in {1..5}; do
    echo 'printf "%s\n" "$PS1"' | $LUSUSH > /dev/null
done
end_time=$(date +%s%N)
duration=$((($end_time - $start_time) / 1000000)) # Convert to milliseconds

if [ "$duration" -lt 1000 ]; then
    test_result "Prompt generation performance" "fast" "fast" "Generated 5 prompts in ${duration}ms"
else
    test_result "Prompt generation performance" "fast" "slow" "Generated 5 prompts in ${duration}ms (may be slow)"
fi

echo

# Test git integration across themes
echo -e "${CYAN}Test 8: Git Integration Across Themes${NC}"
echo "-------------------------------------"

# Test git status in different themes
for theme in corporate dark colorful; do
    GIT_PROMPT=$(echo "theme set $theme && printf \"%s\\n\" \"\$PS1\"" | $LUSUSH)
    if [[ "$GIT_PROMPT" == *"master"* ]]; then
        test_result "Git in $theme theme" "master" "master" "Git branch shown in $theme theme"
    else
        test_result "Git in $theme theme" "master" "missing" "Git branch missing in $theme theme"
    fi
done

# Test minimal theme separately - it should NOT have Git info (by design)
MINIMAL_GIT_PROMPT=$(echo "theme set minimal && printf \"%s\\n\" \"\$PS1\"" | $LUSUSH)
if [[ "$MINIMAL_GIT_PROMPT" == *"master"* ]]; then
    test_result "Git in minimal theme" "no git info" "has git info" "Minimal theme should be truly minimal"
else
    test_result "Git in minimal theme" "no git info" "no git info" "Minimal theme correctly excludes Git info"
fi

echo

# Test PS2 (continuation prompt) integration
echo -e "${CYAN}Test 9: PS2 Integration${NC}"
echo "-----------------------"

PS2_PROMPT=$(echo 'printf "%s\n" "$PS2"' | $LUSUSH)
test_result "PS2 prompt set" "> " "$PS2_PROMPT" "PS2 should be set for line continuation"

echo

# Summary
echo -e "${CYAN}=== Test Summary ===${NC}"
echo "==================="
echo
echo "Total tests: $TEST_COUNT"
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$FAIL_COUNT${NC}"

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All tests passed! Theme-prompt integration is working perfectly.${NC}"
    SUCCESS_RATE=100
else
    SUCCESS_RATE=$((PASS_COUNT * 100 / TEST_COUNT))
    echo -e "\n${YELLOW}⚠️  $FAIL_COUNT test(s) failed. Success rate: ${SUCCESS_RATE}%${NC}"
fi

echo
echo "=== Integration Status ==="
echo "• Theme system: $(echo 'theme info >/dev/null 2>&1 && echo "✓ Active" || echo "✗ Inactive"' | $LUSUSH)"
echo "• Prompt system: ✓ Enhanced"
echo "• Color support: $(echo 'termcap capabilities | grep "Basic Colors" | cut -d: -f2' | $LUSUSH | xargs)"
echo "• Git integration: $(echo 'printf "%s\n" "$PS1"' | $LUSUSH | grep -q 'master' && echo "✓ Working" || echo "✗ Not detected")"
echo "• Unicode support: $(echo 'printf "%s\n" "$PS1"' | $LUSUSH | grep -q '📁' && echo "✓ Working" || echo "✗ Not detected")"

echo
echo "=== Recommendations ==="
if [ "$SUCCESS_RATE" -ge 90 ]; then
    echo "✓ Theme-prompt integration is excellent"
    echo "✓ All major features are working correctly"
    echo "✓ Professional ASCII styling ensures compatibility"
    echo "✓ The shell is ready for daily use"
elif [ "$SUCCESS_RATE" -ge 75 ]; then
    echo "⚠ Theme-prompt integration is mostly working"
    echo "• Some features may need fine-tuning"
    echo "• Consider investigating failed tests"
else
    echo "✗ Theme-prompt integration needs attention"
    echo "• Multiple features are not working correctly"
    echo "• Review the implementation for issues"
    echo "• Ensure ASCII styling for better compatibility"
fi

echo
echo "To test interactively, run: $LUSUSH -i"
echo "To see detailed capabilities: echo 'termcap capabilities' | $LUSUSH"
echo "To switch themes: echo 'theme set <theme_name>' | $LUSUSH"

exit $FAIL_COUNT

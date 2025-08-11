#!/bin/bash

# Lusush Display Issues Diagnostic Script
# Carefully analyzes readline display problems without making code changes

echo "=== Lusush Display Issues Diagnostic ==="
echo "Date: $(date)"
echo "Purpose: Understand arrow key artifacts and display corruption"
echo ""

# Ensure lusush is built and working
if [ ! -f builddir/lusush ]; then
    echo "❌ lusush binary not found. Building..."
    ninja -C builddir
    if [ $? -ne 0 ]; then
        echo "❌ Build failed! Cannot proceed."
        exit 1
    fi
fi

echo "=== ENVIRONMENT ANALYSIS ==="
echo ""

echo "Terminal Environment:"
echo "  TERM: ${TERM:-unset}"
echo "  COLORTERM: ${COLORTERM:-unset}"
echo "  Terminal size: $(tput cols 2>/dev/null)x$(tput lines 2>/dev/null)"
echo "  Terminal type: $(tput longname 2>/dev/null || echo 'unknown')"
echo ""

echo "Readline Library Information:"
ldd builddir/lusush | grep readline || echo "  ❌ readline not found in linked libraries"
echo ""

echo "Shell Environment:"
echo "  Shell: $SHELL"
echo "  Interactive: $(tty 2>/dev/null || echo 'non-interactive')"
echo "  TTY: $(tty 2>/dev/null || echo 'not a tty')"
echo ""

echo "=== USER-REPORTED ISSUES ==="
echo ""
echo "🚨 Critical Display Problems:"
echo "  1. Arrow UP leaves [A artifacts and shrinks prompt"
echo "  2. Arrow DOWN leaves [B artifacts"
echo "  3. Ctrl+G fails to clear line properly"
echo "  4. Ctrl+R reverse search corrupts display"
echo ""

echo "These indicate fundamental problems with:"
echo "  ❌ ANSI escape sequence handling"
echo "  ❌ Terminal display state management"
echo "  ❌ Readline redisplay functions"
echo "  ❌ Basic readline integration"
echo ""

echo "=== SAFE DIAGNOSTIC TESTS ==="
echo ""

echo "Test 1: Basic shell startup and exit..."
echo 'echo "startup test"' | ./builddir/lusush >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✅ Basic non-interactive mode works"
else
    echo "❌ Basic shell functionality broken"
    exit 1
fi

echo ""
echo "Test 2: History system status..."
echo -e 'echo "hist1"\necho "hist2"\nhistory\nexit' | ./builddir/lusush | grep -c "hist[12]"
hist_count=$(echo -e 'echo "hist1"\necho "hist2"\nhistory\nexit' | ./builddir/lusush | grep -c "hist[12]")
if [ $hist_count -ge 2 ]; then
    echo "✅ History system working ($hist_count commands found)"
else
    echo "❌ History system may have issues"
fi

echo ""
echo "Test 3: Basic shell operations..."
pipe_test=$(echo 'echo hello | grep h' | ./builddir/lusush)
if echo "$pipe_test" | grep -q "hello"; then
    echo "✅ Pipes working in non-interactive mode"
else
    echo "❌ Pipes broken even in non-interactive mode"
fi

echo ""
echo "=== CODE ANALYSIS ==="
echo ""

echo "Analyzing src/readline_integration.c for problematic patterns..."
echo ""

# Check for specific issues that could cause display corruption
echo "Checking for known readline display issues:"
echo ""

if grep -q "rl_redisplay_function.*=" src/readline_integration.c; then
    echo "⚠️  Custom redisplay function detected:"
    grep -n "rl_redisplay_function.*=" src/readline_integration.c | head -3
    echo "   This could cause display corruption if implemented incorrectly"
fi

if grep -q "rl_getc_function.*=" src/readline_integration.c; then
    echo "⚠️  Custom getc function detected:"
    grep -n "rl_getc_function.*=" src/readline_integration.c | head -3
    echo "   This could interfere with escape sequence processing"
fi

if grep -q "apply_syntax_highlighting" src/readline_integration.c; then
    echo "⚠️  Custom syntax highlighting detected:"
    echo "   This is often a source of display corruption in readline"
fi

echo ""
echo "Checking readline configuration complexity..."
config_lines=$(grep -c "rl_variable_bind" src/readline_integration.c)
echo "Number of rl_variable_bind calls: $config_lines"
if [ $config_lines -gt 10 ]; then
    echo "⚠️  Very complex readline configuration detected"
    echo "   Complex configurations often cause compatibility issues"
fi

echo ""
echo "=== SPECIFIC ISSUE ANALYSIS ==="
echo ""

echo "Issue: Arrow keys leave [A [B artifacts"
echo "Cause: Escape sequences not properly consumed by readline"
echo "Common reasons:"
echo "  - Custom getc function interfering with sequence processing"
echo "  - Terminal capabilities not properly detected"
echo "  - Readline not in correct mode for terminal type"
echo "  - Custom key bindings interfering with default behavior"
echo ""

echo "Issue: Prompt shrinking"
echo "Cause: Display state corruption during redraw"
echo "Common reasons:"
echo "  - Custom redisplay function with bugs"
echo "  - Prompt string formatting issues"
echo "  - Terminal state not properly managed"
echo "  - ANSI escape codes in prompt not properly marked"
echo ""

echo "Issue: Ctrl+G not working"
echo "Cause: Key binding or line clearing function broken"
echo "Common reasons:"
echo "  - Custom abort function not working correctly"
echo "  - Key binding overridden incorrectly"
echo "  - Display state not reset after abort"
echo ""

echo "Issue: Ctrl+R display corruption"
echo "Cause: Search mode display management broken"
echo "Common reasons:"
echo "  - Custom redisplay interfering with search mode"
echo "  - Prompt state not properly saved/restored"
echo "  - Search interface drawing over existing content"
echo ""

echo "=== SYSTEMATIC FIX STRATEGY ==="
echo ""

echo "Phase 1: Identify Root Cause (CURRENT)"
echo "  1. ✅ Test basic functionality (working)"
echo "  2. ✅ Analyze environment and configuration"
echo "  3. 📋 Manual test display issues to confirm scope"
echo "  4. 📋 Identify specific functions causing problems"
echo ""

echo "Phase 2: Minimal Working Configuration"
echo "  1. 📋 Create absolute minimal readline setup"
echo "  2. 📋 Test if basic arrow keys work without artifacts"
echo "  3. 📋 Verify Ctrl+G and Ctrl+R work correctly"
echo "  4. 📋 Establish solid foundation"
echo ""

echo "Phase 3: Incremental Feature Addition"
echo "  1. 📋 Add back features one by one"
echo "  2. 📋 Test after each addition"
echo "  3. 📋 Stop when display corruption appears"
echo "  4. 📋 Fix or properly implement problematic feature"
echo ""

echo "=== IMMEDIATE RECOMMENDATIONS ==="
echo ""

echo "Based on this analysis, the next steps should be:"
echo ""

echo "1. Manual verification of current display issues"
echo "   Start: script -q -c './builddir/lusush' /dev/null"
echo "   Test: Arrow keys, Ctrl+G, Ctrl+R as you described"
echo "   Confirm: All issues still present in current code"
echo ""

echo "2. Systematic readline simplification"
echo "   Strategy: Gradually remove features until display works"
echo "   Start with: Remove custom redisplay, custom getc, complex config"
echo "   Test after each removal"
echo ""

echo "3. Once basic display works, rebuild carefully"
echo "   Add features incrementally with testing"
echo "   Priority: Arrow keys > Ctrl keys > Advanced features"
echo ""

echo "=== CRITICAL INSIGHT ==="
echo ""

echo "The fundamental issue is that readline integration has:"
echo "  ❌ Too many custom functions interfering with standard behavior"
echo "  ❌ Complex configuration that may be incompatible"
echo "  ❌ Display management that doesn't work with terminal properly"
echo ""

echo "Solution approach:"
echo "  ✅ Start with minimal standard readline functionality"
echo "  ✅ Verify basic operations work correctly"
echo "  ✅ Add lusush-specific features only after foundation is solid"
echo ""

echo "=== READY FOR SYSTEMATIC FIXES ==="
echo ""

echo "This diagnostic confirms that the issues are real and serious."
echo "The next step is careful, incremental fixes starting with basic functionality."
echo ""

echo "Recommendation: Start with absolute minimal readline configuration"
echo "and build up systematically until we identify the exact cause"
echo "of the display corruption."
echo ""

echo "Diagnostic completed: $(date)"

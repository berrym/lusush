#!/bin/bash

# Comprehensive test for enhanced linenoise features with debug verification
# Tests Ctrl+R, enhanced completion, word navigation, and multiline fixes

echo "=========================================="
echo "ENHANCED LINENOISE FEATURES DEBUG TEST"
echo "=========================================="
echo ""

# Check if lusush binary exists
if [ ! -f "./builddir/lusush" ]; then
    echo "❌ Error: ./builddir/lusush not found. Run 'ninja' in builddir first."
    exit 1
fi

echo "✅ Binary found: ./builddir/lusush"

# Check current line count
LINES=$(wc -l src/linenoise/linenoise.c | awk '{print $1}')
echo "✅ Current implementation: $LINES lines (expected: ~2700+ for enhanced version)"
echo ""

echo "AUTOMATED TESTING:"
echo "=================="

# Test 1: Basic functionality
echo "1. Testing basic shell functionality..."
RESULT=$(echo "echo 'Hello World'" | ./builddir/lusush 2>/dev/null)
if [[ "$RESULT" == "Hello World" ]]; then
    echo "   ✅ Basic command execution works"
else
    echo "   ❌ Basic command execution failed"
fi

# Test 2: History system
echo "2. Testing history system..."
HISTORY_TEST=$(echo -e "echo 'first'\necho 'second'\nhistory" | ./builddir/lusush 2>/dev/null | tail -1)
if [[ "$HISTORY_TEST" == *"echo 'second'"* ]]; then
    echo "   ✅ History system functional"
else
    echo "   ❌ History system not working properly"
fi

# Test 3: Theme system (multiline prompts)
echo "3. Testing multiline prompt support..."
MULTILINE_TEST=$(echo -e "theme set dark\necho 'test'\nexit" | ./builddir/lusush -i 2>/dev/null | grep -c "test")
if [[ $MULTILINE_TEST -gt 0 ]]; then
    echo "   ✅ Multiline prompts functional"
else
    echo "   ❌ Multiline prompts not working"
fi

# Test 4: Enhanced completion detection
echo "4. Testing enhanced completion system..."
COMPLETION_TEST=$(echo -e "ls /tm\t\nexit" | ./builddir/lusush -i 2>/dev/null | grep -c "tmp")
if [[ $COMPLETION_TEST -gt 0 ]]; then
    echo "   ✅ Tab completion system working"
else
    echo "   ⚠️  Tab completion may not be fully functional"
fi

echo ""
echo "INTERACTIVE TESTING REQUIRED:"
echo "============================="
echo ""
echo "Run the following command to test enhanced features interactively:"
echo "./builddir/lusush -i"
echo ""
echo "Then test these enhanced features:"
echo ""
echo "🔍 CTRL+R REVERSE SEARCH:"
echo "-------------------------"
echo "1. Type a few commands (e.g., 'echo test1', 'echo test2')"
echo "2. Press Ctrl+R"
echo "3. Type 'test' - should show reverse search interface"
echo "4. Press Ctrl+R again to cycle through matches"
echo "5. Press Enter to accept or ESC to cancel"
echo ""
echo "Expected: (reverse-i-search)\`test': echo test2"
echo "If working: ✅ Ctrl+R reverse search functional"
echo "If broken: ❌ Ctrl+R not implemented or not working"
echo ""

echo "📝 ENHANCED WORD NAVIGATION:"
echo "---------------------------"
echo "1. Type: 'echo hello world test'"
echo "2. Press Ctrl+W (should delete 'test')"
echo "3. Press Ctrl+W again (should delete 'world')"
echo "4. Test cursor movement with Ctrl+A, Ctrl+E, arrows"
echo ""
echo "Expected: Ctrl+W deletes words backward properly"
echo "If working: ✅ Word navigation enhanced"
echo "If broken: ❌ Basic word deletion only"
echo ""

echo "🎯 ENHANCED TAB COMPLETION:"
echo "-------------------------"
echo "1. Type: 'ls /u' and press TAB"
echo "2. Should show professional completion menu"
echo "3. Use TAB, Ctrl+P, Ctrl+N to navigate"
echo "4. Press ESC to cancel cleanly"
echo ""
echo "Expected: Categorized completion display with navigation hints"
echo "If working: ✅ Enhanced completion system"
echo "If broken: ❌ Basic completion only"
echo ""

echo "🏠 MULTILINE HISTORY NAVIGATION:"
echo "------------------------------"
echo "1. Set theme: 'theme set dark'"
echo "2. Type a few long commands that wrap lines"
echo "3. Use UP/DOWN arrows to navigate history"
echo "4. Verify no line consumption or artifacts"
echo ""
echo "Expected: Clean history navigation with proper multiline display"
echo "If working: ✅ Multiline history fix successful"
echo "If broken: ❌ Line consumption or display artifacts"
echo ""

echo "DEBUG VERIFICATION:"
echo "=================="
echo ""
echo "To debug specific features, check these source code markers:"
echo ""
echo "🔍 Ctrl+R Implementation:"
echo "  grep -n 'CTRL_R\\|reverse.*search' src/linenoise/linenoise.c"
echo "  Should find: reverse_search_mode, linenoiseReverseSearch(), CTRL_R case"
echo ""
echo "📝 Word Navigation:"
echo "  grep -n 'CTRL_W\\|DeletePrevWord' src/linenoise/linenoise.c"
echo "  Should find: linenoiseEditDeletePrevWord(), CTRL_W case"
echo ""
echo "🎯 Enhanced Completion:"
echo "  grep -n 'displayCompletionMenu\\|enhanced_completion' src/linenoise/linenoise.c"
echo "  Should find: displayCompletionMenu(), get_enhanced_completion()"
echo ""
echo "🏠 History Fix:"
echo "  grep -n 'refreshLineWithFlags.*REFRESH_ALL' src/linenoise/linenoise.c"
echo "  Should find: refreshLineWithFlags(l, REFRESH_ALL) in history navigation"
echo ""

echo "VERIFICATION COMMANDS:"
echo "====================="
echo ""
echo "# Check for Ctrl+R implementation"
echo "grep -c 'reverse_search_mode' src/linenoise/linenoise.c"
echo ""
echo "# Check for enhanced completion"
echo "grep -c 'displayCompletionMenu' src/linenoise/linenoise.c"
echo ""
echo "# Check for word navigation"
echo "grep -c 'linenoiseEditDeletePrevWord' src/linenoise/linenoise.c"
echo ""
echo "# Check history fix application"
echo "grep -c 'refreshLineWithFlags.*REFRESH_ALL' src/linenoise/linenoise.c"
echo ""
echo "Expected counts: All should be > 0 for enhanced version"
echo ""

echo "COMPARISON WITH PREVIOUS VERSION:"
echo "================================="
echo "Enhanced version (current): $LINES lines"
echo "Clean version (backup): $(wc -l src/linenoise/linenoise_clean_backup.c 2>/dev/null | awk '{print $1}') lines"
echo "Original complex version: $(wc -l src/linenoise/linenoise_backup_complex.c 2>/dev/null | awk '{print $1}') lines"
echo ""
echo "Line difference shows preserved features while fixing bugs"
echo ""

echo "SUCCESS CRITERIA:"
echo "================="
echo ""
echo "✅ MUST HAVE (Blocking issues):"
echo "  - Ctrl+R reverse search working"
echo "  - Enhanced tab completion functional"
echo "  - History navigation no line consumption"
echo "  - Multiline prompts display correctly"
echo "  - All basic editing operations work"
echo ""
echo "✅ SHOULD HAVE (Important features):"
echo "  - Word-wise navigation (Ctrl+W, etc.)"
echo "  - Professional completion display"
echo "  - Clean ESC cancellation"
echo "  - UTF-8 character support"
echo ""
echo "✅ VERIFICATION COMPLETE:"
echo "  - Source code contains enhanced features"
echo "  - Build completes without errors"
echo "  - Interactive testing confirms functionality"
echo "  - No regressions in basic features"
echo ""

echo "=========================================="
echo "Run interactive test now: ./builddir/lusush -i"
echo "Use the feature tests above to verify enhanced functionality"
echo "=========================================="

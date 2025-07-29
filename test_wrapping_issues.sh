#!/bin/bash

# Test Script for Syntax Highlighting and Tab Completion Wrapping Issues
# This script reproduces the issues identified after the line wrapping fix

echo "=== Lusush Line Editor: Syntax Highlighting and Tab Completion Wrapping Issues Test ==="
echo
echo "This script tests for issues that emerged after fixing the terminal width detection:"
echo "  1. Syntax highlighting stops working on wrapped lines"
echo "  2. Tab completion doesn't render properly across wrapped lines"
echo

# Create test directory with various files for tab completion
echo "Setting up test environment..."
mkdir -p wrapping_issues_test
cd wrapping_issues_test

# Create files with different name lengths to test tab completion wrapping
touch very_long_filename_that_will_definitely_cause_line_wrapping_in_most_terminals.txt
touch another_extremely_long_filename_for_comprehensive_testing_purposes.txt
touch super_duper_extra_long_filename_that_exceeds_normal_terminal_width.txt
touch short.txt
touch medium_length_file.txt
touch test.txt

echo "Created test files:"
ls -1

echo
echo "=== Current Terminal Information ==="
echo "Terminal width: $(tput cols) columns"
echo "Terminal height: $(tput lines) rows"
echo

echo "=== ISSUE 1: Syntax Highlighting on Wrapped Lines ==="
echo
echo "PROBLEM: Syntax highlighting stops working when command lines wrap to next line"
echo
echo "TEST INSTRUCTIONS:"
echo "1. Start lusush with syntax highlighting enabled"
echo "2. Type a very long command that wraps to the next line"
echo "3. Observe if syntax highlighting continues on the wrapped portion"
echo
echo "EXAMPLE COMMANDS TO TEST:"
echo "  echo 'this is a very long command with string literals that should be highlighted'"
echo "  ls -la /very/long/path/that/exceeds/terminal/width/and/continues/on/next/line"
echo "  grep 'pattern' /path/to/file/with/very/long/name/that/causes/wrapping.txt"
echo
echo "EXPECTED BEHAVIOR:"
echo "  - First word (command) should be highlighted in blue"
echo "  - String literals should be highlighted in green"
echo "  - Highlighting should continue correctly on wrapped lines"
echo
echo "CURRENT ISSUE:"
echo "  - Syntax highlighting may stop at wrap boundary"
echo "  - Wrapped portion may lose color formatting"
echo "  - Color state may be reset incorrectly"
echo

echo "=== ISSUE 2: Tab Completion Rendering on Wrapped Lines ==="
echo
echo "PROBLEM: Tab completion doesn't render properly when completion list crosses wrapped lines"
echo
echo "TEST INSTRUCTIONS:"
echo "1. Type a long command that nearly fills the terminal width"
echo "2. Add a partial filename and press Tab"
echo "3. Observe if completion list renders correctly"
echo
echo "EXAMPLE COMMANDS TO TEST:"
echo "  echo 'long prefix text that fills most of the terminal width' very<TAB>"
echo "  ls /some/long/path/that/fills/terminal/width super<TAB>"
echo "  grep 'pattern' another<TAB>"
echo
echo "EXPECTED BEHAVIOR:"
echo "  - Completion list should appear below current line"
echo "  - Multiple completions should be formatted properly"
echo "  - List should not interfere with existing text"
echo "  - Cursor should return to correct position after completion"
echo
echo "CURRENT ISSUES:"
echo "  - Completion list may appear at wrong position"
echo "  - List formatting may be corrupted when cursor is on wrapped line"
echo "  - Cursor positioning after completion may be incorrect"
echo

echo "=== DETAILED TEST CASES ==="
echo

echo "TEST CASE 1: Command Syntax Highlighting Across Wrap"
echo "  Command: echo 'very long string that should cause line wrapping and maintain highlighting'"
echo "  Expected: 'echo' in blue, string in green, highlighting continues on wrapped line"
echo "  Issue: Highlighting may stop at wrap boundary"
echo

echo "TEST CASE 2: Path Completion on Wrapped Line"
echo "  Setup: Type long prefix until cursor is near terminal width"
echo "  Command: [long prefix] very<TAB>"
echo "  Expected: Completion list appears properly formatted below"
echo "  Issue: List may appear at wrong position or be formatted incorrectly"
echo

echo "TEST CASE 3: Multiple Completions on Wrapped Line"
echo "  Setup: Type long prefix, then ambiguous completion"
echo "  Command: [long prefix] test<TAB>"
echo "  Expected: Multiple matching files shown in formatted list"
echo "  Issue: List formatting may be corrupted, positioning incorrect"
echo

echo "TEST CASE 4: String Highlighting in Wrapped Command"
echo "  Command: grep 'search pattern' very_long_filename_that_will_definitely_cause_line_wrapping_in_most_terminals.txt"
echo "  Expected: 'grep' highlighted, quotes/pattern highlighted, filename may be highlighted"
echo "  Issue: Highlighting state may be lost on wrapped portion"
echo

echo "TEST CASE 5: Tab Completion with Long Prefix"
echo "  Command: cat very_long_filename_that_will_definitely_cause_line_wrapping_in_most_terminals.txt another<TAB>"
echo "  Expected: Completion works, cursor returns to correct position"
echo "  Issue: Cursor positioning may be incorrect due to wrap calculations"
echo

echo "=== ROOT CAUSE ANALYSIS ==="
echo
echo "LIKELY CAUSES:"
echo
echo "1. SYNTAX HIGHLIGHTING ISSUES:"
echo "   - Syntax highlighting render function may have independent wrap logic"
echo "   - Color state management may not account for line wrapping"
echo "   - Terminal output positioning may conflict with main display system"
echo "   - lle_display_render_with_syntax_highlighting() may not use correct geometry"
echo
echo "2. TAB COMPLETION ISSUES:"
echo "   - Completion display may not use updated terminal geometry"
echo "   - Cursor position calculations may assume hardcoded width"
echo "   - Completion list positioning may not account for wrapped lines"
echo "   - Enhanced tab completion may not integrate with new width detection"
echo

echo "=== EXPECTED CODE LOCATIONS TO INVESTIGATE ==="
echo
echo "SYNTAX HIGHLIGHTING:"
echo "  - src/line_editor/display.c: lle_display_render_with_syntax_highlighting()"
echo "  - src/line_editor/syntax.c: syntax region calculations"
echo "  - Color state management across line boundaries"
echo
echo "TAB COMPLETION:"
echo "  - src/line_editor/completion_display.c: completion rendering"
echo "  - src/line_editor/enhanced_tab_completion.c: enhanced completion logic"
echo "  - Cursor position calculations for completion placement"
echo

echo "=== DEBUGGING ENVIRONMENT SETUP ==="
echo
# Export debug flags for comprehensive output
export LLE_DEBUG=1
export LLE_DEBUG_CURSOR=1
export LLE_DEBUG_DISPLAY=1

echo "Debug flags enabled:"
echo "  LLE_DEBUG=1 (general debug output)"
echo "  LLE_DEBUG_CURSOR=1 (cursor math debug)"
echo "  LLE_DEBUG_DISPLAY=1 (display system debug)"
echo

echo "=== STARTING INTERACTIVE TEST SESSION ==="
echo
echo "Instructions for testing:"
echo "1. Look for debug output showing terminal width detection"
echo "2. Test syntax highlighting with long commands"
echo "3. Test tab completion on wrapped lines"
echo "4. Note any positioning or formatting issues"
echo "5. Compare behavior with and without line wrapping"
echo
echo "Key things to observe:"
echo "  - Does debug output show correct terminal width (not 80)?"
echo "  - Does syntax highlighting work across wrap boundaries?"
echo "  - Do completion lists appear at correct positions?"
echo "  - Does cursor return to correct position after completion?"
echo

echo "Starting lusush with debug output..."
echo "Type 'exit' when finished testing"
echo

# Change to parent directory to test with our files in path
cd ..

# Start lusush for interactive testing
echo "=== LUSUSH INTERACTIVE SESSION ==="
./builddir/lusush

# Cleanup after testing
echo
echo "=== CLEANUP ==="
rm -rf wrapping_issues_test
echo "Test environment cleaned up"

echo
echo "=== ANALYSIS CHECKLIST ==="
echo "After testing, check if you observed:"
echo "□ Syntax highlighting stops at wrap boundary"
echo "□ Completion list appears at wrong position"
echo "□ Cursor positioning incorrect after completion"
echo "□ Terminal width detection working correctly in debug output"
echo "□ Color state reset issues on wrapped lines"
echo "□ Completion list formatting corruption"
echo
echo "If these issues are confirmed, the fixes needed are:"
echo "1. Update syntax highlighting to use display system's wrap handling"
echo "2. Fix completion display to use dynamic terminal width"
echo "3. Coordinate cursor positioning between all rendering systems"
echo "4. Ensure all components use the fixed terminal geometry detection"

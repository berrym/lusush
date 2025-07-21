#!/bin/bash

# Test for Embedded Newline Fix in Multiline Prompts
# This test validates that prompts with embedded newlines display correctly

echo "Embedded Newline Fix Test"
echo "========================="
echo ""
echo "This test validates that multiline prompts with embedded newlines"
echo "position the cursor correctly at column 0 after each newline."
echo ""

LUSUSH="./builddir/lusush"

if [ ! -x "$LUSUSH" ]; then
    echo "❌ Error: lusush binary not found at $LUSUSH"
    echo "Please build first with: ninja -C builddir"
    exit 1
fi

echo "✅ Found lusush binary"
echo ""

# Test 1: Dark theme multiline prompt (uses \n in template)
echo "Test 1: Dark Theme Multiline Prompt"
echo "-----------------------------------"
echo "Testing theme that uses bare \\n in template..."

DARK_OUTPUT=$(echo 'theme set dark; echo "PROMPT_START"; echo "$PS1"; echo "PROMPT_END"' | $LUSUSH 2>/dev/null)
echo "$DARK_OUTPUT"

if [[ "$DARK_OUTPUT" == *"┌─"* && "$DARK_OUTPUT" == *"└─"* ]]; then
    echo "✅ Dark theme generates multiline prompt"
else
    echo "❌ Dark theme failed to generate multiline prompt"
fi

echo ""

# Test 2: Manual multiline prompt test
echo "Test 2: Manual Multiline Prompt Test"
echo "------------------------------------"
echo "This requires interactive testing. Run:"
echo ""
echo "  $LUSUSH"
echo ""
echo "Then execute:"
echo "  theme set dark"
echo ""
echo "Expected behavior:"
echo "  ✅ First line: ┌─[user@host]─[/path] git-info"
echo "  ✅ Second line: └─$ (starts at column 0)"
echo "  ✅ Cursor positioned immediately after $ symbol"
echo "  ✅ No horizontal offset or misalignment"
echo ""

# Test 3: History navigation test
echo "Test 3: History Navigation Test"
echo "------------------------------"
echo "In the interactive shell with dark theme:"
echo "  1. Type: echo hello world"
echo "  2. Press ENTER"
echo "  3. Press UP arrow to recall command"
echo "  4. Verify cursor positions correctly"
echo "  5. Verify no prompt duplication or offset"
echo ""

# Test 4: Terminal resize test
echo "Test 4: Terminal Resize Test"
echo "----------------------------"
echo "In the interactive shell:"
echo "  1. Set dark theme"
echo "  2. Resize terminal to different widths"
echo "  3. Verify prompt always displays correctly"
echo "  4. Verify cursor always at proper position"
echo ""

# Test 5: Compare with traditional prompt
echo "Test 5: Traditional vs Themed Prompt"
echo "------------------------------------"
echo "Traditional prompts used \\n\\r (newline + carriage return)"
echo "Themed prompts use just \\n (newline only)"
echo ""
echo "The fix automatically converts bare \\n to \\n\\r for proper"
echo "cursor positioning while maintaining clean theme templates."
echo ""

# Test 6: Technical validation
echo "Test 6: Technical Implementation"
echo "-------------------------------"
echo "The fix works by:"
echo "  1. Detecting bare \\n characters in prompts"
echo "  2. Converting them to \\n\\r automatically"
echo "  3. Avoiding duplicate \\r if already present"
echo "  4. Applying to both single-line and multi-line refresh"
echo ""
echo "Code locations modified:"
echo "  - refreshSingleLine() function"
echo "  - refreshMultiLine() function"
echo "  - Both now preprocess prompts before terminal output"
echo ""

# Test 7: Success criteria
echo "Test 7: Success Criteria"
echo "------------------------"
echo "The fix is successful if:"
echo "  ✅ Dark theme displays proper box-style prompt"
echo "  ✅ Second line starts at column 0 (no offset)"
echo "  ✅ Cursor appears immediately after $ symbol"
echo "  ✅ History navigation works without artifacts"
echo "  ✅ Terminal resizing doesn't break alignment"
echo "  ✅ No regression in single-line prompts"
echo ""

echo "MANUAL TESTING REQUIRED"
echo "======================"
echo "Run: $LUSUSH"
echo "Execute: theme set dark"
echo "Verify cursor positioning and interaction behavior"
echo ""
echo "If all criteria are met, the newline fix is SUCCESSFUL!"

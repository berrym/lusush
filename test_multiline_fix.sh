#!/bin/bash

echo "Testing Multiline Prompt Fix"
echo "============================"
echo ""

LUSUSH="./builddir/lusush"

if [ ! -x "$LUSUSH" ]; then
    echo "Error: lusush binary not found at $LUSUSH"
    echo "Please build first with: ninja -C builddir"
    exit 1
fi

echo "1. Testing PRO_PROMPT style (intentional newlines)"
echo "--------------------------------------------------"
echo "Setting prompt style to 'pro' which has intentional newlines..."

# Test PRO_PROMPT which has intentional newlines
PRO_TEST=$(echo 'config set prompt_style pro; rebuild_prompt; printf "Prompt test: [%s]\n" "$PS1"' | $LUSUSH 2>/dev/null)
echo "$PRO_TEST"

echo ""
echo "2. Testing themed multiline prompts (CRITICAL TEST)"
echo "---------------------------------------------------"
echo "Testing dark theme which has multiline box-style prompt:"

# Test dark theme which has explicit \n in template
DARK_TEST=$(echo 'theme set dark; rebuild_prompt; printf "Dark theme: [%s]\n" "$PS1"' | $LUSUSH 2>/dev/null)
echo "$DARK_TEST"

echo ""
echo "Expected dark theme format:"
echo "┌─[user@host]─[/path]"
echo "└─$ "
echo ""

echo "Testing corporate theme:"
CORP_TEST=$(echo 'theme set corporate; rebuild_prompt; printf "Corporate: [%s]\n" "$PS1"' | $LUSUSH 2>/dev/null)
echo "$CORP_TEST"

echo ""
echo "3. Traditional prompt styles comparison:"
echo "----------------------------------------"

echo "Normal prompt:"
NORMAL_TEST=$(echo 'config set prompt_style normal; rebuild_prompt; printf "Normal: [%s]\n" "$PS1"' | $LUSUSH 2>/dev/null)
echo "$NORMAL_TEST"

echo ""
echo "Color prompt:"
COLOR_TEST=$(echo 'config set prompt_style color; rebuild_prompt; printf "Color: [%s]\n" "$PS1"' | $LUSUSH 2>/dev/null)
echo "$COLOR_TEST"

echo ""
echo "Git prompt:"
GIT_TEST=$(echo 'config set prompt_style git; rebuild_prompt; printf "Git: [%s]\n" "$PS1"' | $LUSUSH 2>/dev/null)
echo "$GIT_TEST"

echo ""
echo "4. Interactive test instructions:"
echo "--------------------------------"
echo "Run the following commands in lusush to test manually:"
echo ""
echo "   $LUSUSH"
echo ""
echo "# Test traditional multiline prompt:"
echo "   config set prompt_style pro"
echo "   rebuild_prompt"
echo ""
echo "# Test themed multiline prompts:"
echo "   theme set dark"
echo "   # Should show box-style prompt with correct cursor positioning"
echo ""
echo "   theme set corporate"
echo "   # Should show corporate-style prompt"
echo ""
echo "Expected behavior for ALL multiline prompts:"
echo "  ✅ First line displays correctly"
echo "  ✅ Second line starts at column 0 (no offset)"
echo "  ✅ Cursor positioned immediately after prompt symbol"
echo "  ✅ History navigation doesn't cause misalignment"
echo "  ✅ Line wrapping works correctly"
echo ""

echo "5. Visual test case - Manual prompt creation:"
echo "--------------------------------------------"
echo "Create a prompt with intentional newlines for testing:"
echo ""
echo 'export PS1="┌─[\\u@\\h]-[\\w]"$'"'"'\\n'"'"'"└─\\$ "'
echo ""
echo "This should display as:"
echo "┌─[user@host]-[/path]"
echo "└─$ "
echo ""
echo "With cursor positioned right after the $ symbol."
echo ""

echo "6. Testing ALL multiline prompt sources:"
echo "---------------------------------------"
echo "This fix should work for:"
echo "  ✅ Traditional PRO_PROMPT (\\n\\r in prompt.c)"
echo "  ✅ Themed prompts with \\n (theme templates)"
echo "  ✅ Manual PS1 with embedded newlines"
echo "  ✅ Any prompt with intentional line breaks"
echo ""

echo "Test complete!"
echo ""
echo "KEY SUCCESS CRITERIA:"
echo "✅ ALL multiline prompts display on correct lines"
echo "✅ Second line always starts at column 0"
echo "✅ Cursor positioned immediately after prompt symbol"
echo "✅ No visual artifacts or misalignment"
echo "✅ History navigation works perfectly"
echo "✅ Both traditional and themed prompts work"

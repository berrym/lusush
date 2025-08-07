#!/bin/bash

# Test script to verify the prompt redrawing fix
# This verifies that backspace works without causing repeated prompt redraws

echo "=================================================================================="
echo "PROMPT REDRAW FIX VERIFICATION"
echo "=================================================================================="
echo ""
echo "This test verifies that backspace works correctly WITHOUT causing prompt cascading."
echo "The fix positions cursor after prompt instead of redrawing the entire prompt."
echo ""

# Build the project first
echo "Building project..."
if ! scripts/lle_build.sh build; then
    echo "❌ Build failed - cannot test"
    exit 1
fi

echo "✅ Build successful"
echo ""

echo "EXPECTED BEHAVIOR AFTER FIX:"
echo "- Single prompt remains visible during backspace"
echo "- Characters disappear correctly (including across line boundaries)"
echo "- NO prompt cascading or stacking"
echo "- Clean visual updates"
echo ""

echo "PREVIOUS BROKEN BEHAVIOR:"
echo "- Multiple prompts appeared: [prompt][prompt][prompt]..."
echo "- Prompt was redrawn on every backspace operation"
echo "- Created visual cascade effect"
echo ""

echo "DEBUG INDICATORS TO LOOK FOR:"
echo "✅ GOOD: [VISUAL_DEBUG] SUCCESS: Moved cursor to position X (after prompt)"
echo "✅ GOOD: [VISUAL_DEBUG] SUCCESS: Cleared to end of line (prompt preserved)"
echo "❌ BAD:  [LLE_PROMPT_RENDER] (should not appear during backspace)"
echo ""

echo "Press Enter to start interactive test (Ctrl+C to cancel)"
read -r

echo "Starting lusush with debug logging..."
echo "Debug output will be saved to /tmp/no_prompt_redraw_test.log"
echo ""

echo "TEST INSTRUCTIONS:"
echo "1. Type: echo hello world"
echo "2. Press backspace multiple times"
echo "3. Look for SINGLE prompt (not multiple)"
echo "4. Verify characters disappear correctly"
echo "5. Press Ctrl+D to exit"
echo ""

# Run with debug output
LLE_DEBUG=1 ./builddir/lusush 2>/tmp/no_prompt_redraw_test.log

echo ""
echo "=================================================================================="
echo "TEST ANALYSIS"
echo "=================================================================================="
echo ""

# Check debug logs for key indicators
echo "Analyzing debug logs for prompt redrawing..."

prompt_renders=$(grep -c "LLE_PROMPT_RENDER" /tmp/no_prompt_redraw_test.log)
cursor_positions=$(grep -c "Moved cursor to position.*after prompt" /tmp/no_prompt_redraw_test.log)
preserved_prompts=$(grep -c "prompt preserved" /tmp/no_prompt_redraw_test.log)

echo "Debug Analysis Results:"
echo "- Prompt renders during backspace: $prompt_renders (should be 0 after initial)"
echo "- Cursor positioned after prompt: $cursor_positions"
echo "- Prompt preservation messages: $preserved_prompts"
echo ""

if [ "$prompt_renders" -le 1 ]; then
    echo "✅ GOOD: Minimal prompt rendering (only initial render)"
else
    echo "❌ BAD: Too many prompt renders ($prompt_renders) - still redrawing prompts"
fi

if [ "$cursor_positions" -gt 0 ]; then
    echo "✅ GOOD: Cursor positioned after prompt ($cursor_positions times)"
else
    echo "❌ BAD: No cursor positioning after prompt detected"
fi

if [ "$preserved_prompts" -gt 0 ]; then
    echo "✅ GOOD: Prompt preservation active ($preserved_prompts times)"
else
    echo "❌ BAD: No prompt preservation messages"
fi

echo ""
echo "VISUAL VERIFICATION:"
echo "Did you see a single clean prompt without cascading? (y/n)"
read -r visual_response

echo ""
echo "FUNCTIONAL VERIFICATION:"
echo "Did backspace work correctly across line boundaries? (y/n)"
read -r functional_response

echo ""
echo "=================================================================================="
echo "TEST RESULTS"
echo "=================================================================================="

if [[ "$visual_response" =~ ^[Yy] ]] && [[ "$functional_response" =~ ^[Yy] ]]; then
    echo "✅ SUCCESS: Prompt redraw fix verified!"
    echo ""
    echo "ACHIEVEMENTS:"
    echo "- ✅ Single prompt maintained (no cascading)"
    echo "- ✅ Backspace works across line boundaries"
    echo "- ✅ Clean visual updates"
    echo "- ✅ Cursor positioned correctly after prompt"
    echo ""
    echo "The fix successfully eliminates prompt redrawing while maintaining"
    echo "backspace functionality across line boundaries."

elif [[ "$visual_response" =~ ^[Yy] ]] && [[ "$functional_response" =~ ^[Nn] ]]; then
    echo "⚠️  PARTIAL SUCCESS: Prompt fixed but backspace broken"
    echo ""
    echo "- ✅ No prompt cascading"
    echo "- ❌ Backspace functionality issue"
    echo ""
    echo "Need to debug backspace functionality while preserving prompt fix."

elif [[ "$visual_response" =~ ^[Nn] ]] && [[ "$functional_response" =~ ^[Yy] ]]; then
    echo "⚠️  PARTIAL SUCCESS: Backspace works but prompt still cascading"
    echo ""
    echo "- ❌ Still seeing prompt cascading"
    echo "- ✅ Backspace functionality works"
    echo ""
    echo "Need to debug cursor positioning and prompt preservation logic."

else
    echo "❌ FAILURE: Both visual and functional issues remain"
    echo ""
    echo "- ❌ Prompt cascading still occurring"
    echo "- ❌ Backspace functionality broken"
    echo ""
    echo "Need to review the cursor positioning and content rewrite strategy."
fi

echo ""
echo "Debug logs available at: /tmp/no_prompt_redraw_test.log"
echo "Key debug commands:"
echo "  View cursor positioning: grep 'cursor.*position' /tmp/no_prompt_redraw_test.log"
echo "  View prompt renders: grep 'PROMPT_RENDER' /tmp/no_prompt_redraw_test.log"
echo "  View visual debug: grep 'VISUAL_DEBUG' /tmp/no_prompt_redraw_test.log"

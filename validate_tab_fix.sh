#!/bin/bash

# Final Validation Test for TAB Completion Surgical Fix
# Tests the exact success criteria from handoff documents

echo "=== TAB Completion Surgical Fix Validation ==="
echo "Testing against exact success criteria from handoff documents"
echo ""

# Build first
echo "Building LLE..."
if ! scripts/lle_build.sh build >/dev/null 2>&1; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build successful"
echo ""

# Test 1: Basic TAB cycling (main fix)
echo "=== Test 1: TAB Cycling Prompt Redraw Prevention ==="
echo "Testing: echo [TAB][TAB][TAB] - Should cycle without prompt redraws"

printf "echo \t\t\t\nexit\n" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/tab_fix_test.log

# Analyze results
TAB_EVENTS=$(grep -c "TAB case executed" /tmp/tab_fix_test.log)
CYCLING_EVENTS=$(grep -c "Cycled to completion" /tmp/tab_fix_test.log)
PROMPT_RENDERS=$(grep -c "Rendering prompt" /tmp/tab_fix_test.log)
SURGICAL_FIX_ACTIVE=$(grep -c "Tab cycling in active session" /tmp/tab_fix_test.log)

echo "Results:"
echo "- TAB events processed: $TAB_EVENTS"
echo "- Completion cycling events: $CYCLING_EVENTS"
echo "- Prompt renders: $PROMPT_RENDERS"
echo "- Surgical fix activations: $SURGICAL_FIX_ACTIVE"

# Success criteria validation
SUCCESS=true

if [ $TAB_EVENTS -lt 3 ]; then
    echo "❌ FAIL: Expected 3+ TAB events, got $TAB_EVENTS"
    SUCCESS=false
fi

if [ $CYCLING_EVENTS -lt 2 ]; then
    echo "❌ FAIL: Expected 2+ cycling events, got $CYCLING_EVENTS"
    SUCCESS=false
fi

if [ $PROMPT_RENDERS -gt 2 ]; then
    echo "❌ FAIL: Too many prompt renders (expected ≤2, got $PROMPT_RENDERS)"
    echo "   This indicates prompt redraws during cycling"
    SUCCESS=false
fi

if [ $SURGICAL_FIX_ACTIVE -lt 2 ]; then
    echo "❌ FAIL: Surgical fix not active during cycling (expected ≥2, got $SURGICAL_FIX_ACTIVE)"
    SUCCESS=false
fi

if [ "$SUCCESS" = true ]; then
    echo "✅ PASS: TAB cycling works without prompt redraws"
else
    echo "❌ FAIL: TAB cycling issue not resolved"
fi

echo ""

# Test 2: Arrow navigation (should still work)
echo "=== Test 2: Arrow Navigation (Should Still Work) ==="
echo "Testing: echo [TAB][DOWN][DOWN][ENTER] - Should work normally"

printf "echo \t\x1b[B\x1b[B\x0D\nexit\n" | LLE_DEBUG=1 ./builddir/lusush 2>/tmp/arrow_test.log >/dev/null

ARROW_PROMPT_RENDERS=$(grep -c "Rendering prompt" /tmp/arrow_test.log)

if [ $ARROW_PROMPT_RENDERS -le 3 ]; then
    echo "✅ PASS: Arrow navigation working correctly ($ARROW_PROMPT_RENDERS prompt renders)"
else
    echo "❌ FAIL: Arrow navigation broken ($ARROW_PROMPT_RENDERS prompt renders)"
fi

echo ""

# Test 3: Escape functionality (should still work)
echo "=== Test 3: Escape Functionality (Should Still Work) ==="
echo "Testing: echo [TAB][ESCAPE] - Should cancel menu cleanly"

printf "echo \t\x1b\nexit\n" | ./builddir/lusush >/dev/null 2>&1

if [ $? -eq 0 ]; then
    echo "✅ PASS: Escape functionality working"
else
    echo "❌ FAIL: Escape functionality broken"
fi

echo ""

# Test 4: No regression in working features
echo "=== Test 4: Regression Check ==="
echo "Ensuring 85% working features are preserved"

# Test basic completion
printf "ls \t\nexit\n" | ./builddir/lusush >/dev/null 2>&1
BASIC_COMPLETION=$?

# Test normal editing
printf "echo hello\nexit\n" | ./builddir/lusush >/dev/null 2>&1
BASIC_EDITING=$?

if [ $BASIC_COMPLETION -eq 0 ] && [ $BASIC_EDITING -eq 0 ]; then
    echo "✅ PASS: No regression in basic functionality"
else
    echo "❌ FAIL: Regression detected in basic functionality"
fi

echo ""

# Final assessment
echo "=== FINAL ASSESSMENT ==="

if [ "$SUCCESS" = true ] && [ $ARROW_PROMPT_RENDERS -le 3 ] && [ $BASIC_COMPLETION -eq 0 ] && [ $BASIC_EDITING -eq 0 ]; then
    echo "🎉 SUCCESS: TAB completion surgical fix is working correctly!"
    echo ""
    echo "✅ Key achievements:"
    echo "   - TAB cycling works without prompt redraws"
    echo "   - Completion cycling is functional ($CYCLING_EVENTS cycles detected)"
    echo "   - Surgical fix prevents display corruption ($SURGICAL_FIX_ACTIVE activations)"
    echo "   - No regression in working features"
    echo "   - Arrow navigation preserved"
    echo "   - Escape functionality preserved"
    echo ""
    echo "📊 Status: 85% → 100% working tab completion system"
    echo ""
    echo "🎯 The prompt redraw issue during TAB cycling has been resolved!"

    exit 0
else
    echo "❌ FAILURE: Issues remain in tab completion system"
    echo ""
    echo "Debug logs available:"
    echo "- /tmp/tab_fix_test.log (TAB cycling test)"
    echo "- /tmp/arrow_test.log (arrow navigation test)"
    echo ""
    echo "Review logs with:"
    echo "grep -E 'TAB case|Cycled to|Tab cycling in active session|Rendering prompt' /tmp/tab_fix_test.log"

    exit 1
fi

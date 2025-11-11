#!/bin/bash
# Quick UTF-8 Test Runner
# Provides easy copy/paste test strings for manual testing

echo "=========================================="
echo "Phase 1 UTF-8 Quick Test - Copy/Paste Strings"
echo "=========================================="
echo ""
echo "Copy each line below and paste into lusush to test:"
echo ""

echo "Test 1: Basic 2-byte UTF-8"
echo "café"
echo ""

echo "Test 2: 3-byte CJK character"
echo "日本"
echo ""

echo "Test 3: 4-byte emoji"
echo "🎉🎊"
echo ""

echo "Test 4: Family emoji (complex grapheme)"
echo "👨‍👩‍👧‍👦"
echo ""

echo "Test 5: Flag emoji"
echo "🇺🇸"
echo ""

echo "Test 6: Mixed ASCII and UTF-8"
echo "echo Hello 世界"
echo ""

echo "Test 7: Skin tone modifier"
echo "👋🏽"
echo ""

echo "=========================================="
echo "How to test:"
echo "1. Run: ./builddir/lusush"
echo "2. Copy/paste each line above"
echo "3. Test cursor movement with arrow keys"
echo "4. Test backspace - should delete entire character"
echo "5. For Test 6, press Enter to execute command"
echo "=========================================="

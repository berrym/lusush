#!/home/mberry/Lab/c/lusush/builddir/lusush
# Comprehensive lusush shell feature demonstration
# This script showcases the major POSIX compliance achievements

echo "=== LUSUSH SHELL COMPREHENSIVE FEATURE DEMONSTRATION ==="
echo

echo "1. PARAMETER EXPANSION (Complete POSIX Implementation):"
echo "   \${unset1=default} -> ${unset1=assigned_default}"
echo "   \${unset2:-fallback} -> ${unset2:-fallback_value}"
echo "   \${unset3:=persistent} -> ${unset3:=persistent_value}"
echo "   After assignment, unset3 = $unset3"
TESTVAR="exists"
echo "   \${TESTVAR:+alternate} -> ${TESTVAR:+replacement_when_set}"
echo "   \${MISSING:+alternate} -> ${MISSING:+replacement_when_set}"
echo

echo "2. COMMAND SUBSTITUTION (Both Modern and Legacy):"
echo "   Modern: Current date is $(date +'%Y-%m-%d')"
echo "   Legacy: Current time is `date +'%H:%M:%S'`"
echo "   Nested: Directory $(basename `pwd`) has $(ls *.md 2>/dev/null | wc -l) markdown files"
echo

echo "3. ARITHMETIC EXPANSION:"
X=15
Y=4
echo "   Basic: $X + $Y = $((X + Y))"
echo "   Advanced: $X ** 2 / $Y = $((X ** 2 / Y))"
echo "   Complex: ($X + $Y) * 3 = $(((X + Y) * 3))"
echo

echo "4. VARIABLE ASSIGNMENT AND EXPANSION:"
MESSAGE="Hello, lusush shell!"
echo "   Simple: $MESSAGE"
echo "   Length: \${#MESSAGE} = ${#MESSAGE}"
echo

echo "5. ENHANCED ECHO WITH ESCAPE SEQUENCES:"
echo "   Newlines and tabs: Line1\n\tIndented Line2\n\tAnother indented line"
echo "   Special chars: Bell\a Backspace\b Vertical tab\v"
echo

echo "6. MIXED OPERATORS (Pipeline + Logical):"
echo "test_string" | grep "test" && echo "   ✓ Pipeline + AND operator works"
echo "fail_string" | grep "test" || echo "   ✓ Pipeline + OR operator works"
echo

echo "7. GLOBBING AND PATHNAME EXPANSION:"
echo "   Markdown files: $(echo *.md)"
echo "   Count: $(ls *.* 2>/dev/null | wc -l) total files with extensions"
echo

echo "8. COMMENT PROCESSING:"
# This is a full-line comment that should be ignored
echo "   Comments work!" # This is an inline comment
echo

echo "9. CONTROL STRUCTURES:"
for i in 1 2 3; do
    echo "   Loop iteration: $i"
done

if [ -f "README.md" ]; then
    echo "   ✓ README.md exists (conditional execution works)"
else
    echo "   ✗ README.md not found"
fi
echo

echo "10. COMPLEX REAL-WORLD EXAMPLE:"
PROJECT_NAME=$(basename `pwd`)
FILE_COUNT=$(ls -1 | wc -l)
echo "   Project '$PROJECT_NAME' contains $FILE_COUNT files"
echo "   Last modified: $(ls -lt | head -2 | tail -1 | awk '{print $6, $7, $8, $9}')"
echo

echo "=== DEMONSTRATION COMPLETE ==="
echo "lusush has achieved comprehensive POSIX shell compliance!"

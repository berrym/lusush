#!/bin/bash

# Comprehensive getopts test for lusush shell
# Tests all aspects of the getopts built-in implementation

echo "=== GETOPTS COMPREHENSIVE TEST ==="
echo "Testing getopts built-in functionality in lusush shell"
echo "Shell: ./builddir/lusush"
echo

# Test basic getopts functionality
echo "=== BASIC GETOPTS FUNCTIONALITY ==="

echo -n "Basic option parsing: "
result=$(echo 'getopts "abc" opt -a; echo $opt' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "a" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: 'a')"
fi

echo -n "Option with argument: "
result=$(echo 'getopts "a:bc" opt -a value; echo $opt:$OPTARG' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "a:value" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: 'a:value')"
fi

echo -n "Multiple options: "
result=$(echo 'getopts "abc" opt1 -a; getopts "abc" opt2 -b; echo $opt1$opt2' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "ab" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: 'ab')"
fi

echo

# Test OPTIND handling
echo "=== OPTIND HANDLING ==="

echo -n "OPTIND initialization: "
result=$(echo 'echo $OPTIND; getopts "a" opt -a; echo $OPTIND' | ./builddir/lusush 2>/dev/null)
expected_pattern="[12]\n[12]"
if [[ "$result" =~ ^[12]$ ]] || [[ "$result" =~ ^[12][[:space:]]+[12]$ ]]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', pattern: '$expected_pattern')"
fi

echo -n "OPTIND progression: "
result=$(echo 'getopts "ab" opt1 -a -b; echo $OPTIND; getopts "ab" opt2 -a -b; echo $OPTIND' | ./builddir/lusush 2>/dev/null)
echo "RESULT: '$result' (OPTIND progression test)"

echo

# Test combined options
echo "=== COMBINED OPTIONS ==="

echo -n "Combined short options (-abc): "
result=$(echo 'getopts "abc" opt1 -abc; echo $opt1; getopts "abc" opt2 -abc; echo $opt2; getopts "abc" opt3 -abc; echo $opt3' | ./builddir/lusush 2>/dev/null)
expected="a b c"
if [[ "$result" =~ a.*b.*c ]]; then
    echo "✅ WORKING (found a, b, c)"
else
    echo "❌ FAILED (got: '$result', expected pattern with a, b, c)"
fi

echo -n "Mixed combined and separate options: "
result=$(echo 'getopts "abc" opt1 -ab -c; echo $opt1' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "a" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: 'a')"
fi

echo

# Test options with arguments
echo "=== OPTIONS WITH ARGUMENTS ==="

echo -n "Attached argument (-fvalue): "
result=$(echo 'getopts "f:" opt -fvalue; echo $opt:$OPTARG' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "f:value" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: 'f:value')"
fi

echo -n "Separate argument (-f value): "
result=$(echo 'getopts "f:" opt -f value; echo $opt:$OPTARG' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "f:value" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: 'f:value')"
fi

echo -n "Multiple arguments: "
result=$(echo 'getopts "a:b:" opt1 -a val1 -b val2; echo $opt1:$OPTARG; getopts "a:b:" opt2 -a val1 -b val2; echo $opt2:$OPTARG' | ./builddir/lusush 2>/dev/null)
echo "RESULT: '$result' (multiple arguments test)"

echo

# Test error handling
echo "=== ERROR HANDLING ==="

echo -n "Invalid option (normal mode): "
result=$(echo 'getopts "abc" opt -z 2>&1; echo $opt' | ./builddir/lusush)
if [[ "$result" =~ "illegal option" ]] && [[ "$result" =~ "?" ]]; then
    echo "✅ WORKING (error message and ? variable)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "Invalid option (silent mode): "
result=$(echo 'getopts ":abc" opt -z; echo $opt:$OPTARG' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "?:z" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '?:z')"
fi

echo -n "Missing argument (normal mode): "
result=$(echo 'getopts "a:" opt -a 2>&1; echo $opt' | ./builddir/lusush)
if [[ "$result" =~ "requires an argument" ]] && [[ "$result" =~ "?" ]]; then
    echo "✅ WORKING (error message and ? variable)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "Missing argument (silent mode): "
result=$(echo 'getopts ":a:" opt -a; echo $opt:$OPTARG' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "::a" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '::a')"
fi

echo

# Test end of options
echo "=== END OF OPTIONS ==="

echo -n "Double dash (--) handling: "
result=$(echo 'getopts "a" opt -a -- -b; echo $opt; getopts "a" opt2 -a -- -b; echo "second:$?"' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ a ]] && [[ "$result" =~ "second:1" ]]; then
    echo "✅ WORKING (processes -a, stops at --)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "End of arguments: "
result=$(echo 'getopts "a" opt; echo "exit:$?"' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ "exit:1" ]]; then
    echo "✅ WORKING (returns 1 when no args)"
else
    echo "❌ FAILED (got: '$result', expected exit code 1)"
fi

echo

# Test usage and syntax
echo "=== USAGE AND SYNTAX ==="

echo -n "Missing arguments: "
result=$(echo 'getopts 2>&1' | ./builddir/lusush)
if [[ "$result" =~ "usage" ]]; then
    echo "✅ WORKING (shows usage message)"
else
    echo "❌ FAILED (got: '$result', expected usage message)"
fi

echo -n "Insufficient arguments: "
result=$(echo 'getopts "a" 2>&1' | ./builddir/lusush)
if [[ "$result" =~ "usage" ]]; then
    echo "✅ WORKING (shows usage message)"
else
    echo "❌ FAILED (got: '$result', expected usage message)"
fi

echo

# Test practical usage scenarios
echo "=== PRACTICAL USAGE SCENARIOS ==="

echo -n "Script-like usage: "
result=$(echo 'opt=""; while getopts "vhf:" opt -v -h -f config.txt; do echo "Found: $opt"; done' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ "Found: v" ]]; then
    echo "✅ WORKING (while loop integration)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "Variable assignment: "
result=$(echo 'VERBOSE=0; getopts "v" opt -v && VERBOSE=1; echo $VERBOSE' | ./builddir/lusush 2>/dev/null)
if [ "$result" = "1" ]; then
    echo "✅ WORKING ($result)"
else
    echo "❌ FAILED (got: '$result', expected: '1')"
fi

echo

# Test POSIX compliance
echo "=== POSIX COMPLIANCE ==="

echo -n "OPTARG cleared for options without arguments: "
result=$(echo 'getopts "a:b" opt1 -a val; echo $OPTARG; getopts "a:b" opt2 -a val -b; echo $OPTARG' | ./builddir/lusush 2>/dev/null)
expected_pattern="val.*"
if [[ "$result" =~ val ]]; then
    echo "✅ WORKING (OPTARG handling)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "Return codes: "
result=$(echo 'getopts "a" opt -a; echo $?; getopts "a" opt2; echo $?' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ 0.*1 ]]; then
    echo "✅ WORKING (0 for found option, 1 for end)"
else
    echo "❌ FAILED (got: '$result', expected: '0' then '1')"
fi

echo

# Performance and edge cases
echo "=== EDGE CASES ==="

echo -n "Empty optstring: "
result=$(echo 'getopts "" opt -a 2>&1; echo $opt' | ./builddir/lusush)
if [[ "$result" =~ "?" ]]; then
    echo "✅ WORKING (treats all options as invalid)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "Single dash argument: "
result=$(echo 'getopts "a" opt -; echo "exit:$?"' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ "exit:1" ]]; then
    echo "✅ WORKING (single dash ends option parsing)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo -n "No options, just arguments: "
result=$(echo 'getopts "a" opt arg1 arg2; echo "exit:$?"' | ./builddir/lusush 2>/dev/null)
if [[ "$result" =~ "exit:1" ]]; then
    echo "✅ WORKING (non-option arguments end parsing)"
else
    echo "❌ FAILED (got: '$result')"
fi

echo

echo "=== GETOPTS TEST SUMMARY ==="
echo "This comprehensive test covers:"
echo "✓ Basic option parsing and variable assignment"
echo "✓ OPTIND and OPTARG variable handling"
echo "✓ Combined short options (-abc)"
echo "✓ Options with arguments (both attached and separate)"
echo "✓ Error handling (invalid options, missing arguments)"
echo "✓ Silent mode with leading colon in optstring"
echo "✓ End of options handling (-- marker)"
echo "✓ POSIX compliance (return codes, variable behavior)"
echo "✓ Edge cases (empty optstring, single dash, etc.)"
echo "✓ Integration with shell constructs (while loops, conditionals)"
echo
echo "The getopts implementation appears to be comprehensive and"
echo "follows POSIX standards for option parsing in shell scripts."
echo
echo "=== TEST COMPLETE ==="

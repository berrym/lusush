#!/bin/bash

# Lusush Shell - Comprehensive Autosuggestions Test Suite
# Professional validation of autosuggestions functionality after history corruption fix
# Tests both layered display enabled/disabled states with systematic coverage

set -e

LUSUSH_BINARY="./builddir/lusush"
TEST_LOG="autosuggestions_test_results.log"
TEMP_INPUT="temp_test_input.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${BLUE}=== LUSUSH COMPREHENSIVE AUTOSUGGESTIONS TEST SUITE ===${NC}"
echo "Testing autosuggestions functionality after history corruption fix"
echo "Timestamp: $(date)"
echo ""

# Function to run a test and capture output
run_test() {
    local test_name="$1"
    local input_commands="$2"
    local expected_patterns="$3"
    local description="$4"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -e "${YELLOW}TEST $TOTAL_TESTS: $test_name${NC}"
    echo "Description: $description"

    # Create input file
    echo -e "$input_commands" > "$TEMP_INPUT"

    # Run test and capture output
    local output
    output=$($LUSUSH_BINARY -i < "$TEMP_INPUT" 2>&1)

    # Check for expected patterns
    local all_patterns_found=true
    local pattern_results=""

    IFS='|' read -ra PATTERNS <<< "$expected_patterns"
    for pattern in "${PATTERNS[@]}"; do
        if echo "$output" | grep -q "$pattern"; then
            pattern_results+="\n  ✓ Found: '$pattern'"
        else
            pattern_results+="\n  ✗ Missing: '$pattern'"
            all_patterns_found=false
        fi
    done

    if $all_patterns_found; then
        echo -e "${GREEN}PASSED${NC}$pattern_results"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAILED${NC}$pattern_results"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo -e "\n--- OUTPUT SAMPLE ---"
        echo "$output" | tail -20
        echo "--- END OUTPUT ---"
    fi

    echo ""

    # Log detailed results
    {
        echo "=== TEST $TOTAL_TESTS: $test_name ==="
        echo "Input: $input_commands"
        echo "Expected: $expected_patterns"
        echo "Result: $(if $all_patterns_found; then echo "PASSED"; else echo "FAILED"; fi)"
        echo "Output:"
        echo "$output"
        echo ""
    } >> "$TEST_LOG"

    # Cleanup
    rm -f "$TEMP_INPUT"
}

# Initialize log
echo "Lusush Autosuggestions Test Log - $(date)" > "$TEST_LOG"
echo "" >> "$TEST_LOG"

echo -e "${BLUE}Phase 1: Basic Functionality Tests${NC}"

# Test 1: Basic autosuggestions with display enabled
run_test "Basic Autosuggestions (Display Enabled)" \
"display enable
config set behavior.debug_mode true
echo hello_world
echo hel
exit" \
"Match found.*score=.*echo hello_world|History suggestion generated.*ell" \
"Test basic autosuggestions functionality with layered display enabled"

# Test 2: Basic autosuggestions with display disabled
run_test "Basic Autosuggestions (Display Disabled)" \
"display disable
config set behavior.debug_mode true
echo test_command
echo tes
exit" \
"Match found.*score=.*echo test_command|History suggestion generated.*t_command" \
"Test basic autosuggestions functionality with layered display disabled"

# Test 3: Multiple commands in history
run_test "Multiple Command History" \
"display enable
config set behavior.debug_mode true
echo first_command
echo second_command
echo third_command
echo fir
exit" \
"Match found.*score=.*echo first_command|History suggestion generated.*st_command" \
"Test autosuggestions with multiple similar commands in history"

echo -e "${BLUE}Phase 2: Complex Command Tests${NC}"

# Test 4: Git commands
run_test "Git Command Autosuggestions" \
"display enable
config set behavior.debug_mode true
git status --porcelain
git commit -m \"test commit\"
git st
exit" \
"Match found.*score=.*git status|History suggestion generated" \
"Test autosuggestions with git commands"

# Test 5: File operations
run_test "File Operation Commands" \
"display disable
config set behavior.debug_mode true
ls -la /usr/local
ls -la /var/log
ls -la /usr
exit" \
"Match found.*score=.*ls -la /usr|History suggestion generated" \
"Test autosuggestions with file operation commands"

# Test 6: Complex multiline constructs
run_test "Complex Command Constructs" \
"display enable
config set behavior.debug_mode true
for i in 1 2 3; do echo \"Loop item: \$i\"; done
if [ -f /etc/passwd ]; then echo \"File exists\"; fi
for i in 4 5 6; do echo \"Another loop: \$i\"; done
for
exit" \
"Match found.*score=.*for i in|History suggestion generated" \
"Test autosuggestions with complex multiline constructs"

echo -e "${BLUE}Phase 3: Edge Cases and Error Conditions${NC}"

# Test 7: Empty input
run_test "Empty Input Handling" \
"display enable
config set behavior.debug_mode true
echo test_empty

exit" \
"Skipping - no input or cursor not at end" \
"Test autosuggestions behavior with empty input"

# Test 8: Special characters
run_test "Special Character Commands" \
"display disable
config set behavior.debug_mode true
echo \"quoted string with spaces\"
echo 'single quoted'
echo \$HOME/test
echo \"quo
exit" \
"Match found.*score=.*echo \"quoted string|History suggestion generated" \
"Test autosuggestions with special characters and quotes"

# Test 9: Very long commands
run_test "Long Command Handling" \
"display enable
config set behavior.debug_mode true
echo \"This is a very long command that should test the terminal width safety mechanisms and ensure that autosuggestions work properly\"
echo \"This is a very
exit" \
"Match found.*score=.*This is a very long|History suggestion generated" \
"Test autosuggestions with very long commands"

echo -e "${BLUE}Phase 4: Performance and Ranking Tests${NC}"

# Test 10: Suggestion ranking
run_test "Suggestion Ranking" \
"display disable
config set behavior.debug_mode true
echo apple_pie
echo apple_juice
echo apple_sauce
echo app
exit" \
"Match found.*score=.*apple|History suggestion generated.*le" \
"Test autosuggestions ranking with multiple similar matches"

# Test 11: Recent vs frequent commands
run_test "Recent vs Frequent Commands" \
"display enable
config set behavior.debug_mode true
echo frequent_command
echo frequent_command
echo frequent_command
echo recent_command
echo fre
exit" \
"Match found.*score=.*frequent_command|History suggestion generated" \
"Test autosuggestions prioritization of frequent vs recent commands"

echo -e "${BLUE}Phase 5: Configuration and State Tests${NC}"

# Test 12: Configuration changes
run_test "Configuration State Changes" \
"display enable
config set display.autosuggestions false
config set behavior.debug_mode true
echo config_test
echo con
config set display.autosuggestions true
echo con
exit" \
"lusush_get_suggestion returned NULL|Match found.*score=.*config_test" \
"Test autosuggestions behavior with configuration changes"

# Test 13: Display state transitions
run_test "Display State Transitions" \
"display disable
config set behavior.debug_mode true
echo state_test_command
echo sta
display enable
echo sta
display disable
echo sta
exit" \
"Match found.*score=.*state_test_command" \
"Test autosuggestions consistency across display state changes"

echo -e "${BLUE}Phase 6: Integration Tests${NC}"

# Test 14: Theme integration
run_test "Theme Integration" \
"display enable
config set behavior.debug_mode true
theme set dark
echo theme_dark_test
echo the
theme set corporate
echo the
exit" \
"Match found.*score=.*theme_dark_test|History suggestion generated" \
"Test autosuggestions integration with theme system"

# Test 15: History persistence
run_test "History Persistence" \
"display enable
config set behavior.debug_mode true
echo persistent_command
history | grep persistent
echo per
exit" \
"persistent_command|Match found.*score=.*persistent_command" \
"Test autosuggestions with history persistence"

# Final comprehensive test
run_test "Comprehensive Integration" \
"display enable
theme set colorful
config set behavior.debug_mode true
echo comprehensive_test_start
ls -la
git status
for i in 1 2; do echo \"Test \$i\"; done
echo comp
display disable
echo comp
display enable
echo comp
exit" \
"Match found.*score=.*comprehensive_test_start|History suggestion generated.*rehensive_test_start" \
"Final comprehensive integration test covering all major features"

# Summary
echo -e "${BLUE}=== TEST RESULTS SUMMARY ===${NC}"
echo "Total Tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}🎉 ALL TESTS PASSED! Autosuggestions system is fully functional.${NC}"
    exit 0
else
    echo -e "\n${RED}⚠️  Some tests failed. Check $TEST_LOG for detailed results.${NC}"
    echo -e "Success rate: $(( PASSED_TESTS * 100 / TOTAL_TESTS ))%"
    exit 1
fi

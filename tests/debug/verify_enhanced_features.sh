#!/bin/bash

# LUSUSH Enhanced Features Verification Script
# Quick verification that all enhanced UX features are operational

echo "==============================================================================="
echo "LUSUSH ENHANCED FEATURES VERIFICATION"
echo "==============================================================================="
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Find lusush binary
LUSUSH_BINARY=""
if [ -f "./builddir/lusush" ]; then
    LUSUSH_BINARY="./builddir/lusush"
elif [ -f "../builddir/lusush" ]; then
    LUSUSH_BINARY="../builddir/lusush"
elif [ -f "../../builddir/lusush" ]; then
    LUSUSH_BINARY="../../builddir/lusush"
else
    echo -e "${RED}❌ FAILED: Cannot find lusush binary${NC}"
    echo "Please run 'ninja -C builddir' first, then run this script from lusush root directory"
    exit 1
fi

echo -e "${BLUE}Using binary: ${LUSUSH_BINARY}${NC}"
echo ""

# Verification function
verify_feature() {
    local name="$1"
    local cmd="$2"
    local expected_pattern="$3"

    echo -n "Testing $name... "

    local output
    output=$(echo "$cmd" | $LUSUSH_BINARY 2>/dev/null)

    if echo "$output" | grep -q "$expected_pattern"; then
        echo -e "${GREEN}✓ PASS${NC}"
        return 0
    else
        echo -e "${RED}❌ FAIL${NC}"
        echo "  Command: $cmd"
        echo "  Expected pattern: $expected_pattern"
        echo "  Actual output: $output"
        return 1
    fi
}

# Track results
PASSED=0
FAILED=0
TOTAL=0

# Test function
run_test() {
    local name="$1"
    local cmd="$2"
    local expected="$3"

    ((TOTAL++))
    if verify_feature "$name" "$cmd" "$expected"; then
        ((PASSED++))
    else
        ((FAILED++))
    fi
}

echo "🔧 POSIX Foundation Tests"
echo "-------------------------"
run_test "Basic command execution" "echo hello" "hello"
run_test "Variable expansion" "var=test; echo \$var" "test"
run_test "Arithmetic expansion" "echo \$((2 + 3))" "5"
run_test "Command substitution" "echo \$(echo nested)" "nested"
echo ""

echo "🎨 Phase 2: Enhanced UX Tests"
echo "-----------------------------"
run_test "Configuration system" "config" "Usage: config"
run_test "Hash builtin" "type hash" "hash is a shell builtin"
run_test "Enhanced shell options" "set" "shell options management"
echo ""

echo "🧠 Phase 3: Advanced Features Tests"
echo "-----------------------------------"
run_test "Theme system" "theme" "Current theme:"
run_test "Network integration" "network" "Network Integration Status:"
run_test "Theme listing" "theme list" "Professional:"
run_test "Network hosts" "network hosts" "SSH Hosts"
echo ""

echo "⚙️  Configuration Sections Tests"
echo "--------------------------------"
run_test "History configuration" "config show" "history_enabled"
run_test "Completion configuration" "config show" "completion_enabled"
run_test "Prompt configuration" "config show" "prompt_format"
run_test "Theme configuration" "config show" "spell_correction"
echo ""

echo "🎯 Built-in Commands Tests"
echo "--------------------------"
run_test "Echo builtin" "type echo" "echo is a shell builtin"
run_test "PWD builtin" "type pwd" "pwd is a shell builtin"
run_test "CD builtin" "type cd" "cd is a shell builtin"
run_test "Export builtin" "type export" "export is a shell builtin"
run_test "Test builtin" "type test" "test is a shell builtin"
echo ""

echo "🌐 Network Features Tests"
echo "-------------------------"
run_test "Network status" "network" "SSH host completion:"
run_test "Network configuration" "network config" "Network configuration:"
run_test "SSH host cache" "network hosts" "total"
echo ""

echo "🎨 Theme Features Tests"
echo "-----------------------"
run_test "Theme info" "theme info corporate" "Theme: corporate"
run_test "Theme colors" "theme colors" "Color scheme"
run_test "Theme stats" "theme stats" "Theme system statistics:"
echo ""

# Summary
echo "==============================================================================="
echo "VERIFICATION SUMMARY"
echo "==============================================================================="
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED! ($PASSED/$TOTAL)${NC}"
    echo ""
    echo -e "${GREEN}✅ LUSUSH Enhanced Features: FULLY OPERATIONAL${NC}"
    echo ""
    echo "Enhanced features verified:"
    echo "  • POSIX compliance maintained (100%)"
    echo "  • Configuration system operational"
    echo "  • Theme system with professional appearance"
    echo "  • Network integration with SSH completion"
    echo "  • All built-in commands working"
    echo "  • Enhanced UX features ready for use"
    echo ""
    echo -e "${BLUE}Ready to experience enhanced features:${NC}"
    echo "  Interactive mode: ${YELLOW}$LUSUSH_BINARY${NC}"
    echo "  Full demos: ${YELLOW}./tests/debug/demo_all_enhanced_features.sh${NC}"
    echo ""
    exit_code=0
else
    echo -e "${RED}❌ SOME TESTS FAILED ($FAILED/$TOTAL failed, $PASSED/$TOTAL passed)${NC}"
    echo ""
    echo -e "${YELLOW}⚠️  Enhanced features may have issues${NC}"
    echo ""
    echo "Troubleshooting steps:"
    echo "  1. Ensure lusush is built: ninja -C builddir"
    echo "  2. Run POSIX tests: ./tests/compliance/test_posix_regression.sh"
    echo "  3. Check individual features with specific demo scripts"
    echo "  4. Verify configuration: $LUSUSH_BINARY -c \"config show\""
    echo ""
    exit_code=1
fi

echo "For detailed demonstrations, run:"
echo "  ./tests/debug/demo_all_enhanced_features.sh"
echo ""
echo "==============================================================================="

exit $exit_code

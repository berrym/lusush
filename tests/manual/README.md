# LUSUSH Manual Testing Suite

This directory contains manual testing scripts for verifying LUSUSH shell functionality that requires interactive testing or visual verification.

## Overview

Manual tests complement the automated test suite by covering scenarios that require:
- Interactive user input
- Visual verification of terminal behavior
- Real-time testing of editing operations
- Cross-platform terminal compatibility verification

## Available Test Scripts

### Core Functionality Tests

#### `test_history_bottom_line.sh`
**Purpose**: Tests history navigation at the bottom of the terminal screen
**Focus**: Verifies that history operations don't consume lines above the prompt
**Usage**: 
```bash
./test_history_bottom_line.sh
```
**What it tests**:
- History navigation with UP/DOWN arrows
- Line consumption prevention
- Bottom-line editing stability
- Visual integrity of terminal content

#### `test_cursor_positioning_fix.sh`
**Purpose**: Verifies cursor positioning accuracy after character operations
**Focus**: Tests that cursor stays at the correct position after backspace
**Usage**:
```bash
./test_cursor_positioning_fix.sh
```
**What it tests**:
- Backspace cursor positioning
- Character deletion accuracy
- Visual cursor placement
- Multi-character operations

#### `test_prompt_length_fix.sh`
**Purpose**: Tests prompt integrity during editing operations
**Focus**: Verifies prompt doesn't get corrupted or truncated
**Usage**:
```bash
./test_prompt_length_fix.sh
```
**What it tests**:
- Prompt preservation during backspace
- Prompt integrity during history navigation
- Complete prompt string display
- Length calculation accuracy

## Testing Procedures

### Before Running Tests

1. **Build LUSUSH**:
   ```bash
   cd ../../..  # Return to project root
   ninja -C builddir
   ```

2. **Verify POSIX Compliance**:
   ```bash
   ./tests/compliance/test_posix_regression.sh
   ```

3. **Prepare Terminal**:
   - Use a standard terminal with ANSI color support
   - Ensure terminal is at least 80x24 characters
   - Test in both light and dark themes

### Running Individual Tests

```bash
# Navigate to manual tests directory
cd lusush/tests/manual

# Run specific test
./test_history_bottom_line.sh

# Run all manual tests
for test in test_*.sh; do
    echo "Running $test..."
    ./"$test"
    echo "Press Enter to continue to next test..."
    read
done
```

### Test Environment Setup

Each test script will:
1. Check for LUSUSH binary existence
2. Fill terminal with marker lines for visual reference
3. Provide clear instructions for manual verification
4. Guide you through specific test scenarios
5. Ask for confirmation of expected behaviors

## Test Categories

### Terminal Editing Tests
- **Focus**: Core line editing functionality
- **Scripts**: `test_cursor_positioning_fix.sh`, `test_prompt_length_fix.sh`
- **Verification**: Visual cursor behavior, prompt integrity

### History Navigation Tests  
- **Focus**: Command history and navigation
- **Scripts**: `test_history_bottom_line.sh`
- **Verification**: Line consumption, navigation accuracy

### Cross-Platform Tests
- **Focus**: Terminal compatibility across different systems
- **Scripts**: All tests should work on Linux, macOS, BSD
- **Verification**: Consistent behavior across platforms

## Expected Results

### Successful Test Indicators
✅ **Cursor Positioning**: Cursor appears exactly where expected after operations
✅ **Prompt Integrity**: Complete prompt always visible and unchanged
✅ **No Line Consumption**: Terminal content above prompt remains intact
✅ **Smooth Navigation**: History navigation replaces current line cleanly
✅ **Visual Consistency**: No artifacts, jumping, or visual glitches

### Failure Indicators
❌ **Cursor Jumping**: Cursor appears in wrong location
❌ **Prompt Corruption**: Parts of prompt missing or mangled
❌ **Line Loss**: Content above prompt disappears
❌ **Navigation Issues**: New prompts created instead of line replacement
❌ **Visual Artifacts**: Screen corruption or display issues

## Troubleshooting

### Common Issues

**Test Scripts Won't Run**:
```bash
# Make scripts executable
chmod +x test_*.sh

# Check shebang line
head -1 test_*.sh
```

**LUSUSH Binary Not Found**:
```bash
# Verify build
ls -la ../../../builddir/lusush

# Update PATH or use full path
export PATH="../../../builddir:$PATH"
```

**Terminal Issues**:
- Ensure terminal supports ANSI escape sequences
- Test with different terminal emulators
- Verify terminal size is adequate (80x24 minimum)

### Platform-Specific Notes

**Linux**:
- Works with all major terminal emulators
- Test with both X11 and Wayland

**macOS**:
- Test with both Terminal.app and iTerm2
- Verify compatibility with different macOS versions

**BSD Systems**:
- Test with console and X11 terminals
- Verify ANSI escape sequence support

## Contributing New Tests

### Test Script Guidelines

1. **Clear Purpose**: Each test should have a specific, focused objective
2. **User Guidance**: Provide clear instructions for what to do and observe
3. **Error Checking**: Verify LUSUSH binary exists before testing
4. **Visual Markers**: Use marker lines or clear indicators for verification
5. **Documentation**: Include comments explaining test logic

### Test Script Template

```bash
#!/bin/bash
# TEST_NAME.sh - Description of what this test verifies

set -e

echo "==============================================================================="
echo "TEST NAME - DESCRIPTION"
echo "==============================================================================="

# Check prerequisites
if [ ! -f "../../../builddir/lusush" ]; then
    echo "ERROR: LUSUSH binary not found"
    exit 1
fi

# Test setup
echo "Setting up test environment..."

# Test execution with clear instructions
echo "Test Instructions:"
echo "1. Step one"
echo "2. Step two"
echo "3. What to verify"

# Launch LUSUSH for testing
"../../../builddir/lusush"

# Post-test verification
echo "Did the test pass? (Y/N)"
```

### Submitting New Tests

1. Create test script following the template
2. Test on multiple platforms if possible
3. Document the test purpose and procedures
4. Update this README with test description
5. Submit pull request with clear description

## Test Results Documentation

### Recording Results
When running tests, document:
- Platform and OS version
- Terminal emulator used
- Test results (pass/fail)
- Any observed issues
- Performance characteristics

### Reporting Issues
If tests fail:
1. Note exact failure behavior
2. Include system information
3. Document steps to reproduce
4. Submit issue with test results

## Integration with CI/CD

While these are manual tests, they can be integrated into CI/CD pipelines using:
- Automated terminal recording tools
- Screenshot comparison tools
- Headless terminal environments
- Virtual display servers

See the main test suite documentation for automated testing procedures.

---

**Note**: Manual testing is essential for verifying user experience and visual behavior that automated tests cannot adequately cover. These tests ensure LUSUSH provides a professional, reliable terminal editing experience across all supported platforms.
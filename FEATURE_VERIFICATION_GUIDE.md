# Feature Verification Guide: Tab Completion and Syntax Highlighting

This guide helps verify that the newly implemented tab completion and syntax highlighting features are working correctly in Lusush.

## Overview

Two major interactive features have been implemented:

1. **Tab Completion**: File completion for the current directory
2. **Syntax Highlighting**: Real-time shell command syntax highlighting

Both features are enabled by default and should work immediately in interactive shell sessions.

**RECENT FIXES APPLIED:**
- Tab completion now cycles through multiple matches with repeated Tab presses
- Syntax highlighting now works in incremental display updates
- Both features have been debugged and improved for better user experience

## Prerequisites

- Build the latest version: `scripts/lle_build.sh build`
- Ensure you're in a directory with some files for testing tab completion
- Use a terminal that supports ANSI colors for syntax highlighting
- For debugging, use: `LLE_DEBUG=1 ./builddir/lusush`

## Testing Tab Completion

### Basic File Completion

1. **Start interactive shell:**
   ```bash
   ./builddir/lusush
   ```

2. **Test basic completion:**
   ```bash
   # Type this but don't press Enter - press Tab after the 't'
   echo t<TAB>
   ```
   **Expected:** Should complete to one of the files starting with 't' in current directory
   
   **NEW:** Press Tab again to cycle through other matches!

3. **Test prefix matching:**
   ```bash
   # If you have files like test_file.txt, test_completion.sh
   ls test_<TAB>
   ```
   **Expected:** Should complete to the first matching file
   
   **NEW:** Press Tab multiple times to cycle through all matches (test_file.txt, test_completion.sh, etc.)

4. **Test with longer prefixes:**
   ```bash
   cat test_file<TAB>
   ```
   **Expected:** Should complete to `test_file.txt` if it exists
   
   **NEW:** If multiple files match (like test_file.txt and test_file.c), Tab cycles through them

### Advanced Tab Completion Tests

5. **Test with hidden files (if any exist):**
   ```bash
   ls .<TAB>
   ```
   **Expected:** Should show hidden files starting with '.'

6. **Test with non-matching prefix:**
   ```bash
   echo nonexistent<TAB>
   ```
   **Expected:** Should do nothing (no completion available)

7. **Test in middle of command:**
   ```bash
   # Type this and press Tab after 'test_'
   cat test_<TAB> | grep hello
   ```
   **Expected:** Should complete the filename portion

## Testing Syntax Highlighting

Syntax highlighting applies colors in real-time as you type. **FIXED:** Now works correctly with incremental display updates.

Test these scenarios:

### Basic Command Highlighting

1. **Simple command:**
   ```bash
   echo hello
   ```
   **Expected:** `echo` should be highlighted as a command

2. **String highlighting:**
   ```bash
   echo "hello world"
   ```
   **Expected:** The quoted text should be highlighted differently

3. **Variable highlighting:**
   ```bash
   echo $HOME
   ```
   **Expected:** `$HOME` should be highlighted as a variable

### Advanced Syntax Highlighting

4. **Pipes and operators:**
   ```bash
   ls -la | grep test
   ```
   **Expected:** The `|` operator should be highlighted

5. **Comments:**
   ```bash
   echo hello # this is a comment
   ```
   **Expected:** Everything after `#` should be highlighted as a comment

6. **Complex command:**
   ```bash
   find /tmp -name "*.txt" | head -5
   ```
   **Expected:** Multiple syntax elements should be highlighted appropriately

## Troubleshooting

### Tab Completion Not Working

**Symptoms:** Pressing Tab does nothing or inserts literal tab character

**Checks:**
1. Verify you're in interactive mode (not piping input)
2. Check that files exist in current directory: `ls`
3. Enable debug mode: `LLE_DEBUG=1 ./builddir/lusush`

**Common Issues:**
- Non-interactive shell: Tab completion only works in interactive mode
- No matching files: Tab completion requires matching files in current directory
- Terminal compatibility: Ensure terminal supports raw mode

### Syntax Highlighting Not Working

**Symptoms:** No colors appear, all text looks the same

**Checks:**
1. Verify terminal supports colors: `echo -e "\033[31mRED\033[0m"`
2. Check if colors are enabled in terminal settings
3. Test with simple command like `echo hello`

**Common Issues:**
- Terminal doesn't support ANSI colors
- Color scheme conflicts with syntax highlighting colors  
- SSH sessions may disable colors by default

**RECENT FIX:** Syntax highlighting now properly integrates with incremental display updates and should show colors immediately as you type.

## Debug Mode

For detailed troubleshooting, enable debug output:

```bash
# Enable LLE debug output
LLE_DEBUG=1 ./builddir/lusush

# Enable integration debug output  
LLE_INTEGRATION_DEBUG=1 ./builddir/lusush
```

This will show detailed information about:
- LLE initialization
- Component setup (syntax highlighter, theme integration)
- Key input processing
- Display rendering

## Implementation Details

### Tab Completion
- Scans current directory for matching files
- Uses case-insensitive prefix matching
- Skips hidden files unless prefix starts with '.'
- Integrates with LLE completion framework
- Supports basic file completion (extensible to commands/variables)

### Syntax Highlighting
- Real-time highlighting as you type
- Supports shell keywords, strings, variables, comments, operators
- Integrates with LLE theme system for colors
- Uses efficient region-based highlighting
- Configurable syntax rules and color schemes

## Expected Behavior Summary

| Feature | Input | Expected Result |
|---------|--------|----------------|
| Tab Completion | `echo t<TAB>` | Completes to matching filename |
| Tab Completion | `echo t<TAB><TAB>` | Cycles through multiple matches |
| Tab Completion | `ls nonexist<TAB>` | No action (no matches) |
| Syntax Highlighting | `echo "hello"` | `echo` and quoted string highlighted |
| Syntax Highlighting | `ls | grep test` | Command, pipe operator highlighted |
| Syntax Highlighting | `echo $HOME` | Command and variable highlighted |

## Success Criteria

✅ **Tab Completion Working:** Pressing Tab completes filenames when matches exist  
✅ **Tab Completion Cycling:** Multiple Tab presses cycle through all matches  
✅ **Syntax Highlighting Working:** Commands show colors in real-time as you type  
✅ **No Regressions:** Basic shell functionality still works normally  
✅ **Performance:** No noticeable lag when typing or completing

## Next Steps

If both features work correctly:
1. Tab completion can be extended to support command completion
2. Syntax highlighting can be customized with different color schemes
3. Both features integrate with the existing LLE framework for future enhancements

If issues are found, please report them with:
- Terminal type and version
- Specific commands that fail
- Debug output (using `LLE_DEBUG=1`)
- Expected vs actual behavior
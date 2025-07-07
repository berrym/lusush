# Bottom-Line Cursor Jumping Test Guide

## Current Status
We have reverted to v1.0.0 and **disabled hints by default** to test if the hints system was causing the bottom-line cursor jumping issue.

## Test Configuration
- **Version**: v1.0.0 base with hints disabled
- **Hints Status**: `hints_enabled = false` (disabled by default)
- **Bottom-line Protection**: Original v1.0.0 protection logic in place
- **All Tests**: ✅ 49/49 POSIX + 136/136 comprehensive tests passing

## Manual Testing Instructions

### Step 1: Verify Hints Are Disabled
```bash
./builddir/lusush -c 'config get hints_enabled'
# Should output: false
```

### Step 2: Start Interactive Shell
```bash
./builddir/lusush
```

### Step 3: Create Bottom-Line Scenario
1. **Resize your terminal** to be relatively small (e.g., 20-25 lines tall)
2. **Fill the screen** with content to push the prompt to the bottom:
   ```bash
   for i in {1..30}; do echo "Line $i: Testing bottom-line behavior"; done
   ```
3. The prompt should now be at or near the **bottom line** of your terminal

### Step 4: Test Bottom-Line Behavior

#### Test A: Character Typing
- **Type characters**: `echo hello world`
- **Expected**: Cursor stays at bottom line, no jumping up
- **Watch for**: Any line consumption or cursor movement upward

#### Test B: History Navigation
- **Press UP arrow** to recall previous commands
- **Press DOWN arrow** to navigate through history
- **Expected**: Smooth history navigation without cursor jumping
- **Watch for**: Cursor jumping up or consuming previous lines

#### Test C: Character Deletion
- **Type some text**: `echo test message`
- **Use backspace** to delete characters
- **Expected**: Normal character deletion without affecting previous lines
- **Watch for**: Previous lines being consumed or overwritten

#### Test D: Command Completion
- **Type partial command**: `ec` then press TAB
- **Expected**: Normal tab completion without cursor issues
- **Watch for**: Completion not causing terminal scrolling

### Step 5: Document Results

Record your observations:

#### ✅ FIXED (Hints were the culprit)
If the bottom-line behavior is now working correctly:
- [ ] No cursor jumping when typing
- [ ] History navigation works normally
- [ ] Character deletion works normally
- [ ] Tab completion works normally

**Conclusion**: The hints system was causing the bottom-line cursor jumping issue.
**Next Steps**: We can either:
1. Keep hints disabled (simplest solution)
2. Fix the hints system to work properly at bottom line
3. Make hints conditional on bottom-line detection

#### ❌ STILL BROKEN (Deeper issue)
If cursor jumping still occurs:
- [ ] Cursor still jumps when typing
- [ ] History navigation still problematic
- [ ] Character deletion still consumes lines
- [ ] Tab completion causes issues

**Conclusion**: The issue is deeper than just the hints system.
**Next Steps**: Investigation needed in linenoise refresh logic or terminal handling.

## Technical Details

### Hints System Status
- **Callbacks**: Not set when `hints_enabled = false`
- **refreshShowHints()**: Function exists but not called
- **Performance**: No hints-related processing overhead
- **Memory**: No hints-related allocations

### Bottom-Line Protection
The original v1.0.0 protection logic is active:
- **iTerm2**: One-time margin creation at bottom
- **Standard terminals**: One-time protective newline
- **Static protection**: `protection_applied` flag prevents repeated application

### Verification Commands
```bash
# Check hints status
./builddir/lusush -c 'config get hints_enabled'

# Check shell functionality
./builddir/lusush -c 'echo "Shell working correctly"'

# Run comprehensive tests
./tests/compliance/test_posix_regression.sh
./tests/compliance/test_shell_compliance_comprehensive.sh
```

## Expected Outcome

If hints were the problem, this test should demonstrate that:
1. **Bottom-line typing works correctly** without cursor jumping
2. **All shell functionality** remains intact
3. **Performance** is maintained or improved
4. **Original bottom-line protection** is sufficient when hints don't interfere

This will help us decide the best path forward for fixing the bottom-line cursor jumping issue while preserving the enhanced features of LUSUSH v1.0.0.
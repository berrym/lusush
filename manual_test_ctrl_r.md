# Manual Test for Ctrl+R Reverse Search

## Purpose
Test if Ctrl+R reverse search works without display corruption after the fix.

## Test Procedure

### Step 1: Start Interactive Shell
```bash
cd lusush
script -q -c './builddir/lusush' /dev/null
```

### Step 2: Build History
Type these commands to create history entries:
```
echo "first command"
echo "second command"  
echo "third command"
theme set dark
```

### Step 3: Test Ctrl+R Search
1. Press `Ctrl+R`
2. **Expected**: Should see `(reverse-i-search)` prompt appear cleanly
3. **Check**: No overlap with the lusush themed prompt
4. **Check**: No ANSI color artifacts or corruption

### Step 4: Search for Command
1. Type: `second`
2. **Expected**: Should find and highlight "second command"
3. **Check**: Display should be clean, no corrupted text
4. **Check**: Search interface should overlay properly

### Step 5: Cancel Search
1. Press `Ctrl+G` to cancel
2. **Expected**: Should return to normal themed prompt
3. **Check**: No leftover search artifacts
4. **Check**: Prompt should be clean and properly formatted

### Step 6: Test Search and Execute
1. Press `Ctrl+R` again
2. Type: `theme`
3. **Expected**: Should find "theme set dark"
4. Press `Enter` to execute
5. **Check**: Command should execute normally
6. **Check**: Return to clean prompt

## Success Criteria
- ✅ Ctrl+R shows search prompt without corruption
- ✅ Search interface doesn't overlap with lusush prompt
- ✅ Search results display cleanly
- ✅ Canceling search returns to clean prompt
- ✅ Executing found command works normally
- ✅ No ANSI artifacts or display corruption at any step

## Failure Indicators
- ❌ Search prompt overlaps with lusush prompt
- ❌ ANSI color codes visible as text
- ❌ Corrupted display during search
- ❌ Artifacts left after canceling search
- ❌ Prompt corruption after executing search result

## Notes
- This test requires manual verification
- Pay special attention to display cleanliness
- Test both canceling and executing search results
- Verify themed prompt remains intact throughout
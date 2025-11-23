# LLE Known Issues and Blockers

**Date**: 2025-11-15  
**Status**: ⚠️ ACTIVE DEVELOPMENT - Known Issues Tracked  
**Implementation Status**: Phase 1 complete, Groups 1-6 keybindings implemented, UTF-8 cell storage complete

---

## Executive Summary

**Current State**: Active development - Spec 12 completion system integrated, duplicates fixed

- ⚠️ **4 Active Issues** - Completion enhancements (2 MEDIUM), syntax highlighting (2 MEDIUM)
- ✅ **1 Issue Fixed** - Invalid command highlighting (Issue #4)
- ✅ **Spec 12 v2 completion integrated** - Duplicates eliminated (Session 23)
- ✅ **No Blockers** (all issues are enhancements/cosmetic)
- ✅ **Living document enforcement active**
- ✅ **Meta/Alt keybindings working** (Session 14)
- ✅ **Multi-line prompts working** (Session 14)
- ✅ **Multiline ENTER display bug fixed** (Session 15)
- ✅ **break/continue in loops fixed** (Session 16)
- ✅ **Multiline pipeline execution fixed** (Session 16)
- ✅ **Continuation prompts with full Unicode support** (Session 17-18)
- ✅ **Completion generation proper architecture** (Session 23)

---

## Active Issues

### Issue #7: Completion Menu - Category Disambiguation Not Implemented
**Severity**: MEDIUM  
**Discovered**: 2025-11-22 (Session 23 Part 2)  
**Status**: Not yet fixed (documented for future work)  
**Component**: Completion system v2 / source manager  

**Description**:
When the same command exists in multiple categories (e.g., builtin `echo` vs external `/usr/bin/echo`, or alias `ls` vs external `ls`), the completion system currently deduplicates based on text only, not considering the type/category. This means only one entry appears in the menu, and users cannot disambiguate which version to execute.

**Example Scenarios**:
1. **Builtin vs External**:
   - Builtin `echo` 
   - External `/usr/bin/echo`
   - Current: Only "echo" appears once
   - Problem: Selecting "echo" always executes builtin (due to Lusush prioritization)
   - User cannot explicitly select external version

2. **Alias vs External**:
   - Alias `ls='ls --color=auto'`
   - External `/usr/bin/ls`
   - Current: Only "ls" appears once
   - Problem: Selecting "ls" executes alias
   - User cannot bypass alias to run plain external command

**Current Deduplication Logic** (src/lle/completion/completion_system_v2.c):
```c
static lle_result_t deduplicate_results(lle_completion_result_t *result) {
    // Compares text ONLY, not type:
    if (strcmp(result->items[check].text, text) == 0) {
        duplicate = true;  // Removes regardless of category!
    }
}
```

**Proper Behavior** (recommended):
1. **Keep duplicates with different types**:
   ```c
   if (strcmp(result->items[check].text, text) == 0 &&
       result->items[check].type == result->items[read_pos].type) {
       duplicate = true;  // Only remove if BOTH text AND type match
   }
   ```

2. **Display with category indicators**:
   ```
   echo [builtin]
   echo [command: /usr/bin/echo]
   ls [alias]
   ls [command: /usr/bin/ls]
   ```

3. **Smart insertion on selection**:
   - Builtin selected: insert "echo" (executes builtin due to priority)
   - Command selected: insert "/usr/bin/echo" (full path bypasses builtin)
   - Alias selected: insert "ls" (executes alias)

4. **Future: `command` builtin** (like zsh):
   ```bash
   command echo  # Forces external command, bypasses builtins/aliases
   ```

**Current Workaround**:
Users can type the full path manually: `/usr/bin/echo` instead of relying on completion.

**Why Not Fixed Now**:
1. True conflicts (builtin + external with same name) are rare in practice
2. The critical bug we fixed was "echo" appearing **twice in same category** (both as builtin)
3. Category disambiguation is an **enhancement**, not a blocker
4. Requires changes to:
   - Deduplication logic (compare type + text)
   - Menu display (show category indicators)
   - Insertion logic (insert full path for external commands when disambiguating)
   - Metadata tracking (source functions need to provide full paths)

**Priority**: MEDIUM (enhancement for power users, not critical for basic usage)

**Resolution Plan**:
1. Modify `deduplicate_results()` to compare both text and type
2. Update menu renderer to show category labels for duplicates
3. Implement smart insertion logic based on selected type
4. (Future) Add `command` builtin for explicit external command execution

**Status**: DOCUMENTED - Fix deferred to future session

---

### Issue #8: Completion Menu Single-Column Display (Inefficient Space Usage)
**Severity**: LOW  
**Discovered**: 2025-11-22 (Session 23 Part 2)  
**Status**: Not yet fixed (documented for investigation)  
**Component**: Completion menu display / legacy bash completion integration  

**Description**:
Completion menu currently displays one item per line (single-column), which is inefficient use of screen space. Modern shells like zsh display completions in multiple columns with category headers, making better use of terminal width.

**Current Behavior**:
```
Display all 112 possibilities? (y or n)
e2freefrag
e2fsck
e2fsdroid
e2image
e2label
echo
ed
... (one per line, continues)
```

**Expected Behavior** (zsh-style):
```
completing external command
e2freefrag  e2fsck      e2fsdroid   e2image     e2label
completing builtin command
echo        echotc      echoti
```

**Investigation Findings**:
1. **LLE has multi-column support**: The completion_menu_renderer.c already supports multi-column layout:
   ```c
   .use_multi_column = true,
   ```

2. **Problem**: Menu is likely going through **legacy bash completion display** instead of using the LLE menu renderer

3. **Evidence**: The "Display all X possibilities?" prompt is bash's completion pager, not LLE's menu system

**Root Cause** (suspected):
- v2 completion generates results correctly
- Results not being routed through LLE menu renderer
- Instead falling back to bash/readline completion display
- Need to verify menu rendering path in keybinding integration

**Why This Matters**:
- Screen space efficiency (especially on small terminals)
- Better visual organization with category grouping
- Improved UX matching modern shell expectations
- Professional appearance

**Priority**: LOW (functionality works, this is UX enhancement)

**Resolution Plan**:
1. Trace completion display path in `lle_complete()` keybinding action
2. Verify LLE menu renderer is being called vs bash fallback
3. If using bash fallback: Wire v2 results to LLE menu renderer
4. Enable multi-column display with category headers
5. Test with various terminal widths

**Status**: DOCUMENTED - Investigation needed

**Note**: This may be related to screen_buffer integration that was planned in Session 23. Menu needs to go through proper LLE display system, not bypass to bash completion.

---

### Issue #4: Invalid Commands Highlighted as Valid (Green) ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-11-16 (Session 18+)  
**Fixed**: 2025-11-16 (Session 18+)
**Component**: Syntax highlighting / command validation  

**Description**:
The first word of a command prompt always gets highlighted green (valid command color), even when the command doesn't exist or is invalid. Green highlighting should only appear for:
- Valid executable commands (in PATH)
- Valid builtins
- Valid aliases/functions
- Partial input matching valid commands that can be completed

**Reproduction**:
```bash
$ ehello
  ^^^^^^ - highlighted green but "ehello" is not a valid command
```

**Expected Behavior**:
- Valid commands: Green highlighting
- Invalid commands: Red highlighting (not red background)
- Partial matches during typing: Green if completable, red if invalid

**Fix Applied** (commit 04fde1f):
- Added `command_exists_in_path()` to check PATH for executables
- Modified `classify_token()` to validate commands using:
  - `is_shell_builtin()` for builtins (e.g., echo, cd)
  - `lookup_alias()` for aliases (fixed double-free bug)
  - `command_exists_in_path()` for PATH executables
- Returns `COMMAND_TOKEN_ERROR` for invalid commands
- Changed error_color from red background to red foreground

**Status**: ✅ FIXED AND VERIFIED
- Invalid commands now show as red text
- Valid builtins/aliases/PATH commands show as green
- No crashes when typing aliases

---

### Issue #5: Multiline Input - Builtins Not Highlighted
**Severity**: MEDIUM  
**Discovered**: 2025-11-16 (Session 18+)  
**Status**: Not yet fixed  
**Component**: Syntax highlighting / multiline command processing  

**Description**:
In multiline input, builtin commands are not getting syntax highlighting. Single-line commands highlight correctly, but the same commands in multiline constructs (if/while/for blocks) don't get highlighted.

**Reproduction**:
```bash
$ if true; then
if> echo done
     ^^^^ - "echo" not highlighted (should be green as builtin)
if> fi
```

**Expected Behavior**:
Builtin commands like `echo`, `cd`, `export`, etc. should be highlighted green regardless of whether they appear in single-line or multiline input.

**Root Cause** (suspected):
- Syntax highlighter may only process first line
- Continuation prompt lines may not be passed through highlighter
- Multiline parsing may strip highlighting information

**Impact**:
- Inconsistent user experience between single-line and multiline
- Reduced readability of complex multiline commands
- Makes multiline editing harder to validate visually

**Priority**: MEDIUM (affects multiline editing UX)

---

### Issue #6: Continuation Prompt Incorrectly Highlighted in Quotes
**Severity**: LOW  
**Discovered**: 2025-11-16 (Session 18+)  
**Status**: Not yet fixed  
**Component**: Syntax highlighting / continuation prompt rendering  

**Description**:
When a quoted string spans multiple lines (open quote with continuation), the continuation prompt itself gets highlighted with the quote color (yellow), not just the content after the prompt.

**Reproduction**:
```bash
$ echo "hello
quote> world"
^^^^^^ - continuation prompt "quote> " incorrectly highlighted yellow
```

**Expected Behavior**:
- Continuation prompt: Normal prompt color (not highlighted)
- Content after prompt: Quoted string color (yellow)

**Visual Example**:
```
Current (incorrect):
$ echo "hello
quote> world"
└─────────────┘ all yellow including "quote> "

Expected (correct):
$ echo "hello
quote> world"
       └─────┘ only content highlighted yellow
```

**Root Cause** (suspected):
- Syntax highlighter doesn't distinguish continuation prompt from command content
- Highlighting applied to entire line including prompt prefix
- Prompt prefixes not excluded from highlighting scope

**Impact**:
- Minor visual inconsistency
- Continuation prompt less readable when inside quotes

**Priority**: LOW (cosmetic, doesn't affect functionality)

---

---

## Resolved Issues

### Issue #1: Multiline ENTER Display Bug ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-11-14 (Session 14, Meta/Alt testing)  
**Fixed**: 2025-11-15 (Session 15)
**Component**: Display finalization / lle_readline.c  

**Description**:
When pressing ENTER on a non-final line of multiline input, the output appeared on the line below where ENTER was pressed, rather than after the complete multiline input.

**Root Cause**: 
When cursor was not at end of buffer and ENTER was pressed, the display system rendered with cursor at the wrong position (middle of buffer instead of end). Shell output then appeared at that cursor position instead of after the complete command.

**Fix**:
Move buffer cursor to end using pure LLE API before accepting input. This ensures the display system renders with cursor at the correct position:

```c
/* Move buffer cursor to end */
ctx->buffer->cursor.byte_offset = ctx->buffer->length;
ctx->buffer->cursor.codepoint_index = ctx->buffer->length;
ctx->buffer->cursor.grapheme_index = ctx->buffer->length;

/* Sync cursor_manager with new position */
if (ctx->editor && ctx->editor->cursor_manager) {
    lle_cursor_manager_move_to_byte_offset(
        ctx->editor->cursor_manager,
        ctx->buffer->length
    );
}

/* Refresh display to render cursor at new position */
refresh_display(ctx);
```

**Architecture**:
This fix follows LLE design principles:
- Uses pure LLE buffer and cursor APIs (no direct terminal writes)
- LLE manages buffer state; display system handles rendering
- Clean separation of concerns maintained

**Verification**:
- ✅ ENTER on line 1: Output appears after all 3 lines
- ✅ ENTER on line 2: Output appears after all 3 lines
- ✅ ENTER on line 3: Output appears after all 3 lines
- ✅ Line wrapping works correctly with multi-line prompts
- ✅ Works with both default and themed prompts
- ✅ No regressions in history navigation, multi-line editing, incomplete input, edge cases, or long line wrapping

**Files Modified**:
- `src/lle/lle_readline.c` (lle_accept_line_context function)

**Status**: ✅ FIXED AND VERIFIED

---

### Issue #2: break Statement Inside Loops Broken ✅ FIXED
**Severity**: HIGH  
**Discovered**: Pre-2025-11-14 (existed in v1.3.0, recently discovered)  
**Fixed**: 2025-11-15 (Session 16)
**Component**: Shell interpreter / loop execution  

**Description**:
The `break` and `continue` statements did not work inside `for`, `while`, or `until` loops. Error message "not currently in a loop" was displayed and loops continued executing.

**Reproduction**:
```bash
for i in 1 2 3 4 5; do
    echo $i
    if [ $i -eq 3 ]; then
        break
    fi
done
# Expected: Prints 1, 2, 3 then stops
# Actual (before fix): Printed "break: not currently in a loop", continued to 5
```

**Root Causes Identified**:
1. **`loop_depth` never incremented**: Loop functions (`execute_while`, `execute_for`, `execute_until`) never incremented/decremented `executor->loop_depth`, so `bin_break` and `bin_continue` always saw `loop_depth <= 0`

2. **`loop_control` never checked**: Loop functions never checked `executor->loop_control` state after executing loop body

3. **Command chains/lists didn't stop**: `execute_command_chain` and `execute_command_list` continued executing remaining commands even when `loop_control` was set

**Fix Applied**:
Modified 5 functions in `src/executor.c`:

1. **`execute_while`**: Added loop_depth increment/decrement, loop_control checking
2. **`execute_for`**: Added loop_depth increment/decrement, loop_control checking  
3. **`execute_until`**: Added loop_depth increment/decrement, loop_control checking
4. **`execute_command_chain`**: Added loop_control check after each command
5. **`execute_command_list`**: Added loop_control check after each command

**Verification**:
- ✅ `break` works in `while` loops
- ✅ `break` works in `for` loops
- ✅ `break` works in `until` loops
- ✅ `continue` works in `while` loops
- ✅ `continue` works in `for` loops
- ✅ `continue` works in `until` loops
- ✅ Break/continue in nested command structures (if blocks, command chains)

**Files Modified**:
- `src/executor.c` (execute_while, execute_for, execute_until, execute_command_chain, execute_command_list)

**Status**: ✅ FIXED AND VERIFIED

**Note**: This was a core shell interpreter bug, not an LLE bug. Fixed in feature/lle branch as pragmatic decision.

---

### Issue #3: Pipe Character - Continuation Works But Execution Fails ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: Pre-2025-11-14, continuation fixed 2025-11-15, execution fixed 2025-11-15 (Session 16)
**Fixed**: 2025-11-15 (Session 16)
**Component**: Shell parser (NOT input continuation)  

**Description**:
When a line ends with a pipe character `|`, continuation detection worked (shows continuation prompt), but the shell parser failed to execute the multiline pipeline with "Expected command name" error.

**Status**: ✅ COMPLETELY FIXED
- ✅ Continuation detection: FIXED (Session 15)
- ✅ Pipeline execution: FIXED (Session 16)

**Reproduction** (Before Fix):
```bash
# Type this and press ENTER:
echo hello |
# Result: Continuation prompt appears (fixed in Session 15)

# Then type:
wc -l
# Result: "Expected command name" error (parser bug)
```

**Root Causes Identified**:
1. ✅ FIXED (Session 15): Pipe not detected by continuation parser
   - Fixed in src/input_continuation.c
   - Now checks for trailing pipe character

2. ✅ FIXED (Session 16): Shell parser didn't handle newlines in pipelines
   - Parser didn't skip newlines after pipe token
   - In `parse_pipeline()` at line 321, after consuming `|` token, immediately called `parse_pipeline()` recursively
   - Newline token between `|` and next command caused "Expected command name" error

**Fix Applied** (src/parser.c:322-328):
After consuming the pipe token, skip any newlines and whitespace before parsing right side:
```c
tokenizer_advance(parser->tokenizer); // consume |

// Skip newlines after pipe - allows multiline pipelines
while (tokenizer_match(parser->tokenizer, TOK_NEWLINE) ||
       tokenizer_match(parser->tokenizer, TOK_WHITESPACE)) {
    tokenizer_advance(parser->tokenizer);
}

node_t *right = parse_pipeline(parser);
```

**Verification**:
- ✅ Simple multiline pipeline: `echo hello |\nwc -l` outputs `1`
- ✅ Multi-stage pipeline: `echo one two three |\ntr " " "\n" |\nwc -l` outputs `3`
- ✅ Extra whitespace: `echo hello |\n\n   wc -l` outputs `1`
- ✅ Single-line pipes still work correctly

**Files Modified**:
- src/parser.c (parse_pipeline function, lines 322-328)

**Impact**:
- Multiline pipelines now work correctly
- Natural shell syntax supported (like bash)
- Continuation detection + execution both working

**Status**: ✅ FIXED AND VERIFIED

**Note**: This was a shell parser bug, not an LLE bug. Fixed in feature/lle branch as pragmatic decision.  

---

## Resolved Issues

### ✅ Screen Buffer Single-Byte Cell Limitation (Session 15)
**Resolved**: 2025-11-15  
**Severity**: MEDIUM  
**Component**: Screen buffer cell storage

**Description**: Screen buffer cells stored only the first byte of UTF-8 sequences, limiting internal representation to ASCII or first byte of multi-byte characters. While prompts and commands displayed correctly (direct STDOUT write), this limitation affected:
- Future diff-based rendering
- Prefix rendering for continuation prompts
- Any feature needing to reconstruct text from cells

**Fix Applied**:
- Upgraded `screen_cell_t` structure from single byte to full UTF-8 sequence storage
- Changed from `char ch` to `char utf8_bytes[4]` with metadata (`byte_len`, `visual_width`)
- Updated all code paths that read/write cells
- Memory increased from 2 bytes/cell to 8 bytes/cell (~410 KB max)

**Capabilities Now Supported**:
- ASCII (1 byte, 1 column)
- Extended Latin (2 bytes, 1 column)
- CJK ideographs (3 bytes, 2 columns)
- Emoji (4 bytes, 2 columns)
- Box-drawing characters (3 bytes, 1 column)

**Files Changed**:
- `include/display/screen_buffer.h` - Updated structure, added stdint.h
- `src/display/screen_buffer.c` - Updated all cell operations

**Testing Results**:
- ✅ Zero regressions in baseline testing
- ✅ Emoji rendering perfect (🚀 💻)
- ✅ Cursor positioning accurate
- ✅ All editing operations natural
- ✅ Multi-line input working
- ✅ Alt keybindings working

---

### ✅ Multi-line Prompt Cursor Positioning Bug (Session 14)
**Resolved**: 2025-11-14  
**Severity**: HIGH  
**Component**: Screen buffer prompt rendering / display_controller.c

**Description**: Multi-line prompts (e.g., dark theme with 2-line prompt) had incorrect cursor positioning and display corruption on character input.

**Root Causes**:
1. `screen_buffer_render()` didn't handle `\n` in prompt text (only in command text)
2. `display_controller.c` always moved to row 0 before clearing (assumed single-line prompt)
3. `display_controller.c` used total prompt width instead of actual command start column

**Fixes Applied**:
1. Added `\n`, `\r`, `\t` handling to prompt rendering loop (screen_buffer.c:231-261)
2. Added `command_start_row` and `command_start_col` fields to `screen_buffer_t`
3. Updated `display_controller.c` to use `command_start_row` instead of hardcoded row 0
4. Updated `display_controller.c` to use `command_start_col` instead of calculated prompt width

**Files Changed**:
- `include/display/screen_buffer.h` - Added command_start_row/col fields
- `src/display/screen_buffer.c` - Handle newlines in prompts, set command start position
- `src/display/display_controller.c` - Use actual command start position for clearing

**Testing Results**:
- ✅ Cursor positioned correctly on second prompt line
- ✅ Character input without display corruption
- ✅ Line wrapping and navigation across boundaries working
- ✅ UTF-8 box-drawing characters rendering correctly

---

### ✅ Meta/Alt Key Detection Not Implemented
**Resolved**: 2025-11-14  
**Resolution**: Implemented ESC+character detection in key_detector.c and event routing in lle_readline.c

**Changes**:
- Added Meta/Alt sequences to key_mappings table
- Fixed hex escape sequence bug (`"\x1Bf"` → `"\x1B" "f"`)
- Fixed missing keycode field in event conversion
- All Group 6 keybindings now functional (M-f, M-b, M-<, M->)

---

## Prevention Measures

To prevent future issues:

1. ✅ **Living Document Enforcement** - Pre-commit hooks enforce document updates
2. ✅ **Known Issues Tracking** - This document updated with all discovered bugs
3. ✅ **Spec Compliance Mandate** - Only implement exact spec APIs
4. ✅ **Comprehensive Testing** - All features tested before commit
5. ⚠️ **Issue Documentation** - **MUST update this file when bugs discovered**

---

## Issue Reporting Protocol

**When new issues are discovered**:

1. **Document in this file immediately** with:
   - Severity (BLOCKER, CRITICAL, HIGH, MEDIUM, LOW)
   - Description with reproduction steps
   - Root cause (if known, otherwise "Unknown")
   - Impact on users
   - Priority
   - Workaround (if available)
   - Resolution plan
   - Discovered date

2. **Update living documents**:
   - AI_ASSISTANT_HANDOFF_DOCUMENT.md (note issue in Known Issues section)
   - Mark as blocker in status if severity is BLOCKER

3. **DO NOT let issues be forgotten**:
   - Check this file before each session
   - Reference issue numbers in commits related to bugs
   - Update status when work progresses on issue

---

## Severity Definitions

- **BLOCKER**: Prevents all work, must fix immediately
- **CRITICAL**: Core functionality broken, high priority
- **HIGH**: Important feature broken or major bug
- **MEDIUM**: Functionality works but with issues, or useful feature missing
- **LOW**: Minor issue, cosmetic, or edge case

---

## Current Status

**Active Issues**: 4  
**Blockers**: 0  
**High Priority**: 0  
**Medium Priority**: 4 (Issues #5, #6 - syntax highlighting; #7 - category disambiguation)  
**Low Priority**: 1 (Issue #8 - menu display format)  
**Fixed This Session**: 0 (Issues #7 and #8 documented for future work)
**Implementation Status**: Spec 12 v2 completion integrated, duplicates eliminated  
**Next Action**: 
- Interactive menu features (arrow navigation, Enter to accept)
- (Future) Category disambiguation for completion conflicts
- (Future) Multi-column menu display investigation

---

**Last Updated**: 2025-11-22  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS

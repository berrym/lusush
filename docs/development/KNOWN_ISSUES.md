# Lusush Known Issues and Blockers

**Date**: 2026-01-13 (Updated: Session 120)  
**Status**: Major bug fixes - 100% compatibility test pass rate achieved!  
**Implementation Status**: ANSI-C quoting, mapfile/readarray, nameref, coproc - all implemented  
**Memory Status**: Zero memory leaks verified with valgrind  
**Compatibility Test**: 100% pass rate on all 63 applicable tests

### Compatibility Test Summary (Session 120)

#### Reference Shell Baselines (Target: 100%)

| Shell  | Applicable Tests | Passed | Skipped | Pass Rate |
|--------|-----------------|--------|---------|-----------|
| bash   | 63              | 63     | 24      | **100%** ✓ |
| zsh    | 63              | 63     | 24      | **100%** ✓ |

#### Lusush Results

| Mode | Applicable Tests | Passed | Failed | Pass Rate | Notes |
|------|-----------------|--------|--------|-----------|-------|
| `set -o lusush` (default) | 63 | 63 | 0 | **100%** ✓ | All tests passing |
| `set -o bash` | 63 | 63 | 0 | **100%** ✓ | Bash-compatible mode |
| `set -o zsh` | 63 | 33 | 30 | **52%** | Zsh syntax not yet implemented |
| `set -o posix` | 63 | 25 | 38 | **39%** | POSIX-only (expected) |

#### Session 120 Fixes

All 7 HIGH severity bugs fixed:
- Issue #40/#44: Array element assignment `arr[n]=value` - FIXED
- Issue #42: Brace expansion suffix-only and Cartesian products - FIXED
- Issue #43: pushd/popd/dirs in script mode - FIXED
- Issue #45/#46: Associative array initialization and keys - FIXED
- Issue #47: Herestring `<<<` - FIXED

---

## Session 119 Milestone: Bash/Zsh Compatibility Features Complete

**Date**: 2026-01-12

Major Bash/Zsh compatibility features implemented:
- **ANSI-C quoting (`$'...'`)**: Full escape sequence support including `\n`, `\t`, `\xNN`, `\uNNNN`, `\UNNNNNNNN`
- **mapfile/readarray builtin**: Read lines into arrays with `-t`, `-n`, `-O`, `-s`, `-d`, `-C`, `-c` options
- **Nameref variables (`declare -n`, `local -n`)**: Variable references with full resolution chain
- **Coproc (coprocesses)**: Bidirectional pipes with `coproc [NAME] command` syntax

---

## Executive Summary

**Current State**: Major feature implementation complete, minor limitations documented

- All prior LLE blockers resolved
- Bash/Zsh compatibility features working
- Minor limitations documented below

---

## Active Issues

### Issue #31: Coproc FD Redirection Syntax Not Supported
**Severity**: MEDIUM  
**Discovered**: 2026-01-12 (Session 118/119 - Coproc implementation)  
**Status**: Documented limitation  
**Component**: Redirection system (src/redirection.c, src/executor.c)

**Description**:
The coproc feature correctly creates bidirectional pipes and stores file descriptors in an array variable (e.g., `COPROC[0]=5`, `COPROC[1]=4`). However, the shell's redirection syntax does not support variable expansion in file descriptor positions.

**Working**:
```bash
coproc cat
echo ${COPROC[0]}  # Outputs: 5
echo ${COPROC[1]}  # Outputs: 4
echo ${COPROC_PID} # Outputs: 12345
```

**Not Working**:
```bash
# These syntaxes are NOT supported:
echo "hello" >&${COPROC[1]}    # Variable expansion in fd position
read response <&${COPROC[0]}   # Variable expansion in fd position

# Workaround - use numeric fd directly:
echo "hello" >&4    # Works if you know the fd number
read response <&5   # Works if you know the fd number
```

**Root Cause**:
The redirection parser (`src/executor.c` and `src/redirection.c`) expects literal numeric file descriptors in `>&N` and `<&N` syntax. Variable expansion (`${VAR}`) is not performed in the file descriptor position before the redirection is processed.

**Bash Behavior**:
Bash supports `>&${VAR}` by expanding the variable before processing the redirection. This requires the parser/executor to:
1. Recognize `>&` followed by `${...}` or `$VAR`
2. Expand the variable
3. Convert the result to an integer file descriptor

**Impact**:
- Coprocesses work correctly (pipes created, FDs stored)
- Users must use numeric FDs directly or use `eval` workaround
- Not a coproc bug - this is a separate redirection feature gap

**Workaround**:
```bash
coproc cat
# Use eval to expand variable in fd position:
eval "echo hello >&\${COPROC[1]}"
eval "read response <&\${COPROC[0]}"
```

**Priority**: MEDIUM (coproc works, just requires workaround for FD usage)

**Status**: DOCUMENTED - Future enhancement for redirection system

---

### Issue #26: LLE Complete Freeze/Hang - No Input Accepted
**Severity**: CRITICAL  
**Discovered**: 2025-12-27 (Session 73 - Transient prompt testing)  
**Status**: Not reproducible - observed once  
**Component**: Unknown - possibly autosuggestion/input handling  

**Description**:
LLE can enter a state where it completely freezes, accepting no input whatsoever. The shell process must be externally killed (e.g., via `kill` from another terminal or Ctrl+C if it works). Even the panic recovery mechanism (Ctrl+G) has no effect.

**Reproduction** (uncertain):
The issue was observed once during transient prompt testing:
1. Typed `ls`
2. Either accepted an autosuggestion and backspaced back to `ls`, OR dismissed an autosuggestion
3. At some point, input stopped working entirely
4. Cursor could not move, no characters accepted, Ctrl+G had no effect
5. Required external kill of the lusush process

**Symptoms**:
- Shell appears frozen - no response to any keyboard input
- Cursor does not move
- Ctrl+G (panic recovery) does not trigger reset
- Ctrl+C may or may not work
- Only fix is to kill the process externally

**Potential Causes** (speculation):
- Autosuggestion state corruption
- Input buffer deadlock
- Event loop blocking condition
- Terminal mode misconfiguration

**Impact**:
- Complete loss of shell session
- Unsaved work in that shell is lost
- User must manually kill process

**Priority**: CRITICAL (when it occurs, shell is completely unusable)

**Mitigation**:
- Issue only observed once, may be rare race condition
- No known reliable reproduction steps
- Monitor for additional occurrences to identify pattern

**Status**: DOCUMENTED - Needs investigation if reproducible

---

### Issue #25: macOS Cursor Flicker on Multiline Input
**Severity**: LOW  
**Discovered**: 2025-12-27 (Session 73 - Transient prompt testing)  
**Status**: Known issue, harmless  
**Platform**: macOS only  
**Component**: LLE display/cursor positioning  

**Description**:
On macOS, when LLE enters multiline input mode (for loops, if statements, or when command text wraps to multiple lines), the cursor briefly flickers to an incorrect position before syncing back to the correct location.

**Symptoms**:
- Cursor briefly appears far to the right of where input is happening
- May appear on wrong row momentarily
- Quickly corrects itself to proper position
- Only occurs when on a line other than the original prompt line

**Root Cause** (suspected):
LLE's internal state authority model correctly tracks cursor position, but there may be a timing issue between terminal output and cursor positioning sequences on macOS. The flicker-then-correct behavior suggests LLE's state sync is working, but there's a brief visual desync.

**Impact**:
- Visual annoyance only
- No functional impact - input works correctly
- Cursor always ends up in correct position

**Workaround**:
None needed - issue is purely cosmetic and self-correcting.

**Priority**: LOW (harmless visual glitch, macOS-specific)

**Status**: DOCUMENTED - May be terminal emulator specific

---

### Issue #36: `read` Builtin - `-t`, `-n`, `-s` Options Not Implemented
**Severity**: MEDIUM  
**Discovered**: Pre-existing (documented in source code)  
**Status**: Documented limitation  
**Component**: src/builtins/builtins.c (bin_read)

**Description**:
The `read` builtin accepts but does not implement several commonly-used options:

**Options parsed but not functional**:
- **`-t timeout`**: Should timeout after specified seconds, currently ignored
- **`-n nchars`**: Should read only specified number of characters, currently ignored
- **`-s`**: Silent mode (don't echo input), currently ignored

**Current Behavior**:
```bash
read -t 5 var      # Timeout parsed but read still blocks indefinitely
read -n 1 var      # Nchars parsed but reads full line
read -s password   # Silent flag parsed but input is still echoed
```

**Expected Behavior** (Bash-compatible):
```bash
read -t 5 var      # Returns after 5 seconds if no input
read -n 1 var      # Returns after 1 character
read -s password   # Input not echoed to terminal
```

**Source Code Evidence** (src/builtins/builtins.c):
```c
// Line 1783: TODO: Implement timeout functionality
// Line 1796: TODO: Implement nchars functionality
// Line 1800: TODO: Implement silent mode
// Line 1883-1885: Variables suppressed as "not yet implemented"
```

**Impact**:
- Scripts using `read -t` for user prompts will block indefinitely
- Interactive menus using `read -n 1` won't work as expected
- Password prompts using `read -s` will show input

**Priority**: MEDIUM (commonly used features)

**Status**: DOCUMENTED - Needs implementation

---

### Issue #37: `exec` Builtin - Redirection-Only Mode Not Implemented
**Severity**: LOW  
**Discovered**: Pre-existing (documented in source code)  
**Status**: Documented limitation  
**Component**: src/builtins/builtins.c (bin_exec)

**Description**:
The `exec` builtin only supports command replacement mode. Redirection-only mode (changing file descriptors without replacing the shell) is not implemented.

**Working**:
```bash
exec /bin/bash    # Replace shell with bash - works
exec ls           # Replace shell with ls - works
```

**Not Working**:
```bash
exec 3>&1         # Redirect fd 3 to stdout - not implemented
exec 3< file.txt  # Open file on fd 3 - not implemented
exec >&2          # Redirect stdout to stderr - not implemented
```

**Source Code Evidence** (src/builtins/builtins.c:2627-2630):
```c
// TODO: Implement redirection-only exec
// For now, we'll focus on command replacement exec
fprintf(stderr, "exec: redirection-only exec not yet implemented\n");
return 1;
```

**Impact**:
- Scripts using `exec` for file descriptor manipulation fail
- Common patterns like `exec 3>&1` for fd juggling don't work

**Priority**: LOW (less common than command replacement)

**Status**: DOCUMENTED - Needs implementation

---

### Issue #38: `declare -p` - List All Variables Not Implemented
**Severity**: LOW  
**Discovered**: Pre-existing (documented in source code)  
**Status**: Documented limitation  
**Component**: src/builtins/builtins.c (bin_declare)

**Description**:
The `declare -p` option without arguments should list all declared variables with their attributes, but this is not implemented.

**Working**:
```bash
declare -p VAR    # Print declaration for specific variable - works
declare -i num    # Declare integer variable - works
declare -a arr    # Declare array - works
```

**Not Working**:
```bash
declare -p        # List all variables - not implemented
```

**Source Code Evidence** (src/builtins/builtins.c:3706-3708):
```c
// TODO: Implement listing all declared variables with their attributes
printf("declare: listing all variables not yet implemented\n");
return 0;
```

**Impact**:
- Cannot dump all variable state for debugging
- Scripts relying on `declare -p` for introspection fail

**Priority**: LOW (convenience feature)

**Status**: DOCUMENTED - Needs implementation

---

### Issue #39: Brace Expansion - Sequential/Nested Braces Limited
**Severity**: LOW  
**Discovered**: Session 115 (documented limitation)  
**Status**: Documented limitation  
**Component**: src/executor.c (brace expansion)

**Description**:
Brace expansion works for single patterns but does not produce cartesian products for sequential or nested brace patterns.

**Working**:
```bash
echo {1..3}           # 1 2 3
echo {a,b,c}          # a b c
echo file{1..3}.txt   # file1.txt file2.txt file3.txt
```

**Limited**:
```bash
echo {1..3}{a..c}     # Expected: 1a 1b 1c 2a 2b 2c 3a 3b 3c (9 items)
                      # Actual: Only first pattern expands
echo pre{1..2}mid{a,b}post  # Expected: pre1midapost pre1midbpost pre2midapost pre2midbpost
                             # Actual: Only first pattern expands
```

**Impact**:
- Advanced brace expansion patterns don't produce expected output
- Workaround: Use separate expansions or loops

**Priority**: LOW (advanced feature, workarounds exist)

**Status**: DOCUMENTED - Future enhancement

---

### Issue #40: Array Element Assignment `arr[n]=value` Parsed as Command
**Severity**: MEDIUM  
**Discovered**: 2026-01-12 (Session 119 - Valgrind testing)  
**Status**: Documented limitation  
**Component**: src/parser.c, src/tokenizer.c

**Description**:
Assignment to existing array elements using the syntax `arr[n]=value` is incorrectly parsed as a command rather than an assignment.

**Working**:
```bash
arr=(one two three)       # Array creation - works
echo "${arr[0]}"          # Array access - works
declare -a arr            # Declare array - works
```

**Not Working**:
```bash
arr=(one two three)
arr[5]=six                # Error: arr[5: command not found
```

**Error Output**:
```
error[E1101]: arr[5: command not found
  --> script.sh:2:1
   = help: did you mean 'ar', 'arp', or 'ark'?
```

**Root Cause**:
The tokenizer/parser does not recognize `name[index]=value` as an assignment when the array already exists. It treats `arr[5` as an attempted command and `=six` as a separate token.

**Impact**:
- Cannot modify individual array elements after creation
- Workaround: Use `declare` or recreate the entire array

**Workaround**:
```bash
# Use symtable_set_array_element via command substitution (if available)
# Or recreate array with new value
```

**Priority**: MEDIUM (common array operation)

**Status**: DOCUMENTED - Parser enhancement needed

---

### Issue #41: Coproc Blocks in Non-Interactive Script Mode
**Severity**: MEDIUM  
**Discovered**: 2026-01-12 (Session 119 - Valgrind testing)  
**Status**: Documented limitation  
**Component**: src/executor.c (execute_coproc)

**Description**:
Coprocess commands that read from stdin (like `cat`) block indefinitely when run in script mode because they wait for input that never comes.

**Blocking Example**:
```bash
#!/usr/bin/env lusush
coproc cat              # Blocks - cat waits for stdin
echo "never reached"
```

**Non-Blocking Examples**:
```bash
coproc { sleep 0.1; }   # Works - command exits on its own
coproc { echo hello; }  # Works - command exits after output
```

**Root Cause**:
When a coproc is started with a command like `cat` that reads from stdin, the coproc's stdin (the write end of the pipe) is still open in the parent. The child `cat` process waits for input, but since we don't write anything or close the pipe, it blocks forever.

**Impact**:
- Scripts with coproc + stdin-reading commands hang
- Interactive mode may behave differently

**Workaround**:
```bash
# Close the write end of the coproc pipe immediately if not needed:
coproc mycat { cat; }
exec {mycat[1]}>&-      # Close write end - cat will see EOF and exit
wait $mycat_PID
```

**Priority**: MEDIUM (affects common coproc patterns)

**Status**: DOCUMENTED - May need automatic pipe management

---

### Issue #42: Brace Expansion - Suffix-Only and Cartesian Product Not Working
**Severity**: MEDIUM  
**Discovered**: 2026-01-12 (Session 119 - Validated test suite)  
**Status**: Documented limitation  
**Component**: src/executor.c (brace expansion)

**Description**:
Brace expansion has two limitations:

1. **Suffix-only patterns don't expand**: `{a,b,c}_suffix` remains literal instead of expanding to `a_suffix b_suffix c_suffix`

2. **Cartesian product doesn't work**: Multiple adjacent brace groups like `{1..2}{a..b}` remain literal instead of producing `1a 1b 2a 2b`

**Working**:
```bash
echo {1..5}           # 1 2 3 4 5 ✓
echo {a,b,c}          # a b c ✓
echo file{1..3}.txt   # file1.txt file2.txt file3.txt ✓
echo pre{a,b,c}fix    # preafix prebfix precfix ✓
```

**Not Working**:
```bash
echo {a,b,c}_suffix   # Expected: a_suffix b_suffix c_suffix
                      # Actual: {a,b,c}_suffix (literal)

echo {1..2}{a..b}     # Expected: 1a 1b 2a 2b
                      # Actual: {1..2}{a..b} (literal)
```

**Root Cause**:
The brace expansion implementation requires a prefix before the brace group. Suffix-only patterns and cartesian products require additional expansion logic.

**Priority**: MEDIUM (common patterns in bash scripts)

**Status**: DOCUMENTED - Parser/expansion enhancement needed

---

### Issue #43: pushd/popd/dirs Not Persisting in Script Mode
**Severity**: HIGH  
**Discovered**: 2026-01-12 (Session 119 - Validated test suite)  
**Status**: Active bug  
**Component**: src/builtins/builtins.c (bin_pushd, bin_popd, bin_dirs)

**Description**:
When running scripts with `lusush script.sh`, the pushd/popd/dirs builtins do not work correctly. The directory stack appears to reset or not persist between commands.

**Working** (Interactive):
```bash
$ pushd /usr
/usr ~
$ pwd
/usr
$ popd
~
```

**Not Working** (Script mode):
```bash
# script.sh:
cd /tmp
pushd /usr >/dev/null
pwd
# Expected: /usr
# Actual: /tmp (pushd had no effect)
```

**Test Results**:
- `pushd: changes directory` - FAIL: Returns /tmp instead of /usr
- `popd: returns to previous` - FAIL: "directory stack empty"
- `dirs: shows stack` - FAIL: Shows only current dir, not stack

**Root Cause**:
The directory stack may be stored in a way that doesn't persist across command execution in non-interactive mode, or there's a scoping issue with the stack data structure.

**Priority**: HIGH (breaks common shell script patterns)

**Status**: ACTIVE BUG - Needs investigation

---

### Issue #44: Associative Array Element Assignment Parsed as Command
**Severity**: HIGH  
**Discovered**: 2026-01-12 (Session 119 - Comprehensive test suite)  
**Status**: Active bug  
**Component**: src/parser.c, src/tokenizer.c

**Description**:
Setting individual elements in an associative array using `arr[key]=value` is incorrectly parsed as a command rather than an assignment.

**Not Working**:
```bash
declare -A arr
arr[key]=value    # Error: arr[key: command not found
```

**Error Output**:
```
error[E1101]: arr[key: command not found
  --> script.sh:2:1
   = help: did you mean 'ar', 'arp', or 'ark'?
```

**Related To**: Issue #40 (same root cause for indexed arrays)

**Priority**: HIGH (breaks basic associative array usage)

**Status**: ACTIVE BUG - Parser enhancement needed

---

### Issue #45: Associative Array Initialization with Bash Syntax Fails
**Severity**: HIGH  
**Discovered**: 2026-01-12 (Session 119 - Comprehensive test suite)  
**Status**: Active bug  
**Component**: src/parser.c

**Description**:
Bash-style associative array initialization with `([key]=value)` syntax does not work correctly.

**Not Working**:
```bash
declare -A arr
arr=([k1]=v1 [k2]=v2)
echo "${arr[k1]} ${arr[k2]}"
# Expected: v1 v2
# Actual: v2 v2 (last value assigned to all keys)
```

**Root Cause**:
The `([key]=value ...)` syntax is being parsed but the key assignments are not correctly associated with their values.

**Priority**: HIGH (breaks common array pattern)

**Status**: ACTIVE BUG - Needs investigation

---

### Issue #46: Associative Array `${!arr[@]}` Returns Wrong Keys
**Severity**: HIGH  
**Discovered**: 2026-01-12 (Session 119 - Comprehensive test suite)  
**Status**: Active bug  
**Component**: src/executor.c

**Description**:
The `${!arr[@]}` expansion for getting all keys of an associative array returns incorrect results.

**Not Working**:
```bash
declare -A arr
arr=([a]=1 [b]=2 [c]=3)
echo "${!arr[@]}"
# Expected: a b c (or some order of these keys)
# Actual: 0 (returns index 0 instead of keys)
```

**Root Cause**:
The indirect expansion for associative arrays is not correctly distinguishing between indexed array indices and associative array keys.

**Priority**: HIGH (breaks associative array introspection)

**Status**: ACTIVE BUG - Needs investigation

---

### Issue #47: Herestring `<<<` Not Working
**Severity**: HIGH  
**Discovered**: 2026-01-12 (Session 119 - Manual testing)  
**Status**: Active bug  
**Component**: src/tokenizer.c, src/input_continuation.c

**Description**:
Herestring syntax `<<<` triggers incorrect input continuation instead of passing the string to stdin.

**Not Working**:
```bash
cat <<< "Hello, world!"
# Expected: Hello, world!
# Actual: Shows continuation prompt ">" and hangs waiting for input
```

**Terminal Output**:
```
$ cat <<< "Hello, world!"
> 
```

**Root Cause** (suspected):
The tokenizer or input continuation logic is matching `<<<` as heredoc `<<` plus an extra `<`. The input continuation system then waits for a heredoc delimiter that will never come.

**Fix Required**:
1. Tokenizer must recognize `<<<` as `TOK_HERESTRING` BEFORE matching `<<` as `TOK_HEREDOC`
2. Input continuation logic must be herestring-aware

**Priority**: HIGH (common bash/zsh syntax, blocks compatibility)

**Status**: ACTIVE BUG - Likely simple fix in tokenizer

---

## Recently Resolved Issues (Session 118/119)

### Issue #32: ANSI-C Quoting Not Expanding in NODE_STRING_LITERAL - FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-11 (Session 118)  
**Fixed**: 2026-01-11 (Session 118)  
**Component**: src/executor.c (expand_quoted_string)

**Description**:
ANSI-C quoted strings (`$'...'`) were being tokenized correctly but not expanded when they appeared as `NODE_STRING_LITERAL` nodes. The expansion only happened for `NODE_WORD` nodes.

**Root Cause**:
`expand_quoted_string()` was only called for `NODE_WORD` nodes in `expand_arguments()`. When `$'...'` appeared as a complete argument, it became a `NODE_STRING_LITERAL` which bypassed the expansion path.

**Fix Applied**:
Added ANSI-C quoting check in `expand_string_literal()`:
```c
// Check for ANSI-C quoting prefix
if (len >= 2 && text[0] == '$' && text[1] == '\'') {
    return expand_ansi_c_quoting(text + 2, len - 3); // Skip $' and trailing '
}
```

**Status**: FIXED AND VERIFIED

---

### Issue #33: Mapfile -n Option Not Consuming Remaining Input - FIXED
**Severity**: MEDIUM  
**Discovered**: 2026-01-11 (Session 118)  
**Fixed**: 2026-01-11 (Session 118)  
**Component**: src/builtins/builtins.c (bin_mapfile)

**Description**:
When using `mapfile -n COUNT`, only the first COUNT lines were read but the remaining input was not consumed, leaving data in the input stream.

**Root Cause**:
After reading the requested number of lines, `bin_mapfile()` returned immediately without draining the remaining input from the file descriptor.

**Fix Applied**:
Added loop to consume remaining input after reaching count limit:
```c
// Drain remaining input when -n limit is reached
if (count > 0 && lines_read >= count) {
    while (fgets(line_buffer, sizeof(line_buffer), fp)) {
        // Discard remaining lines
    }
}
```

**Status**: FIXED AND VERIFIED

---

### Issue #34: Nameref Array Access Failed - FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-11 (Session 118)  
**Fixed**: 2026-01-11 (Session 118)  
**Component**: src/symtable.c, src/executor.c

**Description**:
When accessing array elements through a nameref variable, the element access failed. Example: `declare -n ref=arr; echo ${ref[0]}` would not return the array element.

**Root Cause**:
The nameref resolution in `symtable_get_var()` didn't properly follow the reference chain when array subscripts were involved.

**Fix Applied**:
Updated nameref resolution to handle array subscripts:
1. Resolve nameref to get target variable name
2. If subscript present, combine target name with subscript
3. Perform array element lookup on resolved name

**Status**: FIXED AND VERIFIED

---

### Issue #35: Unset Through Nameref Failed - FIXED
**Severity**: MEDIUM  
**Discovered**: 2026-01-11 (Session 118)  
**Fixed**: 2026-01-11 (Session 118)  
**Component**: src/builtins/builtins.c (bin_unset)

**Description**:
Using `unset` on a nameref variable didn't unset the target variable. Example: `declare -n ref=var; unset ref` should unset `var`, not just the reference.

**Root Cause**:
`bin_unset()` didn't resolve nameref variables before performing the unset operation.

**Fix Applied**:
Added nameref resolution in `bin_unset()`:
```c
// Resolve nameref if present
symbol_t *sym = symtable_lookup(name);
if (sym && (sym->flags & SYMVAR_NAMEREF)) {
    name = sym->val.str;  // Use target name
}
symtable_unset_var(name);
```

**Status**: FIXED AND VERIFIED

---

## Resolved Issues (Previous Sessions)

### Issue #30: macOS Memory Leaks Not Detected by Valgrind - FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-02 (Session 102 - macOS `leaks` tool analysis)  
**Fixed**: 2026-01-02 (Session 102)  
**Platform**: macOS (Linux valgrind showed 0 bytes definitely lost)  
**Component**: Multiple - editor cleanup, display integration, error handling, memory pool  

**Description**:
While Linux valgrind reported 0 bytes "definitely lost", macOS `leaks` tool reported **1,585 leaks totaling ~93KB**.

**Resolution**:
Multiple leak sources fixed including pool malloc fallback tracking, history system destruction, keybinding manager cleanup, and more.

**Status**: FIXED AND VERIFIED (0 leaks, 0 bytes)

---

### Issue #29: Critical Memory Leaks (~79KB per session) - FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-02 (Session 101 - Valgrind analysis)  
**Fixed**: 2026-01-02 (Session 101)  
**Component**: Multiple - parser.c, autocorrect.c, lusush.c, completion system  

**Description**:
Valgrind reported ~79KB of memory "definitely lost" during normal shell operation.

**Resolution**:
Fixed parser AST node `val_type` not being set, autocorrect temp suggestions not freed, and input buffer leaks.

**Status**: FIXED AND VERIFIED

---

### Issue #28: POSIX Variable Scoping Regression - FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-01 (Session 98 - Compliance test analysis)  
**Fixed**: 2026-01-01 (Session 98)  
**Component**: src/executor.c  

**Description**:
Variable assignments inside functions and loops were incorrectly using local scope instead of global scope.

**Resolution**:
Changed `symtable_set_local_var()` to `symtable_set_global_var()` in `execute_assignment()` and `execute_for()`.

**Status**: FIXED AND VERIFIED

---

### Issue #24: Transient Prompt System - FULLY IMPLEMENTED
**Severity**: LOW  
**Discovered**: 2025-12-27 (Session 71 - Audit)  
**Resolved**: 2025-12-27 (Session 72-73)  
**Component**: LLE widget hooks, display controller, config system  

**Description**:
Transient prompts (simplifying previous prompts after command execution) are now fully implemented.

**Status**: FULLY IMPLEMENTED AND TESTED

---

### Issue #21: Theme System Not User-Extensible - FULLY RESOLVED
**Severity**: HIGH  
**Discovered**: 2025-12-21 (Session 54 - theme variable investigation)  
**Resolved**: 2025-12-28 (Session 77 - Theme file loading implementation)  
**Component**: LLE Spec 25 prompt/theme system  

**Description**:
The OLD theme system was completely hardcoded in C.

**Resolution**:
Session 77 implemented a complete theme file loading system with TOML parser, hot reload, and theme export.

**Status**: FULLY RESOLVED

---

### Issue #16: Prompt/Cursor Desync Display Corruption - FULLY RESOLVED
**Severity**: HIGH  
**Discovered**: 2025-12-17 (Session 52 - manual testing)  
**Resolved**: 2025-12-27 (Shell-level event system verified working)  
**Component**: Prompt/theme system cache architecture  

**Description**:
Display corruption where the prompt and cursor position become desynchronized.

**Resolution**:
Fixed with `prompt_cache_invalidate()` in `bin_cd()` and Spec 26 Shell Integration providing persistent event system.

**Status**: FULLY RESOLVED

---

### Issue #14: Git-Aware Prompt Not Displaying Git Information - FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-12-02 (macOS LLE compatibility session)  
**Fixed**: 2026-01-01 (Session 94 verification)  
**Component**: src/lle/prompt/segment.c, src/lle/core/async_worker.c  

**Description**:
The git-aware themed prompt was not displaying git information.

**Resolution**:
Replaced by LLE Spec 25 prompt system with full async support.

**Status**: FIXED

---

### Issue #7: Completion Menu - Category Disambiguation Not Implemented - FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-11-22 (Session 23 Part 2)  
**Fixed**: 2026-01-01 (Session 100)  
**Component**: Completion system / source manager  

**Description**:
When the same command exists in multiple categories, users can now see and select both versions.

**Resolution**:
Implemented type-aware deduplication, full path storage for shadowing commands, and smart insertion.

**Status**: RESOLVED

---

## Severity Definitions

- **BLOCKER**: Prevents all work, must fix immediately
- **CRITICAL**: Core functionality broken, high priority
- **HIGH**: Important feature broken or major bug
- **MEDIUM**: Functionality works but with issues, or useful feature missing
- **LOW**: Minor issue, cosmetic, or edge case

---

## Current Status

**Active Issues**: 7

### HIGH Severity (1):
- Issue #26: LLE freeze/hang (CRITICAL but not reproducible)

### MEDIUM Severity (3):
- Issue #36: `read` builtin `-t`, `-n`, `-s` options not implemented
- Issue #31: Coproc FD redirection syntax (workaround available)
- Issue #41: Coproc blocks in non-interactive script mode

### LOW Severity (3):
- Issue #37: `exec` redirection-only mode (LOW)
- Issue #38: `declare -p` list all variables (LOW)
- Issue #25: macOS cursor flicker (LOW - cosmetic only)

**Session 120 Resolved**: 7 HIGH severity issues fixed:
- Issue #40/#44: Array element assignment - FIXED (tokenizer context tracking)
- Issue #42: Brace expansion edge cases - FIXED (suffix + Cartesian products)
- Issue #43: pushd/popd/dirs script mode - FIXED (builtin_can_fork check)
- Issue #45/#46: Associative array issues - FIXED (preserve is_associative flag)
- Issue #47: Herestring `<<<` - FIXED (exclude from heredoc detection)

**Session 118/119 Resolved**: 4 issues (ANSI-C quoting, mapfile -n, nameref array access, unset through nameref)

**Bash/Zsh Compatibility Status (100% pass rate)**:
- ANSI-C quoting (`$'...'`): IMPLEMENTED ✓
- mapfile/readarray: IMPLEMENTED ✓
- Nameref variables: IMPLEMENTED ✓
- Coproc: IMPLEMENTED (with FD redirection limitation noted)
- Extended glob (bash-style): IMPLEMENTED ✓
- Extended test `[[]]`: IMPLEMENTED ✓
- Process substitution: IMPLEMENTED ✓
- Arithmetic expansion: IMPLEMENTED ✓
- Case modification: IMPLEMENTED ✓
- Array element assignment: IMPLEMENTED ✓
- Associative arrays: IMPLEMENTED ✓
- Brace expansion (full): IMPLEMENTED ✓
- Herestrings: IMPLEMENTED ✓
- pushd/popd/dirs: IMPLEMENTED ✓

**Builtin Gaps Documented**:
- `read -t/-n/-s`: Parsed but not functional
- `exec` redirections: Not implemented
- `declare -p` (no args): Not implemented

---

**Last Updated**: 2026-01-13 (Session 120)  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS

# Lusush Known Issues and Blockers

**Date**: 2026-01-14 (Updated: Session 121)  
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

### Issue #48: `shift` Builtin Does Not Modify Positional Parameters Inside Functions
**Severity**: HIGH  
**Discovered**: 2026-01-14 (Session 121 - Function/closure testing)  
**Status**: Active bug  
**Component**: src/builtins/builtins.c (bin_shift)

**Description**:
The `shift` builtin inside a function does not actually shift the positional parameters (`$@`, `$1`, `$2`, etc.). The parameters remain unchanged after the `shift` command.

**Not Working**:
```bash
test_shift() {
    echo "before shift: $@"
    shift
    echo "after shift: $@"
}
test_shift a b c d
# Expected:
#   before shift: a b c d
#   after shift: b c d
# Actual:
#   before shift: a b c d
#   after shift: a b c d
```

**Impact**:
This breaks common shell patterns including:
- Callback/higher-order function patterns: `fn=$1; shift; $fn "$@"`
- Option parsing loops: `while [ $# -gt 0 ]; do case $1 in ...; esac; shift; done`
- Argument processing: extracting first arg then passing rest to another command

**Example Broken Pattern**:
```bash
# Higher-order map function - BROKEN
map() {
    local fn=$1
    shift                    # shift doesn't work - fn=$1 and $@ unchanged
    for item in "$@"; do
        $fn "$item"          # Wrong: still includes original $1
    done
}
```

**Root Cause** (suspected):
The `bin_shift()` function may be modifying a copy of the positional parameters or not correctly updating the function's local scope positional parameters.

**Comparison with zsh**:
```bash
# zsh correctly handles shift in functions:
$ zsh -c 'f() { echo "$@"; shift; echo "$@"; }; f a b c'
a b c
b c
```

**Priority**: HIGH (breaks common shell idioms and higher-order function patterns)

**Status**: FIXED (Session 121) - Modified bin_shift to handle function scope

---

### Issue #49: `for` Loop Does Not Expand `$@` in Function Scope
**Severity**: HIGH  
**Discovered**: 2026-01-14 (Session 121 - Function/closure testing)  
**Status**: Active bug  
**Component**: src/executor.c (execute_for)

**Description**:
When using `$@` or `$*` in a `for` loop word list inside a function, the loop does not iterate over the positional parameters. The variable expands correctly when echoed but not in the `for` loop context.

**Not Working**:
```bash
test_loop() {
    echo "args: $@"        # Works: outputs "args: a b c"
    for item in $@; do     # Bug: loop body never executes
        echo "item: $item"
    done
}
test_loop a b c
# Expected:
#   args: a b c
#   item: a
#   item: b
#   item: c
# Actual:
#   args: a b c
#   (nothing - loop doesn't iterate)
```

**Working** (explicit list):
```bash
test_loop() {
    for item in a b c; do  # Works with literal list
        echo "item: $item"
    done
}
```

**Impact**:
This breaks common shell patterns including:
- Iterating over function arguments: `for arg in "$@"; do ... done`
- Processing all arguments in a function
- Higher-order functions that iterate over remaining arguments

**Root Cause** (suspected):
The `execute_for()` function may be expanding `$@` using global `shell_argv` instead of the function's local positional parameters, resulting in an empty or incorrect word list.

**Comparison with zsh**:
```bash
# zsh correctly handles $@ in for loops inside functions:
$ zsh -c 'f() { for i in $@; do echo $i; done; }; f a b c'
a
b
c
```

**Priority**: HIGH (breaks common shell idioms for argument processing)

**Status**: FIXED (Session 121) - Modified execute_for to check function scope and use local positional parameters

---

### Issue #50: Zsh Parameter Flags Don't Work with Positional Parameters
**Severity**: MEDIUM  
**Discovered**: 2026-01-14 (Session 121 - Higher-order function testing)  
**Status**: Active bug  
**Component**: src/executor.c (parse_parameter_expansion)

**Description**:
Zsh-style parameter flags like `${(U)var}` work correctly with named variables but not with positional parameters (`$1`, `$2`, etc.).

**Not Working**:
```bash
set -o zsh
shout() { echo "SHOUT: ${(U)1}"; }
shout hello
# Expected: SHOUT: HELLO
# Actual:   SHOUT:
```

**Working** (named variable):
```bash
set -o zsh
x=hello
echo "${(U)x}"
# Output: HELLO
```

**Workaround**:
```bash
shout() {
    local tmp=$1
    echo "SHOUT: ${(U)tmp}"
}
```

**Comparison with zsh**:
```bash
# zsh correctly handles flags with positional parameters:
$ zsh -c 'f() { echo "${(U)1}"; }; f hello'
HELLO
```

**Root Cause** (suspected):
The zsh parameter flag parsing in `parse_parameter_expansion()` may not be resolving positional parameters from function scope before applying the transformation.

**Priority**: MEDIUM (workaround available, but reduces zsh compatibility)

**Status**: FIXED (Session 121) - Added function scope check for positional parameters in parse_parameter_expansion

---

### Issue #51: `N<` Input Redirection Not Tokenized Correctly
**Severity**: HIGH  
**Discovered**: 2026-01-15 (Session 123 - macOS redirection testing)  
**Status**: Active bug  
**Component**: src/tokenizer.c

**Description**:
The tokenizer handles `N>` (digit followed by output redirection) correctly but fails to recognize `N<` (digit followed by input redirection). This causes `exec 3<file` to fail while `exec 3>file` works.

**Not Working**:
```bash
exec 3</tmp/test.txt
# Error: exec: no command specified
# Exit code: 1
```

**Working**:
```bash
exec 3>/tmp/test.txt   # Works correctly
exec 3<&0              # Works (N<& pattern is handled)
```

**Root Cause**:
In `src/tokenizer.c`, the digit-followed-by-redirection handling (around line 1570-1680) has:
- `N>` handled at line ~1626: creates `TOK_REDIRECT_ERR` token
- `N>>` handled: creates `TOK_APPEND_ERR` token  
- `N>&` handled: creates `TOK_REDIRECT_FD` token
- `N<&` handled: creates `TOK_REDIRECT_FD` token
- `N<` **NOT handled**: falls through, `N` treated as word, `<` as separate redirect

The condition only checks for `N<&` but not plain `N<`:
```c
} else if (tokenizer->input[tokenizer->position] == '<' &&
           tokenizer->position + 1 < tokenizer->input_length &&
           tokenizer->input[tokenizer->position + 1] == '&') {
    // Only handles N<& - missing plain N<
```

**Fix Required**:
Add handling for `N<` pattern similar to how `N>` is handled, returning a `TOK_REDIRECT_IN` token with the fd number prefix.

**Impact**:
- `exec 3<file` fails
- Any input redirection with explicit fd number fails
- Affects scripts that use numbered input redirections

**Priority**: HIGH (breaks standard shell redirection functionality)

**Status**: FIXED (Session 123) - Added `TOK_REDIRECT_IN_FD` and `NODE_REDIR_IN_FD` types for proper `N<` handling

---

### Issue #52: File Redirections Fail When Shell Runs in Subshell Capture
**Severity**: HIGH  
**Discovered**: 2026-01-15 (Session 123 - macOS redirection testing)  
**Status**: FIXED (Session 123)  
**Component**: src/executor.c, src/redirection.c

**Description**:
When lusush is invoked inside a `$()` command substitution (subshell capture), file redirections produce empty files even though the same command works correctly when run directly.

**Root Cause**:
The switch from `exit()` to `_exit()` in forked child processes (to fix the pipeline infinite loop bug) caused stdio buffers not to be flushed. `_exit()` exits immediately without flushing buffers, so output redirected to files was lost.

**Fix**:
Added explicit `fflush(stdout)` and `fflush(stderr)` calls before all `_exit()` calls in forked child processes:
- Pipeline left/right children (execute_pipeline)
- Background job children (executor_execute_background)
- Builtin with captured stdout children
- Coprocess children
- Process substitution children
- Also added fflush in `restore_file_descriptors()` before restoring fds

---

### Issue #53: Writing to Closed File Descriptor Doesn't Fail
**Severity**: MEDIUM  
**Discovered**: 2026-01-15 (Session 123 - macOS redirection testing)  
**Status**: FIXED (Session 123)  
**Component**: src/redirection.c (setup_fd_redirection), src/builtins/builtins.c (bin_echo)

**Description**:
When a file descriptor is closed with `exec N>&-`, subsequent writes to that fd should fail with "bad file descriptor" error and return non-zero exit status.

**Root Cause**:
Two issues were found:
1. The `setup_fd_redirection()` function used `dup2()` which succeeds if the target fd is open to *anything* (even if inherited from parent and unsuitable for the operation). The shell inherited fds from parent processes that happened to be open.
2. The `bin_echo()` builtin always returned 0 without checking for write errors.

**Fix Applied**:
1. Added fd validation in `setup_fd_redirection()` using `fcntl(fd, F_GETFL)` to:
   - Check if target fd is actually open (returns -1 with EBADF if not)
   - Verify fd access mode matches redirection direction (writable for output, readable for input)
2. Added write error checking in `bin_echo()` using `fflush(stdout)` and `ferror(stdout)` to detect and report I/O errors

**Now Working** (matches bash/zsh behavior):
```bash
$ ./build/lusush -c 'exec 3>/tmp/t.txt; exec 3>&-; echo test >&3'
error[E1121]: 3: Bad file descriptor

$ ./build/lusush -c 'echo test >&3; echo "exit: $?"'
error[E1121]: 3: Bad file descriptor

exit: 1
```

**Status**: FIXED AND VERIFIED

---

### Issue #54: `pwd -P` Does Not Resolve Physical Path on macOS
**Severity**: LOW  
**Discovered**: 2026-01-15 (Session 123 - macOS compatibility testing)  
**Status**: FIXED (Session 123)  
**Platform**: macOS (and likely other systems with symlinked directories)  
**Component**: src/builtins/builtins.c (bin_pwd)

**Description**:
The `pwd -P` option should print the physical path with all symlinks resolved, but lusush was returning the logical path instead.

**Root Cause**:
The `bin_pwd()` function only checked `shell_opts.physical_mode` global setting and completely ignored the `-P` and `-L` command-line options.

**Fix Applied**:
Added option parsing to `bin_pwd()` to handle `-P` (physical) and `-L` (logical) flags, which override the shell's default physical_mode setting.

**Now Working** (matches bash/zsh behavior):
```bash
$ ./build/lusush -c 'cd /tmp; pwd; pwd -P; pwd -L'
/tmp
/private/tmp
/tmp
```

**Status**: FIXED AND VERIFIED

---

### Issue #31: Coproc FD Redirection Syntax Not Supported
**Severity**: MEDIUM  
**Discovered**: 2026-01-12 (Session 118/119 - Coproc implementation)  
**Status**: FIXED (Session 122)  
**Fixed**: 2026-01-14  
**Component**: Redirection system (src/redirection.c, src/tokenizer.c)

**Description**:
The coproc feature correctly creates bidirectional pipes and stores file descriptors in an array variable (e.g., `COPROC[0]=5`, `COPROC[1]=4`). Previously, the shell's redirection syntax did not support variable expansion in file descriptor positions.

**Fix Details**:
- Modified tokenizer to recognize `>&$VAR`, `>&${VAR}`, `<&$VAR`, `<&${VAR}` patterns
- Updated `setup_fd_redirection()` to expand variables before performing dup2
- Added support for `>&-` and `<&-` fd close syntax
- Added `SHELL_ERR_BAD_FD` error code for structured error reporting

**Now Working**:
```bash
coproc cat
echo "hello" >&${COPROC[1]}    # Write to coproc stdin
exec {COPROC[1]}>&-            # Close write end (send EOF)
read response <&${COPROC[0]}   # Read from coproc stdout
echo "Got: $response"          # Outputs: Got: hello
```

**All Supported Patterns**:
```bash
>&N          # Redirect stdout to fd N (single digit 0-9)
<&N          # Redirect stdin from fd N (single digit 0-9)
N>&M         # Redirect fd N to fd M (single digits)
>&$VAR       # Variable expansion for fd target (any valid fd)
>&${VAR}     # Brace-style variable expansion
>&${arr[0]}  # Array subscript expansion
>&-          # Close stdout
<&-          # Close stdin
N>&-         # Close fd N
```

**Remaining Limitation**:
Literal multi-digit file descriptors (e.g., `>&10`) are not yet supported in the tokenizer. 
The tokenizer currently only recognizes single-digit literal fds (0-9). However, multi-digit
fds work correctly through variable expansion:

```bash
# This does NOT work (tokenizer limitation):
echo "test" >&10       # Parsed as >&1 followed by 0

# This DOES work (variable expansion):
FD=10
echo "test" >&$FD      # Correctly redirects to fd 10
```

This limitation does not affect coproc usage since coproc fds are accessed via array variables.

**{varname} FD Allocation Syntax - IMPLEMENTED (Session 122)**:
The `{varname}` syntax for automatic fd allocation (bash 4.1+/zsh feature) is now fully supported:

```bash
# All patterns now work:
exec {myfd}>/tmp/file    # Allocate fd (≥10), open file for write, store in $myfd
exec {myfd}>>/tmp/file   # Allocate fd, open file for append
exec {myfd}</tmp/file    # Allocate fd, open file for read
exec {myfd}>&-           # Close fd stored in $myfd
exec {myfd}>&2           # Allocate fd and dup to stderr

# Usage example:
exec {myfd}>/tmp/test.txt
echo "Writing to allocated fd: $myfd"
echo "Data via fd" >&$myfd
exec {myfd}>&-
```

Implementation details:
- Tokenizer recognizes `{identifier}` followed by `>`, `>>`, `<`, `>&`, `<&` as `TOK_REDIRECT_FD_ALLOC`
- Parser creates `NODE_REDIR_FD_ALLOC` nodes with proper target handling
- Redirection system allocates fd ≥10 using `fstat()` to find available slots
- Variable is set to allocated fd number via symtable
- Closing with `{varname}>&-` looks up the variable value and closes that fd

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
**Status**: FIXED (Session 121)  
**Component**: src/builtins/builtins.c (bin_read)

**Description**:
The `read` builtin previously accepted but did not implement several commonly-used options.

**Now Working** (Bash-compatible):
```bash
read -t 5 var      # Returns exit 1 after 5 seconds if no input
read -n 1 var      # Returns after 1 character (non-canonical mode)
read -s password   # Input not echoed to terminal
```

**Implementation Details**:
- **`-t timeout`**: Uses `select()` with timeout before reading
- **`-n nchars`**: Uses `termios` non-canonical mode, reads char by char
- **`-s`**: Uses `termios` to disable `ECHO` flag, restores on completion
- Terminal settings properly restored even on timeout/error
- Works correctly with both TTY and pipe input

**Priority**: MEDIUM (commonly used features)

**Status**: FIXED (Session 121) - Full implementation with termios and select()

---

### Issue #37: `exec` Builtin - Redirection-Only Mode Not Implemented
**Severity**: LOW  
**Discovered**: Pre-existing (documented in source code)  
**Status**: FIXED (Session 122)  
**Fixed**: 2026-01-14  
**Component**: src/executor.c, src/tokenizer.c

**Description**:
The `exec` builtin now supports redirection-only mode for changing file descriptors without replacing the shell.

**Now Working**:
```bash
exec /bin/bash    # Replace shell with bash - works
exec ls           # Replace shell with ls - works
exec 3>&1         # Redirect fd 3 to stdout - works
exec 5>&-         # Close fd 5 - works
exec >&2          # Redirect stdout to stderr - works
```

**Fix Details**:
1. **Tokenizer**: Added recognition of `N>&M`, `N>&-`, `N>&$VAR`, `N<&M`, `N<&-`, `N<&$VAR` patterns as single redirection tokens (e.g., `5>&-` is now one token instead of `5` + `>&-`)
2. **Executor**: Modified to NOT restore file descriptors after `exec` builtin execution - redirections performed by `exec` are now permanent as expected

**Implementation Notes**:
- The `exec` builtin's redirections persist in the shell session
- Other builtins have their redirections restored after execution (existing behavior)
- Single-digit literal fds (0-9) supported; multi-digit fds work via variable expansion

**Status**: FIXED AND VERIFIED (6/6 zsh tests, 6/7 bash tests pass)

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

### Issue #41: Coproc Blocks When Command Reads stdin
**Severity**: LOW (Expected Behavior)  
**Discovered**: 2026-01-12 (Session 119 - Valgrind testing)  
**Status**: NOT A BUG - Expected behavior (confirmed Session 122)  
**Component**: src/executor.c (execute_coproc)

**Description**:
Coprocess commands that read from stdin (like `cat`) block until they receive EOF. This is expected POSIX behavior - the parent must close the write end of the pipe to signal EOF.

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

### Issue #55: `for var; do...done` POSIX Syntax Not Supported
**Severity**: MEDIUM  
**Discovered**: 2026-01-15 (Session 124 - Unit test development)  
**Status**: FIXED (Session 124)  
**Fixed**: 2026-01-15  
**Component**: src/parser.c

**Description**:
POSIX shell syntax allows `for var; do...done` without the `in` keyword, which iterates over the positional parameters (`$@`).

**Now Working**:
```bash
$ ./build/lusush -c 'set -- a b c; for x; do echo $x; done'
a
b
c
```

**Fix Applied**:
Modified `parse_for_statement()` to make `in` keyword optional. When `in` is omitted and the parser sees `;`, newline, or `do` after the variable name, it implicitly creates a word list containing `"$@"` per POSIX specification.

**Status**: FIXED AND VERIFIED

---

### Issue #56: `function name { }` Syntax Not Supported (ksh/bash Style)
**Severity**: MEDIUM  
**Discovered**: 2026-01-15 (Session 124 - Unit test development)  
**Status**: FIXED (Session 124)  
**Fixed**: 2026-01-15  
**Component**: src/parser.c

**Description**:
The ksh/bash-style function definition `function name { body; }` without parentheses.

**Now Working**:
```bash
$ ./build/lusush -c 'function foo { echo bar; }; foo'
bar
```

All three function definition styles are now supported:
- `function name { body; }` - ksh/bash style (no parentheses)
- `function name() { body; }` - bash style (with parentheses)
- `name() { body; }` - POSIX style

**Fix Applied**:
Modified `parse_function_definition()` to check if the `function` keyword was used. When it was, and the next token is `{` instead of `(`, the parser skips parameter parsing and goes directly to body parsing.

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

**Active Issues**: 5

### HIGH Severity (1):
- Issue #26: LLE freeze/hang (CRITICAL but not reproducible)

### MEDIUM Severity (1):
- Issue #40: Array element assignment (workaround: recreate array)

### LOW Severity (3):
- Issue #38: `declare -p` list all variables (LOW)
- Issue #25: macOS cursor flicker (LOW - cosmetic only)
- Issue #41: Coproc stdin behavior (LOW - expected behavior, not a bug)

**Session 122 Resolved**: 3 features implemented:
- Issue #31: FD variable expansion in redirections - FIXED (tokenizer + redirection.c)
- Issue #37: `exec` redirection-only mode - FIXED (tokenizer + executor.c)
- `{varname}` FD allocation syntax - IMPLEMENTED (bash 4.1+/zsh feature)

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

### Issue #57: Negation Command `! cmd` Causes Memory Corruption
**Severity**: HIGH  
**Discovered**: 2026-01-16 (Unit test coverage session)  
**Status**: FIXED (2026-01-16)  
**Component**: src/parser.c, src/executor.c

**Description**:
Using the negation operator `!` before a command causes a double-free or invalid free memory error. The shell crashes with "malloc: pointer being freed was not allocated".

**Reproduction**:
```bash
! false  # Should return 0
! true   # Should return 1
# Both cause: malloc: *** error for object 0x4: pointer being freed was not allocated
```

**Root Cause**:
The negation command handling in `execute_node()` or related functions has a memory management bug. The AST node or associated data is being freed twice, or a non-heap pointer is being freed.

**Impact**:
- `! command` syntax crashes the shell
- Affects any script using negation for conditional logic
- Common pattern `if ! cmd; then ...` will crash

**Priority**: HIGH (common shell construct)

**Fix Applied**:
The `!` character was being tokenized as a word and not recognized as a pipeline prefix.
- Added `NODE_NEGATE` type to node.h
- Modified `parse_pipeline()` in parser.c to check for `!` prefix and wrap pipeline in `NODE_NEGATE`
- Added `execute_negate()` in executor.c to execute child and invert exit status

**Now Working**:
```bash
! false   # Returns 0
! true    # Returns 1
! exit 5  # Returns 0
```

**Status**: FIXED AND VERIFIED

---

### Issue #58: Command Substitution Exit Status Not Preserved in $?
**Severity**: MEDIUM  
**Discovered**: 2026-01-16 (Unit test coverage session)  
**Status**: FIXED (2026-01-16)  
**Component**: src/executor.c

**Description**:
After a command substitution, `$?` does not reflect the exit status of the command that ran inside the substitution. It always returns 0 regardless of whether the inner command succeeded or failed.

**Reproduction**:
```bash
X=$(false)
echo $?  # Expected: 1, Actual: 0

X=$(exit 42)
echo $?  # Expected: 42, Actual: 0
```

**Expected Behavior (bash/zsh)**:
```bash
$ bash -c 'X=$(false); echo $?'
1
$ bash -c 'X=$(exit 42); echo $?'
42
```

**Root Cause**:
The exit status from the child process in command substitution was being captured but not propagated back to the executor's `exit_status` field after the substitution completes.

**Impact**:
- Scripts cannot check if command substitution succeeded
- Pattern `VAR=$(cmd) || handle_error` won't work correctly
- Reduces POSIX compliance

**Priority**: MEDIUM (workaround: check command separately)

**Fix Applied**:
Two changes in src/executor.c:
1. Modified `expand_command_substitution()` to propagate the child's exit status to `executor->exit_status` after waitpid
2. Modified `execute_assignment()` to save the exit status after expansion and restore it after the variable is set

**Now Working**:
```bash
X=$(false); echo $?   # Output: 1
X=$(exit 42); echo $?  # Output: 42
X=$(true); echo $?    # Output: 0
```

**Status**: FIXED AND VERIFIED

---

---

### Issue #60: Single Quotes Do Not Prevent Variable Expansion
**Severity**: HIGH  
**Discovered**: 2026-01-16 (Unit test coverage session)  
**Status**: FIXED (2026-01-16)  
**Component**: src/parser.c, src/executor.c

**Description**:
Single-quoted strings are incorrectly expanding variables. In all POSIX-compliant shells, single quotes must prevent ALL expansion - this is fundamental shell behavior, not a mode-dependent feature.

**Reproduction**:
```bash
# /tmp/test_quote.sh
VAR=value
RESULT='$VAR'
echo RESULT=$RESULT
```

**Expected (bash, zsh, all POSIX shells)**:
```
RESULT=$VAR
```

**Actual (lusush)**:
```
RESULT=value
```

**Root Cause**:
The executor has code at line 2955-2956 stating "Regular single-quoted strings: no expansion at all" but the expansion is happening anyway. Either:
1. The tokenizer is not correctly identifying single-quoted strings as `NODE_STRING` (non-expandable)
2. Or the single-quoted content is being passed through expansion before reaching that code path

**Impact**:
- Breaks fundamental shell quoting semantics
- Scripts relying on single quotes to protect special characters fail
- This is NOT mode-dependent - single quote behavior is POSIX-mandated

**Priority**: HIGH (fundamental shell semantics violation)

**Root Cause**:
Two issues combined:
1. The tokenizer strips quotes from token text (by design) but parser didn't preserve quote type
2. `expand_if_needed()` had no way to know content came from single quotes

**Fix Applied**:
1. Modified parser's assignment value collection to re-add single quotes around `TOK_STRING` token content
2. Modified `expand_if_needed()` to detect single quotes and preserve content literally
3. Added exception for command substitution `$(...)` which may contain internal quotes

**Now Working**:
```bash
VAR=xxx; RESULT='$VAR'; echo $RESULT  # Output: $VAR (literal)
A=1; B=2; RESULT="$A and $B"; echo $RESULT  # Output: 1 and 2 (expanded)
RESULT=$(cat <<< 'hello'); echo $RESULT  # Output: hello (here string works)
```

**Status**: FIXED AND VERIFIED

---

### Issue #59: Variable Concatenation `${A}_${B}` Causes Memory Corruption
**Severity**: HIGH  
**Discovered**: 2026-01-16 (Unit test coverage session)  
**Status**: FIXED (2026-01-16)  
**Component**: src/parser.c (assignment parsing)

**Description**:
When using braced variable expansion with text between expansions (e.g., `${A}_${B}`), the shell crashes with a memory corruption error.

**Reproduction**:
```bash
A=hello
B=world
RESULT=${A}_${B}
# Crashes with: malloc: *** error for object 0x...: pointer being freed was not allocated
```

**Also broken**:
```bash
RESULT=${A}_${B}  # underscore between
# Error output: error[E1101]: _: command not found
# Then crashes with malloc error

PREFIX=hello
RESULT=${PREFIX}world  # braced var followed by text
# Same malloc error - pointer being freed was not allocated
```

**Expected Result**:
```bash
$ bash -c 'A=hello; B=world; RESULT=${A}_${B}; echo $RESULT'
hello_world
```

**Root Cause**:
The variable expansion parsing appears to incorrectly handle text following a closing brace `}` when another expansion immediately follows. The underscore `_` is being parsed as a command rather than as literal text to concatenate.

**Impact**:
- Common pattern `${VAR1}_${VAR2}` crashes the shell
- Affects filename construction, variable naming patterns
- Workaround: use quotes `"${A}_${B}"` (but this may also fail)

**Priority**: HIGH (common shell pattern)

**Root Cause**:
The assignment parser was only consuming a single token as the value. When parsing
`RESULT=${A}_${B}`, it took only `${A}` as the value, leaving `_${B}` as separate
tokens interpreted as a command.

**Fix Applied**:
Modified `parse_simple_command()` in parser.c to collect all consecutive value tokens
when parsing assignments. The loop continues until hitting whitespace, a separator,
or a word followed by `=` (which indicates a new assignment).

**Now Working**:
```bash
A=hello; B=world; RESULT=${A}_${B}; echo $RESULT  # Output: hello_world
PREFIX=hello; RESULT=${PREFIX}world; echo $RESULT  # Output: helloworld
A=1 B=2 C=3 cmd  # Multiple assignments still work correctly
```

**Status**: FIXED AND VERIFIED

---

**Last Updated**: 2026-01-16 (Unit test coverage session)  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS

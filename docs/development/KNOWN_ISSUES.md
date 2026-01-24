# Lush Known Issues and Blockers

**Date**: 2026-01-23 (Updated: Session 126)  
**Status**: Critical tokenizer and executor bugs fixed  
**Implementation Status**: ANSI-C quoting, mapfile/readarray, nameref, coproc - all implemented  
**Memory Status**: Zero memory leaks verified with valgrind  
**Compatibility Test**: 100% pass rate on all 80 tests

---

## Session 126: Backslash-Newline Line Continuation & Parameter Expansion Execution Fixes

**Date**: 2026-01-23

### Issues Discovered and Fixed

This session addressed two critical bugs that caused lush to diverge from bash/zsh behavior in fundamental ways. Both bugs were discovered through empirical testing of a legacy test script that had worked years ago before recent refactoring.

---

### Issue #70: Backslash-Newline Line Continuation Not Removed in Double-Quoted Strings
**Severity**: HIGH  
**Discovered**: 2026-01-23 (Session 126 - test.sh regression testing)  
**Fixed**: 2026-01-23 (Session 126)  
**Component**: src/tokenizer.c (quoted string tokenization)

**Description**:
When a backslash immediately precedes a newline inside a double-quoted string, POSIX requires that both the backslash and newline be removed entirely (line continuation). Lush was keeping both characters literally in the output.

**Not Working (Before Fix)**:
```bash
echo "Hello, \
world!"
# Expected: Hello, world!
# Actual:   Hello, \
#           world!
```

**Test Script Symptom**:
```
Hello, \
world!
\

It's \
a \
fantastic \
Friday!
```

**Expected Output** (bash/zsh):
```
Hello, world!

It's a fantastic Friday!
```

**Root Cause**:
The tokenizer at lines 671-910 used `memcpy()` to bulk-copy the content of double-quoted strings from `segment_start` to the closing quote position. While there was code to advance `tokenizer->position` past `\<newline>` sequences (for line/column tracking), this didn't affect the actual token content - the raw input bytes including the backslash-newline were still copied verbatim.

**Technical Detail**:
The problematic code path was:
1. Enter quoted string parsing at line 671
2. Scan through content, advancing position past special sequences
3. On finding closing quote, execute: `memcpy(&result[result_len], &tokenizer->input[segment_start], segment_len)`
4. This copied the **raw input** including `\<newline>` sequences

**Fix Applied**:
Rewrote the double-quoted string tokenization to build the result **character by character** instead of bulk copying. This allows proper handling of:

1. **Line continuation (`\<newline>`)**: Both backslash and newline are completely skipped - nothing added to result
2. **Command substitution (`$(...)` and backticks)**: Copied verbatim for later expansion
3. **Other escape sequences (`\$`, `\"`, `\\`, etc.)**: Preserved for later escape processing
4. **Literal newlines**: Kept in result (newlines without preceding backslash are valid in double quotes)

Single-quoted strings continue to use bulk copy since no escape processing happens inside them per POSIX.

**Files Changed**:
- `src/tokenizer.c`: Lines 671-910 rewritten with character-by-character processing for double-quoted strings

**Verification**:
```bash
# All now work correctly:
echo "Hello, \
world!"                          # Output: Hello, world!

echo "Line 1\
Line 2\
Line 3"                          # Output: Line 1Line 2Line 3

echo "Tab:\there, newline:\
continued"                       # Proper escape + continuation
```

**Status**: FIXED AND VERIFIED (80/80 tests pass)

---

### Issue #71: Braced Parameter Expansion `${var}` Not Executing in Command Position
**Severity**: HIGH  
**Discovered**: 2026-01-23 (Session 126 - test.sh regression testing)  
**Fixed**: 2026-01-23 (Session 126)  
**Component**: src/executor.c (command execution)

**Description**:
When a braced parameter expansion `${var}` appeared in command position (as the command to execute), lush would expand the variable, **discard the result**, and return success without executing anything. Simple `$var` expansion worked correctly, creating an inconsistency.

**Not Working (Before Fix)**:
```bash
CMD=echo
${CMD} "hello"        # Expected: hello, Actual: (nothing)
${CMD=echo} "hello"   # Expected: hello, Actual: (nothing)

# Pipeline also broken:
echo "test" | ${FILTER=cat}   # Expected: test, Actual: (nothing)
```

**Inconsistent Behavior**:
```bash
$CMD "hello"          # Worked correctly: output "hello"
${CMD} "hello"        # Broken: silent no-op
```

**Root Cause**:
The executor at lines 1022-1046 had special-case handling that detected commands starting with `${` and:
1. Called `expand_variable()` to expand the parameter
2. **Immediately freed the result** with `free(result)`
3. Returned 0 (success) without executing anything

A second problematic block at lines 1079-1100 did the same for commands where ALL arguments were `${...}` expansions.

**The Buggy Code** (now removed):
```c
// Check for standalone parameter expansion ${...}
if (command->val.str && command->val.str[0] == '$' &&
    command->val.str[1] == '{') {
    char *result = expand_variable(executor, command->val.str);
    if (result) {
        free(result);  // BUG: Discards the expanded command name!
    }
    // ... similar for arguments ...
    return 0; // BUG: Returns without executing!
}
```

**Why This Code Existed**:
This appears to have been a misguided attempt to handle statements like `${var=default}` for side-effect-only assignment. However, the correct behavior per POSIX is that `${var=default}` BOTH assigns the value AND expands to that value, which should then be executed as a command (or produce "command not found").

**Fix Applied**:
Removed both problematic early-return blocks entirely. The existing `build_argv_from_ast()` function already correctly handles `${var}` expansion via `expand_if_needed()` - the early-return was bypassing this working code path.

**Files Changed**:
- `src/executor.c`: Removed lines 1022-1046 and 1079-1100 (the early-return blocks)

**Verification**:
```bash
# All now work correctly (matching bash/zsh):
CMD=echo; ${CMD} "hello"              # Output: hello
${CMD=echo} "hello from CMD"          # Output: hello from CMD
echo "test" | ${FILTER=cat}           # Output: test
result=$(${GEN=echo} "captured")      # result="captured"

# And the "command not found" case now correctly errors:
${n1=4} ${n2=6} ${n3=2}
# Output: error[E1101]: 4: command not found
# (Variables are still set as side effect, matching bash/zsh)
```

**POSIX Compliance Note**:
The correct idiom for assignment-only parameter expansion (without execution) is:
```bash
: ${var=default}    # Use null command ':'
# OR
var=default         # Direct assignment
```

Using `${var=default}` alone in command position is technically valid but will attempt to execute the expanded value as a command.

**Status**: FIXED AND VERIFIED (80/80 tests pass)

---

### Session 126 Summary

Both bugs were "elementary" issues that should have been caught earlier but slipped through during component rewrites. They highlight the importance of:

1. **Regression testing with real scripts**: The test.sh script that exposed these bugs had worked years ago
2. **Empirical verification**: The `${var}` bug was initially speculated to cause certain failures, but testing revealed the actual scope was different than expected
3. **Understanding POSIX semantics**: Both bugs involved fundamental POSIX shell behavior (line continuation, parameter expansion)

**Test Results After Fixes**:
- All 80 unit tests pass
- test.sh produces output matching bash/zsh
- No memory leaks introduced (verified via existing test suite)

---

## Session 125: Syntax Highlighting & History Expansion Fixes

**Date**: 2026-01-23

### Features Implemented

- **PROMPT_COMMAND support (bash 5.1+ style)**: Both string and array forms now supported
  - String form: `PROMPT_COMMAND="echo hello"` - executed before each prompt
  - Array form: `PROMPT_COMMAND=("echo hook1" "echo hook2")` - each element executed in order
  - Fixed array literal word-splitting bug where quoted elements like `"echo hello"` were incorrectly split

- **Hook function/array system enhancements**:
  - Zsh-style hook arrays: `precmd_functions+=()`, `preexec_functions+=()`, `chpwd_functions+=()`, `periodic_functions+=()`
  - Simple hook arrays (`FEATURE_SIMPLE_HOOK_ARRAYS`): `precmd+=()`, `preexec+=()`, `chpwd+=()` - shorthand syntax
  - Fixed `lle_shell_hooks_init()` to register POST_COMMAND handler when either `FEATURE_HOOK_FUNCTIONS` or `FEATURE_PROMPT_COMMAND` is enabled

- **Function name syntax highlighting**: All function definition forms now highlighted
  - `function foo { }` (ksh style)
  - `function foo() { }` (ksh/bash hybrid)  
  - `foo() { }` (POSIX style)
  - Calling defined functions highlights them with `LLE_TOKEN_COMMAND_FUNCTION`
  - Added `lle_shell_function_exists()` weak/strong symbol pattern for shell integration

- **Hook arrays syntax highlighting**: Special variable highlighting for shell hook arrays
  - `precmd_functions`, `preexec_functions`, `chpwd_functions`, `periodic_functions`
  - `precmd`, `preexec`, `chpwd` (simple hook arrays)
  - `PROMPT_COMMAND` (bash-style)
  - Works in `$VAR`, `${VAR}` expansions and assignments

- **Function completions**: Added shell functions to completion sources
  - New `LLE_COMPLETION_TYPE_FUNCTION` completion type
  - Functions appear in command-position completions

- **Directory completions with autocd**: Directories appear in empty-line completion menu when `FEATURE_AUTO_CD` enabled
  - Added `lle_shell_autocd_enabled()` weak/strong symbol pattern for shell integration
  - Directories shown at command position when autocd allows them as valid commands

### Issue #70: History Expansion Triggered Inside Quoted Strings - FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-23 (Session 125)  
**Fixed**: 2026-01-23 (Session 125)  
**Component**: src/lle/history/history_expansion.c

**Description**:
The shell would exit unexpectedly when entering commands containing `!` inside quoted strings during interactive function definition. For example:

```bash
hello() {
echo "Hello! From precmd!"
}
```

Pressing Enter after the `echo` line would cause the shell to exit silently.

**Root Cause**:
The `find_expansion_marker()` function in history expansion detected `!` characters as expansion markers without checking if they were inside quoted strings. The comment even noted "Check if it's escaped or in quotes - basic implementation" but the quote checking was never implemented.

When `!"` appeared (as in `precmd!"`), it was interpreted as a history expansion for "last command ending with `"`", which failed and caused unexpected behavior.

**Fix Applied**:
Added proper quote state tracking to `find_expansion_marker()`:
- Track `in_single_quote` and `in_double_quote` state while scanning
- Single quotes only toggle when not inside double quotes (handles `"It's"`)
- Double quotes only toggle when not inside single quotes (handles `'Say "hi"'`)
- Escape sequences handled correctly (only outside single quotes)
- `!` only recognized as expansion marker when outside all quotes

**Files Changed**:
- `src/lle/history/history_expansion.c`: `find_expansion_marker()` rewritten with quote tracking

**Verification**:
```bash
echo "Hello!"                    # Works
echo 'Hello!'                    # Works
echo "It's exciting!"            # Works (nested single quote)
echo 'Say "wow!"'                # Works (nested double quote)
echo "nested 'single' quotes!"   # Works
echo 'nested "double" quotes!'   # Works
```

**Status**: FIXED AND VERIFIED

---

### Compatibility Test Summary (Session 120)

#### Reference Shell Baselines (Target: 100%)

| Shell  | Applicable Tests | Passed | Skipped | Pass Rate |
|--------|-----------------|--------|---------|-----------|
| bash   | 63              | 63     | 24      | **100%** ✓ |
| zsh    | 63              | 63     | 24      | **100%** ✓ |

#### Lush Results

| Mode | Applicable Tests | Passed | Failed | Pass Rate | Notes |
|------|-----------------|--------|--------|-----------|-------|
| `set -o lush` (default) | 63 | 63 | 0 | **100%** ✓ | All tests passing |
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
When lush is invoked inside a `$()` command substitution (subshell capture), file redirections produce empty files even though the same command works correctly when run directly.

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
$ ./build/lush -c 'exec 3>/tmp/t.txt; exec 3>&-; echo test >&3'
error[E1121]: 3: Bad file descriptor

$ ./build/lush -c 'echo test >&3; echo "exit: $?"'
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
The `pwd -P` option should print the physical path with all symlinks resolved, but lush was returning the logical path instead.

**Root Cause**:
The `bin_pwd()` function only checked `shell_opts.physical_mode` global setting and completely ignored the `-P` and `-L` command-line options.

**Fix Applied**:
Added option parsing to `bin_pwd()` to handle `-P` (physical) and `-L` (logical) flags, which override the shell's default physical_mode setting.

**Now Working** (matches bash/zsh behavior):
```bash
$ ./build/lush -c 'cd /tmp; pwd; pwd -P; pwd -L'
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
5. Required external kill of the lush process

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
#!/usr/bin/env lush
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
When running scripts with `lush script.sh`, the pushd/popd/dirs builtins do not work correctly. The directory stack appears to reset or not persist between commands.

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
$ ./build/lush -c 'set -- a b c; for x; do echo $x; done'
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
$ ./build/lush -c 'function foo { echo bar; }; foo'
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
**Component**: Multiple - parser.c, autocorrect.c, lush.c, completion system  

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

**Actual (lush)**:
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

### Issue #61: Adjacent Quoted String Concatenation Produces Spaces
**Severity**: MEDIUM  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug  
**Component**: src/tokenizer.c or src/executor.c

**Description**:
When multiple quoted strings are concatenated without spaces (a common shell idiom), lush incorrectly outputs spaces between them instead of concatenating directly.

**Not Working**:
```bash
echo 'a'"b"'c'
# Expected: abc
# Actual:   a b c

echo 'a'$'b''c'
# Expected: abc
# Actual:   a bc
```

**Working** (bash comparison):
```bash
$ bash -c "echo 'a'\"b\"'c'"
abc
$ bash -c "echo 'a'\$'b''c'"
abc
```

**Root Cause** (suspected):
The tokenizer is treating each quoted segment as a separate token/word instead of recognizing that adjacent quotes without whitespace should form a single word. This is a fundamental shell behavior where `'a'"b"'c'` should be equivalent to `abc`.

**Impact**:
- Common shell idiom for embedding special characters: `'prefix'"$var"'suffix'`
- ANSI-C quoting concatenation: `$'\n''literal text'`
- Any mixed quoting patterns

**Priority**: MEDIUM (workaround: use a single quoting style or variables)

---

### Issue #62: Quote Continuation Idiom `'\''` Not Supported
**Severity**: MEDIUM  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug  
**Component**: src/tokenizer.c

**Description**:
The classic shell idiom for including a literal single quote inside single-quoted strings (`'\''`) is not supported. This pattern works by: ending the single quote, adding an escaped single quote, then starting a new single quote.

**Not Working**:
```bash
echo 'it'\''s a test'
# Expected: it's a test
# Actual:   error[E1003]: unterminated quoted string
```

**Working** (bash comparison):
```bash
$ bash -c "echo 'it'\''s a test'"
it's a test
```

**Root Cause**:
The tokenizer doesn't recognize that `'\''` is a valid pattern: `'...'` + `\'` + `'...'`. It appears to be failing to parse the `\'` as an escaped single quote outside of any quoting context.

**Note**: This is related to Issue #61 - both involve adjacent quoted strings not being properly concatenated.

**Impact**:
- Cannot include single quotes in single-quoted strings using standard idiom
- Breaks many existing shell scripts that use this pattern

**Workaround**:
Use ANSI-C quoting instead: `$'it\'s a test'`

**Priority**: MEDIUM (workaround available via ANSI-C quoting)

---

### Issue #63: printf Does Not Reuse Format for Multiple Arguments
**Severity**: LOW  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug  
**Component**: src/builtins/printf.c

**Description**:
When `printf` is given more arguments than format specifiers, it should reuse the format string for remaining arguments. Lush only processes arguments for the first iteration.

**Not Working**:
```bash
printf '%s\n' 'line1' 'line2'
# Expected:
#   line1
#   line2
# Actual:
#   line1
```

**Working** (bash comparison):
```bash
$ bash -c "printf '%s\n' 'line1' 'line2'"
line1
line2
```

**POSIX Requirement**:
> "The format operand shall be reused as often as necessary to satisfy the argument operands."

**Priority**: LOW (echo can be used as workaround)

---

### Issue #64: ANSI-C Quoting Not Working in Variable Assignments
**Severity**: HIGH  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug  
**Component**: src/tokenizer.c (word tokenization)

**Description**:
ANSI-C quoting (`$'...'`) works correctly when used as a command argument but fails when used as the value in a variable assignment. The `$` is treated literally and the single-quoted content is processed separately.

**Not Working**:
```bash
x=$'hello\nworld'
echo "$x"
# Expected:
#   hello
#   world
# Actual:
#   $hello
#   world
```

**Working** (as command argument):
```bash
echo $'hello\nworld'
# Output:
#   hello
#   world
```

**Working** (bash comparison):
```bash
$ bash -c "x=\$'hello\\nworld'; echo \"\$x\""
hello
world
```

**Root Cause**:
The tokenizer stops scanning a word when it encounters `$` after `=`. The assignment `x=$'hello'` is being tokenized as the word `x=` followed by a separate `$'hello'` token, rather than as a single word `x=$'hello'`.

**Impact**:
- Cannot use ANSI-C quoting for variable values
- Breaks scripts that assign escape sequences to variables

**Priority**: HIGH (common pattern for assigning newlines/tabs to variables)

---

### Issue #65: ANSI-C Quoting Incorrectly Expanded Inside Double Quotes
**Severity**: MEDIUM  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug - INTRODUCED BY RECENT CHANGE  
**Component**: src/executor.c (expand_if_needed)

**Description**:
Inside double quotes, `$'...'` should NOT be expanded - it should remain literal. However, lush incorrectly expands ANSI-C quoting even when inside double quotes.

**Not Working**:
```bash
echo "test$'\n'test"
# Expected: test$'\n'test
# Actual:   test
#           test
```

**Working** (bash comparison):
```bash
$ bash -c "echo \"test\$'\\n'test\""
test$'\n'test
```

**Root Cause**:
The recent change to `expand_if_needed()` in src/executor.c added handling for `$'...'` patterns, but this handling doesn't check whether the pattern is inside double quotes. Inside double quotes, only `$var`, `${var}`, `$(cmd)`, and `$((expr))` should be expanded, NOT `$'...'`.

**Impact**:
- Changes behavior of double-quoted strings containing literal `$'`
- Could break scripts that expect `$'` to be literal inside double quotes

**Priority**: MEDIUM (regression from recent change)

---

### Issue #66: Backslash Escape Outside Quotes Causes Parse Error
**Severity**: HIGH  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug (PRE-EXISTING)  
**Component**: src/tokenizer.c

**Description**:
Using a backslash to escape characters outside of quotes causes an "unterminated quoted string" parse error. This is fundamental shell functionality that should work.

**Not Working**:
```bash
echo \$HOME
# Expected: $HOME
# Actual:   error[E1003]: unterminated quoted string

echo a\\b
# Expected: a\b  (or a\\b depending on echo behavior)
# Actual:   error[E1003]: unterminated quoted string

echo \\
# Expected: \
# Actual:   error[E1003]: unterminated quoted string
```

**Working** (bash comparison):
```bash
$ bash -c 'echo \$HOME'
$HOME
$ bash -c 'echo a\\b'
a\b
```

**Root Cause**:
The tokenizer appears to be treating `\` followed by certain characters (like `$` or `\`) as the start of a quoted context that never ends, rather than as a simple escape sequence.

**Impact**:
- Cannot escape special characters outside quotes
- Breaks many shell scripts that use `\$` to print literal dollar signs
- Fundamental POSIX shell feature broken

**Priority**: HIGH (breaks basic shell escaping)

---

### Issue #67: ANSI-C Quoting in Array Literals Not Working
**Severity**: MEDIUM  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug  
**Component**: src/executor.c or src/parser.c

**Description**:
ANSI-C quoting inside array literal syntax `=(...)` does not work correctly. The array elements end up empty or incorrect.

**Not Working**:
```bash
arr=($'a\nb' $'c\nd')
echo "${arr[0]}"
# Expected:
#   a
#   b
# Actual:
#   (empty or incorrect)
```

**Working** (bash comparison):
```bash
$ bash -c "arr=(\$'a\\nb' \$'c\\nd'); echo \"\${arr[0]}\""
a
b
```

**Root Cause** (suspected):
The array literal parsing may not properly handle `$'...'` tokens, or the expansion happens at the wrong stage.

**Impact**:
- Cannot use escape sequences in array initialization
- Affects scripts that build arrays with special characters

**Priority**: MEDIUM (workaround: assign elements individually)

---

### Issue #68: Special Variables $RANDOM, $SECONDS, $LINENO Empty
**Severity**: LOW  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Active bug (or missing feature)  
**Component**: src/symtable.c or src/init.c

**Description**:
Several bash special variables that should have dynamic values are empty or unset in lush.

**Not Working**:
```bash
echo $RANDOM   # Expected: random number, Actual: empty
echo $SECONDS  # Expected: seconds since shell start, Actual: empty
echo $LINENO   # Expected: current line number, Actual: empty
echo $BASHPID  # Expected: current process PID, Actual: empty
```

**Working**:
```bash
echo $$        # Works: shows PID
echo $?        # Works: shows exit status
echo $!        # Works: shows background PID (after bg job)
echo $#        # Works: shows argument count
echo $PPID     # Works: shows parent PID
```

**Impact**:
- Scripts using `$RANDOM` for random behavior won't work
- Scripts timing operations with `$SECONDS` won't work
- Debug scripts using `$LINENO` won't show line numbers

**Priority**: LOW (these are bash extensions, not POSIX required)

---

### Issue #69: `hash -t` Option Not Supported
**Severity**: LOW  
**Discovered**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Status**: Missing feature  
**Component**: src/builtins/hash.c

**Description**:
The `hash -t` option (show path for a single command) is not supported.

**Not Working**:
```bash
hash -t ls
# Error: hash: invalid option
```

**Working** (bash):
```bash
$ bash -c 'hash ls; hash -t ls'
/bin/ls
```

**Impact**:
- Minor - can use `type -p` or `command -v` as alternatives

**Priority**: LOW (workaround available)

---

**Last Updated**: 2026-01-20 (Session 124 - ANSI-C quoting investigation)  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS

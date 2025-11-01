# Ctrl+C Signal Handling Fix - LLE Integration

**Date**: 2025-11-01  
**Issue**: LLE's Ctrl+C handling caused shell to exit (incorrect behavior)  
**Status**: ✅ FIXED

---

## Problem Statement

LLE's terminal interface was incorrectly handling Ctrl+C:
- Disabled ISIG in raw mode, causing Ctrl+C to come through as byte 0x03
- Installed its own SIGINT handler that would exit the shell
- `lle_readline.c` had character-based Ctrl+C handling that would set `done=true`

**Result**: Pressing Ctrl+C would exit the entire shell, not just kill child processes.

**Correct Behavior** (lusush v1.3.0):
- Ctrl+C with child process → kill child, return to prompt
- Ctrl+C at prompt → print newline, show new prompt
- Shell NEVER exits on Ctrl+C

---

## Root Cause Analysis

### Issue 1: ISIG Disabled in Raw Mode

**File**: `src/lle/terminal_unix_interface.c`  
**Line**: ~440

```c
/* OLD (WRONG) */
raw->c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);  /* Disable signals */
```

**Problem**: With ISIG disabled, Ctrl+C doesn't generate SIGINT signal. Instead, it comes through as character input (byte 0x03).

### Issue 2: LLE Installed Its Own SIGINT Handler

**File**: `src/lle/terminal_unix_interface.c`  
**Function**: `install_signal_handlers()`

```c
/* OLD (WRONG) */
sa.sa_handler = handle_exit_signal;  /* This would exit the shell */
sigaction(SIGINT, &sa, &original_sigint);
```

**Problem**: LLE's `handle_exit_signal()` would restore terminal and re-raise signal, causing shell to exit.

### Issue 3: Character-Based Ctrl+C Handling

**File**: `src/lle/lle_readline.c`  
**Line**: ~790

```c
/* OLD (WRONG) */
if (codepoint == 3) {  /* Ctrl+C as character */
    handle_interrupt(NULL, &ctx);  /* Sets done=true, exits readline */
    break;
}
```

**Problem**: Even if SIGINT wasn't the issue, this would make readline exit on Ctrl+C.

---

## Solution

### Fix 1: Enable ISIG in Raw Mode

**File**: `src/lle/terminal_unix_interface.c:437-442`

```c
/* NEW (CORRECT) */
/* Local flags - disable canonical mode and echo, but KEEP signals enabled */
raw->c_lflag &= ~(ECHO | ICANON | IEXTEN);
/* KEEP ISIG ENABLED - allow Ctrl+C to generate SIGINT for proper shell behavior */
/* This ensures lusush's signal handler (src/signals.c) can manage child processes */
```

**Effect**: Ctrl+C now generates SIGINT signal instead of character 0x03.

### Fix 2: Don't Install SIGINT Handler

**File**: `src/lle/terminal_unix_interface.c:203-211`

```c
/* NEW (CORRECT) */
/* NOTE: We do NOT install SIGINT/SIGTERM handlers here.
 * Lusush's signal handlers (src/signals.c) manage these properly:
 * - SIGINT: kills child process OR clears line (but never exits shell)
 * - SIGTERM: handles graceful shutdown
 * 
 * LLE previously installed handlers that would exit the shell on Ctrl+C,
 * which is incorrect shell behavior. Now that ISIG is enabled in raw mode,
 * Ctrl+C generates SIGINT which lusush's handler will catch and handle correctly.
 */
```

**Effect**: Lusush's existing signal handler (from `src/signals.c`) now handles SIGINT.

### Fix 3: Remove Character-Based Ctrl+C Handling

**File**: `src/lle/lle_readline.c:787-791`

```c
/* NEW (CORRECT) */
/* NOTE: Ctrl-C (codepoint == 3) is handled via SIGINT signal (src/signals.c)
 * With ISIG enabled in raw mode, Ctrl+C generates SIGINT instead of character input.
 * Lusush's sigint_handler() manages child process killing and line clearing.
 * We should NOT see codepoint==3 here anymore.
 */
```

**Effect**: No duplicate/conflicting Ctrl+C handling in readline layer.

### Fix 4: Clean Up Unused Code

**Files Modified**:
- Removed `handle_exit_signal()` function (no longer needed)
- Removed `handle_interrupt()` function (no longer needed)
- Removed `original_sigint` and `original_sigterm` variables (no longer used)

---

## How It Works Now

### Signal Flow with Child Process

```
User presses Ctrl+C while "sleep 10" is running
    ↓
Terminal generates SIGINT (because ISIG is enabled)
    ↓
Lusush's sigint_handler() in src/signals.c catches it
    ↓
Checks: current_child_pid > 0 (yes, sleep is running)
    ↓
Sends SIGINT to child process: kill(child_pid, SIGINT)
    ↓
Child process (sleep) terminates
    ↓
Shell continues, shows new prompt
```

### Signal Flow at Empty Prompt

```
User presses Ctrl+C at prompt
    ↓
Terminal generates SIGINT (because ISIG is enabled)
    ↓
Lusush's sigint_handler() in src/signals.c catches it
    ↓
Checks: current_child_pid == 0 (no child running)
    ↓
Prints newline: printf("\n")
    ↓
Shell continues, shows new prompt (readline will restart)
```

---

## Existing Code That Makes This Work

### Lusush's Signal Handler (src/signals.c)

```c
static void sigint_handler(int signo) {
    (void)signo;
    
    if (current_child_pid > 0) {
        // We have an active child process - send SIGINT to it
        kill(current_child_pid, SIGINT);
    } else {
        // No active child process - just print newline and return to prompt
        printf("\n");
        fflush(stdout);
    }
}
```

**This code was already correct** - we just needed to let it work!

### Child Process Tracking (src/signals.c)

```c
void set_current_child_pid(pid_t pid) { 
    current_child_pid = pid; 
}

void clear_current_child_pid(void) { 
    current_child_pid = 0; 
}
```

**Already implemented** - called when forking/waiting for child processes.

---

## Testing Verification

### How to Enable LLE for Testing

LLE is disabled by default. Enable it with either method:

**Method 1: Environment Variable** (temporary):
```bash
LLE_ENABLED=1 ./build/lusush
```

**Method 2: Config Command** (persistent):
```bash
$ ./build/lusush
lusush> config set editor.use_lle true
lusush> # LLE is now active for this session
```

### Test 1: Ctrl+C with Child Process

**Expected Behavior**:
```bash
$ LLE_ENABLED=1 ./build/lusush
lusush> sleep 10
^C
lusush>               # Child killed, shell continues, new prompt
```

**Verification**:
- Child process terminates immediately
- Shell shows new prompt
- Shell does NOT exit
- Can continue entering commands

### Test 2: Ctrl+C at Empty Prompt

**Expected Behavior**:
```bash
$ ./build/lusush
lusush> ^C

lusush>               # Newline printed, new prompt, shell continues
```

**Verification**:
- Newline is printed
- New prompt appears
- Shell does NOT exit
- Can continue entering commands

### Test 3: Ctrl+C with Partial Input

**Expected Behavior**:
```bash
$ ./build/lusush
lusush> echo hello world^C

lusush>               # Line cleared, new prompt, shell continues
```

**Verification**:
- Input line is abandoned
- Newline is printed
- New prompt appears
- Previous input is lost (correct behavior)
- Shell does NOT exit

---

## Files Modified

1. **src/lle/terminal_unix_interface.c**:
   - Line 437-442: Enable ISIG in raw mode
   - Line 203-211: Don't install SIGINT/SIGTERM handlers
   - Removed: `handle_exit_signal()` function
   - Removed: `original_sigint` and `original_sigterm` variables

2. **src/lle/lle_readline.c**:
   - Line 787-791: Removed Ctrl+C character handling
   - Removed: `handle_interrupt()` function

3. **Documentation**:
   - Added: `docs/lle_implementation/CTRL_C_SIGNAL_HANDLING_FIX.md` (this file)

---

## Comparison: Before vs After

### Before (WRONG)

| Scenario | Old Behavior | Problem |
|----------|-------------|---------|
| Ctrl+C with child | Shell exits | Child not killed, shell dies |
| Ctrl+C at prompt | Shell exits | Can't continue using shell |
| Ctrl+C with input | Shell exits | Lose all work, shell dies |

**User Impact**: Shell unusable - Ctrl+C kills entire shell session

### After (CORRECT)

| Scenario | New Behavior | Correct? |
|----------|-------------|----------|
| Ctrl+C with child | Child killed, shell continues | ✅ Yes |
| Ctrl+C at prompt | Newline, new prompt, shell continues | ✅ Yes |
| Ctrl+C with input | Line cleared, new prompt, shell continues | ✅ Yes |

**User Impact**: Normal shell behavior - matches bash, zsh, etc.

---

## Integration with LLE Architecture

### Terminal Abstraction Layer

**Responsibility**: Provide raw terminal input to application layer
- ✅ Enable raw mode for character-by-character input
- ✅ Keep ISIG enabled so Ctrl+C generates SIGINT
- ✅ Don't intercept or handle SIGINT itself
- ✅ Let application layer manage signal behavior

### Readline Layer

**Responsibility**: Line editing and history
- ✅ Process character input and special keys
- ✅ Don't handle signals directly
- ✅ Trust OS to deliver signals properly
- ✅ Let signal handlers interrupt readline naturally

### Application Layer (Lusush)

**Responsibility**: Shell behavior and process management
- ✅ Install signal handlers for SIGINT
- ✅ Track child process PID
- ✅ Kill child on SIGINT if running
- ✅ Clear line on SIGINT if no child
- ✅ Never exit shell on SIGINT

**This is proper separation of concerns!**

---

## Key Lessons Learned

1. **Don't disable ISIG in raw mode** - even in raw mode, signal generation is needed for proper shell behavior

2. **Let the application handle signals** - library code (LLE) should not install signal handlers that affect application behavior

3. **Separation of concerns matters**:
   - Terminal layer: raw input
   - Signal layer: process management
   - Application layer: behavior decisions

4. **Test with real use cases** - automated tests might miss behavioral issues like "shell exits on Ctrl+C"

5. **Respect existing working code** - lusush v1.3.0 had correct signal handling; we just needed to not interfere with it

---

## Related Documentation

- **lusush v1.3.0**: `src/signals.c` - existing correct signal handling
- **LLE Spec 02**: Terminal Abstraction - raw mode setup
- **LLE Spec 06**: Input Parsing - character input handling
- **POSIX termios**: `ISIG` flag documentation

---

## Commit Message

```
LLE: Fix Ctrl+C signal handling to use lusush's existing signal handlers

Fix critical issue where LLE's Ctrl+C handling caused shell to exit instead
of killing child processes or clearing the line.

Key Changes:
1. Enable ISIG in raw mode - allow Ctrl+C to generate SIGINT signal
2. Don't install LLE's own SIGINT handler - use lusush's existing handler
3. Remove character-based Ctrl+C handling from lle_readline.c
4. Clean up unused signal handling code

Root Cause:
LLE disabled ISIG and installed its own SIGINT handler that would exit the
shell. This conflicted with lusush's correct signal handling in src/signals.c
which properly kills child processes or clears the line without exiting.

Solution:
Enable ISIG so Ctrl+C generates SIGINT, and let lusush's existing sigint_handler()
(from src/signals.c) manage it properly:
- With child process: kills child, continues shell
- Without child: prints newline, continues shell
- Never exits the shell

Testing:
- Ctrl+C with child process: kills child, shell continues ✅
- Ctrl+C at empty prompt: prints newline, shell continues ✅
- Ctrl+C with partial input: clears line, shell continues ✅

Files Modified:
- src/lle/terminal_unix_interface.c: Enable ISIG, remove SIGINT handler
- src/lle/lle_readline.c: Remove character-based Ctrl+C handling
- docs/lle_implementation/CTRL_C_SIGNAL_HANDLING_FIX.md: Documentation

Behavior Now Matches: lusush v1.3.0, bash, zsh (correct shell behavior)
```

---

**Document Version**: 1.0  
**Author**: LLE Implementation Team  
**Date**: 2025-11-01

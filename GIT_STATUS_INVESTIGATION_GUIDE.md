# Git Status Investigation Guide

## 🔍 TECHNICAL INVESTIGATION: Git Status Interactive Mode Issue

**Problem**: `git status` and similar external commands fail to display output when run in interactive mode with TTY, but work perfectly in non-interactive mode.

## 📊 EVIDENCE SUMMARY

### ✅ Working Cases
```bash
# Non-interactive mode (piped input) - WORKS PERFECTLY:
echo "git status" | ./builddir/lusush
echo "git log --oneline -5" | ./builddir/lusush
echo "git branch" | ./builddir/lusush

# All produce correct output immediately
```

### ❌ Failing Cases  
```bash
# Interactive mode (real TTY) - NO OUTPUT:
./builddir/lusush
lusush$ git status
# Returns to prompt with no output
# OR hangs waiting for something
```

### ✅ Working Interactive Commands
```bash
# These work perfectly in interactive mode:
lusush$ whoami          # Works
lusush$ date             # Works  
lusush$ ls               # Works
lusush$ echo "test"      # Works
lusush$ pwd              # Works
```

## 🎯 ROOT CAUSE ANALYSIS

### Primary Hypothesis: TTY/Terminal State Conflict
Git behaves differently when it detects a TTY vs non-TTY:
- **Non-TTY**: Git outputs directly to stdout (works)
- **TTY**: Git expects proper terminal control for paging, colors, etc. (fails)

### Secondary Hypothesis: Signal Handling Interference
Readline's signal handling may interfere with git's signal expectations:
- Git may be waiting for signals that readline intercepts
- Child process signal handling may be corrupted

### Tertiary Hypothesis: File Descriptor Issues
Git may not be receiving proper TTY file descriptors:
- stdin/stdout/stderr inheritance from parent process
- Terminal device access for child processes

## 🔬 DIAGNOSTIC PROCEDURES

### Phase 1: Confirm the Issue Pattern
```bash
# Test basic git functionality
./builddir/lusush
lusush$ git --version           # Does this work?
lusush$ git config --get user.name  # Does this work?
lusush$ git status >/tmp/test.txt && cat /tmp/test.txt  # Does redirection work?
```

### Phase 2: TTY Analysis
```bash
# Check TTY state in interactive shell
./builddir/lusush
lusush$ tty                     # Should show TTY device
lusush$ echo $TERM              # Check terminal type
lusush$ stty -a                 # Check terminal settings
```

### Phase 3: Process Tracing
```bash
# Trace system calls to see what git is doing
./builddir/lusush
lusush$ strace -e trace=write,read,ioctl,execve git status 2>&1 | head -20
lusush$ timeout 5 git status    # Check if it hangs
```

### Phase 4: Signal Investigation
```bash
# Check signal handling
./builddir/lusush  
lusush$ strace -e trace=signal git status 2>&1
```

## 🔧 CODE INVESTIGATION POINTS

### 1. Readline Signal Configuration
**File**: `src/readline_integration.c` lines 682-706

Current configuration:
```c
rl_catch_signals = 0; // Let shell handle signals for child processes like git
rl_catch_sigwinch = 1; // Handle window resize only
```

**Investigation**: Check if this is sufficient or if additional signal management needed.

### 2. External Command Execution
**File**: `src/executor.c` lines 1609-1620, 1978-1988

Current implementation:
```c
// Reset terminal state before forking for external commands
if (is_interactive_shell()) {
    fflush(stdout);
    fflush(stderr);
}

pid_t pid = fork();
if (pid == -1) {
    set_executor_error(executor, "Failed to fork");
    return 1;
}
```

**Investigation**: Check if terminal state needs to be reset more comprehensively.

### 3. Child Process TTY Inheritance
**Key Question**: Does the child process (git) receive proper TTY file descriptors?

Check in child process setup:
- Are stdin/stdout/stderr properly inherited?
- Does the child have access to controlling terminal?
- Are signal handlers properly reset in child?

## 🛠️ POTENTIAL SOLUTIONS

### Solution 1: Terminal State Reset
```c
// In execute_external_command_with_redirection()
if (is_interactive_shell()) {
    // Save current terminal state
    // Reset to canonical mode for child
    // Restore after child exits
}
```

### Solution 2: Explicit TTY Management
```c
// Ensure child gets proper TTY access
if (is_interactive_shell()) {
    // Explicitly pass TTY file descriptors
    // Reset signal handlers in child
    // Ensure proper process group management
}
```

### Solution 3: Readline Terminal Hooks
```c
// Use readline's terminal management hooks
rl_prep_term_function = reset_for_child;
rl_deprep_term_function = restore_after_child;
```

### Solution 4: Process Group Management
```c
// Ensure proper process group for child processes
if (is_interactive_shell()) {
    setpgid(pid, pid);  // Create new process group
    tcsetpgrp(STDIN_FILENO, pid);  // Give terminal control
}
```

## 🧪 TESTING METHODOLOGY

### Minimal Test Case
```bash
# Create simple test script
echo '#!/bin/bash
echo "Before git"
git status
echo "After git"
' > test_git.sh
chmod +x test_git.sh

# Test in both modes
echo "./test_git.sh" | ./builddir/lusush  # Should work
./builddir/lusush
lusush$ ./test_git.sh                     # Should work if fix is correct
```

### Comprehensive Test Suite
1. **Git commands**: status, log, branch, diff
2. **Other external commands**: less, more, man, vim
3. **Interactive programs**: top, htop (if available)
4. **Pipelines**: `git log | head -5`
5. **Redirection**: `git status > file.txt`

## 📋 IMPLEMENTATION CHECKLIST

### Pre-Implementation
- [ ] Reproduce the issue consistently
- [ ] Identify exact failure mode (hang vs no output vs error)
- [ ] Determine which external commands are affected
- [ ] Verify other readline features still work

### Implementation Phase
- [ ] Backup current working code
- [ ] Implement targeted fix (avoid breaking existing features)
- [ ] Test incrementally with simple cases first
- [ ] Verify no regression in tab completion, history, themes

### Post-Implementation  
- [ ] Test all git commands work in interactive mode
- [ ] Verify other external commands still work
- [ ] Test performance impact (should be minimal)
- [ ] Test on multiple terminal types if possible

## ⚠️ CRITICAL WARNINGS

### DO NOT BREAK EXISTING FEATURES
The following are working perfectly and must remain functional:
- Tab completion cycling
- History navigation without artifacts
- All key bindings (Ctrl+A/E/L/U/K/W/G)
- Theme switching and colored prompts
- Multiline command support
- Non-interactive mode execution

### AVOID THESE MISTAKES
1. **Don't revert signal handling** - Current config works for most commands
2. **Don't modify readline core setup** - Focus on child process execution
3. **Don't change fork/exec logic extensively** - Targeted fixes only
4. **Don't break performance** - Must maintain sub-millisecond response

## 📚 REFERENCE MATERIALS

### Key Files to Understand
- `src/readline_integration.c` - Readline wrapper and signal handling
- `src/executor.c` - Command execution engine
- `src/signals.c` - Signal management for child processes  
- `debug_git_interactive.sh` - Comprehensive test script

### External References
- GNU Readline manual: Signal handling and terminal management
- POSIX process groups and terminal control documentation
- Git source code: How git detects and handles TTY vs non-TTY output

### Similar Issues in Other Shells
Research how bash/zsh handle:
- Terminal state management for child processes
- Signal handling conflicts with readline
- TTY inheritance for external commands

## 🎯 SUCCESS CRITERIA

### Functional Success
```bash
./builddir/lusush
lusush$ git status           # Shows full repository status
lusush$ git log --oneline -5 # Shows commit history
lusush$ git branch           # Shows branch list
lusush$ man git              # Opens manual page properly
lusush$ less README.md       # Works with proper terminal control
```

### Performance Success
- No measurable impact on command execution time
- Tab completion still responds in <50ms
- History navigation still artifact-free
- Theme switching still instant

### Stability Success
- No crashes or hangs
- Proper signal handling maintained
- All existing features continue working
- Cross-platform compatibility preserved

---

**BOTTOM LINE**: This is a specific TTY/terminal state management issue. The fix should be targeted and surgical, preserving all the excellent work already completed in the readline integration.
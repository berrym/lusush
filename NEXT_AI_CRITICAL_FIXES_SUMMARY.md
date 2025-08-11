# Next AI Assistant: Critical Interactive Mode Fixes

## 🚨 URGENT ISSUES TO FIX

### Issue #1: History Navigation Broken (CRITICAL)
**Problem**: UP arrow shows "display all 4418 possibilities y/n" instead of navigating history
**Root Cause**: Arrow keys bound to completion instead of history navigation
**Fix**: Remove custom arrow key bindings in `src/readline_integration.c`

### Issue #2: Ctrl+C Exits Shell (SAFETY CRITICAL)  
**Problem**: Ctrl+C exits entire shell instead of clearing line
**Root Cause**: Signal handling not integrated properly with readline
**Fix**: Modify SIGINT handler in `src/signals.c` to clear line, not exit shell

### Issue #3: Interactive Operations Don't Work
**Problem**: Pipes, redirections, command substitution fail in interactive mode
**Note**: These work perfectly in non-interactive mode
**Fix**: Debug readline integration affecting command execution

## ✅ WHAT WORKS (DON'T BREAK)
- Git status in interactive mode (recently fixed)
- Non-interactive mode (all features work perfectly)
- Theme system and prompts
- Basic command execution
- Build system

## 🎯 QUICK FIXES

### Fix History Navigation (30 minutes)
File: `src/readline_integration.c` lines 670-680
```c
// REMOVE these lines that bind arrow keys to custom functions:
rl_bind_keyseq("\\e[A", lusush_previous_history); // Remove
rl_bind_keyseq("\\e[B", lusush_next_history);     // Remove

// Let readline handle arrow keys natively for history
```

### Fix Signal Handling (30 minutes)  
File: `src/signals.c` sigint_handler function
```c
// Ensure handler clears line but doesn't exit shell
if (current_child_pid > 0) {
    kill(current_child_pid, SIGINT);
} else {
    printf("\n");
    rl_replace_line("", 0);
    rl_forced_update_display();
    // DON'T call exit() or return in a way that exits shell
}
```

## 🧪 TESTING (MANUAL ONLY)
```bash
# MUST test interactively - automated tests won't reveal issues
./builddir/lusush

# Test 1: UP arrow should navigate history, not show completion
# Test 2: Ctrl+C should clear line, not exit shell  
# Test 3: echo hello | grep h should work
# Test 4: git status should still work (don't break this)
```

## 🎯 SUCCESS CRITERIA
1. UP arrow navigates command history
2. Ctrl+C clears current line, shell continues
3. Basic shell operations work in interactive mode
4. Git status still works (no regression)

**Time Estimate**: 2-4 hours for core fixes
**Focus**: Interactive mode usability, not architectural changes
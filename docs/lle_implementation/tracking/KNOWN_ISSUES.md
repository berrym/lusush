# LLE Known Issues and Blockers

**Date**: 2025-12-27 (Updated: Session 71 Audit)  
**Status**: ✅ MAJOR BLOCKERS RESOLVED - Testing in progress  
**Implementation Status**: Spec 25 (Prompt/Theme) VERIFIED, Spec 26 (Shell Integration) VERIFIED

---

## Executive Summary

**Current State**: Major architectural blockers resolved, system functional

- ✅ **Blockers #1-4 RESOLVED** - See Resolved Architectural Blockers section
- ✅ **Issue #16 FIXED** - Shell-level event system working, prompt cache invalidation functional
- ✅ **Issue #20** - Prompt/theme system tested and working (theme switching verified)
- ✅ **Issue #21** - Theme architecture verified working (10 themes available, runtime switching works)
- ⚠️ **Remaining Issues** - Enhancements and minor fixes, no blockers
- ✅ **Issue #17 Fixed** - Command-aware directory completion for cd/rmdir (Session 53, 2025-12-21)
- ℹ️ **Issue #18 Clarified** - pushd/popd are bash extensions, not POSIX; not implemented in lusush (future work)
- ✅ **Issue #14 Documented** - Git-aware prompt not showing git info (2025-12-02)
- ✅ **Issue #15 Documented** - Tab handling uses formula-based calculation (2025-12-02)
- ✅ **Issue #9 Fixed** - Cursor positioning after menu display (Session 25)
- ✅ **Issue #10 Fixed** - Arrow key navigation (Session 25)
- ✅ **Issue #11 Fixed** - Menu dismissal (Session 26)
- ✅ **Issue #12 Fixed** - Column shifting during navigation (Session 26)
- ✅ **Issue #13 Fixed** - UP/DOWN column preservation with category-aware navigation (Session 26)
- ✅ **Spec 12 v2 completion integrated** - Duplicates eliminated (Session 23)
- ✅ **No Blockers** (all issues are enhancements or menu behavior)
- ✅ **Living document enforcement active**
- ✅ **Meta/Alt keybindings working** (Session 14)
- ✅ **Multi-line prompts working** (Session 14)
- ✅ **Multiline ENTER display bug fixed** (Session 15)
- ✅ **break/continue in loops fixed** (Session 16)
- ✅ **Multiline pipeline execution fixed** (Session 16)
- ✅ **Continuation prompts with full Unicode support** (Session 17-18)
- ✅ **Completion generation proper architecture** (Session 23)
- ✅ **Completion menu cursor positioning fixed** (Session 25)
- ✅ **Completion menu dismissal working** (Session 26)

---

## Resolved Architectural Blockers

These blockers have been resolved and verified working as of 2025-12-27.

### Blocker #1: LLE Prompt/Theme System ✅ VERIFIED WORKING
**Previous Status**: CRITICAL - Code exists, zero manual testing  
**Current Status**: ✅ VERIFIED WORKING (2025-12-27)  
**Components**: `include/lle/prompt/`, `src/lle/prompt/`

**Resolution**:
The LLE Spec 25 prompt/theme system has been manually tested and verified:
- ✅ Theme switching via `display lle theme set <name>` works
- ✅ Colors render correctly (user=green, directory=blue, git=magenta, symbol=green)
- ✅ 10 themes now available: minimal, default, classic, powerline, informative, two-line, corporate, dark, light, colorful
- ✅ Theme changes take effect on next prompt
- ✅ All segment types rendering correctly

---

### Blocker #2: LLE Initialization System ✅ IMPLEMENTED (Spec 26)
**Previous Status**: CRITICAL - No spec or design exists  
**Current Status**: ✅ IMPLEMENTED AND WORKING (2025-12-27)  
**Component**: `src/lle/lle_shell_integration.c`, `include/lle/lle_shell_integration.h`

**Resolution**:
Spec 26 Shell Integration System implemented:
- ✅ `g_lle_integration` provides persistent shell-level LLE state
- ✅ Persistent editor (`g_lle_integration->editor`) survives across readline sessions
- ✅ Shell event hub for lifecycle events
- ✅ Proper initialization at shell startup, cleanup at shutdown
- ✅ Double-free bug on Ctrl+D exit was fixed (proved persistent editor is active)

---

### Blocker #3: Shell-Level Event System ✅ IMPLEMENTED AND WORKING
**Previous Status**: HIGH - Architecture gap identified  
**Current Status**: ✅ WORKING (2025-12-27)  
**Component**: Dual event system architecture

**Resolution**:
The system now has two event layers working together:
- **Shell-level events**: `lle_shell_event_hub_t` in shell integration (persistent)
- **Per-readline events**: `lle_event_system_t` created/destroyed per readline session

**Verified Working**:
- ✅ Directory change events fire correctly after `cd` commands
- ✅ Pre/post command events functional
- ✅ Prompt updates correctly after directory changes
- ✅ No stale prompt issues observed (Issue #16 root cause resolved)

---

### Blocker #4: Prompt System Integration ✅ INTEGRATED
**Previous Status**: HIGH - Components exist but not wired together  
**Current Status**: ✅ INTEGRATED AND WORKING (2025-12-27)  
**Components**: LLE prompt system ↔ shell prompt generation

**Resolution**:
- ✅ LLE prompt composer generates prompts
- ✅ Theme registry provides colors and layouts
- ✅ `display lle theme` commands added to builtins for runtime control
- ✅ Segment system renders user, host, directory, git, symbol correctly
- ✅ 4 legacy themes ported (corporate, dark, light, colorful)

---

## Active Issues

### Issue #24: Transient Prompt Not Yet Implemented
**Severity**: LOW  
**Discovered**: 2025-12-27 (Session 71 - Audit)  
**Status**: Infrastructure exists, not wired  
**Component**: src/lle/prompt/composer.c, Spec 25 Section 12  

**Description**:
Transient prompts (simplifying previous prompts after command execution to reduce visual clutter) are specified in Spec 25 Section 12 but not yet implemented.

**Current State**:
- ✅ `enable_transient` config flag exists in `lle_prompt_composer_config_t`
- ✅ `transient_format` field exists in `lle_prompt_layout_t`
- ✅ Themes define transient formats (e.g., powerline has `"❯ "`)
- ✅ Pre-command handler now records command info for transient support
- ❌ Transient prompt rendering not implemented in display layer
- ❌ No code to replace previous prompt with transient version

**What Transient Prompts Do**:
When enabled, after a command executes, the fancy multi-line prompt is replaced with a minimal version in the scrollback. This keeps the terminal cleaner:

```
Before transient (current):
[user@host] ~/long/path (feature/lle *) $ echo hello
hello
[user@host] ~/long/path (feature/lle *) $ ls
...
[user@host] ~/long/path (feature/lle *) $ 

After transient (desired):
❯ echo hello
hello
❯ ls
...
[user@host] ~/long/path (feature/lle *) $ 
```

**Implementation Notes** (from Spec 25):
1. Pre-command event triggers transient prompt replacement
2. Use cursor movement to go back to previous prompt line
3. Overwrite with transient format from theme
4. Requires tracking prompt line position

**Priority**: LOW (enhancement, not a blocker)

**Status**: DOCUMENTED - Future work after core features stable

---

### Issue #23: Extra Space Between Prompt and Cursor
**Severity**: LOW  
**Discovered**: 2025-12-25 (Session 66 - manual testing)  
**Status**: Fix identified but reverted - blocked by Issue #16  
**Component**: src/display/composition_engine.c, src/display/screen_buffer.c  

**Description**:
There are two spaces between the `$` at the end of the prompt theme and where the cursor is located / command input starts. There should only be one space.

**Example**:
```bash
[mberry@fedora-xps13.local] ~/Lab/c/lusush (feature/lle *) $  |
                                                            ^^ two spaces instead of one
```

**Root Cause**:
The composition engine in `compose_simple_strategy()` and `compose_multiline_strategy()` checks if the prompt ends with a space before adding one between prompt and command. However, themed prompts end with ANSI escape sequences like `$ \001\033[0m\002`, so a naive check of `prompt_content[prompt_len - 1] != ' '` fails - it sees the escape sequence terminator, not the space.

**Fix Approach (Documented for Future Implementation)**:
A fix was implemented in Session 66 but reverted because it was intertwined with other changes that caused Issue #16 to worsen. The fix approach that worked:

1. **Add utility function to screen_buffer.c**:
   ```c
   bool screen_buffer_ends_with_visible_space(const char *text, size_t length);
   ```
   This function scans forward through the text, skipping:
   - Readline escape markers (`\001` and `\002`)
   - ANSI escape sequences (ESC followed by CSI sequence ending in letter)
   
   It tracks the last visible grapheme cluster using LLE's TR#29 support:
   - `lle_utf8_sequence_length()` for UTF-8 byte length
   - `lle_is_grapheme_boundary()` for proper grapheme cluster detection
   - `lle_utf8_decode_codepoint()` to get the base codepoint
   
   Returns true if the last visible codepoint is U+0020 (space).

2. **Update composition_engine.c**:
   Replace the naive check in both `compose_simple_strategy()` and `compose_multiline_strategy()`:
   ```c
   // OLD (broken):
   if (prompt_content[prompt_len - 1] != ' ') {
       output[written++] = ' ';
   }
   
   // NEW (correct):
   if (!screen_buffer_ends_with_visible_space(prompt_content, prompt_len)) {
       output[written++] = ' ';
   }
   ```

**Why Fix Was Reverted**:
The fix was committed alongside other changes attempting to fix Issue #16. When Issue #16 proved to have a deeper root cause (stale git prompt data), all Session 66 commits were reverted to restore a known working state. The extra space fix itself worked correctly but should be re-implemented AFTER Issue #16 is properly fixed.

**Priority**: LOW (cosmetic, does not affect functionality)

**Dependencies**:
- Should be re-implemented AFTER Issue #16 is fixed
- Must use LLE's TR#29 grapheme cluster support, not byte-level string operations

**Status**: DOCUMENTED - Fix approach known, blocked by Issue #16

---

### Issue #20: Theme System Overwrites User PS1/PS2 Customization ✅ ARCHITECTURE VERIFIED
**Severity**: HIGH → LOW (architecture working)  
**Discovered**: 2025-12-21 (Session 54 - theme variable investigation)  
**Resolved**: 2025-12-27 (Manual testing verified)  
**Component**: LLE Spec 25 prompt/theme system, src/builtins/builtins.c  

**Description**:
The old theme system unconditionally overwrites PS1/PS2 shell variables.

**Resolution**:
The new LLE prompt/theme system is now integrated and working:
- ✅ `display lle theme set <name>` - Switch themes at runtime
- ✅ `display lle theme list` - View available themes
- ✅ Theme switching verified working in manual testing
- ✅ Colors and segments render correctly

**Remaining Work** (Future Enhancement):
- `respect_user_ps1` config flag not yet exposed to users
- `theme off` command could be added to disable theming entirely

**Status**: ✅ ARCHITECTURE VERIFIED WORKING - Minor enhancements remain

---

### Issue #21: Theme System Not User-Extensible ✅ ARCHITECTURE VERIFIED
**Severity**: HIGH → MEDIUM (architecture working, file loading not implemented)  
**Discovered**: 2025-12-21 (Session 54 - theme variable investigation)  
**Updated**: 2025-12-27 (Manual testing verified)  
**Component**: LLE Spec 25 prompt/theme system (`include/lle/prompt/theme.h`)  

**Description**:
The OLD theme system was completely hardcoded in C.

**Resolution - Architecture Verified Working**:
The new LLE prompt/theme system architecture is verified:
- ✅ 10 built-in themes available and switchable at runtime
- ✅ Theme registry with source tracking (BUILTIN, SYSTEM, USER, RUNTIME)
- ✅ Theme inheritance support (`inherits_from` field)
- ✅ Template-based prompts working
- ✅ Runtime theme switching via `display lle theme set`

**Not Yet Implemented** (Future Enhancement):
- ❌ File parser for theme config files (TOML/INI format)
- ❌ `~/.config/lusush/themes/` directory scanning at startup
- ❌ `lle_theme_load_from_file()` function

**Priority**: MEDIUM (architecture works, file loading is future enhancement)

**Status**: ✅ ARCHITECTURE VERIFIED - File-based themes future work

---

### Issue #22: Template Variables exit_code/jobs Not Implemented
**Severity**: MEDIUM  
**Discovered**: 2025-12-21 (Session 54 - theme variable investigation)  
**Status**: Dead code - flags exist but no implementation  
**Component**: src/themes.c  

**Description**:
The `prompt_template_t` struct has `enable_exit_code` and `enable_job_count` boolean flags, and some themes set these to `true`, but:
1. No template variables (`%?`, `%{exit_code}`, `%{jobs}`) are actually implemented
2. No theme templates use these variables in their template strings
3. The flags are set but never used to add variables to the template context

**Current State**:
```c
// In prompt_template_t:
bool enable_exit_code;    // Set to true in dark, colorful themes
bool enable_job_count;    // Set to true in colorful theme

// But NO code adds these variables:
// Missing: template_add_variable(ctx, "exit_code", ..., NULL, false);
// Missing: template_add_variable(ctx, "jobs", ..., NULL, false);

// And NO templates use them:
// No template contains "%?" or "%{exit_code}" or "%{jobs}"
```

**Available Shell State** (could be used):
- `last_exit_status` - global in `src/globals.c`
- `executor->jobs` - linked list with `job_state_t` (RUNNING, STOPPED, DONE)

**Planned Variables** (from conventions in other shells):
- `%?` or `%{exit_code}` - last command exit status (non-zero typically shown)
- `%{jobs}` - count of background jobs
- `%{running_jobs}` - count of running jobs
- `%{stopped_jobs}` - count of stopped jobs

**Why Not Fixed Now**:
Given Issues #20 and #21 (theme system architectural problems), adding more features to a broken system is not the right approach. These variables should be added as part of the larger theme system redesign.

**Real Fix Required**:
1. First fix Issue #20 (respect user PS1/PS2)
2. First fix Issue #21 (make themes user-extensible)
3. Then implement these variables in the template context
4. Update default themes to use them (for users who opt in)

**Priority**: MEDIUM (dead code, but blocked by more fundamental issues)

**Status**: DOCUMENTED - Blocked by Issues #20, #21

---

### Issue #19: Display Controller Initialization Fails in Editor Terminals
**Severity**: LOW  
**Discovered**: 2025-12-21 (Session 54 - adaptive terminal testing)  
**Status**: Not yet investigated  
**Component**: src/display/display_controller.c  

**Description**:
When lusush runs in editor terminals (Zed, VS Code) that have non-TTY stdin but capable stdout, the display controller initialization fails with error 3:

```
[DC_ERROR] display_controller_init:1199: Failed to initialize base terminal (error 3) - using defaults
```

The shell still functions correctly because it falls back to defaults, but this indicates the display controller's terminal initialization doesn't handle the enhanced/editor terminal mode properly.

**Reproduction**:
Run lusush in Zed's terminal panel (or similar editor terminal):
```bash
echo "echo test; exit" | ./builddir/lusush
```

**Context**:
This was discovered while testing the new adaptive terminal detection integration. The adaptive detection correctly identifies Zed as an interactive terminal (`TERM_PROGRAM=zed`, `force_interactive=true`), but the display controller's base terminal initialization still fails.

**Likely Cause**:
The display controller's terminal initialization probably checks `isatty(STDIN_FILENO)` and fails when stdin is not a TTY, even though the terminal is capable (stdout is TTY, supports colors, cursor positioning, etc.).

**Impact**:
- Shell functions correctly (falls back to defaults)
- Some advanced display features may be disabled
- Error message visible in debug output

**Workaround**:
None needed - shell works correctly with fallback defaults.

**Priority**: LOW (shell works, cosmetic error message)

**Resolution Plan**:
1. Investigate `display_controller_init()` at line 1199
2. Check if it can use adaptive terminal detection results instead of raw `isatty()` checks
3. Allow initialization to succeed for enhanced/editor terminal modes

**Status**: DOCUMENTED - Needs investigation

---

### Issue #18: pushd/popd Not Implemented (Bash Extension)
**Severity**: LOW  
**Discovered**: 2025-12-21 (Session 53 - manual testing)  
**Status**: Future work - not a bug  
**Component**: Shell builtins  

**Description**:
The `pushd` and `popd` commands are not implemented in lusush. They are highlighted red (invalid command) because they genuinely don't exist - this is correct syntax highlighting behavior.

**Background**:
- `pushd`/`popd` are **bash extensions**, not POSIX shell commands
- Lusush currently targets POSIX shell compatibility
- These commands were mistakenly included in the directory completion list

**Resolution**:
- Removed `pushd`/`popd` from the directory-only command list in source_manager.c
- Only `cd` and `rmdir` (POSIX commands) now trigger directory-only completion
- Implementing pushd/popd is future work when bash compatibility is expanded

**Priority**: LOW (future enhancement, not current scope)

**Status**: DOCUMENTED - Future work when adding bash extensions

---

### Issue #16: Prompt/Cursor Desync Display Corruption ✅ FULLY RESOLVED
**Severity**: HIGH → RESOLVED  
**Discovered**: 2025-12-17 (Session 52 - manual testing)  
**Resolved**: 2025-12-27 (Shell-level event system verified working)  
**Status**: ✅ FULLY RESOLVED - Shell-level event system working  
**Component**: Prompt/theme system cache architecture  
**Full Investigation**: See `ISSUE_16_INVESTIGATION.md` for complete technical analysis

**Resolution**:
The issue is now fully resolved with proper architectural solution:

1. **Immediate Fix**: `prompt_cache_invalidate()` in `bin_cd()` after directory change
2. **Architectural Fix**: Spec 26 Shell Integration provides persistent event system:
   - `lle_shell_event_hub_t` - Persistent shell-level event hub
   - `LLE_EVENT_DIRECTORY_CHANGED` - Fires on directory changes
   - Dual event system: shell-level (persistent) + per-readline (session-scoped)

**Verified Working (2025-12-27)**:
- ✅ Directory changes trigger proper prompt refresh
- ✅ Git status updates correctly after `cd` to different repos
- ✅ No stale prompt data observed in testing
- ✅ Pre/post command events functional

**Description**:
Display corruption where the prompt and cursor position become desynchronized. The prompt may be partially overwritten or truncated, and typed input appears in unexpected positions.

**Root Cause: Four Independent Cache Layers with Mismatched Invalidation**

The prompt system has FOUR caches that operate independently:

| Cache | Location | TTL | Directory Aware? |
|-------|----------|-----|------------------|
| **Git Info** | `src/prompt.c` | **10 seconds** | **NO - THIS IS THE BUG** |
| Prompt Content | `src/prompt.c` | 5 seconds | YES |
| Prompt Layer | `src/display/prompt_layer.c` | 5 seconds | NO |
| Theme | `src/themes.c` | 30 seconds | NO |

**The Critical Bug**:
The git info cache in `src/prompt.c` uses pure time-based expiry:

```c
void update_git_info(void) {
    time_t now = time(NULL);
    // BUG: Returns cached info if < 10 seconds old
    // Does NOT check if directory changed!
    if (now - last_git_check < GIT_CACHE_SECONDS) {
        return;  // STALE DATA RETURNED
    }
    get_git_status(&git_info);
    last_git_check = now;
}
```

**The Race Condition**:
```
Time 0s:  User in /git_repo_A, git_info cached (branch: feature/lle)
Time 3s:  User types "cd /tmp" and presses ENTER
Time 4s:  cd completes, now in /tmp (NOT a git repo)
          NO CACHE INVALIDATION OCCURS
Time 5s:  Next readline prompt needed
          - build_prompt() detects directory changed
          - Calls theme_generate_primary_prompt()
          - Calls update_git_info()
          - 5s - 0s = 5s < 10s TTL
          - Returns WITHOUT updating git_info!
          - Prompt shows "(feature/lle)" even though /tmp is not a git repo
```

**Why Previous Fix Attempts Failed**:
All Session 66 fixes targeted the DISPLAY side, but the problem is on the DATA side:
- Clearing display layers just re-renders the same stale data
- The git_info struct contains old repository data
- Only time expiry (10 seconds) clears it

**Missing Integration Point**:
`display_integration_post_command_update()` in `src/display_integration.c`:
- Called after every command completes
- Does NOT detect directory-changing commands
- Does NOT invalidate any caches
- Does NOT force git info refresh

**Symptoms**:
- Stale git branch/status shown after `cd`
- Cursor position calculated wrong due to prompt width mismatch
- Typed commands overlay prompt content
- Autosuggestion ghost text appears in wrong position

**Reproduction**:
```bash
LLE_ENABLED=1 ./builddir/lusush
cd /tmp           # Change to non-git directory
# Observe: git info still appears in prompt
# Type any command - cursor position will be wrong
```

**Workaround**:
- Press Ctrl+L to clear screen and force full redraw
- Wait 10 seconds for git cache to expire

**Solution Options** (from investigation):

1. **Quick Fix** (Not Recommended): Detect `cd` in post-command hook, force git refresh
   - Fragile string matching, doesn't handle all cases

2. **Better Fix**: Track directory at readline entry, invalidate on change
   - More robust but still a band-aid

3. **Best Fix**: Event-driven cache invalidation (PWD_CHANGED event)
   - Clean architecture, handles all cases

4. **Recommended**: Move prompt/theme into LLE as new specification
   - Solves #16, #20, #21, #22 together
   - Uses LLE's existing event system
   - Single unified cache management

**Related Issues**:
- Issue #20: Theme overwrites user PS1/PS2 (same architectural problem)
- Issue #21: Themes not user-extensible (same system)
- Issue #22: Template variables dead code (blocked by same issues)
- Issue #23: Extra space after prompt (may be symptom of width mismatch)

**Priority**: HIGH (affects basic usability, root cause of multiple symptoms)

**Status**: ROOT CAUSE IDENTIFIED - Requires architectural redesign of prompt/theme system

---

### Issue #14: Git-Aware Prompt Not Displaying Git Information
**Severity**: MEDIUM  
**Discovered**: 2025-12-02 (macOS LLE compatibility session)  
**Status**: Not yet fixed  
**Component**: src/prompt.c - git status integration  

**Description**:
The git-aware themed prompt is not displaying any git information (branch name, status indicators). The prompt shows username, hostname, and path correctly, but the git branch/status section is missing entirely.

**Reproduction**:
```bash
$ ./builddir/lusush
[mberry@Michaels-Mac-mini.local] ~/Lab/c/lusush $
# ^ No git branch shown, should show branch and status in a git repository
```

**Expected Behavior**:
```bash
[mberry@Michaels-Mac-mini.local] ~/Lab/c/lusush (feature/lle) $
# Or with status indicators like *, +, etc.
```

**Root Cause**:
A blanket fix was applied to suppress git stderr messages (which were causing display corruption when running `cd /tmp` outside of a git repository). The fix added `2>/dev/null` redirection to git commands in `src/prompt.c`:

```c
// In get_git_branch():
return run_command("git branch --show-current 2>/dev/null", branch, branch_size);

// In get_git_status():
if (run_command("git rev-parse --git-dir 2>/dev/null", NULL, 0) != 0) {
    return; // Not in a git repository
}
```

The stderr redirection is correct, but the issue is likely that:
1. The `run_command()` function may not be returning output correctly with the redirection
2. Or the git commands are failing for another reason on macOS
3. Or there's a PATH issue where git isn't being found

**Investigation Needed**:
1. Check if `run_command()` properly captures stdout while redirecting stderr
2. Verify git is in PATH when lusush runs
3. Test git commands directly to ensure they work
4. May need to use `2>/dev/null` only for the stderr while still capturing stdout

**Original Issue Being Fixed**:
When `cd /tmp` was executed (outside a git repo), git commands output:
```
fatal: not a git repository (or any of the parent directories): .git
```
This stderr output corrupted the display because it wasn't being suppressed.

**Proper Fix Strategy**:
The stderr redirection is correct for suppressing error messages, but we need to ensure stdout is still captured. The issue may be in how `run_command()` handles the command string with shell redirection.

**Workaround**:
None currently - git information simply doesn't appear in prompts.

**Priority**: MEDIUM (affects user experience but shell is functional)

**Files Involved**:
- `src/prompt.c` - `get_git_branch()` and `get_git_status()` functions

**Status**: DOCUMENTED - Needs investigation

---

### Issue #15: Tab Character Handling Uses Formula-Based Calculation
**Severity**: LOW  
**Discovered**: 2025-12-02 (macOS LLE compatibility session)  
**Status**: ✅ FIXED (2025-12-02)  
**Component**: src/lle/keybinding_actions.c, src/display/screen_buffer.c, src/lle/display_bridge.c  

**Description**:
Tab character handling had two issues:
1. Hardcoded 8-column tab stops instead of using `config.tab_width` (default 4)
2. Inserting literal `\t` characters violated character-by-character design principle

**Fix Applied (Two-Part Solution)**:

**Part 1: Tab Input Expansion** (`src/lle/keybinding_actions.c`)
The `lle_tab_insert()` function now expands tabs to spaces at input time based on
the current visual column position. This ensures the buffer never contains literal
tab characters, maintaining true character-by-character tracking:

```c
lle_result_t lle_tab_insert(lle_editor_t *editor) {
    /* Expand tab to spaces based on visual column position */
    int tab_width = config.tab_width > 0 ? config.tab_width : 4;
    size_t visual_col = editor->buffer->cursor.visual_column;
    size_t spaces_to_insert = tab_width - (visual_col % tab_width);
    
    char spaces[16];
    memset(spaces, ' ', spaces_to_insert);
    spaces[spaces_to_insert] = '\0';
    
    return lle_buffer_insert_text(editor->buffer, ..., spaces, spaces_to_insert);
}
```

**Part 2: Display Layer Fallback** (`screen_buffer.c`, `display_bridge.c`)
Updated all 6 display locations to use `config.tab_width` as a fallback for any
tabs that may come from external sources (pasted text, history, etc.):

```c
if (ch == '\t') {
    int tw = config.tab_width > 0 ? config.tab_width : 4;
    size_t tab_width = tw - (col % tw);
    // ...
}
```

**Resolution**: 
- Tab insertion now expands to spaces at input time (character-by-character compliant)
- Display layer respects `config.tab_width` for any tabs from external sources
- User's `behavior.tab_width` setting is now honored throughout

---

### Issue #7: Completion Menu - Category Disambiguation Not Implemented
**Severity**: MEDIUM  
**Discovered**: 2025-11-22 (Session 23 Part 2)  
**Status**: Not yet fixed (documented for future work)  
**Component**: Completion system v2 / source manager  

**Description**:
When the same command exists in multiple categories (e.g., builtin `echo` vs external `/usr/bin/echo`, or alias `ls` vs external `ls`), the completion system currently deduplicates based on text only, not considering the type/category. This means only one entry appears in the menu, and users cannot disambiguate which version to execute.

**ACTUAL OBSERVATION** (Session 23 Part 3 - 2025-11-22):
Testing shows `echo` appears ONLY in the "builtin command" section, NOT in "external command" section at all. This suggests the deduplication is working as designed, but external commands that shadow builtins are being removed from the external category. User cannot see or select `/usr/bin/echo` from completion menu.

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

### Issue #17: Command-Aware Directory Completion ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-12-21 (Session 53)  
**Fixed**: 2025-12-21 (Session 53)  
**Component**: src/lle/completion/source_manager.c, src/lle/completion/completion_sources.c  

**Description**:
When using `cd` or `rmdir` commands, the completion menu showed both files and directories. For these directory-only commands, only directories should be shown as completion candidates.

**Root Cause**:
The file source generator ignored the `context->command_name` field, always returning both files and directories regardless of what command was being completed.

**Fix Applied**:
1. Added `is_directory_only_command()` helper in `source_manager.c` to identify POSIX commands that only accept directory arguments (cd, rmdir)

2. Added `lle_completion_source_directories()` function in `completion_sources.c` that only returns directories (excludes regular files)

3. Modified `file_source_generate()` to check `context->command_name`:
   - If command is cd/rmdir: use `lle_completion_source_directories()`
   - Otherwise: use `lle_completion_source_files()` (both files and directories)

**Note**: pushd/popd are bash extensions not implemented in lusush (see Issue #18).

**Files Modified**:
- `src/lle/completion/source_manager.c` - Command-aware file source generation
- `src/lle/completion/completion_sources.c` - Added `lle_completion_source_directories()`
- `include/lle/completion/completion_sources.h` - Added function declaration

**Verification**:
- ✅ `cd <TAB>` shows only directories
- ✅ `rmdir <TAB>` shows only directories
- ✅ `ls <TAB>` shows both files and directories
- ✅ All 51 tests pass

**Status**: ✅ FIXED

---

### Issue #13: UP/DOWN Navigation Doesn't Preserve Column Position ✅ FIXED
**Severity**: LOW  
**Discovered**: 2025-11-25 (Session 26)  
**Fixed**: 2025-11-26 (Session 26 continued)  
**Component**: completion_menu_logic.c  

**Description**:
When navigating UP/DOWN in the completion menu, the selection jumped to different columns when crossing category boundaries. The navigation treated all items as one continuous grid, but categories visually separate rows.

**Root Cause**:
The navigation functions (`move_up`, `move_down`, `move_left`, `move_right`) calculated row/column positions globally (`index / columns`, `index % columns`) without accounting for category boundaries. Each category has its own visual rows, so:
- Moving DOWN from the last row of category 1 should go to the first row of category 2, same column
- Moving UP from the first row of category 2 should go to the last row of category 1, same column

**Fix Applied**:
1. Added `find_category_for_index()` helper to find category boundaries for any item
2. Rewrote `move_up` and `move_down` to be category-aware:
   - Calculate row/column position within current category
   - When moving past category boundary, jump to adjacent category preserving column
   - Use `target_column` for sticky column behavior (falls back to last item if row is shorter)
3. Updated `move_left` and `move_right` to also be category-aware for consistent column calculation

**Files Modified**:
- `src/lle/completion/completion_menu_logic.c` - Category-aware navigation

**Expected Behavior Now**:
- UP/DOWN moves between rows within category, then jumps to adjacent category
- Column position preserved using `target_column` (set by LEFT/RIGHT)
- If target row has fewer items, selects last item in that row
- Navigation wraps at boundaries (last category → first, and vice versa)

**Status**: ✅ FIXED

---

### Issue #12: Completion Menu Column Shifting During Navigation ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-11-25 (Session 25)  
**Fixed**: 2025-11-25 (Session 26)  
**Component**: completion_menu_renderer.c  

**Description**:
Menu columns shifted position during navigation, making the menu layout unstable.

**Root Cause**:
Two issues:
1. Renderer recalculated column width/count on every render instead of using cached layout
2. `visual_width()` function didn't skip ANSI escape sequences, so selected items (with highlighting codes) had incorrect width calculations, causing padding to be wrong

**Fix Applied**:
1. Modified `lle_completion_menu_render()` to use pre-cached `state->column_width` and `state->num_columns` instead of recalculating
2. Updated `visual_width()` to properly skip ANSI escape sequences (CSI format: ESC [ ... final_byte)
3. Added terminal resize handling to recalculate layout when window size changes

**Files Modified**:
- `src/lle/completion/completion_menu_renderer.c` - Use cached layout, fix visual_width
- `src/lle/lle_readline.c` - Recalculate layout on WINDOW_RESIZE

**Status**: ✅ FIXED AND VERIFIED

---

### Issue #11: Completion Menu Dismissal Not Working ✅ FIXED
**Severity**: HIGH  
**Discovered**: 2025-11-25 (Session 25)  
**Fixed**: 2025-11-25 (Session 26)  
**Component**: keybinding_actions.c, lle_readline.c, sequence_parser.c, terminal_unix_interface.c  

**Description**:
There was no way to dismiss/clear the completion menu. ESC, Ctrl+G, typing characters, backspace, and ENTER did not dismiss the menu.

**Root Causes and Fixes**:

1. **ESC Key**: Parser waited indefinitely for escape sequence bytes
   - Added `lle_sequence_parser_check_timeout()` to return standalone ESC after 50ms
   - Modified `terminal_unix_interface.c` to use 60ms timeout when parser is accumulating
   - Added `lle_escape_context()` handler to dismiss menu

2. **Ctrl+G**: Didn't check for menu state
   - Modified `lle_abort_line_context()` to dismiss menu on first press
   - Second press aborts line as normal

3. **Character Input/Backspace**: v2 completion system not checked
   - Added v2 checks to `handle_character_input()` and keybinding actions

4. **ENTER Key**: Buffer duplication bug
   - Inline preview already updates buffer during navigation
   - Old code tried to replace based on stale context, causing "echocho"
   - Fixed by just clearing menu (buffer already correct)

**Files Modified**:
- `src/lle/sequence_parser.c` - Added timeout check function
- `include/lle/input_parsing.h` - Added declaration
- `src/lle/terminal_unix_interface.c` - Shorter timeout when accumulating, timeout check
- `src/lle/lle_readline.c` - ESC handler, abort line fix, character input fix, ENTER fix
- `src/lle/keybinding_actions.c` - v2 completion checks

**Status**: ✅ FIXED AND VERIFIED

---

### Issue #10: Completion Menu Arrow Key Navigation Broken ✅ FIXED
**Severity**: HIGH  
**Discovered**: 2025-11-25 (Session 25)  
**Fixed**: 2025-11-25 (Session 25)  
**Component**: keybinding_actions.c / completion_menu_logic.c  

**Description**:
Arrow key navigation moved selection incorrectly - UP/DOWN moved left/right instead of between rows.

**Root Cause**:
Navigation functions used linear index movement instead of row-based calculation.

**Fix Applied**:
- Added `terminal_width`, `column_width`, `num_columns` to menu state
- Added `lle_completion_menu_update_layout()` for dynamic column calculation
- Changed `move_up/move_down` to use row-based navigation with `num_columns`

**Files Modified**:
- `include/lle/completion/completion_menu_state.h` - New layout fields
- `src/lle/completion/completion_menu_state.c` - Layout calculation
- `src/lle/completion/completion_menu_logic.c` - Row-based navigation
- `src/display/display_controller.c` - Call layout update when menu set

**Status**: ✅ FIXED AND VERIFIED

---

### Issue #9: Completion Menu Cursor Positioning Bug ✅ FIXED
**Severity**: CRITICAL  
**Discovered**: 2025-11-25 (Session 24 - Recovery from lost work)  
**Fixed**: 2025-11-25 (Session 25)  
**Component**: display_controller.c cursor positioning after menu display  

**Description**:
After displaying the completion menu and updating inline text, the cursor was positioned one row too high. This caused each subsequent completion to overwrite the previous line, consuming terminal rows on each TAB press.

**Detailed Symptoms** (before fix):
1. First TAB press: Text updates correctly (e.g., 'e' → 'echo'), menu displays
2. Cursor then moves one row UP from where it should be
3. Next TAB press redraws everything from the wrong position
4. Each completion consumes the previous terminal row

**Root Cause**:
Off-by-one error in the cursor positioning calculation. The code was:
```c
int current_terminal_row = final_row + 1 + menu_lines;  // WRONG
```

The `+1` for the separator newline was incorrect. The separator newline moves the cursor FROM the command row TO the first menu row, but `menu_lines` already counts starting from that position.

**Fix Applied** (Session 25):
Changed the calculation to:
```c
int current_terminal_row = final_row + menu_lines;  // CORRECT
```

Example with command on row 0 and 9-line menu:
- Row 0: "prompt> echo" (command)
- Row 1-9: menu (9 lines)
- Terminal cursor at row 9 after writing menu
- To get back to row 0: move up 9 rows = (0 + 9) - 0 = 9 ✓

**Debug Values** (from testing):
```
cursor_row=0, cursor_col=65, final_row=0, menu_lines=9
OLD: current_terminal_row=10, rows_to_move_up=10 (WRONG - moved up too far)
NEW: current_terminal_row=9, rows_to_move_up=9 (CORRECT)
```

**Files Modified**:
- `src/display/display_controller.c` (lines ~470-500)

**Verification**:
- ✅ Cursor stays on correct row after TAB completion
- ✅ Multiple TAB presses don't consume terminal rows
- ✅ Completion cycling works without display corruption

**Status**: ✅ FIXED AND VERIFIED

---

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

**Active Issues**: 11  
**Merge Blockers**: 2 (Issues #20, #21 - theme system violates user choice principles)  
**High Priority**: 4 (Issues #16 - cursor desync/stale prompt; #20, #21, #22 - theme/prompt system)  
**Medium Priority**: 4 (Issues #5, #6 - syntax highlighting; #7 - category disambiguation; #14 - git prompt)  
**Low Priority**: 3 (Issue #8 - menu display format; #19 - editor terminal display controller; #23 - extra space after prompt)  
**Session 66 (2025-12-25)**: Root cause identified for Issue #16, reverted failed fix attempts, documented Issue #23 fix approach
**Implementation Status**: Spec 12 v2 completion integrated, menu fully functional  

**Pre-Merge Requirements (BLOCKING)**:
1. **Issue #20**: Theme system must respect user PS1/PS2 customization
2. **Issue #21**: Theme system must be user-extensible (config file themes, not hardcoded C)

**Critical Path**:
1. **Issue #16 FIRST**: Fix stale git prompt / cursor desync (root cause now understood)
2. **Issue #23**: Re-implement extra space fix AFTER Issue #16 is resolved
3. **Issues #20, #21**: Theme system architectural work

**Next Action**: 
- (HIGH) Fix Issue #16 - trace prompt generation flow, ensure fresh prompt each readline session
- (BLOCKING) Design and implement user choice for prompt system
- (BLOCKING) Implement theme file loading from user directories
- (Future) Re-implement Issue #23 fix (extra space) after #16 is fixed
- (Future) Fix git-aware prompt (Issue #14)
- (Future) Category disambiguation for completion conflicts
- (Future) Multi-column menu display investigation

---

**Last Updated**: 2025-12-25  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS

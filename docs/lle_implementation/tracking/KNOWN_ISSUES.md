# LLE Known Issues and Blockers

**Date**: 2026-01-02 (Updated: Session 102)  
**Status**: ✅ MAJOR MILESTONE - GNU Readline Removed, LLE-Only, Zero Memory Leaks  
**Implementation Status**: Spec 25 (Prompt/Theme) VERIFIED, Spec 26 (Shell Integration) VERIFIED

---

## Session 91 Milestone: GNU Readline Removal Complete

**Date**: 2025-12-31

LLE is now the **sole line editing system** in lusush. GNU readline support has been completely removed:
- ~5,200+ lines of legacy code deleted
- 7 source files and 7 header files removed
- `config.use_lle` option removed (LLE always enabled)
- History file standardized to `.lusush_history`
- All 58 unit tests passing
- All 49 POSIX regression tests passing

**Bugs fixed during migration**:
- Prompt not displaying on startup (NULL prompt handling)
- History builtin showing no output (bridge not initialized)

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
- ✅ **Issue #14 Fixed** - Git-aware prompt now working via LLE prompt system (Session 94, 2026-01-01)
- ✅ **Issue #15 Fixed** - Tab handling uses formula-based calculation (2025-12-02)
- ✅ **Issue #8 Fixed** - Multi-column completion menu implemented (Session 94, 2026-01-01)
- ✅ **Issue #5 Fixed** - Multiline builtin highlighting working (Session 94, 2026-01-01)
- ✅ **Issue #6 Fixed** - Continuation prompt separate from content highlighting (Session 94, 2026-01-01)
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

**Reproduction**:
```bash
$ for i in 1 2 3; do
>     echo $i        # Typing here causes cursor flicker
>     # Any input on continuation lines triggers it
> done
```

Or with line wrapping:
```bash
$ echo "this is a very long command that wraps to the next line and any typing here..."
# Cursor flickers during input on wrapped portion
```

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

### Issue #24: Transient Prompt System ✅ FULLY IMPLEMENTED
**Severity**: LOW  
**Discovered**: 2025-12-27 (Session 71 - Audit)  
**Status**: ✅ RESOLVED (Session 72-73)  
**Resolved**: 2025-12-27  
**Component**: LLE widget hooks, display controller, config system  

**Description**:
Transient prompts (simplifying previous prompts after command execution to reduce visual clutter) are specified in Spec 25 Section 12.

**Resolution** (Final Implementation - Session 73):
The original implementation using direct terminal writes was rejected as it violated LLE's architecture. The final implementation uses proper LLE systems:

**Architecture**:
1. **New Widget Hook**: `LLE_HOOK_LINE_ACCEPTED` fires after Enter but before `dc_finalize_input()`
2. **Widget**: `widget_transient_prompt()` registered on LINE_ACCEPTED hook
3. **Display Controller**: `dc_apply_transient_prompt()` handles rendering through screen buffer
4. **Config Integration**: `config.display_transient_prompt` with `display lle transient on|off` command

**Key Files**:
- `include/lle/widget_hooks.h` - Added LLE_HOOK_LINE_ACCEPTED
- `src/lle/widget/widget_hooks.c` - Added "line-accepted" to HOOK_NAMES
- `src/lle/lle_readline.c` - Fire LINE_ACCEPTED before dc_finalize_input
- `src/lle/widget/builtin_widgets.c` - widget_transient_prompt implementation
- `include/display/display_controller.h` - dc_apply_transient_prompt declaration
- `src/display/display_controller.c` - dc_apply_transient_prompt implementation
- `include/config.h` - display_transient_prompt field
- `src/config.c` - Config option registration
- `src/builtins/builtins.c` - display lle transient command

**All 10 Themes Updated**:
Every built-in theme now has `enable_transient = true` and a `transient_format`.

**What Transient Prompts Do**:
```
Before transient:
┌─[user@host]─[~/path]─[(branch *)]
└─$ echo hello
hello
┌─[user@host]─[~/path]─[(branch *)]
└─$ 

After transient:
$ echo hello
hello
┌─[user@host]─[~/path]─[(branch *)]
└─$ 
```

**Configuration**:
```bash
# Toggle via command
display lle transient on
display lle transient off

# Via config system
config set display.transient_prompt true
config save
```

**Status**: ✅ FULLY IMPLEMENTED AND TESTED

---

### Issue #23: Extra Space Between Prompt and Cursor ✅ RESOLVED
**Severity**: LOW  
**Discovered**: 2025-12-25 (Session 66 - manual testing)  
**Resolved**: 2025-12-28 (Spec 25/26 implementation)  
**Status**: ✅ RESOLVED - Fixed by Spec 25 prompt architecture  
**Component**: LLE Spec 25 prompt/theme system  

**Description**:
There were two spaces between the `$` at the end of the prompt theme and where the cursor is located / command input starts. There should only be one space.

**Resolution**:
The issue was caused by the old composition engine's naive space detection that didn't account for ANSI escape sequences. This was fully resolved by the Spec 25 prompt/theme system implementation:

1. The LLE prompt composer now generates prompts with proper spacing control
2. Theme templates explicitly define spacing in the `transient_format` and prompt layouts
3. The display controller integration bypasses the old composition engine's flawed logic

The fix that was reverted in Session 66 is no longer needed - the architectural redesign in Spec 25/26 eliminated the root cause by replacing the old prompt generation path entirely.

**Verification**:
- ✅ Tested on macOS (Session 73)
- ✅ Tested on Linux/Fedora (Session 75)
- ✅ All 10 themes display correct spacing

**Status**: ✅ RESOLVED

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

### Issue #21: Theme System Not User-Extensible ✅ FULLY RESOLVED
**Severity**: HIGH → RESOLVED  
**Discovered**: 2025-12-21 (Session 54 - theme variable investigation)  
**Resolved**: 2025-12-28 (Session 77 - Theme file loading implementation)  
**Component**: LLE Spec 25 prompt/theme system  

**Description**:
The OLD theme system was completely hardcoded in C. Users could not create or customize themes without modifying source code.

**Resolution - Theme File Loading System Implemented**:
Session 77 implemented a complete theme file loading system inspired by starship.rs:

**New Files**:
- `include/lle/prompt/theme_parser.h` - TOML-subset parser API
- `src/lle/prompt/theme_parser.c` - Custom parser (~800 lines, no dependencies)
- `include/lle/prompt/theme_loader.h` - File loading and export API
- `src/lle/prompt/theme_loader.c` - Directory scanning, hot reload, TOML export

**Features Implemented**:
- ✅ Custom TOML-subset parser (dependency-free, ~800 lines)
- ✅ Theme file loading from `~/.config/lusush/themes/` and `/etc/lusush/themes/`
- ✅ All color formats: basic ANSI, 256-color, hex RGB (#ff5500), rgb(255,85,0)
- ✅ Theme export to TOML via `display lle theme export <name>`
- ✅ Hot reload via `display lle theme reload`
- ✅ Automatic loading of user themes at shell startup
- ✅ Theme inheritance support in file format

**Example Theme File** (`~/.config/lusush/themes/ocean.toml`):
```toml
[theme]
name = "ocean"
description = "A calming blue-green theme"
inherits_from = "default"

[colors]
primary = { fg = "cyan", bold = true }
git_branch = { fg = "#00ffff" }

[symbols]
prompt = "~>"
```

**User Workflow**:
```bash
display lle theme export powerline ~/.config/lusush/themes/my-theme.toml
vim ~/.config/lusush/themes/my-theme.toml
display lle theme reload
display lle theme set my-theme
```

**Status**: ✅ FULLY RESOLVED

---

### Issue #22: Template Variables exit_code/jobs Not Implemented ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-12-21 (Session 54 - theme variable investigation)  
**Fixed**: 2025-12-31 (Session 87)  
**Status**: ✅ RESOLVED  
**Component**: LLE prompt/segment system  

**Description**:
The LLE prompt template system had `${status}` and `${jobs}` segments defined, but:
- `lle_prompt_context_t.last_exit_code` was only partially wired
- `lle_prompt_context_t.background_job_count` was never updated - completely dead code

**Resolution (Session 87)**:
1. Verified exit_code wiring was already complete via shell event system
2. Added `executor_count_jobs()` function to count active background jobs
3. Added `lle_prompt_context_set_job_count()` setter function
4. Wired job count update before prompt rendering in `src/prompt.c` and `src/display_integration.c`

**Theme Usage**:
The "minimal" theme demonstrates these variables:
```
PS1:  ${?status:[${status}] }${symbol}    # Shows [1] if last command failed
RPS1: ${time}${?jobs: [${jobs}]}          # Shows [2] if 2 background jobs
```

The `${?var:text}` syntax only shows text when the segment is visible (non-zero).

**Files Modified**:
- `include/executor.h` - Added `executor_count_jobs()` declaration
- `src/executor.c` - Implemented `executor_count_jobs()`
- `include/lle/prompt/segment.h` - Added `lle_prompt_context_set_job_count()` declaration
- `src/lle/prompt/segment.c` - Implemented setter
- `src/prompt.c` - Update job count before render
- `src/display_integration.c` - Update job count before render

**Status**: ✅ FIXED AND VERIFIED

---

### Issue #19: Display Controller Initialization Fails in Editor Terminals ✅ RESOLVED
**Severity**: LOW  
**Discovered**: 2025-12-21 (Session 54 - adaptive terminal testing)  
**Resolved**: 2026-01-01 (Session 98)  
**Status**: ✅ RESOLVED - Error downgraded to debug-only  
**Component**: src/display/display_controller.c  

**Description**:
When lusush runs in editor terminals (Zed, VS Code) that have non-TTY stdin but capable stdout, the display controller initialization fails with error 3:

```
[DC_ERROR] display_controller_init:1199: Failed to initialize base terminal (error 3) - using defaults
```

The shell still functions correctly because it falls back to defaults.

**Resolution**:
The error message was downgraded from `DC_ERROR` to `DC_DEBUG` since:
1. The failure is non-fatal - shell continues with sensible defaults
2. This is expected behavior when stdin is a pipe with `-i` flag
3. The error message was not actionable for users

The message now only appears when `LUSUSH_DISPLAY_DEBUG=1` is set.

**Files Modified**:
- `src/display/display_controller.c` - Changed DC_ERROR to DC_DEBUG for base terminal init failure

**Status**: ✅ RESOLVED

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

### Issue #14: Git-Aware Prompt Not Displaying Git Information ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-12-02 (macOS LLE compatibility session)  
**Fixed**: 2026-01-01 (Session 94 verification)  
**Status**: ✅ FIXED - Superseded by LLE prompt system with async support  
**Component**: src/lle/prompt/segment.c, src/lle/core/async_worker.c  

**Description**:
The git-aware themed prompt was not displaying git information due to a flawed `run_command()` implementation in the old `src/prompt.c`.

**Resolution**:
The old `src/prompt.c` git implementation was replaced by the LLE Spec 25 prompt system. The new implementation in `src/lle/prompt/segment.c` includes **full async support**:

**Async Architecture**:
1. On render, if cache invalid, queue async request via `lle_async_worker_t`
2. Return cached/stale data immediately for non-blocking render
3. When async completes, callback updates state and invalidates prompt
4. Next render uses fresh data

**Key Features**:
- `lle_async_worker_t` runs git commands in background thread
- `pthread_mutex_t` protects state for thread safety
- Non-blocking prompt rendering (never waits for git)
- Proper `popen()` capture with stderr redirection
- Handles detached HEAD state (falls back to short commit hash)
- Tracks ahead/behind counts, stash count, conflicts

**Key Implementation** (`src/lle/prompt/segment.c`):
```c
typedef struct {
    char branch[256];
    int staged, unstaged, untracked, ahead, behind, stash_count;
    bool has_conflicts, is_repo, cache_valid;
    lle_async_worker_t *async_worker;
    pthread_mutex_t async_mutex;
    bool async_pending;
} segment_git_state_t;
```

**Verification**:
- Git branch displays correctly in prompt when in git repository
- No git info shown when outside git repository (correct behavior)
- Status indicators work for modified/staged files
- Prompt never blocks waiting for slow git operations

**Status**: ✅ FIXED

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

### Issue #7: Completion Menu - Category Disambiguation Not Implemented ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-11-22 (Session 23 Part 2)  
**Fixed**: 2026-01-01 (Session 100)  
**Component**: Completion system / source manager  

**Description**:
When the same command exists in multiple categories (e.g., builtin `echo` vs external `/usr/bin/echo`), users can now see and select both versions from the completion menu.

**Solution Implemented** (Session 100):

1. **Type-aware deduplication** (`src/lle/completion/completion_system.c`):
   - `deduplicate_results()` now compares both text AND type
   - Items with the same text but different types are kept separate

2. **Store full path for shadowing commands** (`src/lle/completion/completion_sources.c`):
   - External commands that shadow builtins/aliases store full path in `description` field
   - Uses `lle_shell_is_builtin()` and `lle_shell_is_alias()` helpers

3. **Smart insertion** (`src/lle/keybinding/keybinding_actions.c`):
   - When inserting external commands that shadow builtins, use full path from description
   - Builtin selected → insert "echo" (executes builtin)
   - External selected → insert "/bin/echo" (full path bypasses builtin)

4. **New helper function**:
   - Added `lle_completion_result_add_with_description()` for adding completions with metadata

**Current Behavior**:
```
$ echo<TAB>
completing builtin command
echo
completing external command
echo

# Selecting builtin → inserts "echo"
# Selecting external → inserts "/bin/echo"
```

**Future Enhancement**: Add `command` builtin (like zsh) for explicit external command execution:
```bash
command echo  # Forces external command, bypasses builtins/aliases
```

**Status**: ✅ RESOLVED

---

### Issue #8: Completion Menu Single-Column Display (Inefficient Space Usage) ✅ FIXED
**Severity**: LOW  
**Discovered**: 2025-11-22 (Session 23 Part 2)  
**Fixed**: 2026-01-01 (Session 94 verification)  
**Status**: ✅ FIXED - Multi-column display implemented  
**Component**: src/lle/completion/completion_menu_renderer.c  

**Description**:
Completion menu was displaying one item per line instead of utilizing terminal width efficiently.

**Resolution**:
Multi-column display is now fully implemented in the LLE completion system:

**Implementation** (`src/lle/completion/completion_menu_renderer.c`):
```c
.use_multi_column = true,  // Enabled by default
```

**Key Features**:
- `completion_menu_state.c` calculates optimal column count based on terminal width
- Maximum 6 columns to prevent overcrowding
- Cached layout (`column_width`, `num_columns`) prevents recalculation flicker
- Category-aware navigation preserves column position when moving between sections

**Code Path**:
- `lle_completion_menu_update_layout()` calculates columns from terminal width
- `lle_completion_menu_render()` uses cached `state->column_width` and `state->num_columns`
- Layout recalculates on terminal resize events

**Status**: ✅ FIXED

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
**Fixed**: 2026-01-01 (Session 94 verification)  
**Status**: ✅ FIXED - LLE syntax highlighting processes full buffer  
**Component**: src/lle/display/syntax_highlighting.c  

**Description**:
In multiline input, builtin commands were not getting syntax highlighting.

**Resolution**:
The LLE syntax highlighting system processes the entire buffer, not line-by-line. The `is_builtin()` check from `builtins.h` is used to identify builtin commands regardless of their position in the buffer.

**Key Implementation** (`src/lle/display/syntax_highlighting.c`):
```c
if (is_builtin(command)) {
    type = LLE_TOKEN_COMMAND_BUILTIN;
}
// ...
case LLE_TOKEN_COMMAND_BUILTIN:
    tok->color = c->command_builtin;  // Cyan color
```

**Verification**:
- Builtins like `echo`, `cd`, `export` are highlighted in multiline constructs
- Token type `LLE_TOKEN_COMMAND_BUILTIN` applied correctly
- Full buffer tokenization ensures consistent highlighting

**Status**: ✅ FIXED

---

### Issue #6: Continuation Prompt Incorrectly Highlighted in Quotes ✅ FIXED
**Severity**: LOW  
**Discovered**: 2025-11-16 (Session 18+)  
**Fixed**: 2026-01-01 (Session 94 verification)  
**Status**: ✅ FIXED - LLE prompt system separates prompt from content  
**Component**: src/lle/prompt/, src/lle/lle_shell_integration.c  

**Description**:
Continuation prompt was being highlighted with quote color when inside multiline quoted strings.

**Resolution**:
The LLE prompt system (Spec 25/26) generates prompts separately from command content:
- Continuation prompt (`PS2`) is set via `symtable_set_global("PS2", "> ")`
- Prompt rendering goes through the LLE prompt composer, not through syntax highlighting
- Command content is tokenized and highlighted separately from prompt display

**Architecture**:
1. Prompt layer renders `PS2` with theme colors (not syntax highlighting colors)
2. Buffer content is tokenized separately by syntax highlighter
3. Display controller composites prompt + highlighted content
4. No overlap between prompt rendering and content highlighting

**Status**: ✅ FIXED

---

---

## Resolved Issues

### Issue #30: macOS Memory Leaks Not Detected by Valgrind ✅ FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-02 (Session 102 - macOS `leaks` tool analysis)  
**Fixed**: 2026-01-02 (Session 102)  
**Platform**: macOS (Linux valgrind showed 0 bytes definitely lost)  
**Component**: Multiple - editor cleanup, display integration, error handling, memory pool  

**Description**:
While Linux valgrind reported 0 bytes "definitely lost", macOS `leaks` tool reported **1,585 leaks totaling ~93KB**. The discrepancy exists because:
- Valgrind ignores "still reachable" memory (global pointers exist at exit)
- macOS `leaks` reports ALL unfreed memory, even if reachable via global pointers

This caused memory to accumulate during shell sessions, particularly affecting:
- History navigation state
- Display system terminals
- Error contexts
- Memory pool malloc fallbacks

**Leak Sources Identified and Fixed**:

1. **Pool Malloc Fallback Tracking** (Major - caused bulk of leaks)
   - When `lusush_pool_alloc()` exhausted pool memory, it fell back to `malloc()`
   - These malloc allocations were never tracked or freed
   - **Fix**: Added malloc fallback tracking system with `track_malloc_fallback()`, `untrack_malloc_fallback()`, and `free_all_malloc_fallbacks()` called during pool shutdown
   - **File**: `src/lusush_memory_pool.c`

2. **History System Not Destroyed** (`lle_history_core_destroy`)
   - `lle_editor_destroy()` never called `lle_history_core_destroy()`
   - Entire history hash table and entries leaked
   - **Fix**: Added `#include "lle/history.h"` and destruction call in `lle_editor_destroy()`
   - **File**: `src/lle/lle_editor.c`

3. **Keybinding Manager Not Destroyed** (`lle_keybinding_manager_destroy`)
   - Editor cleanup didn't destroy the keybinding manager
   - **Fix**: Added `#include "lle/keybinding.h"` and destruction call
   - **File**: `src/lle/lle_editor.c`

4. **History Navigation Buffer Leak** (`history_nav_seen_hashes`)
   - `editor->history_nav_seen_hashes` array allocated in `lle_history_previous()` never freed
   - **Fix**: Added cleanup in `lle_editor_destroy()`:
     ```c
     if (editor->history_nav_seen_hashes) {
         free(editor->history_nav_seen_hashes);
         editor->history_nav_seen_hashes = NULL;
     }
     ```
   - **File**: `src/lle/lle_editor.c`

5. **Base Terminal Not Destroyed** (`base_terminal_destroy`)
   - `terminal_control_destroy()` didn't destroy its owned `base_terminal`
   - **Fix**: Added `base_terminal_destroy()` call in cleanup
   - **File**: `src/display/terminal_control.c`

6. **Error Context Leak** (`lle_create_error_context`)
   - Error contexts allocated with `malloc()` via `lle_error_pool_alloc()`
   - Display bridge cleanup used wrong free function (`lle_pool_free()`)
   - **Fix**: 
     - Implemented `lle_error_context_destroy()` function
     - Added declaration to `include/lle/error_handling.h`
     - Updated `lle_display_bridge_cleanup()` to use proper destructor
   - **Files**: `src/lle/core/error_handling.c`, `include/lle/error_handling.h`, `src/lle/display/display_bridge.c`

7. **Display Integration Not Cleaned Up**
   - Global `lle_display_integration_t` singleton created in `lle_readline()` never destroyed
   - **Fix**: Added cleanup call in `lle_shell_integration_shutdown()`:
     ```c
     lle_display_integration_t *display_integ = lle_display_integration_get_global();
     if (display_integ) {
         lle_display_integration_cleanup(display_integ);
     }
     ```
   - **File**: `src/lle/lle_shell_integration.c`

8. **symtable_get_var strdup Leaks**
   - `symtable_get_global()` returns `strdup()`'d values that callers must free
   - PS1 retrieval in `lle_shell_integration.c` and HOME in `init.c` weren't freed
   - **Fix**: Track allocated strings and free after use
   - **Files**: `src/lle/lle_shell_integration.c`, `src/init.c`

9. **Execute Command Early Return Leak**
   - Early return in `execute_command()` at redirection setup failure leaked `argv` and `filtered_argv`
   - **Fix**: Added cleanup before early return
   - **File**: `src/executor.c`

**Progress Tracking**:
| Stage | Leaks | Bytes | Reduction |
|-------|-------|-------|-----------|
| Initial | 1,585 | 93KB | - |
| After pool/history/keybinding fixes | 401 | 20KB | 79% |
| After symtable/error context fixes | 84 | 4KB | 96% |
| After remaining fixes | 4 | 784 bytes | 99% |
| Final (display integration cleanup) | 0 | 0 bytes | 100% |

**Verification**:
```
leaks Report Version: 4.0, multi-line stacks
Process 15826: 702 nodes malloced for 593 KB
Process 15826: 0 leaks for 0 total leaked bytes.
```

**Files Modified**:
- `src/lusush_memory_pool.c` - Malloc fallback tracking
- `src/lle/lle_editor.c` - History, keybinding, navigation buffer cleanup
- `src/display/terminal_control.c` - Base terminal destruction
- `src/lle/core/error_handling.c` - Implemented `lle_error_context_destroy()`
- `include/lle/error_handling.h` - Added destroy function declaration
- `src/lle/display/display_bridge.c` - Use proper error context destructor
- `src/lle/lle_shell_integration.c` - Display integration cleanup
- `src/init.c` - Free strdup'd HOME value
- `src/executor.c` - Fix early return leak

**Impact**:
- Shell can now run indefinitely on macOS without memory growth
- Clean shutdown with 0 leaks reported by macOS `leaks` tool
- Proper cleanup chain: shell integration → display integration → display bridge → error context

**Status**: ✅ FIXED AND VERIFIED (0 leaks, 0 bytes)

---

### Issue #29: Critical Memory Leaks (~79KB per session) ✅ FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-02 (Session 101 - Valgrind analysis)  
**Fixed**: 2026-01-02 (Session 101)  
**Component**: Multiple - parser.c, autocorrect.c, lusush.c, completion system  

**Description**:
Valgrind reported ~79KB of memory "definitely lost" during normal shell operation. The leaks accumulated over time, particularly with completion usage, causing unbounded memory growth during extended shell sessions.

**Leak Sources Identified**:

1. **Completion System (62KB + 11KB)** - Fixed by previous session work
   - `lle_completion_item_create_with_description` allocated items that weren't freed
   - `context.word` from `lle_completion_analyze_context` wasn't being freed by callers
   - Fixed by adding `lle_completion_system_clear()` at end of readline and proper cleanup

2. **Parser AST Nodes (11 bytes across multiple locations)** - `val_type` not set
   - When `node->val.str` was set via `strdup()`, `val_type` wasn't set to `VAL_STR`
   - `free_node_tree()` only frees `val.str` when `val_type == VAL_STR`
   - **15 locations fixed** in `src/parser.c`:
     - Line 557: command assignment with value
     - Line 568: command assignment empty value
     - Line 595: command name
     - Line 727: argument node single token
     - Line 748: argument node concatenated
     - Line 942: redirection operator
     - Line 1005: heredoc delimiter
     - Line 1015: heredoc content
     - Line 1025: heredoc expand flag
     - Line 1084: redirection target
     - Line 1591: for loop variable
     - Line 1628: for-in "=" word
     - Line 1732: for-in combined word
     - Line 1803: case test word
     - Line 1921: case item pattern
     - Line 2105: function name

3. **Autocorrect Suggestions (5 bytes each)** - temp suggestions not freed
   - `autocorrect_find_suggestions()` collects suggestions in `temp_suggestions[]`
   - Only suggestions meeting threshold are copied to results
   - Uncopied suggestions' `command` pointers were leaked
   - **Fix**: Free any `temp_suggestions[i].command` not transferred to results

4. **Input Buffer (10-12 bytes)** - line not freed in non-interactive mode
   - `get_unified_input()` returns allocated buffer
   - In non-interactive mode, only `free_input_buffers()` was called (cleans global state)
   - The returned `line` buffer was not freed
   - **Fix**: Always `free(line)`, then call `free_input_buffers()` for non-interactive

**Files Modified**:
- `src/parser.c` - Added `val_type = VAL_STR` in 15 locations
- `src/autocorrect.c` - Free unused temp_suggestions after copy loop
- `src/lusush.c` - Always free(line) in main loop

**Verification**:
```
Before fix:
==1486595== definitely lost: 79,000+ bytes in 100+ blocks

After fix:
==1489298== definitely lost: 0 bytes in 0 blocks
==1489298== still reachable: 294 bytes in 6 blocks  (expected - global state)
```

**Impact**:
- Shell can now run indefinitely without memory growth
- Tab completion no longer leaks 62KB+ per usage
- Parser no longer leaks AST node strings

**Status**: ✅ FIXED AND VERIFIED

---

### Issue #28: POSIX Variable Scoping Regression ✅ FIXED
**Severity**: HIGH  
**Discovered**: 2026-01-01 (Session 98 - Compliance test analysis)  
**Fixed**: 2026-01-01 (Session 98)  
**Component**: src/executor.c  

**Description**:
Variable assignments inside functions and loops were incorrectly using local scope instead of global scope. This violated POSIX shell semantics where variables are global by default, and only the explicit `local` builtin creates local variables.

**Failing Tests** (before fix):
- Test 98: `func() { var=inside; }; func; echo $var` - Expected "inside", got empty
- Test 134: `for i in 1 2 3 4 5; do eval "var$i=value$i"; done; echo $var1$var3$var5` - Expected "value1value3value5", got empty

**Root Cause**:
In a previous development session, the variable assignment logic was accidentally changed to use `symtable_set_local_var()` when inside a function or loop scope, rather than `symtable_set_global_var()`. This caused:
1. Variables assigned inside functions to disappear after function returns
2. Variables assigned inside loops (including via `eval`) to disappear after loop ends

**POSIX Behavior** (correct):
- Variable assignments are GLOBAL by default
- Only the explicit `local` builtin creates function-local variables
- Loop variables persist after loop completion
- `eval` assignments go to global scope

**Fix Applied**:
Modified `src/executor.c` to always use global scope for regular variable assignments:

1. **execute_assignment()** (~line 3258):
   ```c
   // POSIX compliance: variable assignments are GLOBAL by default
   // Local variables are only created via explicit 'local' builtin
   int result;
   result = symtable_set_global_var(executor->symtable, var_name,
                                    value ? value : "");
   ```

2. **execute_for()** (~line 1543):
   ```c
   // Set loop variable in global scope (POSIX compliance)
   if (symtable_set_global_var(executor->symtable, var_name,
                               expanded_words[i]) != 0) {
   ```

**Verification**:
- ✅ Test 98 passes: Function variable assignments persist after function returns
- ✅ Test 134 passes: Loop + eval variable assignments persist after loop ends
- ✅ All 136 compliance tests pass (100% score)
- ✅ All 54 meson unit tests pass
- ✅ `local` builtin still creates function-local variables (separate code path)

**Files Modified**:
- `src/executor.c` - Changed `symtable_set_local_var()` to `symtable_set_global_var()` in `execute_assignment()` and `execute_for()`

**Status**: ✅ FIXED

---

### Issue #27: Display Stress Test Memory Leak (macOS-only) ✅ FIXED
**Severity**: MEDIUM  
**Discovered**: 2025-12-30 (Session 83 - macOS clean build verification)  
**Fixed**: 2025-12-30 (Session 83)  
**Component**: src/lusush_memory_pool.c  

**Description**:
The LLE Display Stress Test was failing on macOS with a memory leak detection error. The test reported ~8MB of memory growth and the macOS `leaks` tool identified 38,650 leaks totaling ~17MB.

**Root Cause**:
When `lusush_pool_alloc()` is called without an initialized global memory pool, it falls back to `malloc()`. However, `lusush_pool_free()` had a bug where if `global_memory_pool` was NULL, it would return early **without calling `free()`** on the malloc'd memory.

The logic was intended to prevent double-frees after pool shutdown, but it didn't distinguish between:
1. Pool was never initialized → memory came from malloc fallback → must `free()`
2. Pool was shut down → memory was already freed during shutdown → must NOT `free()`

**Fix Applied**:
Added a `pool_was_ever_initialized` static flag to track whether the pool was ever initialized:

```c
// Track whether pool was ever initialized (vs never used)
static bool pool_was_ever_initialized = false;

// Set in lusush_pool_init():
pool_was_ever_initialized = true;

// Used in lusush_pool_free():
if (!global_memory_pool) {
    if (pool_was_ever_initialized) {
        // Pool was shut down - don't free (would double-free)
        return;
    } else {
        // Pool never initialized - memory came from malloc
        free(ptr);
        return;
    }
}
```

**Verification**:
- ✅ macOS `leaks` tool reports 0 leaks, 0 bytes leaked
- ✅ Display Stress Test passes (all 6 sub-tests)
- ✅ Full test suite passes (58/58 tests)
- ✅ Memory delta in stress test dropped from 8396 KB to 0 KB

**Files Modified**:
- `src/lusush_memory_pool.c` - Added `pool_was_ever_initialized` flag and fixed free logic

**Status**: ✅ FIXED

---

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

**Last Updated**: 2026-01-01 (Session 98)  
**Next Review**: Before each commit, after each bug discovery  
**Maintainer**: Update this file whenever bugs are discovered - NO EXCEPTIONS

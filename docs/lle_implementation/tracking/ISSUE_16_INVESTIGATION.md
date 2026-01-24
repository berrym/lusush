# Issue #16 Investigation Report: Stale Git Prompt Root Cause Analysis

**Date**: 2025-12-25  
**Session**: 66  
**Investigators**: AI Assistant + User collaborative investigation  
**Status**: Root cause identified, architectural fix required

---

## Executive Summary

Issue #16 (prompt/cursor desync display corruption) has been thoroughly investigated. The root cause is a **timing-based race condition** caused by independent cache layers with different TTLs and invalidation logic. The git prompt information cache does NOT invalidate when the working directory changes, causing stale data to persist until the cache expires by time.

This is not a missing function call or simple bug - it's a **fundamental architectural disconnect** between how caches are managed across the prompt/theme system.

---

## The Four Independent Cache Layers

The prompt system has FOUR caches that operate independently:

| Cache | Location | TTL | Invalidation Trigger | Directory Aware? |
|-------|----------|-----|---------------------|------------------|
| **Git Info** | `src/prompt.c` | **10 seconds** | Time-based only | **NO** |
| Prompt Content | `src/prompt.c` | 5 seconds | Directory change | YES |
| Prompt Layer | `src/display/prompt_layer.c` | 5 seconds | Content hash, theme | NO |
| Theme | `src/themes.c` | 30 seconds | Theme change | NO |

**The Problem**: Only the Prompt Content cache checks working directory. The Git Info cache uses absolute time (`now - last_git_check < 10s`), so it returns stale data even after `cd` changes directories.

---

## Detailed Cache Analysis

### 1. Git Info Cache (`src/prompt.c`)

```c
static git_info_t git_info = {0};
static time_t last_git_check = 0;
static const int GIT_CACHE_SECONDS = 10;

void update_git_info(void) {
    time_t now = time(NULL);
    
    // PROBLEM: Returns cached info if < 10 seconds old
    // Does NOT check if directory changed!
    if (now - last_git_check < GIT_CACHE_SECONDS) {
        return;  // STALE DATA RETURNED
    }
    
    get_git_status(&git_info);
    last_git_check = now;
}
```

**Issue**: This cache is purely time-based. It has no awareness of directory changes. If you `cd` to a new directory within 10 seconds of the last git check, you get stale git info from the previous directory.

### 2. Prompt Content Cache (`src/prompt.c`)

```c
static bool cache_valid = false;
static time_t cache_time = 0;
static char cached_working_dir[256] = {0};
static const int CACHE_VALIDITY_SECONDS = 5;

void build_prompt(void) {
    time_t now = time(NULL);
    char current_dir[256];
    
    if (cache_valid && (now - cache_time <= CACHE_VALIDITY_SECONDS)) {
        if (getcwd(current_dir, sizeof(current_dir)) != NULL &&
            strcmp(current_dir, cached_working_dir) == 0) {
            // Cache hit - directory matches
            symtable_set_global("PS1", cached_prompt);
            return;
        }
    }
    
    // Cache miss - regenerate prompt
    // BUT: calls update_git_info() which may return stale data!
    theme_generate_primary_prompt(prompt, sizeof(prompt));
    // ...
}
```

**Correct Behavior**: This cache properly invalidates when directory changes.

**However**: When it regenerates the prompt, it calls `update_git_info()` which may return stale git data (see above).

### 3. Prompt Layer Cache (`src/display/prompt_layer.c`)

```c
#define PROMPT_LAYER_CACHE_SIZE 8
#define PROMPT_LAYER_CACHE_EXPIRY_MS 5000  // 5 seconds

static prompt_cache_entry_t *find_cache_entry(prompt_layer_t *layer,
                                              const char *content,
                                              const char *theme_name) {
    // Checks content hash and TTL
    // NO directory awareness
}
```

**Issue**: Caches based on content hash and theme, not working directory.

### 4. Theme Cache (`src/themes.c`)

```c
static theme_cache_entry_t theme_cache = {0};
static const int THEME_CACHE_VALIDITY_SECONDS = 30;
```

**Issue**: 30-second TTL, no directory awareness.

---

## The Race Condition Timeline

This is what happens when a user types `cd /tmp` from a git repository:

```
Time 0s:  User in /path/to/git_repo
          - Previous command completed
          - git_info contains branch "feature/lle", dirty status "*"
          - last_git_check = 0s
          - cached_working_dir = "/path/to/git_repo"

Time 0s:  Readline starts, needs prompt
          - lush_readline_with_prompt() called
          - lush_generate_prompt() called
          - build_prompt() called
          - getcwd() returns "/path/to/git_repo"
          - Cache hit (same dir, recent cache)
          - Prompt displays: "[user@host] /path/to/git_repo (feature/lle *) $ "
          
Time 1s:  User types: cd /tmp
          - Prompt is still showing git info (correct for current dir)

Time 2s:  User presses ENTER
          - Command executes
          - bin_cd() runs chdir("/tmp")
          - PWD updated to "/tmp"
          - NO CACHE INVALIDATION
          - display_integration_post_command_update() called
          - Does NOT invalidate any caches

Time 3s:  Next readline starts, needs prompt
          - lush_readline_with_prompt() called
          - lush_generate_prompt() called
          - build_prompt() called
          - getcwd() returns "/tmp" (NEW directory)
          - Compare: "/tmp" != "/path/to/git_repo"
          - CACHE MISS (directory changed) - CORRECT!
          - Calls theme_generate_primary_prompt()
          - Theme calls update_git_info()
          
          NOW THE BUG:
          - update_git_info() checks: now - last_git_check
          - 3s - 0s = 3s < 10s
          - Returns WITHOUT updating git_info!
          - git_info still contains "feature/lle *" from old repo
          
          - format_git_prompt() formats stale git_info
          - Prompt generated: "[user@host] /tmp (feature/lle *) $ "
          - BUT /tmp IS NOT A GIT REPO!

Time 3s:  Stale prompt displayed
          - User sees git branch in non-git directory
          - Prompt width calculated based on stale content
          - LLE cursor positioning uses wrong width
          - Display corruption occurs
```

---

## Why Previous Fix Attempts Failed

### Session 66 Attempt 1: `command_layer_clear()` in `dc_reset_prompt_display_state()`

**What it did**: Cleared command layer content when display state was reset.

**Why it failed**: The command layer was empty - the problem was the PROMPT layer had stale content. Clearing an empty command layer did nothing.

### Session 66 Attempt 2: `prompt_layer_force_render()` 

**What it did**: Forced prompt layer to re-render.

**Why it failed**: The prompt layer re-rendered... but with the same stale git info from `update_git_info()`. Force-rendering doesn't fix stale source data.

### Session 66 Attempt 3: Various display layer changes

**Why they all failed**: All these fixes targeted the DISPLAY side. The problem is on the DATA side - the git info cache returns stale data.

**Lesson**: You can clear and re-render display layers all day, but if the source data (`git_info`) is stale, you'll just re-render stale content.

---

## The Missing Integration Point

There is NO post-command hook that invalidates caches based on what command was executed.

### Current Flow (Broken)

```
bin_cd()
    └── chdir("/tmp")
    └── Update PWD variable
    └── Return success
    └── [NO cache invalidation]

display_integration_post_command_update()
    └── Analyze executed command (for display purposes)
    └── [Does NOT call prompt_cache_invalidate()]
    └── [Does NOT call update_git_info() with force flag]
    └── Return

Next readline session
    └── build_prompt()
        └── getcwd() → "/tmp" (new dir)
        └── Cache miss (dir changed) ✓ CORRECT
        └── theme_generate_primary_prompt()
            └── update_git_info()
                └── 3s < 10s → return cached ✗ STALE!
```

### Required Flow (Correct)

```
bin_cd()
    └── chdir("/tmp")
    └── Update PWD variable
    └── [OPTION A: Emit PWD_CHANGED event]
    └── Return success

[OPTION A: Event subscriber]
on PWD_CHANGED:
    └── prompt_cache_invalidate()
    └── force_git_info_update()

[OPTION B: Post-command hook]
display_integration_post_command_update()
    └── Detect directory-changing command
    └── prompt_cache_invalidate()
    └── force_git_info_update()

Next readline session
    └── build_prompt()
        └── getcwd() → "/tmp"
        └── [Cache invalid or dir changed]
        └── theme_generate_primary_prompt()
            └── update_git_info()
                └── [Forced update OR cache invalid]
                └── get_git_status() → no git repo
                └── git_info cleared
```

---

## Solution Options

### Option 1: Quick Fix - Force Git Update After CD (Least Invasive)

**Location**: `src/display_integration.c`

```c
void display_integration_post_command_update(const char *executed_command) {
    // ... existing code ...
    
    // Detect directory-changing commands
    if (executed_command && strlen(executed_command) > 0) {
        const char *dir_cmds[] = {"cd", "pushd", "popd", NULL};
        for (int i = 0; dir_cmds[i]; i++) {
            if (strstr(executed_command, dir_cmds[i]) != NULL) {
                // Force git info refresh on next prompt
                extern void force_git_info_update(void);
                force_git_info_update();
                break;
            }
        }
    }
}
```

**Pros**: Minimal changes, targets specific issue
**Cons**: String matching is fragile, doesn't handle all cases (e.g., symlinks, `source` that cd's)

### Option 2: Better Fix - Directory Change Detection at Readline Entry

**Location**: `src/readline_integration.c`

```c
char *lush_readline_with_prompt(const char *prompt) {
    static char last_prompt_dir[PATH_MAX] = {0};
    char current_dir[PATH_MAX];
    
    if (getcwd(current_dir, sizeof(current_dir))) {
        if (last_prompt_dir[0] == '\0' || 
            strcmp(current_dir, last_prompt_dir) != 0) {
            // Directory changed since last prompt!
            prompt_cache_invalidate();
            force_git_info_update();
            strncpy(last_prompt_dir, current_dir, sizeof(last_prompt_dir) - 1);
        }
    }
    
    // ... rest of function ...
}
```

**Pros**: Catches ALL directory changes regardless of how they happened
**Cons**: Adds state tracking to readline, still a band-aid on fragmented architecture

### Option 3: Best Fix - Event-Driven Cache Invalidation (Architectural)

**New Event**: `PWD_CHANGED`

**Publisher**: `bin_cd()` after successful `chdir()`

**Subscribers**:
- Prompt content cache
- Git info cache  
- Prompt layer cache
- Theme cache (if needed)

```c
// In bin_cd() after chdir():
lle_event_publish(LLE_EVENT_PWD_CHANGED, &(lle_pwd_changed_event_t){
    .old_path = old_pwd,
    .new_path = new_pwd
});

// In prompt.c:
static void on_pwd_changed(const lle_event_t *event, void *user_data) {
    prompt_cache_invalidate();
    force_git_info_update();
}
```

**Pros**: Clean architecture, handles all cases, extensible
**Cons**: Requires more work, crosses module boundaries, may need LLE integration

### Option 4: Move Prompt/Theme into LLE (Recommended Long-Term)

If prompt/theme becomes part of LLE:
- Uses LLE's existing event system
- Single cache management
- Unified state
- Resolves Issues #16, #20, #21, #22 together

This is a larger undertaking but addresses the fundamental architectural issues.

---

## Related Issues

| Issue | Relationship to #16 |
|-------|---------------------|
| #20 | Theme overwrites PS1/PS2 - same architectural problem |
| #21 | Themes not user-extensible - limits fix options |
| #22 | Template variables dead code - blocked by same issues |
| #23 | Extra space after prompt - may be symptom of width mismatch |

---

## Recommendations

1. **Do NOT apply quick fixes** - Session 66 showed that quick fixes cause cascading issues

2. **Proceed with architectural discussion** - The prompt/theme system needs redesign

3. **Consider LLE integration** - Moving prompt/theme into LLE would solve multiple issues

4. **If temporary fix needed**: Option 2 (directory change detection at readline entry) is safest

---

## Files Involved in Current Architecture

| File | Role | Cache? |
|------|------|--------|
| `src/prompt.c` | Prompt generation, git info | 2 caches (git: 10s, prompt: 5s) |
| `src/themes.c` | Theme templates, variables | 1 cache (30s) |
| `src/display/prompt_layer.c` | Prompt display, caching | 8-entry cache (5s) |
| `src/display/display_controller.c` | Display coordination | No cache, uses others |
| `src/display/composition_engine.c` | Layer composition | No cache |
| `src/readline_integration.c` | Readline wrapper | Generates prompt |
| `src/display_integration.c` | Post-command updates | No cache invalidation |
| `src/builtins/builtins.c` | `bin_cd()` implementation | No event emission |

---

## Conclusion

Issue #16 is caused by the git info cache using time-based expiry (10 seconds) without checking if the working directory changed. The prompt content cache correctly invalidates on directory change, but then calls `update_git_info()` which returns stale data.

The fix requires either:
1. Adding directory-change awareness to the git info cache
2. Event-driven invalidation across all caches
3. Architectural redesign of prompt/theme into LLE

Given the existing Issues #20 and #21 (also prompt/theme architectural problems), the recommendation is to proceed with architectural redesign rather than patching the current system.

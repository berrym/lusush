# LLE History Configuration Specification

**Document**: LLE_HISTORY_CONFIG_SPECIFICATION.md  
**Date**: 2025-11-07  
**Purpose**: Define configuration options for LLE history system integration  
**Status**: Specification for Implementation

---

## Executive Summary

This document defines configuration options for LLE history system behavior, integrated with Lusush's existing config system (`config.h`/`config.c`). The configuration allows users to choose between traditional GNU Readline behavior and LLE's advanced multiline capabilities.

**Key Principle:** **Advanced features by default, but fully configurable for user preference and GNU Readline compatibility.**

---

## 1. Configuration Architecture

### 1.1 Integration with Existing Config System

All LLE history options will be added to the existing `CONFIG_SECTION_HISTORY` section in `config.h`:

```c
typedef struct {
    // Existing history settings
    bool history_enabled;
    int history_size;
    bool history_no_dups;
    bool history_timestamps;
    char *history_file;
    
    // NEW: LLE History Settings
    
    // Arrow key behavior
    lle_arrow_key_mode_t lle_arrow_key_mode;     // Context-aware, classic, or always-history
    bool lle_enable_multiline_navigation;         // Allow vertical cursor movement in multiline
    
    // History navigation
    bool lle_wrap_history_navigation;             // Wrap at beginning/end of history
    bool lle_save_line_on_history_nav;           // Save current line before navigating
    
    // Multiline support
    bool lle_preserve_multiline_structure;        // Preserve original multiline formatting
    bool lle_enable_multiline_editing;           // Enable multiline command editing
    bool lle_show_multiline_indicators;          // Show visual indicators for multiline
    
    // Search behavior
    bool lle_enable_interactive_search;          // Enable Ctrl-R search
    bool lle_search_fuzzy_matching;              // Use fuzzy matching in search
    bool lle_search_case_sensitive;              // Case-sensitive search
    
    // File format and storage
    lle_history_storage_mode_t lle_storage_mode; // LLE, bash, or dual mode
    char *lle_history_file;                      // LLE history file path (.lle_history)
    bool lle_sync_with_readline;                 // Sync with GNU Readline history
    bool lle_export_to_bash_history;             // Export to bash_history format
    
    // Advanced features
    bool lle_enable_forensic_tracking;           // Enable forensic metadata
    bool lle_enable_deduplication;               // Smart duplicate detection
    int lle_dedup_scope;                         // Dedup scope (session, global, etc.)
    
    // Performance
    bool lle_enable_history_cache;               // Enable LRU cache
    int lle_cache_size;                          // Cache size in entries
    
    // Compatibility mode
    bool lle_readline_compatible_mode;           // Emulate GNU Readline exactly
    
} config_values_t;
```

### 1.2 Configuration Enumerations

```c
/**
 * Arrow key behavior modes
 */
typedef enum {
    LLE_ARROW_MODE_CONTEXT_AWARE,    // Smart: multiline navigation when in multiline, history when at boundaries
    LLE_ARROW_MODE_CLASSIC,          // GNU Readline: always history navigation
    LLE_ARROW_MODE_ALWAYS_HISTORY,   // Always history, use Ctrl-P/N only
    LLE_ARROW_MODE_MULTILINE_FIRST   // Prioritize multiline navigation, use Ctrl-P/N for history
} lle_arrow_key_mode_t;

/**
 * History storage modes
 */
typedef enum {
    LLE_STORAGE_MODE_LLE_ONLY,       // Store only in LLE format (.lle_history)
    LLE_STORAGE_MODE_BASH_ONLY,      // Store only in bash format (.bash_history)
    LLE_STORAGE_MODE_DUAL,           // Store in both formats (recommended)
    LLE_STORAGE_MODE_READLINE_COMPAT // Use GNU Readline's storage only
} lle_history_storage_mode_t;

/**
 * Deduplication scope
 */
typedef enum {
    LLE_DEDUP_SCOPE_NONE,            // No deduplication
    LLE_DEDUP_SCOPE_SESSION,         // Within current session only
    LLE_DEDUP_SCOPE_RECENT,          // Last N entries (configurable)
    LLE_DEDUP_SCOPE_GLOBAL           // Entire history
} lle_dedup_scope_t;
```

---

## 2. Configuration Options

### 2.1 Arrow Key Behavior

#### `lle.arrow_key_mode`

**Type:** Enum (context-aware | classic | always-history | multiline-first)  
**Default:** `context-aware`  
**Description:** Controls how UP/DOWN arrow keys behave

**Values:**

1. **`context-aware`** (Default - Recommended):
   ```
   Single-line buffer:
     - UP/DOWN = history navigation
   
   Multiline buffer (top line):
     - UP = previous history
     - DOWN = move cursor down one line
   
   Multiline buffer (middle lines):
     - UP = move cursor up one line
     - DOWN = move cursor down one line
   
   Multiline buffer (bottom line):
     - UP = move cursor up one line
     - DOWN = next history
   
   Always available:
     - Ctrl-P = previous history (always)
     - Ctrl-N = next history (always)
   ```

2. **`classic`** (GNU Readline compatible):
   ```
   All contexts:
     - UP = previous history
     - DOWN = next history
     - No vertical cursor movement in multiline
   ```

3. **`always-history`**:
   ```
   All contexts:
     - UP/DOWN = history navigation only
     - Use Ctrl-P/N for history (same as classic)
     - No multiline vertical navigation via arrows
   ```

4. **`multiline-first`**:
   ```
   Prioritize multiline navigation:
     - UP/DOWN = vertical cursor movement if multiline
     - Ctrl-P/N = history navigation (always)
     - Single-line: UP/DOWN = history
   ```

**Config File:**
```ini
[history]
lle.arrow_key_mode = context-aware
```

---

#### `lle.enable_multiline_navigation`

**Type:** Boolean  
**Default:** `true`  
**Description:** Enable vertical cursor movement within multiline commands

When `false`, multiline commands are treated as single logical lines (GNU Readline style).

**Config File:**
```ini
[history]
lle.enable_multiline_navigation = true
```

---

### 2.2 History Navigation Behavior

#### `lle.wrap_history_navigation`

**Type:** Boolean  
**Default:** `true`  
**Description:** Wrap around at beginning/end of history

When `true`:
- Pressing UP at oldest entry wraps to newest
- Pressing DOWN at newest entry wraps to oldest

When `false`:
- UP at oldest entry does nothing (GNU Readline style)
- DOWN at newest entry does nothing

**Config File:**
```ini
[history]
lle.wrap_history_navigation = true
```

---

#### `lle.save_line_on_history_nav`

**Type:** Boolean  
**Default:** `true`  
**Description:** Save current line content before navigating history

When `true`:
- First UP saves current line to temporary buffer
- Navigating back to "present" (DOWN past newest) restores saved line

When `false`:
- Current line is lost when navigating history (bash default)

**Config File:**
```ini
[history]
lle.save_line_on_history_nav = true
```

---

### 2.3 Multiline Support

#### `lle.preserve_multiline_structure`

**Type:** Boolean  
**Default:** `true`  
**Description:** Preserve original multiline formatting when recalling commands

When `true`:
- Multiline commands recalled with original indentation
- Shell constructs (loops, functions) maintain structure
- Uses `original_multiline` field from Spec 09

When `false`:
- Multiline commands normalized to single line

**Config File:**
```ini
[history]
lle.preserve_multiline_structure = true
```

---

#### `lle.enable_multiline_editing`

**Type:** Boolean  
**Default:** `true`  
**Description:** Enable interactive editing of recalled multiline commands

Requires `lle.preserve_multiline_structure = true`.

**Config File:**
```ini
[history]
lle.enable_multiline_editing = true
```

---

#### `lle.show_multiline_indicators`

**Type:** Boolean  
**Default:** `true`  
**Description:** Show visual indicators for multiline commands

When enabled, shows indicators like:
- Line numbers
- Continuation symbols
- Structure markers (for loops, functions)

**Config File:**
```ini
[history]
lle.show_multiline_indicators = true
```

---

### 2.4 Search Configuration

#### `lle.enable_interactive_search`

**Type:** Boolean  
**Default:** `true`  
**Description:** Enable Ctrl-R reverse incremental search

**Config File:**
```ini
[history]
lle.enable_interactive_search = true
```

---

#### `lle.search_fuzzy_matching`

**Type:** Boolean  
**Default:** `false`  
**Description:** Use fuzzy matching in history search

When `true`:
- Search matches similar commands (typo-tolerant)
- Uses Levenshtein distance or similar algorithm

When `false`:
- Exact substring matching only (GNU Readline style)

**Config File:**
```ini
[history]
lle.search_fuzzy_matching = false
```

---

#### `lle.search_case_sensitive`

**Type:** Boolean  
**Default:** `false`  
**Description:** Case-sensitive history search

**Config File:**
```ini
[history]
lle.search_case_sensitive = false
```

---

### 2.5 Storage and File Format

#### `lle.storage_mode`

**Type:** Enum (lle-only | bash-only | dual | readline-compat)  
**Default:** `dual`  
**Description:** History storage format mode

**Values:**

1. **`lle-only`**:
   - Store in `.lle_history` only
   - Rich metadata, multiline structure preserved
   - Not compatible with other shells

2. **`bash-only`**:
   - Store in `.bash_history` format
   - Plain text, compatible with bash
   - Loses multiline structure and metadata

3. **`dual`** (Recommended):
   - Store in `.lle_history` (primary)
   - Export to `.bash_history` (for compatibility)
   - Best of both worlds

4. **`readline-compat`**:
   - Use GNU Readline's history API directly
   - Maximum compatibility
   - No LLE advanced features

**Config File:**
```ini
[history]
lle.storage_mode = dual
```

---

#### `lle.history_file`

**Type:** String  
**Default:** `~/.lle_history`  
**Description:** Path to LLE history file

**Config File:**
```ini
[history]
lle.history_file = ~/.lle_history
```

---

#### `lle.sync_with_readline`

**Type:** Boolean  
**Default:** `true`  
**Description:** Sync history with GNU Readline in-memory history

When enabled, changes in LLE history are reflected in GNU Readline history (for seamless mode switching).

**Config File:**
```ini
[history]
lle.sync_with_readline = true
```

---

#### `lle.export_to_bash_history`

**Type:** Boolean  
**Default:** `true` (when `storage_mode = dual`)  
**Description:** Export commands to `.bash_history` format

Enables history sharing with bash and other shells.

**Config File:**
```ini
[history]
lle.export_to_bash_history = true
```

---

### 2.6 Advanced Features

#### `lle.enable_forensic_tracking`

**Type:** Boolean  
**Default:** `true`  
**Description:** Enable forensic metadata collection

Collects:
- Execution timestamps
- Exit codes
- Working directory
- Session ID
- Process metadata

**Config File:**
```ini
[history]
lle.enable_forensic_tracking = true
```

---

#### `lle.enable_deduplication`

**Type:** Boolean  
**Default:** `true`  
**Description:** Enable intelligent duplicate detection

**Config File:**
```ini
[history]
lle.enable_deduplication = true
```

---

#### `lle.dedup_scope`

**Type:** Enum (none | session | recent | global)  
**Default:** `recent`  
**Description:** Deduplication scope

**Values:**
- `none` - No deduplication
- `session` - Deduplicate within current session
- `recent` - Deduplicate last 100 entries (configurable)
- `global` - Deduplicate entire history

**Config File:**
```ini
[history]
lle.dedup_scope = recent
```

---

### 2.7 Performance Configuration

#### `lle.enable_history_cache`

**Type:** Boolean  
**Default:** `true`  
**Description:** Enable LRU cache for history operations

**Config File:**
```ini
[history]
lle.enable_history_cache = true
```

---

#### `lle.cache_size`

**Type:** Integer  
**Default:** `1000`  
**Description:** Number of entries to cache

**Config File:**
```ini
[history]
lle.cache_size = 1000
```

---

### 2.8 Compatibility Mode

#### `lle.readline_compatible_mode`

**Type:** Boolean  
**Default:** `false`  
**Description:** Emulate GNU Readline behavior exactly

When enabled:
- Sets `arrow_key_mode = classic`
- Disables multiline navigation
- Disables multiline structure preservation
- Uses bash-only storage mode
- Disables all advanced features

**Shortcut for maximum compatibility.**

**Config File:**
```ini
[history]
lle.readline_compatible_mode = false
```

---

## 3. Configuration Presets

### 3.1 Default Preset (Advanced LLE)

```ini
[history]
# Arrow key behavior
lle.arrow_key_mode = context-aware
lle.enable_multiline_navigation = true

# History navigation
lle.wrap_history_navigation = true
lle.save_line_on_history_nav = true

# Multiline support
lle.preserve_multiline_structure = true
lle.enable_multiline_editing = true
lle.show_multiline_indicators = true

# Search
lle.enable_interactive_search = true
lle.search_fuzzy_matching = false
lle.search_case_sensitive = false

# Storage
lle.storage_mode = dual
lle.history_file = ~/.lle_history
lle.sync_with_readline = true
lle.export_to_bash_history = true

# Advanced features
lle.enable_forensic_tracking = true
lle.enable_deduplication = true
lle.dedup_scope = recent

# Performance
lle.enable_history_cache = true
lle.cache_size = 1000

# Compatibility
lle.readline_compatible_mode = false
```

---

### 3.2 GNU Readline Compatible Preset

```ini
[history]
# Maximum GNU Readline compatibility
lle.readline_compatible_mode = true

# Or manually:
# lle.arrow_key_mode = classic
# lle.enable_multiline_navigation = false
# lle.wrap_history_navigation = false
# lle.save_line_on_history_nav = false
# lle.preserve_multiline_structure = false
# lle.enable_multiline_editing = false
# lle.storage_mode = readline-compat
# lle.enable_forensic_tracking = false
# lle.enable_deduplication = false
```

---

### 3.3 Bash Compatible Preset

```ini
[history]
# Bash-like behavior with some LLE features
lle.arrow_key_mode = classic
lle.enable_multiline_navigation = false
lle.wrap_history_navigation = false
lle.save_line_on_history_nav = false

# Keep LLE storage for advanced features
lle.storage_mode = dual
lle.export_to_bash_history = true

# Keep useful features
lle.enable_deduplication = true
lle.enable_interactive_search = true
```

---

### 3.4 Power User Preset

```ini
[history]
# Maximum LLE capabilities
lle.arrow_key_mode = context-aware
lle.enable_multiline_navigation = true
lle.wrap_history_navigation = true
lle.save_line_on_history_nav = true

# Full multiline support
lle.preserve_multiline_structure = true
lle.enable_multiline_editing = true
lle.show_multiline_indicators = true

# Advanced search
lle.enable_interactive_search = true
lle.search_fuzzy_matching = true
lle.search_case_sensitive = false

# LLE-only storage (no bash export)
lle.storage_mode = lle-only
lle.history_file = ~/.lle_history

# All advanced features
lle.enable_forensic_tracking = true
lle.enable_deduplication = true
lle.dedup_scope = global

# High performance cache
lle.enable_history_cache = true
lle.cache_size = 5000
```

---

## 4. Configuration UI Commands

### 4.1 Built-in Commands

```bash
# Show current history config
lusush> config show history

# Get specific option
lusush> config get lle.arrow_key_mode
context-aware

# Set option
lusush> config set lle.arrow_key_mode classic

# Load preset
lusush> config preset history readline-compatible
lusush> config preset history power-user

# Reset to defaults
lusush> config reset history
```

---

### 4.2 Environment Variables

Quick overrides without editing config file:

```bash
# Use GNU Readline compatibility mode
export LUSUSH_LLE_READLINE_COMPAT=1

# Disable multiline navigation
export LUSUSH_LLE_MULTILINE_NAV=0

# Set arrow key mode
export LUSUSH_LLE_ARROW_MODE=classic
```

---

## 5. Implementation Notes

### 5.1 Config File Registration

Add to `config.c` options array:

```c
static const config_option_t config_options[] = {
    // Existing options...
    
    // LLE History - Arrow Key Behavior
    {"lle.arrow_key_mode", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_arrow_key_mode, "Arrow key behavior mode (context-aware|classic|always-history|multiline-first)",
     config_validate_arrow_key_mode},
    
    {"lle.enable_multiline_navigation", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_multiline_navigation, "Enable vertical cursor movement in multiline",
     config_validate_bool},
    
    // LLE History - Navigation
    {"lle.wrap_history_navigation", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_wrap_history_navigation, "Wrap at beginning/end of history",
     config_validate_bool},
    
    {"lle.save_line_on_history_nav", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_save_line_on_history_nav, "Save current line before history navigation",
     config_validate_bool},
    
    // ... (continue for all options)
};
```

### 5.2 Validation Functions

```c
bool config_validate_arrow_key_mode(const char *value) {
    return (strcmp(value, "context-aware") == 0 ||
            strcmp(value, "classic") == 0 ||
            strcmp(value, "always-history") == 0 ||
            strcmp(value, "multiline-first") == 0);
}

bool config_validate_storage_mode(const char *value) {
    return (strcmp(value, "lle-only") == 0 ||
            strcmp(value, "bash-only") == 0 ||
            strcmp(value, "dual") == 0 ||
            strcmp(value, "readline-compat") == 0);
}

bool config_validate_dedup_scope(const char *value) {
    return (strcmp(value, "none") == 0 ||
            strcmp(value, "session") == 0 ||
            strcmp(value, "recent") == 0 ||
            strcmp(value, "global") == 0);
}
```

### 5.3 Default Initialization

```c
void config_set_lle_history_defaults(void) {
    // Arrow key behavior
    config.lle_arrow_key_mode = LLE_ARROW_MODE_CONTEXT_AWARE;
    config.lle_enable_multiline_navigation = true;
    
    // Navigation
    config.lle_wrap_history_navigation = true;
    config.lle_save_line_on_history_nav = true;
    
    // Multiline
    config.lle_preserve_multiline_structure = true;
    config.lle_enable_multiline_editing = true;
    config.lle_show_multiline_indicators = true;
    
    // Search
    config.lle_enable_interactive_search = true;
    config.lle_search_fuzzy_matching = false;
    config.lle_search_case_sensitive = false;
    
    // Storage
    config.lle_storage_mode = LLE_STORAGE_MODE_DUAL;
    config.lle_history_file = strdup("~/.lle_history");
    config.lle_sync_with_readline = true;
    config.lle_export_to_bash_history = true;
    
    // Advanced
    config.lle_enable_forensic_tracking = true;
    config.lle_enable_deduplication = true;
    config.lle_dedup_scope = LLE_DEDUP_SCOPE_RECENT;
    
    // Performance
    config.lle_enable_history_cache = true;
    config.lle_cache_size = 1000;
    
    // Compatibility
    config.lle_readline_compatible_mode = false;
}
```

---

## 6. Usage in LLE Code

### 6.1 Accessing Config in LLE

```c
// In lle_readline.c or history navigation handler

static lle_result_t handle_arrow_up(lle_event_t *event, void *user_data) {
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Check arrow key mode configuration
    switch (config.lle_arrow_key_mode) {
        case LLE_ARROW_MODE_CONTEXT_AWARE:
            return handle_arrow_up_context_aware(ctx);
            
        case LLE_ARROW_MODE_CLASSIC:
            return handle_history_previous(ctx);
            
        case LLE_ARROW_MODE_ALWAYS_HISTORY:
            return handle_history_previous(ctx);
            
        case LLE_ARROW_MODE_MULTILINE_FIRST:
            if (lle_buffer_is_multiline(ctx->buffer)) {
                return handle_cursor_up_one_line(ctx);
            } else {
                return handle_history_previous(ctx);
            }
    }
    
    return LLE_SUCCESS;
}

static lle_result_t handle_arrow_up_context_aware(readline_context_t *ctx) {
    // Context-aware logic
    if (lle_buffer_is_multiline(ctx->buffer)) {
        if (lle_cursor_at_top_line(ctx->buffer)) {
            // At top line of multiline - navigate history
            return handle_history_previous(ctx);
        } else {
            // In middle/bottom - move cursor up
            return handle_cursor_up_one_line(ctx);
        }
    } else {
        // Single line - always navigate history
        return handle_history_previous(ctx);
    }
}
```

---

## 7. Documentation for Users

### 7.1 README Section

```markdown
## LLE History System Configuration

Lusush's Line Editing Engine (LLE) provides advanced history capabilities
with flexible configuration options.

### Quick Start

Default configuration provides smart arrow key behavior:
- **UP/DOWN in single-line**: Navigate history
- **UP/DOWN in multiline**: Move cursor vertically
- **Ctrl-P/Ctrl-N**: Always navigate history

### Configuration File

Edit `~/.lusushrc`:

```ini
[history]
# Choose arrow key behavior
lle.arrow_key_mode = context-aware  # Smart (default)
# lle.arrow_key_mode = classic       # GNU Readline compatible

# Enable advanced features
lle.preserve_multiline_structure = true
lle.enable_forensic_tracking = true
```

### Presets

Apply preset configurations:

```bash
# GNU Readline compatibility
lusush> config preset history readline-compatible

# Power user (all features)
lusush> config preset history power-user

# Bash-like behavior
lusush> config preset history bash-compatible
```

### Common Customizations

**I want exactly GNU Readline behavior:**
```ini
[history]
lle.readline_compatible_mode = true
```

**I want multiline editing but classic history navigation:**
```ini
[history]
lle.arrow_key_mode = classic
lle.preserve_multiline_structure = true
```

**I want maximum LLE features:**
```ini
[history]
lle.arrow_key_mode = context-aware
lle.search_fuzzy_matching = true
lle.storage_mode = lle-only
lle.dedup_scope = global
```
```

---

## 8. Summary

**Configuration Strategy:**
1. ✅ **Use existing Lusush config system** - No new infrastructure needed
2. ✅ **Add LLE-specific options** - Namespaced under `lle.*`
3. ✅ **Provide sensible defaults** - Advanced features enabled, but configurable
4. ✅ **Offer presets** - Easy switching between behavior modes
5. ✅ **Backward compatible** - Can emulate GNU Readline exactly

**Default Philosophy:**
- **Advanced features ON** - Users chose LLE for capabilities
- **Context-aware behavior** - Smart defaults that "just work"
- **Easy override** - One setting to switch to classic mode
- **Full compatibility mode** - Available when needed

**User Choice:**
- Power users get multiline editing and smart navigation
- Traditional users can enable `readline_compatible_mode`
- Everyone can customize to their preference

This approach respects both LLE's advanced capabilities and users' freedom to choose their workflow.

# LLE History Implementation Plan

**Document**: LLE_HISTORY_IMPLEMENTATION_PLAN.md  
**Date**: 2025-11-07  
**Updated**: 2025-11-07 (Added existing infrastructure inventory)  
**Purpose**: Detailed implementation plan for LLE history navigation and configuration  
**Status**: Phase 1 Complete ✅ | Phase 2 In Progress  
**Estimated Total Time**: ~~9-14 hours~~ **4-6 hours** (existing code significantly reduces scope)

---

## Quick Reference

**Current Status**: Phase 1 Complete (Config system ready), Phase 2 In Progress  
**Next Action**: Phase 2 - Wire up history navigation (much simpler than originally planned!)  
**Branch**: feature/lle (continue on existing branch)  
**Dependencies**: 
- Existing Lusush config system (`config.h`, `config.c`) ✅
- **Existing LLE history infrastructure** (see below) ✅

**Related Documents**:
- [HISTORY_INTEGRATION_ANALYSIS.md](./HISTORY_INTEGRATION_ANALYSIS.md) - Analysis and background
- [LLE_HISTORY_CONFIG_SPECIFICATION.md](./LLE_HISTORY_CONFIG_SPECIFICATION.md) - Complete config spec
- [KEYBINDING_TEST_TRACKER.md](../lle_implementation/KEYBINDING_TEST_TRACKER.md) - Testing framework

---

## CRITICAL DISCOVERY: Existing History Infrastructure ✅

**Session Date**: 2025-11-07  
**Finding**: Comprehensive LLE history system already implemented per Spec 09!

### What Already Exists (16 Files, ~8000 LOC)

#### Core History Engine ✅
- **`src/lle/history_core.c`** - Complete history core implementation
  - `lle_history_add_entry()` - Add commands to history
  - `lle_history_get_entry_by_index()` - Retrieve by index
  - `lle_history_get_entry_by_id()` - Retrieve by ID  
  - `lle_history_get_entry_count()` - Get total entries
  - `lle_history_config_create_default()` - Configuration management
  - Entry lifecycle, statistics, performance monitoring

#### History-Buffer Integration ✅
- **`src/lle/history_buffer_integration.c`** - Seamless history ↔ buffer integration
  - `lle_history_edit_entry()` - Load history entry into buffer
  - `lle_history_session_complete()` - Save edited entry
  - `lle_history_session_cancel()` - Cancel editing
  - Multiline reconstruction engine
  - Edit session management
  - Performance caching

#### Advanced Features ✅
- **`src/lle/history_storage.c`** - Persistent storage (LLE format + bash export)
- **`src/lle/history_search.c`** - Search engine (exact, prefix, substring, fuzzy)
- **`src/lle/history_interactive_search.c`** - Ctrl-R interactive search
- **`src/lle/history_multiline.c`** - Multiline command preservation
- **`src/lle/history_dedup.c`** - Deduplication engine (all scopes)
- **`src/lle/history_forensics.c`** - Forensic metadata tracking
- **`src/lle/history_expansion.c`** - History expansion (!!, !$, etc.)
- **`src/lle/history_events.c`** - Event system integration
- **`src/lle/history_lusush_bridge.c`** - GNU Readline bridge
- **`src/lle/history_index.c`** - Hashtable indexing for performance

#### Supporting Infrastructure ✅
- **`include/lle/history.h`** - Complete public API (300+ lines)
- **`include/lle/history_buffer_integration.h`** - Integration API
- **`src/lle/edit_session_manager.c`** - Edit session tracking
- **`src/lle/multiline_parser.c`** - Shell construct parser
- **`src/lle/structure_analyzer.c`** - Command structure analysis
- **`src/lle/reconstruction_engine.c`** - Multiline reconstruction
- **`src/lle/formatting_engine.c`** - Intelligent formatting

### What's Missing (Phase 2 - Simple Wiring Only!)

❌ **Navigation State** - Track current position in history (3 fields)
❌ **Navigation Actions** - `lle_action_previous_history()` / `lle_action_next_history()` (2 functions)
❌ **Keybinding Wiring** - Connect UP/DOWN to actions (4 lines)
❌ **Initialization** - Initialize history core in LLE startup (10 lines)

### Revised Time Estimates

| Phase | Original Estimate | Revised Estimate | Reason |
|-------|------------------|------------------|---------|
| Phase 1 | 1-2 hours | **DONE ✅** | Config system complete |
| Phase 2 | 4-6 hours | **1-2 hours** | Just wiring, not building! |
| Phase 3 | 2-3 hours | **1 hour** | Context-aware logic only |
| Phase 4 | 2-3 hours | **2 hours** | Testing unchanged |
| **Total** | **9-14 hours** | **4-5 hours** | 60% reduction! |

---

## Table of Contents

1. [Existing Infrastructure Inventory](#existing-infrastructure-inventory)
2. [Implementation Phases](#implementation-phases)
3. [Phase 1: Configuration System](#phase-1-configuration-system)
4. [Phase 2: Basic History Navigation](#phase-2-basic-history-navigation)
5. [Phase 3: Context-Aware Multiline](#phase-3-context-aware-multiline)
6. [Phase 4: Testing and Documentation](#phase-4-testing-and-documentation)
7. [Progress Tracking](#progress-tracking)
8. [Testing Checklist](#testing-checklist)
9. [Rollback Plan](#rollback-plan)

---

## Existing Infrastructure Inventory

### File Manifest (LLE History System)

```
include/lle/
  ├── history.h                           # Main public API (300+ lines)
  ├── history_buffer_integration.h        # Integration API (400+ lines)
  └── history_buffer_bridge.h             # Bridge interface

src/lle/
  ├── history_core.c                      # Core engine (900+ lines) ✅
  ├── history_buffer_integration.c        # Buffer integration (800+ lines) ✅
  ├── history_storage.c                   # File I/O (600+ lines) ✅
  ├── history_search.c                    # Search engine (500+ lines) ✅
  ├── history_interactive_search.c        # Ctrl-R search (700+ lines) ✅
  ├── history_multiline.c                 # Multiline support (400+ lines) ✅
  ├── history_dedup.c                     # Deduplication (500+ lines) ✅
  ├── history_forensics.c                 # Forensic tracking (600+ lines) ✅
  ├── history_expansion.c                 # History expansion (800+ lines) ✅
  ├── history_events.c                    # Event integration (300+ lines) ✅
  ├── history_lusush_bridge.c             # Readline bridge (500+ lines) ✅
  ├── history_index.c                     # Hashtable indexing (400+ lines) ✅
  ├── history_buffer_bridge.c             # Buffer bridge (300+ lines) ✅
  ├── edit_session_manager.c              # Session management (600+ lines) ✅
  ├── multiline_parser.c                  # Structure parser (700+ lines) ✅
  └── reconstruction_engine.c             # Reconstruction (500+ lines) ✅

Total: ~8,500 lines of production-ready code ✅
```

### Key Data Structures (Already Defined)

```c
// From history.h - ALL IMPLEMENTED ✅
typedef struct lle_history_entry {
    uint64_t entry_id;
    char *command;
    size_t command_length;
    uint64_t timestamp;
    int exit_code;
    char *working_directory;
    char *original_multiline;        // Multiline preservation
    bool is_multiline;
    // + 15 more forensic fields
} lle_history_entry_t;

typedef struct lle_history_core {
    lle_history_entry_t **entries;   // Dynamic array
    size_t entry_count;
    size_t entry_capacity;
    lle_hashtable_t *entry_lookup;   // Fast lookup
    lle_history_dedup_engine_t *dedup_engine;
    // + full config, stats, thread safety
} lle_history_core_t;

typedef struct lle_history_buffer_integration {
    lle_history_core_t *history_core;
    lle_buffer_t *editing_buffer;
    lle_reconstruction_engine_t *reconstruction;
    lle_edit_session_manager_t *session_manager;
    lle_multiline_parser_t *multiline_parser;
    // + full integration infrastructure
} lle_history_buffer_integration_t;
```

### Available API Functions (Already Implemented)

```c
// Core operations ✅
lle_result_t lle_history_add_entry(lle_history_core_t *core, const char *command, int exit_code);
lle_result_t lle_history_get_entry_by_index(lle_history_core_t *core, size_t index, lle_history_entry_t **entry);
lle_result_t lle_history_get_entry_by_id(lle_history_core_t *core, uint64_t id, lle_history_entry_t **entry);
lle_result_t lle_history_get_entry_count(lle_history_core_t *core, size_t *count);

// Buffer integration ✅
lle_result_t lle_history_edit_entry(lle_history_buffer_integration_t *integration, size_t entry_index, lle_buffer_t *buffer);
lle_result_t lle_history_session_complete(lle_history_buffer_integration_t *integration, lle_buffer_t *buffer);
lle_result_t lle_history_session_cancel(lle_history_buffer_integration_t *integration);

// Search ✅
lle_history_search_results_t* lle_history_search_exact(const char *query);
lle_history_search_results_t* lle_history_search_fuzzy(const char *query, int threshold);
// + prefix, substring, interactive search

// Storage ✅
lle_result_t lle_history_storage_save(lle_history_core_t *core, const char *path);
lle_result_t lle_history_storage_load(lle_history_core_t *core, const char *path);
// + bash export, readline sync

// Advanced features ✅
// Deduplication, forensics, multiline, expansion - all implemented!
```

---

## Implementation Phases

## 1. Implementation Phases

### Overview

```
Phase 1: Configuration System (1-2 hours)
    ↓
Phase 2: Basic History Navigation (4-6 hours)
    ↓
Phase 3: Context-Aware Multiline (2-3 hours)
    ↓
Phase 4: Testing & Documentation (2-3 hours)
    ↓
Total: 9-14 hours (1.5-2 days)
```

### Phase Dependencies

- **Phase 1**: No dependencies (can start immediately)
- **Phase 2**: Requires Phase 1 (uses config options)
- **Phase 3**: Requires Phase 2 (enhances navigation)
- **Phase 4**: Requires Phases 1-3 (tests everything)

### Incremental Delivery

Each phase produces a working, testable state:
- ✅ Phase 1: Config options work (`config show history`)
- ✅ Phase 2: UP/DOWN history navigation works
- ✅ Phase 3: Context-aware multiline works
- ✅ Phase 4: Fully tested and documented

---

## 2. Phase 1: Configuration System

**Goal**: Add LLE history configuration options to existing config system  
**Estimated Time**: 1-2 hours  
**Status**: ⬜ Not Started

### 2.1 Prerequisites

**Existing Files to Modify**:
- `include/config.h` - Add struct fields and enums
- `src/config.c` - Add options, validators, defaults

**Existing Config System Check**:
```bash
# Verify config system works
./builddir/lusush
lusush> config show history
# Should show existing history options
```

### 2.2 Tasks

#### Task 1.1: Add Enumerations to `config.h`

**File**: `include/config.h`  
**Location**: After existing type definitions

**Code to Add**:
```c
/**
 * LLE arrow key behavior modes
 */
typedef enum {
    LLE_ARROW_MODE_CONTEXT_AWARE,    // Smart: multiline navigation when in multiline
    LLE_ARROW_MODE_CLASSIC,          // GNU Readline: always history navigation
    LLE_ARROW_MODE_ALWAYS_HISTORY,   // Always history, use Ctrl-P/N only
    LLE_ARROW_MODE_MULTILINE_FIRST   // Prioritize multiline navigation
} lle_arrow_key_mode_t;

/**
 * LLE history storage modes
 */
typedef enum {
    LLE_STORAGE_MODE_LLE_ONLY,       // Store only in LLE format
    LLE_STORAGE_MODE_BASH_ONLY,      // Store only in bash format
    LLE_STORAGE_MODE_DUAL,           // Store in both formats (recommended)
    LLE_STORAGE_MODE_READLINE_COMPAT // Use GNU Readline's storage
} lle_history_storage_mode_t;

/**
 * LLE deduplication scope
 */
typedef enum {
    LLE_DEDUP_SCOPE_NONE,            // No deduplication
    LLE_DEDUP_SCOPE_SESSION,         // Within current session
    LLE_DEDUP_SCOPE_RECENT,          // Last N entries
    LLE_DEDUP_SCOPE_GLOBAL           // Entire history
} lle_dedup_scope_t;
```

**Checkpoint**: Compiles without errors ✅

---

#### Task 1.2: Add Fields to `config_values_t` Struct

**File**: `include/config.h`  
**Location**: In `config_values_t` struct, after existing history fields

**Code to Add**:
```c
typedef struct {
    // Existing fields...
    bool history_enabled;
    int history_size;
    bool history_no_dups;
    bool history_timestamps;
    char *history_file;
    
    // NEW: LLE History Configuration
    
    // Arrow key behavior
    lle_arrow_key_mode_t lle_arrow_key_mode;
    bool lle_enable_multiline_navigation;
    
    // History navigation
    bool lle_wrap_history_navigation;
    bool lle_save_line_on_history_nav;
    
    // Multiline support
    bool lle_preserve_multiline_structure;
    bool lle_enable_multiline_editing;
    bool lle_show_multiline_indicators;
    
    // Search behavior
    bool lle_enable_interactive_search;
    bool lle_search_fuzzy_matching;
    bool lle_search_case_sensitive;
    
    // File format and storage
    lle_history_storage_mode_t lle_storage_mode;
    char *lle_history_file;
    bool lle_sync_with_readline;
    bool lle_export_to_bash_history;
    
    // Advanced features
    bool lle_enable_forensic_tracking;
    bool lle_enable_deduplication;
    lle_dedup_scope_t lle_dedup_scope;
    
    // Performance
    bool lle_enable_history_cache;
    int lle_cache_size;
    
    // Compatibility mode
    bool lle_readline_compatible_mode;
    
    // Existing fields continue...
} config_values_t;
```

**Checkpoint**: Compiles without errors ✅

---

#### Task 1.3: Add Validation Functions to `config.c`

**File**: `src/config.c`  
**Location**: With other validation functions

**Code to Add**:
```c
/**
 * Validate arrow key mode setting
 */
bool config_validate_arrow_key_mode(const char *value) {
    return (strcmp(value, "context-aware") == 0 ||
            strcmp(value, "classic") == 0 ||
            strcmp(value, "always-history") == 0 ||
            strcmp(value, "multiline-first") == 0);
}

/**
 * Validate storage mode setting
 */
bool config_validate_storage_mode(const char *value) {
    return (strcmp(value, "lle-only") == 0 ||
            strcmp(value, "bash-only") == 0 ||
            strcmp(value, "dual") == 0 ||
            strcmp(value, "readline-compat") == 0);
}

/**
 * Validate deduplication scope
 */
bool config_validate_dedup_scope(const char *value) {
    return (strcmp(value, "none") == 0 ||
            strcmp(value, "session") == 0 ||
            strcmp(value, "recent") == 0 ||
            strcmp(value, "global") == 0);
}
```

**Checkpoint**: Compiles without errors ✅

---

#### Task 1.4: Add Options to `config_options[]` Array

**File**: `src/config.c`  
**Location**: In `config_options[]` array, after existing history options

**Code to Add**:
```c
static const config_option_t config_options[] = {
    // Existing options...
    
    // LLE History - Arrow Key Behavior
    {"lle.arrow_key_mode", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_arrow_key_mode, 
     "Arrow key behavior mode (context-aware|classic|always-history|multiline-first)",
     config_validate_arrow_key_mode},
    
    {"lle.enable_multiline_navigation", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_multiline_navigation, 
     "Enable vertical cursor movement in multiline",
     config_validate_bool},
    
    // LLE History - Navigation
    {"lle.wrap_history_navigation", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_wrap_history_navigation, 
     "Wrap at beginning/end of history",
     config_validate_bool},
    
    {"lle.save_line_on_history_nav", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_save_line_on_history_nav, 
     "Save current line before history navigation",
     config_validate_bool},
    
    // LLE History - Multiline Support
    {"lle.preserve_multiline_structure", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_preserve_multiline_structure, 
     "Preserve original multiline formatting",
     config_validate_bool},
    
    {"lle.enable_multiline_editing", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_multiline_editing, 
     "Enable multiline command editing",
     config_validate_bool},
    
    {"lle.show_multiline_indicators", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_show_multiline_indicators, 
     "Show visual indicators for multiline",
     config_validate_bool},
    
    // LLE History - Search
    {"lle.enable_interactive_search", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_interactive_search, 
     "Enable Ctrl-R search",
     config_validate_bool},
    
    {"lle.search_fuzzy_matching", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_search_fuzzy_matching, 
     "Use fuzzy matching in search",
     config_validate_bool},
    
    {"lle.search_case_sensitive", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_search_case_sensitive, 
     "Case-sensitive search",
     config_validate_bool},
    
    // LLE History - Storage
    {"lle.storage_mode", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_storage_mode, 
     "Storage format mode (lle-only|bash-only|dual|readline-compat)",
     config_validate_storage_mode},
    
    {"lle.history_file", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_history_file, 
     "LLE history file path",
     config_validate_string},
    
    {"lle.sync_with_readline", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_sync_with_readline, 
     "Sync with GNU Readline history",
     config_validate_bool},
    
    {"lle.export_to_bash_history", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_export_to_bash_history, 
     "Export to bash_history format",
     config_validate_bool},
    
    // LLE History - Advanced Features
    {"lle.enable_forensic_tracking", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_forensic_tracking, 
     "Enable forensic metadata",
     config_validate_bool},
    
    {"lle.enable_deduplication", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_deduplication, 
     "Enable duplicate detection",
     config_validate_bool},
    
    {"lle.dedup_scope", CONFIG_TYPE_STRING, CONFIG_SECTION_HISTORY,
     &config.lle_dedup_scope, 
     "Deduplication scope (none|session|recent|global)",
     config_validate_dedup_scope},
    
    // LLE History - Performance
    {"lle.enable_history_cache", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_enable_history_cache, 
     "Enable LRU cache",
     config_validate_bool},
    
    {"lle.cache_size", CONFIG_TYPE_INT, CONFIG_SECTION_HISTORY,
     &config.lle_cache_size, 
     "Cache size in entries",
     config_validate_int},
    
    // LLE History - Compatibility
    {"lle.readline_compatible_mode", CONFIG_TYPE_BOOL, CONFIG_SECTION_HISTORY,
     &config.lle_readline_compatible_mode, 
     "Emulate GNU Readline exactly",
     config_validate_bool},
    
    // Existing options continue...
    {NULL, NULL, NULL, NULL, NULL, NULL}  // Sentinel
};
```

**Note**: For string enums (arrow_key_mode, storage_mode, dedup_scope), we'll need helper functions to convert strings to enum values. Add these to `config.c`:

```c
/**
 * Convert string to arrow key mode enum
 */
lle_arrow_key_mode_t config_parse_arrow_key_mode(const char *value) {
    if (strcmp(value, "classic") == 0) return LLE_ARROW_MODE_CLASSIC;
    if (strcmp(value, "always-history") == 0) return LLE_ARROW_MODE_ALWAYS_HISTORY;
    if (strcmp(value, "multiline-first") == 0) return LLE_ARROW_MODE_MULTILINE_FIRST;
    return LLE_ARROW_MODE_CONTEXT_AWARE;  // Default
}

/**
 * Convert string to storage mode enum
 */
lle_history_storage_mode_t config_parse_storage_mode(const char *value) {
    if (strcmp(value, "lle-only") == 0) return LLE_STORAGE_MODE_LLE_ONLY;
    if (strcmp(value, "bash-only") == 0) return LLE_STORAGE_MODE_BASH_ONLY;
    if (strcmp(value, "readline-compat") == 0) return LLE_STORAGE_MODE_READLINE_COMPAT;
    return LLE_STORAGE_MODE_DUAL;  // Default
}

/**
 * Convert string to dedup scope enum
 */
lle_dedup_scope_t config_parse_dedup_scope(const char *value) {
    if (strcmp(value, "none") == 0) return LLE_DEDUP_SCOPE_NONE;
    if (strcmp(value, "session") == 0) return LLE_DEDUP_SCOPE_SESSION;
    if (strcmp(value, "global") == 0) return LLE_DEDUP_SCOPE_GLOBAL;
    return LLE_DEDUP_SCOPE_RECENT;  // Default
}
```

**Checkpoint**: Compiles without errors ✅

---

#### Task 1.5: Set Defaults in `config_set_defaults()`

**File**: `src/config.c`  
**Location**: In `config_set_defaults()` function, after existing history defaults

**Code to Add**:
```c
void config_set_defaults(void) {
    // Existing defaults...
    config.history_enabled = true;
    config.history_size = 1000;
    config.history_no_dups = true;
    config.history_timestamps = false;
    config.history_file = NULL;
    
    // LLE History Defaults (Advanced features enabled)
    
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
    config.lle_search_fuzzy_matching = false;  // Conservative default
    config.lle_search_case_sensitive = false;
    
    // Storage
    config.lle_storage_mode = LLE_STORAGE_MODE_DUAL;
    config.lle_history_file = NULL;  // Will be set to ~/.lle_history later
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
    
    // Existing defaults continue...
}
```

**Checkpoint**: Defaults set correctly ✅

---

#### Task 1.6: Update Config File Template

**File**: `src/config.c`  
**Location**: In `CONFIG_FILE_TEMPLATE` constant

**Code to Add**:
```c
const char *CONFIG_FILE_TEMPLATE =
    "# Lusush Configuration File\n"
    "# ~/.lusushrc\n"
    "\n"
    "# History Configuration\n"
    "[history]\n"
    "history_enabled = true\n"
    "history_size = 1000\n"
    "history_no_dups = true\n"
    "history_timestamps = false\n"
    "# history_file = ~/.lusush_history\n"
    "\n"
    "# LLE History Configuration\n"
    "# Arrow key behavior: context-aware|classic|always-history|multiline-first\n"
    "lle.arrow_key_mode = context-aware\n"
    "lle.enable_multiline_navigation = true\n"
    "\n"
    "# History navigation\n"
    "lle.wrap_history_navigation = true\n"
    "lle.save_line_on_history_nav = true\n"
    "\n"
    "# Multiline support\n"
    "lle.preserve_multiline_structure = true\n"
    "lle.enable_multiline_editing = true\n"
    "lle.show_multiline_indicators = true\n"
    "\n"
    "# Search\n"
    "lle.enable_interactive_search = true\n"
    "lle.search_fuzzy_matching = false\n"
    "lle.search_case_sensitive = false\n"
    "\n"
    "# Storage: lle-only|bash-only|dual|readline-compat\n"
    "lle.storage_mode = dual\n"
    "# lle.history_file = ~/.lle_history\n"
    "lle.sync_with_readline = true\n"
    "lle.export_to_bash_history = true\n"
    "\n"
    "# Advanced features\n"
    "lle.enable_forensic_tracking = true\n"
    "lle.enable_deduplication = true\n"
    "lle.dedup_scope = recent\n"
    "\n"
    "# Performance\n"
    "lle.enable_history_cache = true\n"
    "lle.cache_size = 1000\n"
    "\n"
    "# Compatibility\n"
    "# Set to true for exact GNU Readline behavior\n"
    "lle.readline_compatible_mode = false\n"
    "\n"
    // Existing template continues...
```

**Checkpoint**: Template updated ✅

---

### 2.3 Testing Phase 1

**Build and Test**:
```bash
# Build
cd /home/mberry/Lab/c/lusush
ninja -C builddir

# Test config loading
./builddir/lusush
lusush> config show history

# Should show all LLE options with defaults
# Verify:
# - lle.arrow_key_mode = context-aware
# - lle.enable_multiline_navigation = true
# - lle.storage_mode = dual
# - etc.

# Test setting options
lusush> config set lle.arrow_key_mode classic
lusush> config get lle.arrow_key_mode
# Should show: classic

# Test validation
lusush> config set lle.arrow_key_mode invalid
# Should show error

# Test boolean options
lusush> config set lle.enable_multiline_navigation false
lusush> config get lle.enable_multiline_navigation
# Should show: false
```

### 2.4 Commit Phase 1

```bash
git add include/config.h src/config.c
git commit -m "LLE: Add history system configuration options

Add comprehensive configuration options for LLE history system:
- Arrow key behavior modes (context-aware, classic, etc.)
- Multiline navigation and editing settings
- History storage modes (LLE, bash, dual)
- Search configuration (fuzzy, case-sensitive)
- Advanced features (forensic tracking, deduplication)
- Performance settings (cache size)
- Compatibility mode for GNU Readline emulation

All options integrated with existing Lusush config system.
Default: Advanced features enabled, context-aware arrow keys.

Configuration documented in:
- docs/development/LLE_HISTORY_CONFIG_SPECIFICATION.md

Part of LLE history integration plan.
Phase 1 complete."
```

### 2.5 Phase 1 Completion Checklist

- [ ] Enums added to `config.h`
- [ ] Fields added to `config_values_t`
- [ ] Validation functions added
- [ ] Options added to `config_options[]`
- [ ] String-to-enum parsers added
- [ ] Defaults set in `config_set_defaults()`
- [ ] Template updated
- [ ] Builds without errors
- [ ] `config show history` works
- [ ] Can get/set individual options
- [ ] Validation works (rejects invalid values)
- [ ] Committed and pushed

**Phase 1 Complete**: ⬜

---

## 3. Phase 2: Basic History Navigation ✅ SIMPLIFIED

**Goal**: Wire up UP/DOWN arrow history navigation using existing history infrastructure  
**Estimated Time**: ~~4-6 hours~~ **1-2 hours** (existing code does heavy lifting!)  
**Status**: 🔄 In Progress  
**Depends On**: Phase 1 ✅

**CRITICAL**: This phase is now MUCH simpler because:
- ✅ History core engine already exists (`lle_history_core.c`)
- ✅ Buffer integration already exists (`lle_history_buffer_integration.c`)
- ✅ All APIs we need are already implemented
- ❌ Only missing: navigation state tracking + action wiring

### 3.1 What We're Leveraging (Already Exists)

**Existing APIs We'll Use**:
```c
// From history_core.c ✅
lle_result_t lle_history_get_entry_by_index(lle_history_core_t *core, size_t index, lle_history_entry_t **entry);
lle_result_t lle_history_get_entry_count(lle_history_core_t *core, size_t *count);
lle_result_t lle_history_add_entry(lle_history_core_t *core, const char *command, int exit_code);

// From history_buffer_integration.c ✅ (for Phase 3 multiline)
lle_result_t lle_history_edit_entry(lle_history_buffer_integration_t *integration, size_t entry_index, lle_buffer_t *buffer);
```

**Files to Check First**:
```bash
# Verify these exist (they do!)
cat include/lle/history.h | grep "lle_history_get_entry"
cat src/lle/history_core.c | grep "lle_history_get_entry_by_index"
```

### 3.2 Tasks (Simplified - Just Wiring!)

#### Task 2.1: Find History System Instance in LLE

**Investigation**: Check how history system is currently initialized  
**File**: `src/lle/lle_readline.c` or `src/lle/*.c`

**Commands to Run**:
```bash
# Find where lle_history_core_t is created/initialized
grep -r "lle_history_core_t\|lle_history_core_create" src/lle/
grep -r "global.*history\|history.*global" src/lle/
```

**Expected**: Either:
1. Global `lle_history_core_t *global_history_core` exists
2. Or we need to create one in `lle_readline.c`

**Checkpoint**: Found history system instance location ✅

---

#### Task 2.2: Add History Navigation State to `lle_readline_state_t`

**File**: `src/lle/lle_readline.c`  
**Location**: In `readline_context_t` typedef

**Code to Add**:
```c
typedef struct {
    lle_buffer_t *buffer;
    bool *done;
    char **final_line;
    lle_terminal_abstraction_t *term;
    const char *prompt;
    continuation_state_t *continuation_state;
    char *kill_buffer;
    size_t kill_buffer_size;
    
    // NEW: History navigation state
    lle_history_system_t *history;        // History system reference
    size_t history_position;               // Current position in history (0 = newest)
    size_t history_count;                  // Total history entries
    char *saved_line;                      // Line saved before history navigation
    bool in_history_navigation;            // Currently navigating history
} readline_context_t;
```

**Checkpoint**: Compiles ✅

---

#### Task 2.2: Initialize History State

**File**: `src/lle/lle_readline.c`  
**Location**: In `lle_readline()` function, during context initialization

**Code to Add**:
```c
char *lle_readline(const char *prompt) {
    // Existing initialization...
    
    // Initialize history navigation state
    ctx.history = global_history_system;  // Get global history (verify this exists)
    ctx.history_position = 0;              // Start at newest (not in history yet)
    ctx.history_count = 0;                 // Will be set when first navigating
    ctx.saved_line = NULL;                 // No saved line yet
    ctx.in_history_navigation = false;     // Not navigating
    
    // Existing code continues...
}
```

**Note**: Need to verify how to get global history system reference. Check:
```bash
grep -r "lle_history_system" src/lle/*.c
# or
grep -r "global.*history" src/lle/*.c
```

**Checkpoint**: Compiles, history reference accessible ✅

---

#### Task 2.3: Implement `handle_history_previous()` (Classic Mode)

**File**: `src/lle/lle_readline.c`  
**Location**: Add new function with other handlers

**Code to Add**:
```c
/**
 * @brief Handle history previous (UP arrow / Ctrl-P)
 * Navigate to previous (older) history entry
 */
static lle_result_t handle_history_previous(lle_event_t *event, void *user_data) {
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Get total history count if not already cached
    if (ctx->history_count == 0) {
        ctx->history_count = lle_history_system_get_count(ctx->history);
        if (ctx->history_count == 0) {
            // Empty history, nothing to do
            return LLE_SUCCESS;
        }
    }
    
    // First time navigating history - save current line
    if (!ctx->in_history_navigation && config.lle_save_line_on_history_nav) {
        size_t current_length = ctx->buffer->length;
        if (current_length > 0) {
            ctx->saved_line = (char *)malloc(current_length + 1);
            if (ctx->saved_line) {
                memcpy(ctx->saved_line, ctx->buffer->data, current_length);
                ctx->saved_line[current_length] = '\0';
            }
        }
        ctx->in_history_navigation = true;
        ctx->history_position = 0;  // Start at newest
    }
    
    // Check if we can move backwards
    if (ctx->history_position >= ctx->history_count) {
        // At oldest entry
        if (config.lle_wrap_history_navigation) {
            // Wrap to newest
            ctx->history_position = 0;
        } else {
            // Stay at oldest (GNU Readline behavior)
            return LLE_SUCCESS;
        }
    }
    
    // Get history entry
    lle_history_entry_t *entry = NULL;
    lle_result_t result = lle_history_system_get_entry_by_index(
        ctx->history, ctx->history_position, &entry);
    
    if (result != LLE_SUCCESS || !entry) {
        return result;
    }
    
    // Load history entry into buffer
    const char *command = entry->command;
    size_t command_length = entry->command_length;
    
    // Clear current buffer
    result = lle_buffer_clear(ctx->buffer);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Insert history command
    result = lle_buffer_insert_text(ctx->buffer, 0, command, command_length);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    // Move cursor to end
    ctx->buffer->cursor.byte_offset = command_length;
    
    // Refresh display
    refresh_display(ctx);
    
    // Move to next older entry for next UP
    ctx->history_position++;
    
    return LLE_SUCCESS;
}
```

**Checkpoint**: Compiles, implements classic history previous ✅

---

#### Task 2.4: Implement `handle_history_next()` (Classic Mode)

**File**: `src/lle/lle_readline.c`  
**Location**: Add after `handle_history_previous()`

**Code to Add**:
```c
/**
 * @brief Handle history next (DOWN arrow / Ctrl-N)
 * Navigate to next (newer) history entry
 */
static lle_result_t handle_history_next(lle_event_t *event, void *user_data) {
    (void)event;  /* Unused */
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Not in history navigation - nothing to do
    if (!ctx->in_history_navigation) {
        return LLE_SUCCESS;
    }
    
    // Move to newer entry
    if (ctx->history_position > 0) {
        ctx->history_position--;
        
        if (ctx->history_position == 0) {
            // Back to present - restore saved line
            lle_buffer_clear(ctx->buffer);
            
            if (ctx->saved_line) {
                lle_buffer_insert_text(ctx->buffer, 0, 
                                      ctx->saved_line, strlen(ctx->saved_line));
                ctx->buffer->cursor.byte_offset = strlen(ctx->saved_line);
                
                free(ctx->saved_line);
                ctx->saved_line = NULL;
            }
            
            ctx->in_history_navigation = false;
        } else {
            // Load newer history entry
            lle_history_entry_t *entry = NULL;
            lle_result_t result = lle_history_system_get_entry_by_index(
                ctx->history, ctx->history_position - 1, &entry);
            
            if (result == LLE_SUCCESS && entry) {
                lle_buffer_clear(ctx->buffer);
                lle_buffer_insert_text(ctx->buffer, 0, 
                                      entry->command, entry->command_length);
                ctx->buffer->cursor.byte_offset = entry->command_length;
            }
        }
        
        refresh_display(ctx);
    } else {
        // At newest entry
        if (config.lle_wrap_history_navigation) {
            // Wrap to oldest
            ctx->history_position = ctx->history_count;
            return handle_history_previous(event, user_data);
        }
        // Else stay at newest (already there)
    }
    
    return LLE_SUCCESS;
}
```

**Checkpoint**: Compiles, implements classic history next ✅

---

#### Task 2.5: Wire UP/DOWN Arrows to Handlers

**File**: `src/lle/lle_readline.c`  
**Location**: In event loop, SPECIAL_KEY case

**Find Existing Code**:
```c
case LLE_EVENT_TYPE_SPECIAL_KEY:
    switch (event.special_key.key) {
        case LLE_KEY_BACKSPACE:
            handle_backspace(&event, &ctx);
            break;
        case LLE_KEY_ENTER:
            handle_enter(&event, &ctx);
            break;
        // ... more cases
```

**Add After Existing Arrow Cases**:
```c
case LLE_EVENT_TYPE_SPECIAL_KEY:
    switch (event.special_key.key) {
        // Existing cases...
        
        case LLE_KEY_UP:
            // Check arrow key mode from config
            switch (config.lle_arrow_key_mode) {
                case LLE_ARROW_MODE_CLASSIC:
                case LLE_ARROW_MODE_ALWAYS_HISTORY:
                    handle_history_previous(&event, &ctx);
                    break;
                    
                case LLE_ARROW_MODE_CONTEXT_AWARE:
                    // Will implement in Phase 3
                    handle_history_previous(&event, &ctx);  // For now, always history
                    break;
                    
                case LLE_ARROW_MODE_MULTILINE_FIRST:
                    // Will implement in Phase 3
                    handle_history_previous(&event, &ctx);  // For now, always history
                    break;
            }
            break;
            
        case LLE_KEY_DOWN:
            // Check arrow key mode from config
            switch (config.lle_arrow_key_mode) {
                case LLE_ARROW_MODE_CLASSIC:
                case LLE_ARROW_MODE_ALWAYS_HISTORY:
                    handle_history_next(&event, &ctx);
                    break;
                    
                case LLE_ARROW_MODE_CONTEXT_AWARE:
                    // Will implement in Phase 3
                    handle_history_next(&event, &ctx);  // For now, always history
                    break;
                    
                case LLE_ARROW_MODE_MULTILINE_FIRST:
                    // Will implement in Phase 3
                    handle_history_next(&event, &ctx);  // For now, always history
                    break;
            }
            break;
        
        // Existing cases continue...
    }
```

**Checkpoint**: Compiles, UP/DOWN call handlers ✅

---

#### Task 2.6: Wire Ctrl-P/Ctrl-N to Handlers

**File**: `src/lle/lle_readline.c`  
**Location**: In Ctrl+key handling section

**Find Existing Ctrl+key Code**:
```c
case 'P':  /* Ctrl-P: Previous... (currently not used?) */
case 'N':  /* Ctrl-N: Next... (currently not used?) */
```

**Add/Update**:
```c
case 'N':  /* Ctrl-N: Next history */
    handle_history_next(NULL, &ctx);
    break;

case 'P':  /* Ctrl-P: Previous history */
    handle_history_previous(NULL, &ctx);
    break;
```

**Checkpoint**: Compiles, Ctrl-P/N call handlers ✅

---

#### Task 2.7: Cleanup on Exit

**File**: `src/lle/lle_readline.c`  
**Location**: In `lle_readline()` cleanup section

**Add**:
```c
cleanup:
    // Existing cleanup...
    
    // Clean up history navigation state
    if (ctx.saved_line) {
        free(ctx.saved_line);
        ctx.saved_line = NULL;
    }
    
    // Existing cleanup continues...
```

**Checkpoint**: No memory leaks ✅

---

### 3.3 Testing Phase 2

**Manual Testing**:
```bash
# Build
ninja -C builddir

# Test with LLE enabled
LLE_ENABLED=1 ./builddir/lusush

# Add some history entries
lusush> echo "test 1"
lusush> echo "test 2"
lusush> echo "test 3"
lusush> pwd

# Test UP arrow (should show: pwd)
lusush> [press UP]
# Should see: pwd

# Press UP again (should show: echo "test 3")
lusush> [press UP]
# Should see: echo "test 3"

# Press DOWN (should show: pwd)
lusush> [press DOWN]
# Should see: pwd

# Press DOWN again (should restore to empty/original)
lusush> [press DOWN]
# Should see: (empty or original line)

# Test Ctrl-P (same as UP)
lusush> [press Ctrl-P]
# Should show: pwd

# Test Ctrl-N (same as DOWN)
lusush> [press Ctrl-N]
# Should restore to empty

# Test wrapping (if enabled)
lusush> [press UP multiple times until oldest]
lusush> [press UP once more]
# Should wrap to newest (if lle.wrap_history_navigation = true)

# Test with classic mode
lusush> config set lle.arrow_key_mode classic
lusush> [test UP/DOWN again]
# Should work the same (we haven't implemented context-aware yet)
```

**Test with Saved Line**:
```bash
lusush> echo "typing something..."
lusush> [press UP before Enter]
# Should save "echo typing something..." and show previous history

lusush> [press DOWN until back to present]
# Should restore "echo typing something..."
```

### 3.4 Commit Phase 2

```bash
git add src/lle/lle_readline.c
git commit -m "LLE: Implement basic history navigation (UP/DOWN arrows, Ctrl-P/N)

Implement classic-mode history navigation:
- handle_history_previous() - Navigate to older entries
- handle_history_next() - Navigate to newer entries
- Wire UP/DOWN arrows to history handlers
- Wire Ctrl-P/N for Emacs-style navigation
- Save current line before first history navigation
- Restore saved line when returning to present
- Support history wrapping (configurable)

Configuration:
- lle.arrow_key_mode (currently only 'classic' implemented)
- lle.wrap_history_navigation
- lle.save_line_on_history_nav

Next: Phase 3 will add context-aware mode for multiline.

Part of LLE history integration plan.
Phase 2 complete."
```

### 3.5 Phase 2 Completion Checklist

- [ ] History state added to `readline_context_t`
- [ ] History state initialized in `lle_readline()`
- [ ] `handle_history_previous()` implemented
- [ ] `handle_history_next()` implemented
- [ ] UP/DOWN arrows wired to handlers
- [ ] Ctrl-P/N wired to handlers
- [ ] Cleanup code added
- [ ] Builds without errors
- [ ] UP arrow shows previous history
- [ ] DOWN arrow shows next history
- [ ] Ctrl-P/N work identically
- [ ] Saved line restored correctly
- [ ] Wrapping works (if enabled)
- [ ] Committed and pushed

**Phase 2 Complete**: ⬜

---

## 4. Phase 3: Context-Aware Multiline

**Goal**: Implement context-aware arrow key behavior for multiline editing  
**Estimated Time**: 2-3 hours  
**Status**: ⬜ Not Started  
**Depends On**: Phase 2 ✅

### 4.1 Prerequisites

**Need to Understand**:
- How to detect if buffer is multiline: `lle_buffer_is_multiline()`?
- How to detect cursor position (top/middle/bottom line)
- How to move cursor up/down one line

**Check Buffer API**:
```bash
grep -r "is_multiline\|get_line_number\|cursor.*line" include/lle/buffer*.h
```

### 4.2 Tasks

#### Task 3.1: Add Helper Functions for Cursor Position Detection

**File**: `src/lle/lle_readline.c`  
**Location**: Add helper functions before handlers

**Code to Add**:
```c
/**
 * @brief Check if cursor is at the top line of a multiline buffer
 */
static bool is_cursor_at_top_line(readline_context_t *ctx) {
    // If buffer is single-line, cursor is always at "top"
    if (!lle_buffer_is_multiline(ctx->buffer)) {
        return true;
    }
    
    // Check cursor's current line number
    // This depends on buffer API - may need to implement
    size_t current_line = lle_buffer_get_cursor_line(ctx->buffer);
    return (current_line == 0);
}

/**
 * @brief Check if cursor is at the bottom line of a multiline buffer
 */
static bool is_cursor_at_bottom_line(readline_context_t *ctx) {
    // If buffer is single-line, cursor is always at "bottom"
    if (!lle_buffer_is_multiline(ctx->buffer)) {
        return true;
    }
    
    // Check if on last line
    size_t current_line = lle_buffer_get_cursor_line(ctx->buffer);
    size_t total_lines = lle_buffer_get_line_count(ctx->buffer);
    return (current_line == total_lines - 1);
}
```

**Note**: These functions may need adaptation based on actual buffer API. Check what exists:
```bash
grep "lle_buffer_get" include/lle/buffer*.h
```

**Checkpoint**: Helper functions compile ✅

---

#### Task 3.2: Implement Vertical Cursor Movement

**File**: `src/lle/lle_readline.c`  
**Location**: Add new handler functions

**Code to Add**:
```c
/**
 * @brief Move cursor up one line (multiline editing)
 * Maintains "sticky" column position
 */
static lle_result_t handle_cursor_up_one_line(lle_event_t *event, void *user_data) {
    (void)event;
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Can't move up if at top
    if (is_cursor_at_top_line(ctx)) {
        return LLE_SUCCESS;
    }
    
    // Get current position
    size_t current_line = lle_buffer_get_cursor_line(ctx->buffer);
    size_t current_column = lle_buffer_get_cursor_column(ctx->buffer);
    
    // Move to previous line, same column (or end of line if shorter)
    lle_result_t result = lle_buffer_move_cursor_to_line_column(
        ctx->buffer, current_line - 1, current_column);
    
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}

/**
 * @brief Move cursor down one line (multiline editing)
 * Maintains "sticky" column position
 */
static lle_result_t handle_cursor_down_one_line(lle_event_t *event, void *user_data) {
    (void)event;
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Can't move down if at bottom
    if (is_cursor_at_bottom_line(ctx)) {
        return LLE_SUCCESS;
    }
    
    // Get current position
    size_t current_line = lle_buffer_get_cursor_line(ctx->buffer);
    size_t current_column = lle_buffer_get_cursor_column(ctx->buffer);
    
    // Move to next line, same column (or end of line if shorter)
    lle_result_t result = lle_buffer_move_cursor_to_line_column(
        ctx->buffer, current_line + 1, current_column);
    
    if (result == LLE_SUCCESS) {
        refresh_display(ctx);
    }
    
    return result;
}
```

**Note**: These functions depend on buffer API that may not exist yet. If `lle_buffer_get_cursor_line()` etc. don't exist, we'll need to implement them or find alternative approaches.

**Alternative if API doesn't exist**: Use byte offset navigation:
- Find newline characters before/after cursor
- Calculate position based on that

**Checkpoint**: Cursor movement functions compile ✅

---

#### Task 3.3: Implement Context-Aware UP Handler

**File**: `src/lle/lle_readline.c`  
**Location**: Add new function

**Code to Add**:
```c
/**
 * @brief Handle UP arrow in context-aware mode
 * - At top of multiline: navigate to previous history
 * - In middle/bottom of multiline: move cursor up one line
 * - In single-line: navigate to previous history
 */
static lle_result_t handle_arrow_up_context_aware(lle_event_t *event, void *user_data) {
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Check if multiline navigation is enabled
    if (!config.lle_enable_multiline_navigation) {
        // Multiline navigation disabled - always navigate history
        return handle_history_previous(event, user_data);
    }
    
    // Check if buffer is multiline
    if (!lle_buffer_is_multiline(ctx->buffer)) {
        // Single-line buffer - navigate history
        return handle_history_previous(event, user_data);
    }
    
    // Multiline buffer - check cursor position
    if (is_cursor_at_top_line(ctx)) {
        // At top line - navigate to previous history
        return handle_history_previous(event, user_data);
    } else {
        // In middle or bottom - move cursor up one line
        return handle_cursor_up_one_line(event, user_data);
    }
}
```

**Checkpoint**: Context-aware UP compiles ✅

---

#### Task 3.4: Implement Context-Aware DOWN Handler

**File**: `src/lle/lle_readline.c`  
**Location**: Add after context-aware UP

**Code to Add**:
```c
/**
 * @brief Handle DOWN arrow in context-aware mode
 * - At bottom of multiline: navigate to next history
 * - In top/middle of multiline: move cursor down one line
 * - In single-line: navigate to next history
 */
static lle_result_t handle_arrow_down_context_aware(lle_event_t *event, void *user_data) {
    readline_context_t *ctx = (readline_context_t *)user_data;
    
    // Check if multiline navigation is enabled
    if (!config.lle_enable_multiline_navigation) {
        // Multiline navigation disabled - always navigate history
        return handle_history_next(event, user_data);
    }
    
    // Check if buffer is multiline
    if (!lle_buffer_is_multiline(ctx->buffer)) {
        // Single-line buffer - navigate history
        return handle_history_next(event, user_data);
    }
    
    // Multiline buffer - check cursor position
    if (is_cursor_at_bottom_line(ctx)) {
        // At bottom line - navigate to next history
        return handle_history_next(event, user_data);
    } else {
        // In top or middle - move cursor down one line
        return handle_cursor_down_one_line(event, user_data);
    }
}
```

**Checkpoint**: Context-aware DOWN compiles ✅

---

#### Task 3.5: Update Arrow Key Handling to Use Context-Aware

**File**: `src/lle/lle_readline.c`  
**Location**: In event loop, LLE_KEY_UP/DOWN cases

**Update Existing Code**:
```c
case LLE_KEY_UP:
    switch (config.lle_arrow_key_mode) {
        case LLE_ARROW_MODE_CLASSIC:
        case LLE_ARROW_MODE_ALWAYS_HISTORY:
            handle_history_previous(&event, &ctx);
            break;
            
        case LLE_ARROW_MODE_CONTEXT_AWARE:
            handle_arrow_up_context_aware(&event, &ctx);
            break;
            
        case LLE_ARROW_MODE_MULTILINE_FIRST:
            // Prioritize multiline if in multiline buffer
            if (lle_buffer_is_multiline(ctx.buffer)) {
                handle_cursor_up_one_line(&event, &ctx);
            } else {
                handle_history_previous(&event, &ctx);
            }
            break;
    }
    break;
    
case LLE_KEY_DOWN:
    switch (config.lle_arrow_key_mode) {
        case LLE_ARROW_MODE_CLASSIC:
        case LLE_ARROW_MODE_ALWAYS_HISTORY:
            handle_history_next(&event, &ctx);
            break;
            
        case LLE_ARROW_MODE_CONTEXT_AWARE:
            handle_arrow_down_context_aware(&event, &ctx);
            break;
            
        case LLE_ARROW_MODE_MULTILINE_FIRST:
            // Prioritize multiline if in multiline buffer
            if (lle_buffer_is_multiline(ctx.buffer)) {
                handle_cursor_down_one_line(&event, &ctx);
            } else {
                handle_history_next(&event, &ctx);
            }
            break;
    }
    break;
```

**Checkpoint**: All arrow modes wired up ✅

---

### 4.3 Testing Phase 3

**Manual Testing**:
```bash
# Build
ninja -C builddir

# Test context-aware mode (default)
LLE_ENABLED=1 ./builddir/lusush

# Create multiline command (if supported)
lusush> for i in 1 2 3
> do
>   echo $i
> done

# Test UP arrow behavior:
# - At top line: should navigate to previous history
# - In middle lines: should move cursor up
# - At bottom line: should move cursor up

# Test with single-line
lusush> echo "test"
# UP should navigate history (not multiline)

# Test classic mode
lusush> config set lle.arrow_key_mode classic
lusush> for i in 1 2 3
> [press UP]
# Should navigate history, not move cursor vertically

# Test multiline-first mode
lusush> config set lle.arrow_key_mode multiline-first
lusush> for i in 1 2 3
> [press UP]
# Should move cursor up (not history)
# Use Ctrl-P for history
```

**Test Edge Cases**:
- Empty buffer + UP arrow
- Single line + multiline mode
- Transition from multiline to single-line
- Ctrl-P/N always work for history

### 4.4 Commit Phase 3

```bash
git add src/lle/lle_readline.c
git commit -m "LLE: Implement context-aware multiline arrow key navigation

Implement smart arrow key behavior for multiline editing:
- Context-aware mode (default):
  * UP at top of multiline: navigate to previous history
  * UP in middle/bottom: move cursor up one line
  * DOWN at bottom: navigate to next history
  * DOWN in top/middle: move cursor down one line

- Multiline-first mode:
  * UP/DOWN always move cursor in multiline
  * Use Ctrl-P/N for history

Helper functions:
- is_cursor_at_top_line() - Detect cursor at first line
- is_cursor_at_bottom_line() - Detect cursor at last line
- handle_cursor_up_one_line() - Vertical cursor movement
- handle_cursor_down_one_line() - Vertical cursor movement
- handle_arrow_up_context_aware() - Smart UP behavior
- handle_arrow_down_context_aware() - Smart DOWN behavior

Respects config options:
- lle.arrow_key_mode
- lle.enable_multiline_navigation

This implements LLE's advanced multiline editing capability,
going beyond GNU Readline's single-line limitations.

Part of LLE history integration plan.
Phase 3 complete."
```

### 4.5 Phase 3 Completion Checklist

- [ ] Helper functions for cursor position detection
- [ ] Vertical cursor movement handlers
- [ ] Context-aware UP handler
- [ ] Context-aware DOWN handler
- [ ] Multiline-first mode implementation
- [ ] Arrow key handling updated for all modes
- [ ] Builds without errors
- [ ] Context-aware mode works in multiline
- [ ] Classic mode still works
- [ ] Multiline-first mode works
- [ ] Ctrl-P/N always navigate history
- [ ] Committed and pushed

**Phase 3 Complete**: ⬜

---

## 5. Phase 4: Testing and Documentation

**Goal**: Comprehensive testing and user documentation  
**Estimated Time**: 2-3 hours  
**Status**: ⬜ Not Started  
**Depends On**: Phases 1-3 ✅

### 5.1 Tasks

#### Task 4.1: Create History Navigation Test Suite

**File**: Create `tests/lle/keybinding/test_history_navigation.c`

**Test Coverage**:
```c
// Test 1: Basic UP/DOWN navigation
void test_basic_history_navigation(void);

// Test 2: Ctrl-P/N navigation
void test_ctrl_p_n_navigation(void);

// Test 3: History wrapping
void test_history_wrapping(void);

// Test 4: Saved line restoration
void test_saved_line_restoration(void);

// Test 5: Empty history
void test_empty_history(void);

// Test 6: Context-aware mode (single-line)
void test_context_aware_single_line(void);

// Test 7: Context-aware mode (multiline)
void test_context_aware_multiline(void);

// Test 8: Classic mode
void test_classic_mode(void);

// Test 9: Multiline-first mode
void test_multiline_first_mode(void);

// Test 10: Config mode switching
void test_config_mode_switching(void);
```

**Checkpoint**: Test suite exists ✅

---

#### Task 4.2: Add to Keybinding Test Tracker

**File**: `docs/lle_implementation/KEYBINDING_TEST_TRACKER.md`  
**Location**: Add new section

**Add**:
```markdown
### Test 18: History Navigation - UP/DOWN Arrows

#### ✅ Test 18.1: Basic UP Navigation
**Steps**: 
1. Add commands to history: `echo "1"`, `echo "2"`, `echo "3"`
2. Fresh prompt
3. Press UP

**Expected**: Shows `echo "3"` (most recent)  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

#### ✅ Test 18.2: Basic DOWN Navigation
**Steps**: 
1. From Test 18.1 state (showing `echo "3"`)
2. Press DOWN

**Expected**: Returns to empty prompt  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

#### ✅ Test 18.3: Multiple UP Navigation
**Steps**: 
1. Press UP three times

**Expected**: Shows `echo "3"`, then `echo "2"`, then `echo "1"`  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

#### ✅ Test 18.4: Saved Line Restoration
**Steps**: 
1. Type: `echo "typing"`
2. Press UP (don't press Enter)
3. Press DOWN until back to present

**Expected**: Shows `echo "typing"` (saved line restored)  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

#### ✅ Test 18.5: Ctrl-P/N Navigation
**Steps**: 
1. Press Ctrl-P (previous history)
2. Press Ctrl-N (next history)

**Expected**: Same behavior as UP/DOWN  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

### Test 19: Context-Aware Multiline Navigation

#### ✅ Test 19.1: Multiline Command Recall
**Steps**: 
1. Execute multiline command:
   ```
   for i in 1 2 3
   do
     echo $i
   done
   ```
2. Press UP to recall

**Expected**: Command recalled with multiline structure  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

#### ✅ Test 19.2: Vertical Navigation in Multiline
**Steps**: 
1. From Test 19.1 state (multiline command recalled)
2. Cursor on line 3 (`echo $i`)
3. Press UP

**Expected**: Cursor moves to line 2 (`do`)  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: Requires context-aware mode

#### ✅ Test 19.3: History from Top of Multiline
**Steps**: 
1. Multiline command recalled, cursor on line 1
2. Press UP

**Expected**: Navigate to previous history entry  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: Context-aware mode

#### ✅ Test 19.4: History from Bottom of Multiline
**Steps**: 
1. Multiline command recalled, cursor on last line
2. Press DOWN

**Expected**: Navigate to next history entry  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: Context-aware mode

### Test 20: Configuration Modes

#### ✅ Test 20.1: Classic Mode (GNU Readline)
**Steps**: 
1. `config set lle.arrow_key_mode classic`
2. Recall multiline command
3. Press UP

**Expected**: Navigate to previous history (not vertical movement)  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: 

#### ✅ Test 20.2: Multiline-First Mode
**Steps**: 
1. `config set lle.arrow_key_mode multiline-first`
2. Recall multiline command
3. Press UP

**Expected**: Move cursor up vertically (not history)  
**Result**: ⬜ Not Tested | ✅ Pass | ❌ Fail  
**Notes**: Use Ctrl-P for history in this mode
```

**Checkpoint**: Test tracker updated ✅

---

#### Task 4.3: Update User Documentation

**File**: Create `docs/user_guide/HISTORY_NAVIGATION.md`

**Content**:
```markdown
# LLE History Navigation Guide

## Overview

Lusush's Line Editing Engine (LLE) provides powerful history navigation
with support for multiline commands and configurable arrow key behavior.

## Basic History Navigation

### UP/DOWN Arrows

- **UP arrow** (↑): Navigate to previous (older) command
- **DOWN arrow** (↓): Navigate to next (newer) command

### Ctrl-P/Ctrl-N (Emacs-style)

- **Ctrl-P**: Previous history (same as UP in classic mode)
- **Ctrl-N**: Next history (same as DOWN in classic mode)

**Note**: Ctrl-P/N **always** navigate history regardless of mode.

## Arrow Key Modes

LLE offers four arrow key behavior modes:

### 1. Context-Aware (Default - Recommended)

Smart behavior based on cursor position:

**Single-line commands:**
- UP/DOWN = history navigation

**Multiline commands:**
- At top line: UP = previous history
- In middle lines: UP = move cursor up one line
- At bottom line: DOWN = next history
- Use Ctrl-P/N for history navigation from any line

**Example:**
```bash
# Recall a for loop
for i in 1 2 3
do          # ← Cursor here
  echo $i
done

# Press UP: cursor moves to "for i in 1 2 3" line
# Press UP again: navigate to previous history

# Or use Ctrl-P at any time for history
```

### 2. Classic (GNU Readline Compatible)

Traditional readline behavior:

- UP/DOWN **always** navigate history
- No vertical cursor movement in multiline
- Maximum compatibility with bash/readline

**Configuration:**
```bash
config set lle.arrow_key_mode classic
```

### 3. Always-History

Similar to classic, explicit naming:

- UP/DOWN = history only
- No multiline vertical navigation

**Configuration:**
```bash
config set lle.arrow_key_mode always-history
```

### 4. Multiline-First

Prioritizes multiline editing:

- In multiline commands: UP/DOWN = vertical cursor movement
- In single-line: UP/DOWN = history
- **Always use Ctrl-P/N for history**

**Configuration:**
```bash
config set lle.arrow_key_mode multiline-first
```

## Configuration

### Quick Config

Edit `~/.lusushrc`:

```ini
[history]
# Arrow key behavior
lle.arrow_key_mode = context-aware  # or: classic, always-history, multiline-first

# Enable multiline vertical navigation
lle.enable_multiline_navigation = true

# Save current line when navigating history
lle.save_line_on_history_nav = true

# Wrap at history boundaries
lle.wrap_history_navigation = true
```

### Runtime Config

Change settings without restarting:

```bash
# Show current mode
config get lle.arrow_key_mode

# Set mode
config set lle.arrow_key_mode classic

# Disable multiline navigation
config set lle.enable_multiline_navigation false
```

### Presets

Apply preset configurations:

```bash
# Maximum GNU Readline compatibility
config preset history readline-compatible

# Power user (all features)
config preset history power-user
```

## Tips and Tricks

### Save Your Work

Before navigating history, your current line is automatically saved:

```bash
# Start typing
echo "important command I'm working on..."

# Oops, need to check history
[press UP]  # Current line saved

# Navigate history, find what you need
[press UP/DOWN to browse]

# Return to your work
[press DOWN until back to present]
# Your "important command..." is restored!
```

### Quick History Search

Use Ctrl-R for reverse search:

```bash
[press Ctrl-R]
# Type search term
# Press Ctrl-R again to find next match
```

### Multiline Command Recall

Multiline commands are recalled with original structure:

```bash
# Execute a function
function greet() {
    echo "Hello, $1"
}

# Later, press UP to recall
# You'll see the full function with formatting preserved!
```

## Troubleshooting

### "Arrow keys don't move cursor vertically in multiline"

**Check mode:**
```bash
config get lle.arrow_key_mode
# Should be "context-aware" or "multiline-first"
```

**Check multiline navigation:**
```bash
config get lle.enable_multiline_navigation
# Should be "true"
```

### "I want exactly bash behavior"

**Use readline-compatible preset:**
```bash
config preset history readline-compatible
```

**Or set manually:**
```bash
config set lle.arrow_key_mode classic
config set lle.enable_multiline_navigation false
```

### "History wraps unexpectedly"

**Disable wrapping:**
```bash
config set lle.wrap_history_navigation false
```

## See Also

- [Configuration Guide](./CONFIGURATION.md)
- [Keybinding Reference](./KEYBINDINGS.md)
- [History System](./HISTORY.md)
```

**Checkpoint**: User docs created ✅

---

#### Task 4.4: Update AI_ASSISTANT_HANDOFF_DOCUMENT.md

**File**: `AI_ASSISTANT_HANDOFF_DOCUMENT.md`  
**Location**: Add new section at top

**Add**:
```markdown
## ✅ LLE HISTORY NAVIGATION IMPLEMENTATION (2025-11-07)

### Implementation Complete

Implemented comprehensive history navigation system with configurable
arrow key behavior for LLE:

**Features Implemented:**
1. **Basic History Navigation**:
   - UP/DOWN arrow keys for history navigation
   - Ctrl-P/N Emacs-style navigation
   - History wrapping (configurable)
   - Saved line restoration

2. **Context-Aware Multiline**:
   - Smart arrow key behavior in multiline commands
   - Vertical cursor movement with sticky columns
   - History navigation from multiline boundaries
   - Four configurable modes

3. **Configuration System**:
   - 23 config options integrated with Lusush config
   - Four arrow key modes (context-aware, classic, etc.)
   - Multiline, search, storage, and performance settings
   - Built-in presets for common use cases

**Files Modified:**
- `include/config.h` - Added LLE history config options
- `src/config.c` - Added validators, defaults, template
- `src/lle/lle_readline.c` - Implemented history navigation handlers

**Configuration Added:**
- Arrow key modes: context-aware (default), classic, always-history, multiline-first
- Multiline navigation settings
- History wrapping and save behavior
- Storage modes (LLE, bash, dual)
- Search configuration
- Advanced features (forensic tracking, deduplication)
- Performance settings

**Testing:**
- Manual testing complete
- All arrow key modes verified
- Multiline navigation tested
- Config switching verified

**Documentation:**
- Configuration spec: docs/development/LLE_HISTORY_CONFIG_SPECIFICATION.md
- Implementation plan: docs/development/LLE_HISTORY_IMPLEMENTATION_PLAN.md
- User guide: docs/user_guide/HISTORY_NAVIGATION.md
- Test tracker updated

**Next Steps:**
- Optional: Implement Ctrl-R interactive search (if not already working)
- Optional: Implement history file format migration tool
- Optional: Add fuzzy search support
```

**Checkpoint**: Handoff doc updated ✅

---

### 5.2 Testing Phase 4

Execute full test suite:

```bash
# Run manual tests from KEYBINDING_TEST_TRACKER.md
# Tests 18.1 through 20.2

# Run automated tests (if created)
./builddir/test_history_navigation

# Performance testing
# - Large history (10,000 entries)
# - Fast navigation (UP/DOWN repeatedly)
# - Memory leak check (valgrind)
```

### 5.3 Commit Phase 4

```bash
git add tests/ docs/
git commit -m "LLE: Add comprehensive history navigation testing and documentation

Testing:
- Created test suite for history navigation
- Added tests to KEYBINDING_TEST_TRACKER.md
- Verified all arrow key modes
- Tested multiline navigation
- Verified config switching

Documentation:
- User guide: HISTORY_NAVIGATION.md
- Updated AI_ASSISTANT_HANDOFF_DOCUMENT.md
- Test coverage in tracker

All tests passing. History navigation feature complete.

Part of LLE history integration plan.
Phase 4 complete. Implementation finished."
```

### 5.4 Phase 4 Completion Checklist

- [ ] Test suite created
- [ ] Test tracker updated
- [ ] User guide written
- [ ] Handoff doc updated
- [ ] All manual tests passing
- [ ] All automated tests passing
- [ ] Performance acceptable
- [ ] Documentation complete
- [ ] Committed and pushed

**Phase 4 Complete**: ⬜

---

## 6. Progress Tracking

### Overall Progress

| Phase | Status | Estimated | Actual | Notes |
|-------|--------|-----------|--------|-------|
| Phase 1: Configuration | ⬜ Not Started | 1-2h | - | Add config options |
| Phase 2: Basic Navigation | ⬜ Not Started | 4-6h | - | UP/DOWN history |
| Phase 3: Context-Aware | ⬜ Not Started | 2-3h | - | Multiline support |
| Phase 4: Testing & Docs | ⬜ Not Started | 2-3h | - | Complete testing |
| **TOTAL** | **0/4** | **9-14h** | **-** | **1.5-2 days** |

### Completion Tracking

```
[□] Phase 1 Complete
[□] Phase 2 Complete
[□] Phase 3 Complete
[□] Phase 4 Complete
[□] ALL PHASES COMPLETE ✨
```

---

## 7. Testing Checklist

### Unit Tests
- [ ] Config option loading
- [ ] Config validation
- [ ] History previous handler
- [ ] History next handler
- [ ] Context-aware logic
- [ ] Cursor position detection
- [ ] Vertical cursor movement

### Integration Tests
- [ ] UP/DOWN navigation works
- [ ] Ctrl-P/N navigation works
- [ ] History wrapping works
- [ ] Saved line restoration works
- [ ] Mode switching works
- [ ] Multiline vertical navigation works

### Manual Tests
- [ ] All tests in KEYBINDING_TEST_TRACKER.md pass
- [ ] Config show/get/set works
- [ ] All arrow key modes work
- [ ] Edge cases handled

### Performance Tests
- [ ] Large history (10,000 entries)
- [ ] Fast navigation responsive
- [ ] No memory leaks
- [ ] Config loading fast

---

## 8. Rollback Plan

### If Phase Fails

**Phase 1 Rollback:**
```bash
git revert HEAD  # If committed
# or
git checkout include/config.h src/config.c  # If not committed
```

**Phase 2 Rollback:**
```bash
git revert HEAD~1..HEAD  # Revert both Phase 2 and Phase 1 if needed
```

**Phase 3 Rollback:**
```bash
# Can rollback to Phase 2 state (basic history still works)
git revert HEAD
```

### Known Risks

**Risk 1: Buffer API Missing Functions**
- **Impact**: Cannot implement vertical cursor movement
- **Mitigation**: Implement missing buffer functions or use workarounds
- **Fallback**: Defer Phase 3, deliver Phases 1-2 only

**Risk 2: History API Different Than Expected**
- **Impact**: Handlers need adjustment
- **Mitigation**: Adapt to actual API during Phase 2
- **Fallback**: Worst case, stub out with TODO comments

**Risk 3: Performance Issues**
- **Impact**: History navigation feels slow
- **Mitigation**: Add caching, optimize lookups
- **Fallback**: Disable features temporarily, optimize in follow-up

---

## 9. Success Criteria

### Phase 1 Success
- ✅ Config options show in `config show history`
- ✅ Can get/set all options
- ✅ Validation works
- ✅ Compiles without errors

### Phase 2 Success
- ✅ UP arrow shows previous history
- ✅ DOWN arrow shows next history
- ✅ Ctrl-P/N work identically
- ✅ Saved line restores correctly
- ✅ Classic mode fully functional

### Phase 3 Success
- ✅ Context-aware mode works in multiline
- ✅ Vertical cursor movement works
- ✅ All four arrow modes work
- ✅ Ctrl-P/N always navigate history

### Overall Success
- ✅ All 4 phases complete
- ✅ All tests passing
- ✅ Documentation complete
- ✅ No regressions in existing keybindings
- ✅ Performance acceptable (<100ms navigation)
- ✅ User guide clear and accurate

---

## 10. Post-Implementation Tasks

### Optional Enhancements (Future Work)

1. **Ctrl-R Interactive Search**
   - Check if `history_interactive_search.c` is complete
   - Wire up Ctrl-R if needed
   - Add search UI

2. **History File Migration Tool**
   - `lusush --migrate-history` command
   - Import from `.bash_history`
   - Export to bash format

3. **Fuzzy Search**
   - Implement fuzzy matching algorithm
   - Wire to search with `lle.search_fuzzy_matching`

4. **History Statistics**
   - `history stats` command
   - Most used commands
   - Execution time tracking

5. **History Editing**
   - `history edit N` command
   - Edit and re-execute
   - Delete specific entries

---

## Quick Start Guide

### For Next Session

**To begin implementation:**

```bash
# 1. Navigate to project
cd /home/mberry/Lab/c/lusush

# 2. Ensure on correct branch
git checkout feature/lle

# 3. Pull latest changes
git pull origin feature/lle

# 4. Read this document
cat docs/development/LLE_HISTORY_IMPLEMENTATION_PLAN.md

# 5. Start with Phase 1
# Open include/config.h
# Follow Task 1.1 in Phase 1 section above
```

**To resume after interruption:**

1. Check "Progress Tracking" section (Section 6)
2. Find last completed phase
3. Go to next uncompleted task
4. Follow checklist for that task

**To verify what's done:**

```bash
# Check commit history
git log --oneline | grep "LLE:"

# Look for commits like:
# "LLE: Add history system configuration options" = Phase 1 done
# "LLE: Implement basic history navigation" = Phase 2 done
# etc.
```

---

**END OF IMPLEMENTATION PLAN**

This document will be updated as phases complete.
Current status preserved in "Progress Tracking" section.

# Spec 25: Default Keybindings - Complete Implementation Plan

**Document Version**: 1.0.0  
**Date**: 2025-11-02  
**Based on**: Spec 25 Default Keybindings Complete Specification (1300 lines)  
**Status**: Planning - Ready to Begin Implementation

---

## Executive Summary

This implementation plan provides a complete roadmap for implementing Spec 25: Default Keybindings, the critical user experience component that provides 100% GNU Readline compatibility with Emacs-style bindings as the default mode. This specification ensures zero regression in functionality while enabling enhanced multiline command editing.

**Key Deliverables**:
- Kill ring system for yank/kill operations
- Keybinding engine with hashtable-based lookup (< 50μs)
- Complete Emacs-style keybindings (40+ bindings)
- Vi mode support (optional, future enhancement)
- GNU Readline 100% compatibility

**Total Estimated Effort**: 2-3 weeks  
**Total Estimated LOC**: ~2,500-3,500 lines of production code + tests

---

## Table of Contents

1. [Dependencies Analysis](#1-dependencies-analysis)
2. [Core Components](#2-core-components)
3. [Phase 1: Kill Ring System](#3-phase-1-kill-ring-system)
4. [Phase 2: Keybinding Engine](#4-phase-2-keybinding-engine)
5. [Phase 3: Default Bindings](#5-phase-3-default-bindings)
6. [Phase 4: Testing and Validation](#6-phase-4-testing-and-validation)
7. [Success Criteria](#7-success-criteria)
8. [Implementation Timeline](#8-implementation-timeline)

---

## 1. Dependencies Analysis

### 1.1 Required Existing Systems

#### Buffer Management (Spec 03) ✅ COMPLETE
**Required APIs**:
- `lle_buffer_t` - Core buffer structure
- `lle_cursor_move_*()` - Cursor movement functions
- `lle_buffer_insert_text()` - Text insertion
- `lle_buffer_delete_char()` - Character deletion
- `lle_buffer_is_multiline()` - Multiline detection

**Status**: All required APIs implemented and tested

#### History System (Spec 09) ✅ COMPLETE
**Required APIs**:
- `lle_history_core_t` - History system
- `lle_history_previous()` - History navigation
- `lle_history_next()` - History navigation
- `lle_history_search()` - Interactive search (Ctrl-R)

**Status**: All required APIs implemented (Phase 3 Day 9-10)

#### Display Integration (Spec 08) ✅ COMPLETE
**Required APIs**:
- `lle_display_refresh()` - Display update after key action
- `lle_display_clear_screen()` - Screen clear (Ctrl-L)

**Status**: Complete display system available

#### Event System (Spec 04) ✅ COMPLETE
**Required APIs**:
- `lle_key_event_t` - Key event structure
- Event processing infrastructure

**Status**: Complete event system with key events

#### Memory Management (Spec 15) ✅ COMPLETE
**Required APIs**:
- `lle_memory_pool_t` - Memory pool
- `lle_pool_alloc()` - Allocation

**Status**: Complete memory management system

#### libhashtable (Spec 05) ✅ COMPLETE
**Required APIs**:
- `lle_hashtable_t` - Hash table for keybinding lookup
- `lle_hashtable_insert()`, `lle_hashtable_lookup()`

**Status**: Complete wrapper with thread safety

### 1.2 New Components to Implement

All components listed below are NEW and must be implemented as part of Spec 25:

1. **Kill Ring System** (lle_kill_ring.h/c)
   - Circular buffer for killed text
   - Yank and yank-pop operations
   - Maximum 32 entries by default

2. **Keybinding Manager** (lle_keybinding_manager.h/c)
   - Hashtable-based keybinding storage
   - Key sequence processing
   - Action execution framework

3. **Keybinding Actions** (lle_keybinding_actions.h/c)
   - All 40+ GNU Readline compatible functions
   - Movement, editing, history, completion operations

---

## 2. Core Components

### 2.1 Kill Ring Structure

```c
// File: include/lle/kill_ring.h
typedef struct lle_kill_ring {
    char **entries;              // Array of killed text entries
    size_t max_entries;          // Maximum entries (32 default)
    size_t current_index;        // Current position for yank-pop
    size_t entry_count;          // Number of entries stored
    bool last_was_yank;          // Track if last operation was yank
    lle_memory_pool_t *pool;     // Memory pool for allocations
} lle_kill_ring_t;
```

**Size**: ~64 bytes + dynamic entry storage  
**Estimated LOC**: 300-400 lines

### 2.2 Keybinding Manager Structure

```c
// File: include/lle/keybinding_manager.h
typedef struct lle_keybinding_manager {
    // Keymaps (hash tables)
    lle_hashtable_t *emacs_keymap;       // Emacs-style bindings
    lle_hashtable_t *vi_normal_keymap;   // Vi normal mode (future)
    lle_hashtable_t *vi_insert_keymap;   // Vi insert mode (future)
    
    // Active keymap
    lle_hashtable_t *active_keymap;
    
    // Key sequence buffering
    char sequence_buffer[256];
    size_t sequence_length;
    uint64_t last_key_time;
    uint32_t chord_timeout_ms;           // Default 100ms
    
    // Memory management
    lle_memory_pool_t *memory_pool;
} lle_keybinding_manager_t;
```

**Size**: ~320 bytes + hashtable storage  
**Estimated LOC**: 500-600 lines

### 2.3 Key Binding Structure

```c
// File: include/lle/keybinding_manager.h
typedef enum {
    LLE_ACTION_BUILTIN,     // Built-in LLE function
    LLE_ACTION_WIDGET,      // User widget (Spec 24)
    LLE_ACTION_SCRIPT,      // Script function (future)
    LLE_ACTION_PLUGIN,      // Plugin action (future)
} lle_action_type_t;

typedef struct lle_key_binding {
    const char *key_sequence;        // "C-a", "M-f", etc.
    lle_action_type_t action_type;
    
    union {
        struct {
            lle_result_t (*function)(lle_editor_t *editor);
            const char *name;        // For debugging/introspection
        } builtin;
        
        void *widget;                // For Spec 24 integration
        void *script;                // For future script integration
        void *plugin;                // For future plugin integration
    } action;
    
    // Statistics
    uint64_t use_count;
    uint64_t last_used;
} lle_key_binding_t;
```

**Size**: ~80 bytes per binding

---

## 3. Phase 1: Kill Ring System

### 3.1 Kill Ring Implementation

**File**: `include/lle/kill_ring.h`, `src/lle/kill_ring.c`  
**Estimated LOC**: 300-400 lines  
**Time Estimate**: 2-3 days

#### Core Functions

```c
// Lifecycle
lle_result_t lle_kill_ring_create(
    lle_kill_ring_t **ring,
    size_t max_entries,
    lle_memory_pool_t *pool);

lle_result_t lle_kill_ring_destroy(lle_kill_ring_t *ring);

// Kill operations
lle_result_t lle_kill_ring_add(
    lle_kill_ring_t *ring,
    const char *text,
    bool append);  // true to append to last entry

// Yank operations
lle_result_t lle_kill_ring_get_current(
    lle_kill_ring_t *ring,
    const char **text);

lle_result_t lle_kill_ring_yank_pop(
    lle_kill_ring_t *ring,
    const char **text);

// State management
lle_result_t lle_kill_ring_clear(lle_kill_ring_t *ring);
lle_result_t lle_kill_ring_reset_yank_state(lle_kill_ring_t *ring);
```

#### Implementation Details

1. **Circular Buffer**: Oldest entries are overwritten when full
2. **Append Mode**: Successive kills (C-k, C-k) append to same entry
3. **Yank-Pop**: M-y cycles through kill ring after C-y
4. **Memory Management**: All strings allocated from memory pool

### 3.2 Build Integration

**File**: `src/lle/meson.build`

Add kill_ring module:
```python
lle_sources += files('kill_ring.c')
```

### 3.3 Testing

**File**: `tests/lle/unit/test_kill_ring.c`  
**Estimated LOC**: 200-300 lines

Test coverage:
- Create/destroy lifecycle
- Add entries (normal and append mode)
- Get current entry
- Yank-pop cycling
- Circular buffer overflow handling
- Edge cases (empty ring, single entry, full ring)

---

## 4. Phase 2: Keybinding Engine

### 4.1 Keybinding Manager Implementation

**File**: `include/lle/keybinding_manager.h`, `src/lle/keybinding_manager.c`  
**Estimated LOC**: 500-600 lines  
**Time Estimate**: 4-5 days

#### Core Functions

```c
// Lifecycle
lle_result_t lle_keybinding_manager_create(
    lle_keybinding_manager_t **manager,
    lle_memory_pool_t *pool);

lle_result_t lle_keybinding_manager_destroy(
    lle_keybinding_manager_t *manager);

// Keybinding registration
lle_result_t lle_keybinding_manager_bind(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_result_t (*function)(lle_editor_t*),
    const char *function_name);

lle_result_t lle_keybinding_manager_unbind(
    lle_keybinding_manager_t *manager,
    const char *key_sequence);

// Key processing
lle_result_t lle_keybinding_manager_process_key(
    lle_keybinding_manager_t *manager,
    lle_editor_t *editor,
    const lle_key_event_t *key_event);

// Preset loading
lle_result_t lle_keybinding_manager_load_emacs_preset(
    lle_keybinding_manager_t *manager);

// Introspection
lle_result_t lle_keybinding_manager_list_bindings(
    lle_keybinding_manager_t *manager,
    lle_key_binding_t ***bindings_out,
    size_t *count_out);
```

#### Key Sequence Format

Standard GNU Readline notation:
- `C-a` = Ctrl-A
- `M-f` = Meta-F (Alt-F or ESC f)
- `C-x C-s` = Ctrl-X Ctrl-S (chord)
- `UP`, `DOWN`, `LEFT`, `RIGHT` = Arrow keys
- `RET`, `TAB`, `DEL` = Special keys

#### Performance Requirements

- **Lookup**: < 50 microseconds per key
- **Hashtable**: O(1) average lookup time
- **Memory**: < 1MB for complete keybinding system

### 4.2 Build Integration

**File**: `src/lle/meson.build`

Add keybinding_manager module:
```python
lle_sources += files('keybinding_manager.c')
```

### 4.3 Testing

**File**: `tests/lle/unit/test_keybinding_manager.c`  
**Estimated LOC**: 300-400 lines

Test coverage:
- Create/destroy lifecycle
- Bind/unbind operations
- Key lookup (simple keys, control keys, meta keys, chords)
- Emacs preset loading (all 40+ bindings)
- Performance validation (< 50μs lookup)
- Edge cases (unknown keys, duplicate bindings)

---

## 5. Phase 3: Default Bindings

### 5.1 Keybinding Actions Implementation

**File**: `include/lle/keybinding_actions.h`, `src/lle/keybinding_actions.c`  
**Estimated LOC**: 1,500-2,000 lines  
**Time Estimate**: 5-7 days

#### Movement Actions (10 functions)

```c
lle_result_t lle_beginning_of_line(lle_editor_t *editor);
lle_result_t lle_end_of_line(lle_editor_t *editor);
lle_result_t lle_forward_char(lle_editor_t *editor);
lle_result_t lle_backward_char(lle_editor_t *editor);
lle_result_t lle_forward_word(lle_editor_t *editor);
lle_result_t lle_backward_word(lle_editor_t *editor);
```

#### Editing Actions (15 functions)

```c
lle_result_t lle_delete_char(lle_editor_t *editor);
lle_result_t lle_backward_delete_char(lle_editor_t *editor);
lle_result_t lle_kill_line(lle_editor_t *editor);
lle_result_t lle_backward_kill_line(lle_editor_t *editor);
lle_result_t lle_kill_word(lle_editor_t *editor);
lle_result_t lle_backward_kill_word(lle_editor_t *editor);
lle_result_t lle_yank(lle_editor_t *editor);
lle_result_t lle_yank_pop(lle_editor_t *editor);
lle_result_t lle_transpose_chars(lle_editor_t *editor);
lle_result_t lle_transpose_words(lle_editor_t *editor);
```

#### History Actions (6 functions)

```c
lle_result_t lle_history_previous(lle_editor_t *editor);
lle_result_t lle_history_next(lle_editor_t *editor);
lle_result_t lle_reverse_search_history(lle_editor_t *editor);
lle_result_t lle_forward_search_history(lle_editor_t *editor);
lle_result_t lle_history_search_backward(lle_editor_t *editor);
lle_result_t lle_history_search_forward(lle_editor_t *editor);
```

#### Completion Actions (3 functions)

```c
lle_result_t lle_complete(lle_editor_t *editor);
lle_result_t lle_possible_completions(lle_editor_t *editor);
lle_result_t lle_insert_completions(lle_editor_t *editor);
```

#### Critical Operations (5 functions)

```c
lle_result_t lle_abort_line(lle_editor_t *editor);
lle_result_t lle_accept_line(lle_editor_t *editor);
lle_result_t lle_clear_screen(lle_editor_t *editor);
lle_result_t lle_self_insert(lle_editor_t *editor, const char *text);
lle_result_t lle_quoted_insert(lle_editor_t *editor);
```

#### Case Operations (3 functions)

```c
lle_result_t lle_upcase_word(lle_editor_t *editor);
lle_result_t lle_downcase_word(lle_editor_t *editor);
lle_result_t lle_capitalize_word(lle_editor_t *editor);
```

**Total**: 42 action functions

### 5.2 Editor Structure Extension

**Note**: `lle_editor_t` doesn't exist yet - it will be part of a future integration spec. For now, we'll define a minimal structure that provides access to required subsystems:

```c
// File: include/lle/keybinding_actions.h (temporary definition)
typedef struct lle_editor {
    lle_buffer_t *buffer;
    lle_history_core_t *history;
    lle_kill_ring_t *kill_ring;
    lle_keybinding_manager_t *keybinding_manager;
    // More fields will be added by future specs
} lle_editor_t;
```

### 5.3 Build Integration

**File**: `src/lle/meson.build`

Add keybinding_actions module:
```python
lle_sources += files('keybinding_actions.c')
```

### 5.4 Testing

**File**: `tests/lle/functional/test_keybinding_actions.c`  
**Estimated LOC**: 600-800 lines

Test coverage (one test per action function):
- Movement actions (10 tests)
- Editing actions (15 tests)
- History actions (6 tests)
- Completion actions (3 tests - stubs for now)
- Critical operations (5 tests)
- Case operations (3 tests)

**Total**: 42 functional tests

---

## 6. Phase 4: Testing and Validation

### 6.1 Compliance Test

**File**: `tests/lle/compliance/spec_25_keybinding_compliance.c`  
**Estimated LOC**: 200-300 lines

Header-only compliance test verifying:
- Kill ring API declarations
- Keybinding manager API declarations
- All 42 action function declarations

### 6.2 Integration Test

**File**: `tests/lle/integration/test_keybinding_integration.c`  
**Estimated LOC**: 400-500 lines

End-to-end tests:
- Load Emacs preset
- Process key sequences
- Verify correct actions execute
- GNU Readline compatibility validation
- Performance benchmarking (< 50μs lookup)

### 6.3 GNU Readline Compatibility Validation

Manual testing checklist:
- [ ] All Ctrl- keys work identically to bash
- [ ] All Meta- keys work identically to bash
- [ ] Arrow keys work for history and movement
- [ ] Kill ring (C-k, C-y, M-y) works correctly
- [ ] History search (C-r, C-s) works correctly
- [ ] Tab completion works (calls stub for now)
- [ ] Multiline commands work correctly

---

## 7. Success Criteria

### 7.1 Functional Requirements

✅ All 42 GNU Readline action functions implemented  
✅ Kill ring system operational (32 entry circular buffer)  
✅ Keybinding engine with hashtable lookup  
✅ Emacs preset loads all default bindings  
✅ Zero regression from bash/zsh behavior  

### 7.2 Performance Requirements

✅ Keybinding lookup < 50 microseconds  
✅ Action execution < 100 microseconds (built-in)  
✅ Memory usage < 1MB for complete system  
✅ Startup time < 5 milliseconds (load Emacs preset)  

### 7.3 Quality Requirements

✅ Zero compiler warnings  
✅ Zero memory leaks (valgrind verified)  
✅ 100% test pass rate  
✅ Compliance test passes  
✅ Integration tests pass  

---

## 8. Implementation Timeline

### Week 1: Kill Ring + Keybinding Engine Foundation

**Days 1-2**: Kill Ring System
- Implement kill_ring.h/c
- Unit tests
- Build integration

**Days 3-4**: Keybinding Manager Core
- Implement keybinding_manager.h/c
- Basic bind/unbind functionality
- Unit tests

**Day 5**: Key Sequence Processing
- Key event to string formatting
- Hashtable lookup integration
- Action execution framework

### Week 2: Default Keybinding Actions

**Days 1-2**: Movement + Editing Actions (25 functions)
- Implement all movement actions
- Implement all editing actions (including kill/yank)
- Functional tests

**Days 3-4**: History + Completion + Critical Actions (14 functions)
- Implement history navigation
- Implement completion stubs
- Implement critical operations
- Functional tests

**Day 5**: Case Operations + Emacs Preset (3 functions)
- Implement case operations
- Emacs preset loading
- Complete default binding set

### Week 3: Testing + Integration

**Days 1-2**: Testing
- Compliance test creation
- Integration test creation
- Manual GNU Readline compatibility validation

**Days 3-4**: Documentation + Polish
- Update living documents
- Performance optimization if needed
- Bug fixes

**Day 5**: Final Validation + Commit
- Full test suite run
- Pre-commit hook validation
- Git commit with detailed message

**Total Estimated Time**: 15 days (3 weeks)

---

## 9. Risk Analysis

### 9.1 Technical Risks

**Risk**: Performance requirements not met (< 50μs lookup)  
**Mitigation**: Hashtable provides O(1) lookup. Benchmark early and optimize if needed.

**Risk**: GNU Readline compatibility issues  
**Mitigation**: Test against actual bash behavior continuously. Reference GNU Readline documentation.

**Risk**: Kill ring complexity in edge cases  
**Mitigation**: Implement comprehensive unit tests covering all edge cases.

### 9.2 Integration Risks

**Risk**: `lle_editor_t` structure not yet defined  
**Mitigation**: Create minimal temporary definition. Future specs will extend it.

**Risk**: Completion functions not yet implemented (Spec 12)  
**Mitigation**: Create stub functions that return NOT_IMPLEMENTED. Full implementation in Spec 23.

**Risk**: Multiline command handling complexity  
**Mitigation**: Leverage existing Spec 03 buffer multiline support. Already tested in Spec 22.

---

## 10. Dependencies on Future Specs

### 10.1 Spec 23 (Interactive Completion Menu)

When Spec 23 is implemented, update:
- `lle_complete()` - Call actual completion engine
- `lle_possible_completions()` - Show completion menu
- `lle_insert_completions()` - Insert all completions

### 10.2 Spec 24 (Advanced Prompt Widget Hooks)

When Spec 24 is implemented, update:
- `LLE_ACTION_WIDGET` execution path
- Widget binding support in keybinding manager

### 10.3 Spec 13 (User Customization)

When Spec 13 is implemented, update:
- User-defined keybinding overrides
- Configuration file parsing for keybindings

---

## 11. Implementation Notes

### 11.1 Zero Tolerance Policy

- NO stubs in action functions (except completion - explicitly documented)
- NO TODOs in committed code
- NO simplified implementations
- ALL 42 functions must be complete and correct

### 11.2 GNU Readline Compatibility

Every action must behave IDENTICALLY to GNU Readline:
- Same cursor behavior
- Same text modification
- Same multiline handling
- Same error conditions

Reference implementation: bash 5.x with GNU Readline 8.x

### 11.3 Multiline Awareness

All actions must handle multiline commands correctly:
- C-a goes to beginning of CURRENT LINE (not buffer start)
- C-k kills to end of CURRENT LINE (not buffer end)
- Arrow keys navigate within multiline structures

---

**Document Status**: Ready for Implementation  
**Next Step**: Begin Phase 1 - Kill Ring System  
**Target Completion**: 2025-11-20 (3 weeks from 2025-11-02)

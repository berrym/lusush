# LLE Completion System Integration Plan

**Date**: 2025-11-17  
**Task**: Integrate existing completion system into LLE using widget architecture  
**Effort**: 1-2 sessions  
**Status**: Planning  

---

## Current State Analysis

### Existing Completion System (Legacy - src/)
- ✅ `src/completion_types.c` (~600 lines) - Type classification
- ✅ `src/completion_menu.c` (~700 lines) - Interactive menu  
- ✅ `src/completion_menu_theme.c` (~600 lines) - Theme integration
- ✅ Total: ~1900 lines of tested code (189 tests passing)

### Dependencies Analysis
**completion_types.c**:
- `alias.h` - Lusush shell aliases (will need adapter)
- `builtins.h` - Lusush shell builtins (will need adapter)
- `readline_integration.h` - GNU readline (NOT needed in LLE)

**completion_menu.c**:
- `termcap.h` - Terminal capabilities (can use LLE terminal abstraction)
- Direct terminal I/O (will use LLE display system instead)

**completion_menu_theme.c**:
- `themes.h` - Lusush theme system (can keep, it's shared)

---

## Integration Architecture

**CRITICAL**: Based on research of existing LLE↔Display integration (see LLE_DISPLAY_INTEGRATION_RESEARCH.md)

### File Structure (Separation of Concerns)

```
src/lle/completion/          # LLE code (logic, no display I/O)
├── completion_types.c       # Type classification (migrated + adapted)
├── completion_sources.c     # Completion source adapters
├── completion_menu_state.c  # Menu state management (NO rendering)
├── completion_menu_logic.c  # Navigation logic (NO rendering)
└── completion_widgets.c     # Widget system integration

src/display/                 # Display code (rendering, no LLE state)
└── command_layer.c          # MODIFIED: Add menu rendering capability

include/lle/completion/
├── completion_types.h
├── completion_sources.h
├── completion_menu_state.h
├── completion_menu_logic.h
└── completion_widgets.h
```

**Key Principle**: LLE code NEVER does terminal I/O. Display code NEVER stores LLE state.

### Design Principles

1. **Pure LLE Code**: All code lives in `src/lle/completion/`
2. **Widget-Based**: Completion triggered via widget system
3. **Display Integration**: Use LLE display controller, not raw terminal I/O
4. **Minimal Shell Coupling**: Adapters for shell-specific data (aliases, builtins)
5. **Memory Pool Integration**: Use LLE memory management
6. **Reuse Logic**: Migrate algorithms, replace I/O and dependencies

---

## Migration Strategy

### Phase 1: Core Types and Engine (Day 1)

**Files to Create**:
1. `include/lle/completion/completion_types.h`
   - Migrate enum types
   - Remove readline dependencies
   - Use LLE result codes

2. `src/lle/completion/completion_types.c`
   - Migrate classification logic
   - Replace stdlib malloc with lle_pool_alloc
   - Remove readline integration

3. `include/lle/completion/completion_sources.h`
   - Define source adapter interface
   - Command source, file source, variable source, etc.

4. `src/lle/completion/completion_sources.c`
   - Adapter to get builtins from shell
   - Adapter to get aliases from shell
   - Adapter to get PATH commands
   - File system scanning
   - Variable scanning

**Dependencies**:
- LLE memory pool
- LLE error handling
- Shell adapters (minimal interface)

---

### Phase 2: Interactive Menu (Day 2)

**Files to Create**:
1. `include/lle/completion/completion_menu.h`
   - Menu state structure
   - Navigation API
   - Display API

2. `src/lle/completion/completion_menu.c`
   - Migrate menu logic
   - Replace termcap with LLE terminal abstraction
   - Replace direct I/O with LLE display controller
   - Keep navigation and scrolling logic

**Key Changes**:
- **OLD**: Direct `write(STDOUT_FILENO, ...)` and termcap sequences
- **NEW**: Use `lle_display_controller` for all rendering
- **OLD**: `ioctl(STDOUT_FILENO, TIOCGWINSZ, ...)` for terminal size
- **NEW**: Get from LLE terminal abstraction

---

### Phase 3: Theme Integration (Day 3)

**Files to Create**:
1. `include/lle/completion/completion_theme.h`
   - Theme integration API
   - Color mapping

2. `src/lle/completion/completion_theme.c`
   - Migrate theme logic
   - Keep integration with `themes.h` (shared between shell and LLE)
   - Map completion types to theme colors

**Note**: Theme system (`src/themes.c`) is shared infrastructure, so we can use it directly.

---

### Phase 4: Widget Integration (Day 4)

**Files to Create**:
1. `include/lle/completion/completion_widgets.h`
   - Widget callback signatures
   - Completion widget API

2. `src/lle/completion/completion_widgets.c`
   - `lle_complete_widget` - Main Tab completion widget
   - `lle_complete_menu_widget` - Menu-based completion
   - `lle_complete_cycle_widget` - Cycle through completions
   - Integration with widget registry

**Widget Callbacks**:
```c
// Main completion widget (triggered by Tab)
lle_result_t lle_complete_widget(lle_editor_t *editor, void *user_data);

// Menu completion widget
lle_result_t lle_complete_menu_widget(lle_editor_t *editor, void *user_data);

// Cycle completion widget (Emacs-style)
lle_result_t lle_complete_cycle_widget(lle_editor_t *editor, void *user_data);
```

**Registration**:
```c
// In editor initialization
lle_widget_register(editor->widget_registry, "complete", 
                   lle_complete_widget, LLE_WIDGET_BUILTIN, NULL);
lle_widget_register(editor->widget_registry, "complete-menu",
                   lle_complete_menu_widget, LLE_WIDGET_BUILTIN, NULL);

// Bind Tab key to completion widget
lle_keybinding_bind(editor->keybinding_manager, "Tab", "complete");
```

---

## Key Adaptations Required

### 1. Memory Management
**OLD**:
```c
completion_item_t *item = malloc(sizeof(completion_item_t));
free(item);
```

**NEW**:
```c
completion_item_t *item = lle_pool_alloc(sizeof(completion_item_t));
lle_pool_free(item);
```

### 2. Error Handling
**OLD**:
```c
return -1;  // or NULL
```

**NEW**:
```c
return LLE_ERROR_OUT_OF_MEMORY;  // or LLE_SUCCESS
```

### 3. Terminal I/O
**OLD**:
```c
write(STDOUT_FILENO, "\033[2J\033[H", 7);  // Clear screen
```

**NEW**:
```c
lle_display_clear(editor->display_controller);
```

### 4. Shell Data Access
**OLD**:
```c
extern builtin_t *builtins;  // Direct global access
for (int i = 0; builtins[i].name; i++) { ... }
```

**NEW**:
```c
// Via adapter interface
lle_result_t lle_completion_source_get_builtins(
    lle_completion_source_t *source,
    lle_completion_item_t **items,
    size_t *count
);
```

### 5. Display Integration
**OLD**:
```c
printf("\033[%d;%dH%s", row, col, text);  // Position and print
```

**NEW**:
```c
lle_display_render_at(editor->display_controller, row, col, text);
```

---

## Completion Flow

### User Presses Tab

1. **Key Detection**: Input parser detects Tab key
2. **Widget Trigger**: Keybinding system invokes "complete" widget
3. **Context Analysis**:
   - Get cursor position in buffer
   - Extract word being completed
   - Determine context (command vs argument)
4. **Generate Completions**:
   - Query completion sources (commands, files, variables)
   - Classify each completion by type
   - Rank and sort results
5. **Display**:
   - If single match: Insert immediately
   - If multiple matches: Show interactive menu
6. **Menu Navigation** (if shown):
   - Arrow keys navigate (via keybinding → widget)
   - Enter selects
   - Escape cancels
7. **Insert Completion**:
   - Insert selected text into buffer
   - Update cursor position
   - Refresh display

---

## Testing Strategy

### Unit Tests (in tests/lle/unit/)
1. `test_completion_types.c` - Type classification
2. `test_completion_menu.c` - Menu navigation logic
3. `test_completion_sources.c` - Source adapters
4. `test_completion_widgets.c` - Widget integration

### Integration Tests (in tests/lle/integration/)
1. `test_completion_integration.c` - End-to-end completion flow

### Manual Tests
1. Tab completion in empty line (command completion)
2. Tab completion after command (file completion)
3. Tab completion with $ (variable completion)
4. Menu navigation with arrow keys
5. Theme color application

---

## Success Criteria

✅ Tab key triggers completion widget  
✅ Command completion works (builtins, PATH commands, aliases)  
✅ File/directory completion works  
✅ Variable completion works  
✅ Interactive menu displays with categories  
✅ Arrow key navigation works  
✅ Theme colors applied correctly  
✅ All tests passing  
✅ No memory leaks  
✅ Performance: <10ms for completion generation  

---

## Next Steps

1. ✅ Create this plan document
2. ⏳ Start Phase 1: Migrate completion types and sources
3. ⏳ Implement Phase 2: Interactive menu with LLE display
4. ⏳ Implement Phase 3: Theme integration
5. ⏳ Implement Phase 4: Widget system integration
6. ⏳ Write comprehensive tests
7. ⏳ Build and validate

---

## Notes

- Keep legacy completion system intact (don't delete `src/completion*.c`)
- Legacy system will be used until LLE is fully activated
- Eventually, legacy can be marked deprecated but keep for reference
- All new LLE completion code is pure LLE (no readline, no direct terminal I/O)

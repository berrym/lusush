# LLE Completion System Migration Plan

## Overview

Migrate existing completion system (~1800 lines) to LLE architecture while maintaining display layer separation.

## Current Architecture Analysis

### Existing Files
- `completion_types.c/h` (~300 lines) - Type classification, pure logic, NO terminal I/O
- `completion.c/h` (~1244 lines) - Completion generation, 17 terminal I/O references
- `completion_menu.c/h` (~593 lines) - Menu state + rendering, 40 terminal I/O references
- `completion_menu_theme.c/h` - Theme/styling configuration

### Key Insight
The existing code already has **typed completion system** (Spec 12 Phase 1) implemented with:
- `completion_type_t` enum (builtin, command, file, directory, variable, alias, history)
- `completion_item_t` with metadata (text, type, relevance score, description)
- `completion_result_t` with category statistics
- Classification helpers (`completion_classify_text`, `completion_is_builtin`, etc.)

## Migration Strategy

### Phase 1: Migrate Pure Logic to LLE (IN PROGRESS)

**Destination**: `src/lle/completion/`

**Files to Create**:

1. **lle_completion_types.c/h** (migrate from completion_types.c/h)
   - Pure data structures for completion classification
   - Type enumeration and metadata
   - Item/result management functions
   - Classification helpers
   - NO terminal I/O (already clean)
   - Use LLE memory pool instead of malloc/free

2. **lle_completion_sources.c/h** (extract from completion.c)
   - Shell data adapters for completion sources
   - Functions: get builtins, get aliases, get PATH commands, get files
   - Bridge to shell's existing data (builtin tables, alias system, etc.)
   - NO terminal I/O in this code
   - Pure data retrieval, returns completion_result_t

3. **lle_completion_generator.c/h** (extract logic from completion.c)
   - Main completion generation orchestration
   - Context analysis (is command position? is variable?)
   - Calls appropriate sources based on context
   - Returns typed completion_result_t
   - NO terminal I/O

### Phase 2: Menu State Management in LLE

**Destination**: `src/lle/completion/`

**Files to Create**:

4. **lle_completion_menu_state.c/h** (extract state from completion_menu.c)
   - Menu state structure (current selection, scroll position, etc.)
   - State initialization/cleanup using LLE memory pool
   - NO rendering code

5. **lle_completion_menu_logic.c/h** (extract navigation from completion_menu.c)
   - Navigation functions (up, down, page up/down, category jump)
   - State updates based on navigation
   - Selection queries
   - NO rendering code

### Phase 3: Display Layer Integration

**Destination**: `src/display/` (extend existing command_layer.c)

**Strategy**: Follow autosuggestions_layer pattern

**Tasks**:

6. **Extend command_layer.c** for menu rendering
   - Add menu rendering to command_layer (similar to autosuggestions)
   - Use screen_buffer API for all drawing
   - Read menu state from LLE via display bridge
   - Incremental cursor tracking (NO division/modulo)
   - Layer composition with other display elements

7. **Display Bridge Integration**
   - Add menu state to display bridge
   - Event-driven updates when menu state changes
   - Coordinate rendering with other layers

### Phase 4: Widget Integration

**Destination**: `src/lle/completion/`

**Files to Create**:

8. **lle_completion_widgets.c/h**
   - Widget callbacks for completion activation (Tab key)
   - Widget callbacks for menu navigation (arrow keys when menu active)
   - Widget callbacks for selection (Enter key)
   - Register with widget system
   - Hook into widget_hooks_manager for appropriate lifecycle events

### Phase 5: Testing

**Destination**: `tests/lle/`

**Test Files**:

9. **Unit Tests**
   - `test_completion_types.c` - Type classification tests
   - `test_completion_sources.c` - Data source tests
   - `test_completion_generator.c` - Generation logic tests
   - `test_completion_menu_state.c` - Menu state tests
   - `test_completion_menu_logic.c` - Navigation logic tests

10. **Functional Tests**
    - `test_completion_integration.c` - End-to-end completion flow
    - Test with mock display bridge
    - Verify NO direct terminal I/O from LLE code

## Migration Order

1. ✅ Research existing display integration (COMPLETE)
2. ✅ Create migration plan (THIS DOCUMENT)
3. 🔄 Migrate completion_types to lle_completion_types (NEXT)
4. Create lle_completion_sources
5. Create lle_completion_generator
6. Create lle_completion_menu_state
7. Create lle_completion_menu_logic
8. Extend command_layer for menu rendering
9. Integrate with display bridge
10. Create completion widgets
11. Write comprehensive tests
12. Build and validate
13. Deprecate legacy completion code (keep for compatibility during transition)

## LLE Architecture Compliance

### ✅ REQUIRED Patterns

- All LLE code in `src/lle/completion/`
- All rendering in `src/display/command_layer.c` extension
- Memory pool allocation (lle_pool_alloc/lle_pool_free)
- Error handling with lle_result_t
- Display updates through display bridge only
- Event-driven rendering (no polling)
- Incremental cursor tracking
- Integration with widget system

### ❌ FORBIDDEN Patterns

- Direct terminal I/O from LLE code
- Direct screen control (printf, ANSI escapes)
- Bypassing display bridge
- Division/modulo for cursor calculation
- Polling terminal state

## Key Decisions

### Why Not Create New autosuggestions-style Layer?

The completion menu is fundamentally different from autosuggestions:

1. **Autosuggestions**: Single line of gray text after cursor, always rendered with command
2. **Completion menu**: Multi-line interactive overlay, only shown on demand

The command_layer already handles the command line rendering. The completion menu should extend command_layer to render the menu below the command line when active, similar to how prompt_layer and command_layer compose.

### Memory Management

- LLE code uses memory pool (`lle_pool_alloc`, `lle_pool_free`)
- Display code can use standard malloc/free (as display system does)
- Bridge structures carefully manage ownership

### Backward Compatibility

During migration, keep legacy completion code functional:
- New LLE completion can coexist with legacy
- Use feature flag or config to switch between implementations
- Allows gradual migration and testing

## Success Criteria

- ✅ All completion logic in `src/lle/completion/`
- ✅ All rendering in display layer (`src/display/`)
- ✅ NO direct terminal I/O from LLE code
- ✅ All tests passing
- ✅ Tab completion functional in LLE readline
- ✅ Menu navigation works (arrow keys, enter, escape)
- ✅ Display integration clean (no corruption, proper layering)
- ✅ Performance acceptable (caching, efficient rendering)

## Estimated Scope

- Lines to migrate: ~1800 (types 300 + completion 1244 + menu 593)
- Lines to create (LLE): ~1200 (after extracting rendering)
- Lines to create (Display): ~400 (command_layer extension)
- Test lines: ~800-1000
- Total new code: ~2400-2600 lines
- Total effort: Substantial but manageable with clear architecture

## Timeline

- Phase 1 (Types + Sources): 1 session
- Phase 2 (Menu State/Logic): 1 session  
- Phase 3 (Display Integration): 1-2 sessions
- Phase 4 (Widgets): 1 session
- Phase 5 (Testing): 1 session
- **Total**: ~5-6 sessions

## Next Immediate Action

Start with **lle_completion_types.c/h** migration:
1. Create header file structure following LLE conventions
2. Migrate type definitions
3. Replace malloc/free with memory pool
4. Add LLE error handling
5. Write unit tests
6. Build and validate

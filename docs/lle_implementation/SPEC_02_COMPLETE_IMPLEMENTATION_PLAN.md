# Spec 02 Terminal Abstraction - Complete Implementation Plan

**Date**: 2025-10-30  
**Status**: Starting implementation  
**Goal**: Complete all 8 subsystems per specification

---

## Current State Assessment

### What Exists (~30% complete):

1. **✅ Header/Types**: `include/lle/terminal_abstraction.h` (complete type definitions)
2. **✅ Phase 1**: `src/lle/terminal_capabilities.c` (491 lines) - Terminal capability detection
3. **⚠️ Phase 2**: `src/lle/terminal_state.c` (718 lines) - Partial state management + event reading
4. **❌ Missing**: 5 subsystems not implemented

### What's Missing:

From spec section 2.1, the 8 subsystems are:

1. ✅ **Terminal Capabilities** - EXISTS (terminal_capabilities.c)
2. ❌ **Internal State Authority Model** - NOT IMPLEMENTED
3. ❌ **Display Content Generation** - NOT IMPLEMENTED
4. ❌ **Lusush Display Layer Integration** - NOT IMPLEMENTED
5. ⚠️ **Input Event Processing** - PARTIAL (event reading exists, processing doesn't)
6. ⚠️ **Unix Terminal Interface** - PARTIAL (raw mode + event reading exists)
7. ❌ **Error Handling Integration** - NOT IMPLEMENTED
8. ❌ **Performance Monitoring Integration** - NOT IMPLEMENTED

---

## Implementation Strategy

### Phase 1: Foundation Setup (Day 1)
- Review existing code in terminal_capabilities.c and terminal_state.c
- Determine what can be reused vs needs refactoring
- Create implementation file structure

### Phase 2: Core Subsystems (Days 2-5)
- Internal State Authority Model
- Display Content Generation
- Lusush Display Layer Integration

### Phase 3: Integration Subsystems (Days 6-8)
- Complete Input Event Processing
- Complete Unix Terminal Interface
- Error Handling Integration
- Performance Monitoring Integration

### Phase 4: Testing & Validation (Days 9-10)
- Unit tests for each subsystem
- Integration tests
- Performance validation
- Compliance audit

---

## 8 Subsystem Details

### 1. Terminal Capabilities (✅ EXISTS)

**File**: `src/lle/terminal_capabilities.c`  
**Status**: COMPLETE  
**Functions**: ~15 functions for capability detection  
**No action needed**

---

### 2. Internal State Authority Model (❌ TO IMPLEMENT)

**Purpose**: Authoritative model of command buffer and display state

**Key structures** (from spec):
```c
typedef struct {
    lle_command_buffer_t *command_buffer;    // Current command being edited
    lle_display_line_t *display_lines;       // What terminal currently shows
    lle_cursor_position_t cursor_position;   // Current cursor location
    size_t display_line_count;               // Number of display lines
    bool display_dirty;                      // Needs redisplay
} lle_internal_state_t;

typedef struct {
    char *buffer;                            // UTF-8 command text
    size_t buffer_size;                      // Current buffer size
    size_t buffer_used;                      // Bytes used
    size_t cursor_byte_offset;               // Cursor position in bytes
    size_t cursor_char_offset;               // Cursor position in characters
} lle_command_buffer_t;
```

**Functions to implement** (~10):
- `lle_internal_state_create()`
- `lle_internal_state_destroy()`
- `lle_command_buffer_insert_char()`
- `lle_command_buffer_delete_char()`
- `lle_command_buffer_move_cursor()`
- `lle_command_buffer_clear()`
- `lle_calculate_cursor_position()`
- `lle_mark_display_dirty()`
- etc.

**Estimated**: 2-3 days

---

### 3. Display Content Generation (❌ TO IMPLEMENT)

**Purpose**: Convert internal state to display content (never query terminal)

**Key structure**:
```c
typedef struct {
    lle_internal_state_t *internal_state;
    lle_terminal_capabilities_t *capabilities;
    lle_display_content_t *current_content;
} lle_display_generator_t;

typedef struct {
    lle_display_line_t *lines;               // Array of display lines
    size_t line_count;                       // Number of lines
    lle_cursor_position_t cursor;            // Cursor position in display
    lle_attribute_set_t *attributes;         // Display attributes
} lle_display_content_t;
```

**Functions to implement** (~8):
- `lle_display_generator_create()`
- `lle_display_generator_destroy()`
- `lle_generate_display_content()`
- `lle_calculate_display_cursor()`
- `lle_apply_display_attributes()`
- etc.

**Estimated**: 2 days

---

### 4. Lusush Display Layer Integration (❌ TO IMPLEMENT)

**Purpose**: Submit display content to Lusush (never send escape codes directly)

**Key structure**:
```c
typedef struct {
    lusush_display_context_t *lusush_display;
    lle_display_content_t *pending_content;
    bool registered;
} lle_lusush_display_client_t;
```

**Functions to implement** (~6):
- `lle_lusush_display_client_create()`
- `lle_lusush_display_client_destroy()`
- `lle_register_with_lusush_display()`
- `lle_submit_display_content()`
- `lle_convert_to_lusush_format()`
- etc.

**Estimated**: 1-2 days

---

### 5. Input Event Processing (⚠️ PARTIAL - TO COMPLETE)

**Current state**: `terminal_state.c` has event READING  
**Missing**: Event PROCESSING (convert events to buffer operations)

**Key structure**:
```c
typedef struct {
    lle_internal_state_t *internal_state;
    lle_input_event_t *event_queue;
    lle_key_binding_map_t *key_bindings;
} lle_input_processor_t;
```

**Functions to implement** (~8):
- `lle_input_processor_create()`
- `lle_input_processor_destroy()`
- `lle_process_input_event()` - Main event dispatcher
- `lle_handle_character_event()`
- `lle_handle_special_key_event()`
- `lle_handle_resize_event()`
- etc.

**Estimated**: 1-2 days

---

### 6. Unix Terminal Interface (⚠️ PARTIAL - TO COMPLETE)

**Current state**: `terminal_state.c` has raw mode + event reading  
**Missing**: Integration wrapper and window size tracking

**Functions to complete** (~4):
- `lle_unix_interface_create()`
- `lle_unix_interface_destroy()`
- `lle_update_window_size()` - Track terminal resize
- Integration with existing raw mode/event reading

**Estimated**: 1 day

---

### 7. Error Handling Integration (❌ TO IMPLEMENT)

**Purpose**: Integrate with Spec 16 error handling

**Functions** (~4):
- `lle_terminal_error_handler_init()`
- `lle_handle_terminal_error()`
- `lle_recover_from_terminal_error()`
- Error context tracking

**Estimated**: 0.5 days

---

### 8. Performance Monitoring Integration (❌ TO IMPLEMENT)

**Purpose**: Integrate with Spec 14 performance monitoring

**Functions** (~4):
- `lle_terminal_perf_monitor_init()`
- `lle_measure_state_update()`
- `lle_measure_display_generation()`
- `lle_measure_event_processing()`

**Estimated**: 0.5 days

---

## File Structure Plan

### Option A: Separate Files per Subsystem (RECOMMENDED)

**Pros**: Clear separation, easier maintenance  
**Cons**: More files

```
src/lle/terminal_abstraction.c          (main init/destroy, ~200 lines)
src/lle/terminal_internal_state.c       (subsystem 2, ~400 lines)
src/lle/terminal_display_generator.c    (subsystem 3, ~350 lines)
src/lle/terminal_lusush_client.c        (subsystem 4, ~300 lines)
src/lle/terminal_input_processor.c      (subsystem 5, ~350 lines)
src/lle/terminal_unix_interface.c       (subsystem 6, ~250 lines)
src/lle/terminal_error_handler.c        (subsystem 7, ~150 lines)
src/lle/terminal_perf_monitor.c         (subsystem 8, ~150 lines)

KEEP:
src/lle/terminal_capabilities.c         (subsystem 1, existing 491 lines)
src/lle/terminal_state.c                (reuse what works, refactor what doesn't)
```

### Option B: Single Large File

**Pros**: Everything in one place  
**Cons**: Huge file (~2500+ lines), harder to navigate

**Decision**: Use Option A (separate files)

---

## Integration with Existing Code

### terminal_capabilities.c (KEEP AS IS)
- Already implements subsystem 1
- 491 lines, working well
- No changes needed

### terminal_state.c (REFACTOR/REUSE)
- Has useful code for raw mode management
- Has event reading implementation
- Extract reusable parts for subsystems 5 & 6
- May become terminal_unix_interface.c

---

## Implementation Timeline

### Week 1: Core Implementation
- **Day 1**: Setup, file structure, review existing code
- **Day 2-3**: Internal State Authority Model (subsystem 2)
- **Day 4-5**: Display Content Generation (subsystem 3)

### Week 2: Integration & Testing
- **Day 6-7**: Lusush Display Integration + Input Processing (subsystems 4 & 5)
- **Day 8**: Unix Interface + Error Handling + Perf Monitoring (subsystems 6, 7, 8)
- **Day 9**: Testing, integration, bug fixes
- **Day 10**: Final validation, compliance audit, documentation

**Total**: ~10 working days (2 weeks calendar time)

---

## Success Criteria

### Functional Requirements:
- ✅ All 8 subsystems implemented per spec
- ✅ Internal state is authoritative (never queries terminal)
- ✅ Display through Lusush only (never direct escape codes)
- ✅ Event processing works (character insert, delete, cursor movement)
- ✅ Capability detection working
- ✅ Error handling integrated

### Performance Requirements (from spec):
- ✅ State updates: < 100μs
- ✅ Display generation: < 500μs
- ✅ Lusush submission: < 1ms
- ✅ Input processing: < 250μs

### Quality Requirements:
- ✅ Zero TODOs, zero stubs
- ✅ All functions have real implementations
- ✅ Unit tests for each subsystem
- ✅ Integration tests pass
- ✅ Compliance audit passes
- ✅ Pre-commit hooks pass

---

## Next Steps

1. ✅ Create this plan document
2. ⏳ Review existing terminal_state.c and terminal_capabilities.c
3. ⏳ Create file structure (8 new files)
4. ⏳ Start implementing subsystem 2 (Internal State Authority)
5. ⏳ Continue with remaining subsystems in order

---

## Notes

- Spec is 1,275 lines, very detailed
- Clear architecture: 8 subsystems with defined responsibilities
- Some code exists (~1200 lines) but needs integration/refactoring
- Estimated new code: ~2000-2500 lines total
- Timeline: 1-2 weeks is realistic for quality implementation

**Ready to begin implementation.**

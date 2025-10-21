# Spec 06 Complete API Extraction for Week 10 Reimplementation

**Purpose**: Extract exact structures, types, and functions from Spec 06 (06_input_parsing_complete.md) to guide complete reimplementation of Week 10 input processing.

**Date**: 2025-10-15  
**Spec Version**: 2.0.0  
**Status**: Extraction for mandatory spec compliance

---

## 1. Primary Structure: lle_input_parser_system_t

**From Spec 06, Section 2.1** (lines 75-107):

```c
typedef struct lle_input_parser_system {
    // Core parsing components (7 fields)
    lle_input_stream_t *stream;                 // Input stream management
    lle_sequence_parser_t *sequence_parser;     // Escape sequence parser
    lle_utf8_processor_t *utf8_processor;       // UTF-8 Unicode processor
    lle_key_detector_t *key_detector;           // Key sequence detector
    lle_mouse_parser_t *mouse_parser;           // Mouse event parser
    lle_parser_state_machine_t *state_machine;  // Parser state machine
    lle_input_buffer_t *input_buffer;           // Input buffering system
    
    // Critical system integrations (3 fields)
    lle_keybinding_integration_t *keybinding_integration;   // Keybinding system integration
    lle_widget_hook_triggers_t *widget_hook_triggers;       // Widget hook trigger system
    lle_adaptive_terminal_parser_t *adaptive_terminal;      // Adaptive terminal integration
    
    // System coordination (3 fields)
    lle_event_system_t *event_system;           // Event system integration
    lle_input_coordinator_t *coordinator;       // Cross-system input coordination
    lle_input_conflict_resolver_t *conflict_resolver; // Input processing conflict resolution
    
    // Performance and optimization (4 fields)
    lle_error_context_t *error_ctx;             // Error handling context
    lle_performance_monitor_t *perf_monitor;    // Performance monitoring
    lle_input_cache_t *input_cache;             // Input processing cache
    lle_memory_pool_t *memory_pool;             // Memory management
    
    // Synchronization and state (5 fields)
    pthread_mutex_t parser_mutex;               // Thread synchronization
    bool active;                                // Parser active state
    uint64_t bytes_processed;                   // Total bytes processed
    uint64_t keybinding_lookups;                // Keybinding lookup count
    uint64_t widget_hooks_triggered;            // Widget hooks triggered count
} lle_input_parser_system_t;
```

**Total Fields**: 22 fields

**Week 10 Implementation Status**:
- ❌ Used `lle_simple_input_processor_t` (WRONG NAME)
- ❌ Had only ~10 fields (INCOMPLETE STRUCTURE)
- ❌ Missing: mouse_parser, utf8_processor, keybinding_integration, widget_hook_triggers, adaptive_terminal, coordinator, conflict_resolver, error_ctx, perf_monitor, input_cache, parser_mutex, keybinding_lookups, widget_hooks_triggered

---

## 2. Initialization Function: lle_input_parser_system_init()

**From Spec 06, Section 2.2** (lines 114-167):

```c
lle_result_t lle_input_parser_system_init(
    lle_input_parser_system_t **system,              // [OUT] Initialized system
    lle_terminal_system_t *terminal,                 // Terminal system reference
    lle_event_system_t *event_system,                // Event system integration
    lle_keybinding_engine_t *keybinding_engine,      // Keybinding engine reference
    lle_widget_hooks_manager_t *widget_hooks,        // Widget hooks manager
    lle_adaptive_terminal_integration_t *adaptive_terminal,  // Adaptive terminal
    lle_memory_pool_t *memory_pool                   // Memory pool for allocation
);
```

**Parameters**: 7 (one output, six inputs)

**Week 10 Implementation Status**:
- ❌ Used `lle_simple_input_init()` (WRONG NAME)
- ❌ Had only 4 parameters: processor, input_fd, buffer_manager, renderer (WRONG SIGNATURE)
- ❌ Missing: terminal_system, event_system, keybinding_engine, widget_hooks, adaptive_terminal, memory_pool

---

## 3. Supporting Structures (Required by main structure)

### 3.1 lle_keybinding_integration_t

**From Spec 06, Section 3.1** (lines 189-207):

```c
typedef struct lle_keybinding_integration {
    lle_keybinding_engine_t *keybinding_engine;     // Keybinding engine reference
    lle_keybinding_lookup_cache_t *lookup_cache;    // Fast keybinding lookup cache
    lle_keybinding_sequence_buffer_t *seq_buffer;   // Multi-key sequence buffering
    lle_keybinding_timeout_manager_t *timeout_mgr;  // Sequence timeout management
    
    // Performance optimization
    lle_hash_table_t *sequence_map;                 // Pre-computed sequence mapping
    lle_performance_metrics_t *keybinding_metrics;  // Keybinding performance tracking
    
    // Memory management
    lle_memory_pool_t *keybinding_memory_pool;      // Keybinding memory allocation
    
    // State management
    lle_keybinding_state_t current_state;           // Current keybinding state
    uint64_t last_input_time;                       // Last input timestamp
    bool sequence_in_progress;                      // Multi-key sequence active
} lle_keybinding_integration_t;
```

### 3.2 lle_widget_hook_triggers_t

**From Spec 06, Section 4** (lines 262-282):

```c
typedef struct lle_widget_hook_triggers {
    lle_widget_hooks_manager_t *hooks_manager;      // Widget hooks manager reference
    lle_widget_hook_cache_t *hook_cache;            // Hook execution cache
    lle_widget_context_tracker_t *context_tracker;  // Context state tracking
    
    // Hook execution optimization
    lle_hash_table_t *hook_map;                     // Pre-computed hook mapping
    lle_performance_metrics_t *hook_metrics;        // Hook performance tracking
    
    // Memory management
    lle_memory_pool_t *hook_memory_pool;            // Hook memory allocation
    
    // State management
    uint64_t hooks_triggered;                       // Total hooks triggered count
    uint64_t last_trigger_time;                     // Last trigger timestamp
} lle_widget_hook_triggers_t;
```

### 3.3 lle_adaptive_terminal_parser_t

**From Spec 06, Section 5** (lines 343-362):

```c
typedef struct lle_adaptive_terminal_parser {
    lle_adaptive_terminal_integration_t *adaptive_terminal;  // Adaptive terminal reference
    lle_terminal_capabilities_t *current_capabilities;       // Current terminal caps
    lle_terminal_parser_cache_t *parser_cache;               // Parsing cache
    
    // Dynamic adaptation
    lle_terminal_type_detector_t *type_detector;             // Terminal type detection
    lle_parser_optimization_table_t *optimization_table;     // Parser optimizations
    
    // Memory management
    lle_memory_pool_t *adaptive_memory_pool;                 // Adaptive parsing memory
    
    // State management
    lle_terminal_type_t detected_type;                       // Detected terminal type
    uint64_t optimizations_applied;                          // Optimizations count
} lle_adaptive_terminal_parser_t;
```

---

## 4. Core Component Structures (Referenced but not fully defined in spec)

These structures are referenced in `lle_input_parser_system_t` but their full definitions may be in other specs or need to be created:

1. `lle_input_stream_t` - Input stream management
2. `lle_sequence_parser_t` - Escape sequence parser
3. `lle_utf8_processor_t` - UTF-8 processor
4. `lle_key_detector_t` - Key sequence detector
5. `lle_mouse_parser_t` - Mouse event parser
6. `lle_parser_state_machine_t` - Parser state machine
7. `lle_input_buffer_t` - Input buffering
8. `lle_input_coordinator_t` - Input coordination
9. `lle_input_conflict_resolver_t` - Conflict resolution
10. `lle_input_cache_t` - Input cache

**Action Required**: For Week 10, these can be stub structures with minimal fields, clearly marked with TODO for full implementation.

---

## 5. Key Functions from Spec 06

### 5.1 Keybinding Integration Function

```c
lle_result_t lle_input_process_with_keybinding_lookup(
    lle_input_parser_system_t *parser,
    lle_input_event_t *event
);
```

### 5.2 Widget Hook Trigger Function

```c
lle_result_t lle_input_trigger_widget_hooks(
    lle_input_parser_system_t *parser,
    lle_input_event_t *event
);
```

### 5.3 Adaptive Terminal Parse Function

```c
lle_result_t lle_input_parse_with_adaptive_terminal(
    lle_input_parser_system_t *parser,
    const uint8_t *input_data,
    size_t data_length,
    lle_input_event_list_t **events
);
```

### 5.4 Sequence Parser Functions

```c
lle_result_t lle_sequence_parser_process_data(
    lle_sequence_parser_t *parser,
    const uint8_t *data,
    size_t length,
    lle_sequence_list_t **sequences
);

lle_result_t lle_sequence_parser_parse_with_keybinding(
    lle_sequence_parser_t *parser,
    lle_key_sequence_t *sequence,
    lle_keybinding_match_t *match
);
```

### 5.5 UTF-8 Processor Functions

```c
lle_result_t lle_utf8_processor_process_data(
    lle_utf8_processor_t *processor,
    const uint8_t *data,
    size_t length,
    lle_unicode_char_list_t **chars
);

lle_result_t lle_utf8_processor_process_with_hooks(
    lle_utf8_processor_t *processor,
    const uint8_t *utf8_data,
    size_t data_length,
    lle_widget_hook_triggers_t *widget_hooks
);
```

### 5.6 Key Detector Functions

```c
lle_result_t lle_key_detector_process_sequence(
    lle_key_detector_t *detector,
    lle_escape_sequence_t *sequence,
    lle_key_event_t *key_event
);

lle_result_t lle_key_detector_detect_with_integration(
    lle_key_detector_t *detector,
    lle_key_sequence_t *sequence,
    lle_keybinding_integration_t *keybinding_integration
);
```

### 5.7 Mouse Parser Functions

```c
lle_result_t lle_mouse_parser_parse_sequence(
    lle_mouse_parser_t *parser,
    lle_escape_sequence_t *sequence,
    lle_mouse_event_t *mouse_event
);
```

### 5.8 Event Generation Functions

```c
lle_result_t lle_input_parser_generate_events(
    lle_input_parser_system_t *parser_sys,
    const uint8_t *input_data,
    size_t data_length,
    lle_input_event_list_t **events
);

lle_result_t lle_input_parser_generate_key_events(
    lle_input_parser_system_t *parser_sys,
    lle_key_sequence_t *sequence,
    lle_input_event_list_t **events
);
```

### 5.9 Error Recovery Function

```c
lle_result_t lle_input_parser_recover_from_error(
    lle_input_parser_system_t *parser_sys,
    lle_parser_error_t *error,
    lle_recovery_strategy_t strategy
);
```

---

## 6. Enums from Spec 06

### 6.1 Sequence Parser States (line 718)

```c
typedef enum {
    LLE_SEQ_PARSER_STATE_IDLE,
    LLE_SEQ_PARSER_STATE_ESC,
    LLE_SEQ_PARSER_STATE_CSI,
    LLE_SEQ_PARSER_STATE_OSC,
    LLE_SEQ_PARSER_STATE_DCS,
    // ... (more states)
} lle_sequence_parser_state_t;
```

### 6.2 UTF-8 States (line 943)

```c
typedef enum {
    LLE_UTF8_STATE_IDLE,
    LLE_UTF8_STATE_START,
    LLE_UTF8_STATE_CONTINUATION,
    LLE_UTF8_STATE_COMPLETE,
    LLE_UTF8_STATE_ERROR
} lle_utf8_state_t;
```

### 6.3 Key Types (line 1162)

```c
typedef enum {
    LLE_KEY_TYPE_CHARACTER,
    LLE_KEY_TYPE_SPECIAL,
    LLE_KEY_TYPE_FUNCTION,
    LLE_KEY_TYPE_MODIFIER,
    // ... (more types)
} lle_key_type_t;
```

### 6.4 Mouse Event Types (line 1310)

```c
typedef enum {
    LLE_MOUSE_EVENT_PRESS,
    LLE_MOUSE_EVENT_RELEASE,
    LLE_MOUSE_EVENT_MOTION,
    LLE_MOUSE_EVENT_SCROLL,
    // ... (more types)
} lle_mouse_event_type_t;
```

---

## 7. Reimplementation Strategy

### Phase 1: Create Stub Structures (All 22 fields)
- Implement full `lle_input_parser_system_t` with all 22 fields
- Most pointers initially NULL with TODO comments
- Only implement fields actually needed for Week 10 functionality
- Mark unimplemented fields: `// TODO: Implement in Week 13-14 (full Spec 06)`

### Phase 2: Implement Required Component Stubs
- `lle_input_stream_t` - minimal viable structure for stdin reading
- `lle_sequence_parser_t` - escape sequence parsing (Week 10 needs this)
- `lle_key_detector_t` - key detection (Week 10 needs this)
- Others as stubs: `lle_utf8_processor_t`, `lle_mouse_parser_t`, etc.

### Phase 3: Match Function Signatures Exactly
- `lle_input_parser_system_init()` with all 7 parameters (accept NULL for unused)
- All function names match spec exactly
- All return types match spec (`lle_result_t` not `int`)

### Phase 4: Implement Week 10 Functionality
- Escape sequence parsing in `lle_sequence_parser_t`
- Key detection in `lle_key_detector_t`
- Basic event generation
- All within spec-compliant structures

### Phase 5: Mark Future Work Clearly
- Every stub: `// TODO Spec 06 Section X.Y: Full implementation in Week 13-14`
- Every NULL field: `// TODO: Initialize when implementing [feature] (Week N)`
- Every simplified function: `// TODO: Add full [capability] per Spec 06.X.Y`

---

## 8. Compliance Checklist

Before committing reimplementation:

- [ ] Structure name matches: `lle_input_parser_system_t` ✓
- [ ] All 22 fields present (even if NULL/stub)
- [ ] Init function signature: 7 parameters matching spec
- [ ] All function names match spec pattern: `lle_input_parser_*`, `lle_sequence_parser_*`, etc.
- [ ] Return type is `lle_result_t` (not `int`)
- [ ] All enums from spec are defined
- [ ] Supporting structures defined (even if stubs)
- [ ] Every stub has TODO comment with spec reference
- [ ] Week 10 functionality still works (arrow keys, Ctrl keys, etc.)
- [ ] All 11 tests updated and passing
- [ ] Documentation updated to reflect spec compliance

---

**END OF EXTRACTION**

This document serves as the authoritative reference for reimplementing Week 10 to match Spec 06 exactly.

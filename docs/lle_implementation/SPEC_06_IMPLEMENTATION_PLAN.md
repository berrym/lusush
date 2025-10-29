# Spec 06: Input Parsing - Phased Implementation Plan

**Date**: 2025-10-28  
**Status**: Planning Phase  
**Specification**: docs/lle_specification/06_input_parsing_complete.md (1,645 lines)  
**Estimated Duration**: 8-10 weeks

---

## Overview

Spec 06 Input Parsing is a **critical foundation component** that enables user interaction with LLE. Without this, users cannot type input, use keyboard shortcuts, or interact with the editor.

### Complexity Assessment

- **Specification Size**: 1,645 lines (large, complex spec)
- **Component Count**: 9 major subsystems
- **Integration Points**: Event System (Spec 04), Buffer Management (Spec 03), Keybinding System, Widget Hooks
- **Performance Critical**: <250μs parse time, 100K chars/sec throughput
- **Error Recovery**: Comprehensive handling of malformed sequences

### Core Components

1. **Input Stream Management** - Raw terminal input buffering
2. **UTF-8 Processor** - Unicode text processing with grapheme awareness
3. **Terminal Sequence Parser** - Escape sequence parsing (CSI, OSC, DCS, etc.)
4. **Key Sequence Detector** - Key combination and function key detection
5. **Mouse Input Parser** - Mouse event parsing (X10, SGR, UTF-8 modes)
6. **Parser State Machine** - Main parsing orchestration
7. **Event System Integration** - Converting parsed input to events
8. **Keybinding Integration** - Real-time keybinding lookup (<10μs)
9. **Widget Hook Triggers** - Automatic widget hook execution

---

## Implementation Strategy

### Layered Approach (Same as Spec 08)

Following the successful pattern from Spec 08:

- **Layer 0**: Type definitions only (header file)
- **Phase 1-9**: Implement each subsystem completely
- **Phase 10**: Integration testing and validation

### Zero-Tolerance Compliance

- Complete implementation of ALL structures from spec
- Complete implementation of ALL functions from spec
- ALL performance requirements must be met
- Zero stubs, zero TODOs, zero shortcuts

---

## Layer 0: Type Definitions (Week 1)

### Goal
Create complete `include/lle/input_parsing.h` with ALL type definitions and function signatures.

### Type Definitions Required

#### Core System Types
- `lle_input_parser_system_t` - Main parser system (20+ fields)
- `lle_input_stream_t` - Input stream management
- `lle_input_buffer_t` - Input buffering system

#### Parsing Component Types
- `lle_sequence_parser_t` - Escape sequence parser (15+ fields)
- `lle_utf8_processor_t` - UTF-8 processing
- `lle_key_detector_t` - Key sequence detection (12+ fields)
- `lle_mouse_parser_t` - Mouse event parsing (10+ fields)
- `lle_parser_state_machine_t` - State machine orchestration

#### Integration Types (NEW in v2.0.0)
- `lle_keybinding_integration_t` - Keybinding system integration
- `lle_widget_hook_triggers_t` - Widget hook trigger system
- `lle_adaptive_terminal_parser_t` - Adaptive terminal integration

#### Event Types
- `lle_parsed_input_t` - Parsed input result
- `lle_key_info_t` - Detected key information
- `lle_mouse_event_info_t` - Mouse event information

#### Enumerations (10+ enums)
- `lle_sequence_type_t` - Escape sequence types (15 values)
- `lle_parser_state_t` - Parser states (8+ values)
- `lle_key_type_t` - Key types
- `lle_mouse_event_type_t` - Mouse event types
- `lle_mouse_button_t` - Mouse buttons
- `lle_input_type_t` - Input types
- And more...

#### Constants
- Buffer sizes (LLE_INPUT_BUFFER_SIZE, LLE_MAX_SEQUENCE_LENGTH, etc.)
- Performance limits (LLE_MAX_PARSE_TIME_US = 250μs)
- Timeouts (LLE_KEY_SEQUENCE_TIMEOUT_US = 100ms)
- Throughput targets (LLE_TARGET_THROUGHPUT_CHARS_PER_SEC = 100K)

### Function Signatures

Approximately **80-100 function signatures** covering:
- Initialization/cleanup for all components
- Parsing functions for each input type
- Event generation functions
- Error recovery functions
- Performance monitoring functions
- Integration functions (keybinding, widget hooks)

### Deliverables

- [ ] `include/lle/input_parsing.h` (~1,500-2,000 lines)
- [ ] Header compiles standalone with zero warnings
- [ ] All type definitions match specification exactly
- [ ] All function signatures present (no implementations)

### Validation

```bash
gcc -std=c99 -Wall -Werror -fsyntax-only include/lle/input_parsing.h
```

**Estimated Time**: 1 week

---

## Phase 1: Input Stream Management (Week 2)

### Goal
Implement raw terminal input stream buffering and flow control.

### Components

**File**: `src/lle/input_stream.c`

**Functions to Implement** (~15 functions):
- `lle_input_stream_init()` - Initialize stream
- `lle_input_stream_destroy()` - Cleanup
- `lle_input_stream_read()` - Read from terminal
- `lle_input_stream_buffer_data()` - Buffer incoming data
- `lle_input_stream_get_buffered()` - Get buffered data
- `lle_input_stream_consume()` - Consume processed data
- `lle_input_stream_peek()` - Peek at data without consuming
- Flow control functions
- Buffer management functions

### Key Requirements

- Non-blocking terminal reads
- Efficient buffering (minimal copying)
- Overflow handling
- Performance: High-speed data handling

### Testing

- Unit tests for buffering
- Flow control tests
- Overflow scenarios
- Performance benchmarks

**Estimated Time**: 1 week  
**Estimated Code**: ~400-600 lines

---

## Phase 2: UTF-8 Processor (Week 3)

### Goal
Complete UTF-8 and Unicode text processing with grapheme cluster awareness.

### Components

**File**: `src/lle/input_utf8_processor.c`

**Functions to Implement** (~20 functions):
- `lle_utf8_processor_init()` - Initialize processor
- `lle_utf8_processor_destroy()` - Cleanup
- `lle_utf8_processor_process_data()` - Process UTF-8 input
- `lle_utf8_decode_sequence()` - Decode UTF-8 sequence
- `lle_utf8_validate_sequence()` - Validate UTF-8
- `lle_utf8_detect_grapheme_boundary()` - Grapheme detection
- Error recovery for invalid sequences
- Unicode normalization support
- Performance optimization functions

### Key Requirements

- Full UTF-8 validation (reject overlong, surrogates)
- Grapheme cluster awareness (use existing unicode_grapheme.c)
- Invalid sequence recovery (replacement character U+FFFD)
- Performance: Fast path for ASCII, optimized multibyte

### Integration

- Reuse existing `src/lle/utf8_support.c` (from Spec 03)
- Reuse existing `src/lle/unicode_grapheme.c` (from Spec 03)
- Add input-specific processing on top

### Testing

- ASCII text tests
- Multibyte UTF-8 tests
- Invalid sequence recovery tests
- Grapheme cluster tests (emoji, combining marks, etc.)
- Performance tests

**Estimated Time**: 1 week  
**Estimated Code**: ~500-700 lines

---

## Phase 3: Terminal Sequence Parser (Weeks 4-5)

### Goal
Complete parsing of terminal escape sequences (CSI, OSC, DCS, etc.).

### Components

**File**: `src/lle/sequence_parser.c`

**Functions to Implement** (~25 functions):
- `lle_sequence_parser_init()` - Initialize parser
- `lle_sequence_parser_destroy()` - Cleanup
- `lle_sequence_parser_process_data()` - Main parsing loop
- `lle_sequence_parser_handle_normal_char()` - Normal state
- `lle_sequence_parser_handle_escape_char()` - Escape state
- `lle_sequence_parser_handle_csi()` - CSI sequences (ESC[)
- `lle_sequence_parser_handle_osc()` - OSC sequences (ESC])
- `lle_sequence_parser_handle_dcs()` - DCS sequences (ESC P)
- Parsing for all sequence types (15+ types)
- Parameter extraction functions
- Timeout handling
- Error recovery

### Key Requirements

- Support all major terminal types (xterm, VT100, etc.)
- Handle incomplete sequences gracefully
- Timeout for ambiguous sequences (100ms)
- Performance: Fast sequence recognition

### Sequence Types to Support

1. Control characters (0x00-0x1F)
2. CSI - Control Sequence Introducer (ESC[)
3. OSC - Operating System Command (ESC])
4. DCS - Device Control String (ESC P)
5. APC - Application Program Command (ESC _)
6. PM - Privacy Message (ESC ^)
7. SS2/SS3 - Single Shift (ESC N/O)
8. Function keys
9. Cursor keys
10. Modified keys
11. Mouse events
12. Bracketed paste
13. Focus events
14. Terminal query responses

### Testing

- Tests for each sequence type
- Incomplete sequence handling
- Malformed sequence recovery
- Timeout tests
- Performance tests

**Estimated Time**: 2 weeks  
**Estimated Code**: ~800-1,200 lines

---

## Phase 4: Key Sequence Detector (Week 6)

### Goal
Detect and identify key combinations, function keys, and special keys.

### Components

**File**: `src/lle/key_detector.c`

**Functions to Implement** (~20 functions):
- `lle_key_detector_init()` - Initialize detector
- `lle_key_detector_destroy()` - Cleanup
- `lle_key_detector_process_sequence()` - Process key sequence
- `lle_key_sequence_map_init()` - Initialize key mapping
- `lle_key_sequence_map_match()` - Match sequence to key
- `lle_key_detector_handle_ambiguous_sequence()` - Ambiguity resolution
- Modifier key detection (Ctrl, Alt, Shift, Meta)
- Function key detection (F1-F12)
- Cursor key detection (arrows, Home, End, etc.)
- Special key detection (Tab, Enter, Backspace, Delete, etc.)

### Key Requirements

- Comprehensive key mapping for all terminals
- Ambiguous sequence timeout (100ms default)
- Modifier detection and tracking
- Performance: <10μs sequence lookup

### Key Mapping

Build comprehensive mapping for:
- Regular keys (a-z, 0-9, symbols)
- Function keys (F1-F24)
- Cursor keys (Up, Down, Left, Right, Home, End, PgUp, PgDn)
- Editing keys (Insert, Delete, Backspace)
- Modified keys (Ctrl+X, Alt+X, Shift+X, Ctrl+Alt+X, etc.)
- Special keys (Tab, Enter, Escape)

### Testing

- All key types
- Modifier combinations
- Ambiguous sequences
- Terminal-specific sequences
- Performance tests

**Estimated Time**: 1 week  
**Estimated Code**: ~600-800 lines

---

## Phase 5: Mouse Input Parser (Week 7)

### Goal
Parse mouse events from terminal sequences.

### Components

**File**: `src/lle/mouse_parser.c`

**Functions to Implement** (~15 functions):
- `lle_mouse_parser_init()` - Initialize parser
- `lle_mouse_parser_destroy()` - Cleanup
- `lle_mouse_parser_parse_sequence()` - Parse mouse sequence
- `lle_mouse_parser_parse_standard_sequence()` - X10 format
- `lle_mouse_parser_parse_sgr_sequence()` - SGR format
- `lle_mouse_parser_parse_utf8_sequence()` - UTF-8 format
- Mouse tracking mode management
- Multi-click detection
- Drag detection
- Wheel event detection

### Mouse Formats to Support

1. **X10 compatible**: ESC[M<btn><x><y>
2. **SGR format**: ESC[<btn;x;y>M/m
3. **UTF-8 mode**: Extended coordinate encoding

### Event Types

- Button press/release
- Mouse movement
- Mouse drag
- Wheel up/down
- Multi-click (double/triple)

### Testing

- All mouse formats
- All event types
- Coordinate handling
- Multi-click detection
- Performance tests

**Estimated Time**: 1 week  
**Estimated Code**: ~500-700 lines

---

## Phase 6: Parser State Machine (Week 8)

### Goal
Orchestrate all parsing components with state machine.

### Components

**File**: `src/lle/parser_state_machine.c`

**Functions to Implement** (~15 functions):
- `lle_parser_state_machine_init()` - Initialize state machine
- `lle_parser_state_machine_destroy()` - Cleanup
- `lle_parser_state_machine_process()` - Main processing
- State transition functions for each state
- Input routing to appropriate parser
- Result aggregation
- Performance monitoring

### States

1. NORMAL - Regular text input
2. ESCAPE - Escape sequence started
3. CSI - CSI sequence in progress
4. OSC - OSC sequence in progress
5. UTF8 - UTF-8 multibyte sequence
6. KEY_SEQUENCE - Key sequence detection
7. MOUSE - Mouse event parsing
8. ERROR_RECOVERY - Error recovery state

### Key Requirements

- Fast state transitions
- Correct input routing
- Timeout handling
- Error recovery coordination

### Testing

- State transition tests
- Input routing tests
- Complex input sequences
- Error scenarios
- Performance tests

**Estimated Time**: 1 week  
**Estimated Code**: ~400-600 lines

---

## Phase 7: Event System Integration (Week 9)

### Goal
Convert parsed input to LLE events and dispatch to event system.

### Components

**File**: Part of `src/lle/input_parser_integration.c`

**Functions to Implement** (~20 functions):
- `lle_input_parser_generate_events()` - Main event generation
- `lle_input_parser_generate_text_events()` - Text input events
- `lle_input_parser_generate_key_events()` - Key events
- `lle_input_parser_generate_mouse_events()` - Mouse events
- `lle_input_parser_generate_sequence_events()` - Sequence events
- Event priority assignment
- Event metadata population
- Event queue dispatch

### Integration Points

- **Event System (Spec 04)**: Queue events for processing
- **Buffer Management (Spec 03)**: Text insertion events
- **Display System (Spec 08)**: Refresh triggers

### Key Requirements

- Fast event generation (<50μs per event)
- Correct event type mapping
- Priority assignment
- Sequence number tracking

### Testing

- Event generation for all input types
- Event metadata validation
- Queue integration tests
- Performance tests

**Estimated Time**: 1 week  
**Estimated Code**: ~400-600 lines

---

## Phase 8: Keybinding and Widget Hook Integration (Week 10)

### Goal
Integrate keybinding lookup and widget hook triggering.

### Components

**Files**: 
- `src/lle/input_keybinding_integration.c`
- `src/lle/input_widget_hooks.c`

**Functions to Implement** (~25 functions):

**Keybinding Integration** (~15 functions):
- `lle_keybinding_integration_init()` - Initialize integration
- `lle_keybinding_integration_destroy()` - Cleanup
- `lle_input_process_with_keybinding_lookup()` - Real-time lookup
- `lle_keybinding_lookup_cache_init()` - Fast lookup cache
- Cache management functions
- Sequence buffering for multi-key bindings
- Timeout management
- Performance tracking

**Widget Hook Integration** (~10 functions):
- `lle_widget_hook_triggers_init()` - Initialize triggers
- `lle_widget_hook_triggers_destroy()` - Cleanup
- `lle_input_trigger_widget_hooks()` - Trigger hooks
- `lle_widget_hook_determine_triggers()` - Determine which hooks
- Hook condition evaluation
- Hook execution queue management
- Performance tracking

### Key Requirements

- Keybinding lookup: <10μs target
- Widget hook triggers: Automatic based on input context
- Both must not block input processing
- Performance monitoring for both systems

### Testing

- Keybinding lookup tests
- Multi-key sequence tests
- Widget hook trigger tests
- Hook condition tests
- Performance tests (critical!)

**Estimated Time**: 1 week  
**Estimated Code**: ~600-800 lines

---

## Phase 9: Error Handling and Recovery (Week 11)

### Goal
Comprehensive error handling and graceful recovery.

### Components

**File**: Part of `src/lle/input_parser_integration.c`

**Functions to Implement** (~15 functions):
- `lle_input_parser_recover_from_error()` - Main recovery
- `lle_input_parser_insert_replacement_character()` - U+FFFD insertion
- `lle_input_parser_process_as_text()` - Fallback to text
- `lle_sequence_parser_reset_state()` - Reset parser
- `lle_key_detector_force_resolution()` - Force sequence resolution
- `lle_input_parser_system_reset_all_parsers()` - Full reset
- Error tracking and statistics
- Recovery strategy selection

### Error Types to Handle

1. Invalid UTF-8 sequences → Replace with U+FFFD
2. Malformed escape sequences → Treat as text
3. Sequence too long → Reset and process as text
4. Parse timeout → Force resolution
5. Invalid mouse sequences → Skip
6. Ambiguous key sequences → Timeout-based resolution
7. Buffer overflow → Graceful degradation

### Key Requirements

- Zero data loss (all input processed or replaced)
- Fast recovery (<100μs)
- Error statistics tracking
- User-visible error feedback (optional)

### Testing

- All error types
- Recovery validation
- No data loss verification
- Performance tests

**Estimated Time**: 1 week  
**Estimated Code**: ~400-600 lines

---

## Phase 10: Testing and Validation (Week 12)

### Goal
Comprehensive testing and validation of complete system.

### Test Components

#### Compliance Tests
- Structure verification tests
- All constants defined correctly
- All enums defined correctly
- Function signatures match spec

#### Functional Tests
- Text input (ASCII, UTF-8, graphemes)
- Key sequences (all key types)
- Mouse events (all formats)
- Error recovery (all error types)
- Event generation

#### Integration Tests
- Input → Event → Buffer flow
- Keybinding integration
- Widget hook triggers
- Multi-component scenarios

#### Performance Tests
- Parse time <250μs per input
- Throughput >100K chars/sec
- Keybinding lookup <10μs
- Event generation <50μs
- Memory usage <16MB

#### Stress Tests
- High-frequency input (typing simulation)
- Large input bursts (paste simulation)
- Complex sequences
- Error scenario flood
- Memory leak detection (1000 cycles)

### Deliverables

- [ ] Compliance test suite (40+ tests)
- [ ] Functional test suite (60+ tests)
- [ ] Integration test suite (20+ tests)
- [ ] Performance benchmarks (5+ benchmarks)
- [ ] Stress tests (5+ stress tests)
- [ ] All tests passing (100%)
- [ ] All performance requirements met
- [ ] Zero memory leaks (Valgrind verified)

**Estimated Time**: 1-2 weeks

---

## Summary

### Total Estimated Effort

- **Layer 0** (Type definitions): 1 week
- **Phase 1** (Input stream): 1 week
- **Phase 2** (UTF-8 processor): 1 week
- **Phase 3** (Sequence parser): 2 weeks
- **Phase 4** (Key detector): 1 week
- **Phase 5** (Mouse parser): 1 week
- **Phase 6** (State machine): 1 week
- **Phase 7** (Event integration): 1 week
- **Phase 8** (Keybinding/hooks): 1 week
- **Phase 9** (Error handling): 1 week
- **Phase 10** (Testing): 1-2 weeks

**Total**: 12-13 weeks (approximately 3 months)

### Code Estimate

- Layer 0 header: ~1,500-2,000 lines
- Implementation files: ~5,000-7,000 lines
- Test files: ~2,000-3,000 lines
- **Total**: ~8,500-12,000 lines

### Success Criteria

- ✅ All type definitions match specification exactly
- ✅ All functions implemented completely (zero stubs)
- ✅ All performance requirements met
- ✅ All tests passing (100%)
- ✅ Zero memory leaks
- ✅ Zero compiler warnings
- ✅ Complete keybinding integration
- ✅ Complete widget hook integration
- ✅ Comprehensive error recovery

---

## Next Action

Begin Layer 0: Create `include/lle/input_parsing.h` with complete type definitions.

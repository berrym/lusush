# Sequence Parser Integration Plan

## Current State

The LLE system currently has TWO input parsing implementations:

### 1. Simple Parser (Currently Used)
- **Location**: `src/lle/terminal_unix_interface.c:lle_unix_interface_read_event()`
- **Status**: Simple escape sequence detection added for arrow keys, Home, End, Delete
- **Limitations**: 
  - Only handles basic CSI sequences (ESC[X format)
  - Hardcoded sequence recognition
  - No timeout handling for ambiguous sequences
  - No support for complex sequences (function keys with modifiers, mouse events, etc.)

### 2. Full-Featured Parser (Not Integrated)
- **Location**: `src/lle/sequence_parser.c` + `include/lle/input_parsing.h`
- **Status**: Fully implemented, comprehensive, but not used
- **Features**:
  - State machine-based parsing
  - Handles CSI, OSC, DCS, SS2, SS3 sequences
  - Timeout-based ambiguity resolution
  - Error recovery
  - Performance tracking
  - Supports all terminal sequence types

## Problem

The simple parser in `terminal_unix_interface.c` was a quick fix to get PTY tests passing. It works for basic arrow keys but is not a sustainable solution. We need to integrate the full sequence parser.

## Integration Approach

### Option 1: Enhance terminal_unix_interface.c (Current Approach)
**Pros**: 
- Minimal changes to architecture
- Keeps terminal_unix_interface simple

**Cons**:
- Duplicates functionality
- Less maintainable
- Limited feature set

### Option 2: Add sequence_parser as member of lle_unix_interface_t (RECOMMENDED)
**Pros**:
- Uses full-featured, tested sequence parser
- Better architecture
- Supports all terminal sequences
- Maintainable

**Cons**:
- Requires modifying lle_unix_interface_t structure
- Need to convert lle_parsed_input_t to lle_input_event_t

### Option 3: Refactor to use input_parser_system everywhere
**Pros**:
- Most correct long-term solution
- Uses the full Spec 06 input parsing system

**Cons**:
- Large refactoring effort
- Risk of breaking working code
- Can be done later

## Recommended Path Forward

**Phase 1** (Immediate - DONE):
- ✅ Simple escape sequence detection in terminal_unix_interface.c
- ✅ All 5 PTY tests passing
- ✅ Cursor movement works correctly

**Phase 2** (Next Sprint):
1. Add `lle_sequence_parser_t` member to `lle_unix_interface_t`
2. Initialize sequence parser in `lle_unix_interface_init()`
3. Modify `lle_unix_interface_read_event()` to:
   - Read bytes into a small buffer
   - Pass buffer to `lle_sequence_parser_process_data()`
   - Convert resulting `lle_parsed_input_t` to `lle_input_event_t`
   - Handle timeout/incomplete sequences properly
4. Remove simple escape sequence detection code
5. Verify all PTY tests still pass
6. Add tests for complex sequences (F1-F12, Ctrl+Arrow, etc.)

**Phase 3** (Future):
- Full integration with Spec 06 input parsing system
- Replace terminal_abstraction's simple event system with full input_parser_system
- Unified architecture across all input handling

## Implementation Notes

### Converting lle_parsed_input_t to lle_input_event_t

```c
// When sequence parser returns a KEY event:
if (parsed_input->type == LLE_PARSED_INPUT_TYPE_KEY) {
    event->type = LLE_INPUT_TYPE_SPECIAL_KEY;
    event->data.special_key.key = map_keycode_to_lle_key(parsed_input->data.key_info.keycode);
    event->data.special_key.modifiers = parsed_input->data.key_info.modifiers;
}

// When sequence parser returns TEXT:
if (parsed_input->type == LLE_PARSED_INPUT_TYPE_TEXT) {
    event->type = LLE_INPUT_TYPE_CHARACTER;
    event->data.character.codepoint = parsed_input->data.text_info.codepoint;
    memcpy(event->data.character.utf8_bytes, 
           parsed_input->data.text_info.utf8_bytes,
           parsed_input->data.text_info.utf8_length);
    event->data.character.byte_count = parsed_input->data.text_info.utf8_length;
}
```

### Keycode Mapping

The sequence parser uses its own keycode system. We need a mapping function:

```c
static lle_special_key_t map_keycode_to_lle_key(uint32_t keycode) {
    // CSI A = Up arrow, CSI B = Down, CSI C = Right, CSI D = Left
    // CSI H = Home, CSI F = End
    // etc.
}
```

## Testing Strategy

1. Keep current simple parser as fallback with `#ifdef USE_SIMPLE_PARSER`
2. Implement full parser integration
3. Run PTY tests with both parsers
4. Compare results
5. Once full parser passes all tests, remove simple parser

## Timeline

- **Week 1**: Phase 2 implementation
- **Week 2**: Testing and refinement
- **Week 3**: Documentation and Phase 3 planning

## Status

- **Current**: Phase 1 complete, all PTY tests passing with simple parser
- **Next**: Begin Phase 2 - integrate full sequence_parser.c

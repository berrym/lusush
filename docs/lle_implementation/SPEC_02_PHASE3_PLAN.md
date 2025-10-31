# Spec 02 Phase 3 Implementation Plan: Event Reading

**Phase**: Terminal Abstraction - Phase 3  
**Estimated Lines**: ~250 lines  
**Dependencies**: Phase 1 (Capabilities), Phase 2 (State Management), Spec 06 (Input Parsing - optional integration)

## Overview

Implement `lle_unix_interface_read_event()` to read input events from the terminal with timeout support. This phase completes the terminal abstraction layer by providing the critical I/O primitive for real-time input processing.

## Objectives

1. Implement non-blocking input reading with configurable timeout
2. Support basic character input (UTF-8 aware)
3. Generate window resize events from SIGWINCH
4. Handle EOF and error conditions gracefully
5. Integrate with existing input parsing system (Spec 06)
6. Maintain performance target: <1ms for typical reads

## Architecture Decision: Integration vs. Standalone

### Option A: Full Integration with Spec 06 Input Parser
**Pros**:
- Complete Unicode/UTF-8 processing
- Escape sequence parsing
- Special key detection
- Comprehensive event generation

**Cons**:
- Requires Spec 06 parser system instantiation
- More complex initialization
- Potential performance overhead

### Option B: Minimal Standalone Implementation
**Pros**:
- Simple, focused implementation
- Direct byte reading with basic UTF-8
- Minimal dependencies
- Fast and predictable

**Cons**:
- Limited to basic character input
- No escape sequence parsing
- Special keys not detected

### **Decision: Option B (Minimal Standalone)**

**Rationale**:
1. Terminal abstraction should be a **primitive layer** - low-level I/O only
2. Higher-level parsing (Spec 06) should be **optional** and **composable**
3. Allows testing of terminal I/O independently of parsing
4. Simpler, more maintainable code
5. Higher-level integration happens in Spec 06's input coordinator

The terminal abstraction provides raw character reading; Spec 06 provides parsing. This maintains clean separation of concerns.

## Implementation Plan

### Core Function: `lle_unix_interface_read_event()`

**Location**: `src/lle/terminal_state.c` (update existing stub)

**Signature**:
```c
lle_result_t lle_unix_interface_read_event(lle_unix_interface_t *interface,
                                          lle_input_event_t *event,
                                          uint32_t timeout_ms);
```

**Behavior**:
1. Check for pending SIGWINCH (resize event)
2. Use `select()` to wait for input with timeout
3. Read available bytes (non-blocking)
4. Decode UTF-8 to Unicode codepoint
5. Populate event structure
6. Return appropriate status

**Return Values**:
- `LLE_SUCCESS` - Event read successfully
- `LLE_ERROR_TIMEOUT` - Timeout expired, no input
- `LLE_ERROR_EOF` - EOF detected (stdin closed)
- `LLE_ERROR_INVALID_PARAMETER` - NULL parameters
- `LLE_ERROR_SYSTEM_CALL` - System call failed

### Event Types Supported (Phase 3)

1. **Character Input** (`LLE_INPUT_TYPE_CHARACTER`)
   - Read bytes from stdin
   - Decode UTF-8 to Unicode codepoint
   - Store UTF-8 bytes and codepoint
   - Handle multi-byte sequences

2. **Window Resize** (`LLE_INPUT_TYPE_WINDOW_RESIZE`)
   - Check `interface->sigwinch_received` flag
   - Query current window size via `ioctl()`
   - Generate resize event
   - Clear flag

3. **Timeout** (`LLE_INPUT_TYPE_TIMEOUT`)
   - Return when `select()` times out
   - No data available within timeout period

4. **EOF** (`LLE_INPUT_TYPE_EOF`)
   - Detect when stdin is closed
   - `read()` returns 0

5. **Error** (`LLE_INPUT_TYPE_ERROR`)
   - System call failures
   - Invalid UTF-8 sequences (partial support)

### UTF-8 Decoding Strategy

**Simplified UTF-8 Decoder** (~80 lines):
- Determine UTF-8 sequence length from first byte
- Read additional bytes as needed (with timeout)
- Validate sequence structure
- Decode to Unicode codepoint
- Store both UTF-8 bytes and codepoint in event

**Validation**:
- Basic structure validation (continuation bytes)
- No overlong sequence detection (complexity vs. benefit)
- Invalid sequences treated as replacement character (U+FFFD)

**Performance**:
- Single-byte ASCII: 1 read operation
- Multi-byte UTF-8: 1-3 additional reads
- Buffering minimized (read what's available)

## Detailed Implementation

### 1. SIGWINCH Event Detection (~30 lines)

```c
/* Check for window resize first (highest priority) */
if (interface->sigwinch_received) {
    interface->sigwinch_received = false;
    
    size_t width, height;
    lle_result_t result = lle_unix_interface_get_window_size(interface, &width, &height);
    if (result != LLE_SUCCESS) {
        return result;
    }
    
    event->type = LLE_INPUT_TYPE_WINDOW_RESIZE;
    event->timestamp = lle_get_current_time_microseconds();
    event->data.resize.new_width = width;
    event->data.resize.new_height = height;
    interface->size_changed = true;
    
    return LLE_SUCCESS;
}
```

### 2. Input Availability Check (~40 lines)

```c
/* Use select() for timeout support */
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(interface->terminal_fd, &readfds);

struct timeval tv;
if (timeout_ms == UINT32_MAX) {
    /* Infinite timeout - block until input */
    tv.tv_sec = INT_MAX;
    tv.tv_usec = 0;
} else {
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
}

int ready = select(interface->terminal_fd + 1, &readfds, NULL, NULL, &tv);

if (ready == -1) {
    if (errno == EINTR) {
        /* Interrupted by signal - check for resize */
        if (interface->sigwinch_received) {
            /* Recursively handle resize event */
            return lle_unix_interface_read_event(interface, event, timeout_ms);
        }
        /* Other signal - return timeout */
        event->type = LLE_INPUT_TYPE_TIMEOUT;
        return LLE_SUCCESS;
    }
    return LLE_ERROR_SYSTEM_CALL;
}

if (ready == 0) {
    /* Timeout */
    event->type = LLE_INPUT_TYPE_TIMEOUT;
    event->timestamp = lle_get_current_time_microseconds();
    return LLE_SUCCESS;
}
```

### 3. Byte Reading (~30 lines)

```c
/* Read first byte */
unsigned char first_byte;
ssize_t bytes_read = read(interface->terminal_fd, &first_byte, 1);

if (bytes_read == -1) {
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        /* Non-blocking read, no data yet */
        event->type = LLE_INPUT_TYPE_TIMEOUT;
        event->timestamp = lle_get_current_time_microseconds();
        return LLE_SUCCESS;
    }
    /* Actual error */
    event->type = LLE_INPUT_TYPE_ERROR;
    event->timestamp = lle_get_current_time_microseconds();
    event->data.error.error_code = LLE_ERROR_SYSTEM_CALL;
    snprintf(event->data.error.error_message, sizeof(event->data.error.error_message),
             "read() failed: %s", strerror(errno));
    return LLE_ERROR_SYSTEM_CALL;
}

if (bytes_read == 0) {
    /* EOF */
    event->type = LLE_INPUT_TYPE_EOF;
    event->timestamp = lle_get_current_time_microseconds();
    return LLE_SUCCESS;
}
```

### 4. UTF-8 Decoding (~80 lines)

```c
/* Determine UTF-8 sequence length */
static int get_utf8_length(unsigned char first_byte) {
    if ((first_byte & 0x80) == 0x00) return 1;  /* 0xxxxxxx - ASCII */
    if ((first_byte & 0xE0) == 0xC0) return 2;  /* 110xxxxx */
    if ((first_byte & 0xF0) == 0xE0) return 3;  /* 1110xxxx */
    if ((first_byte & 0xF8) == 0xF0) return 4;  /* 11110xxx */
    return -1;  /* Invalid */
}

/* Decode UTF-8 sequence */
static lle_result_t decode_utf8(lle_unix_interface_t *interface,
                               unsigned char first_byte,
                               uint32_t *codepoint_out,
                               char *utf8_bytes,
                               uint8_t *byte_count_out) {
    int expected_bytes = get_utf8_length(first_byte);
    if (expected_bytes < 0) {
        /* Invalid first byte - use replacement character */
        *codepoint_out = 0xFFFD;
        utf8_bytes[0] = (char)first_byte;
        *byte_count_out = 1;
        return LLE_SUCCESS;
    }
    
    utf8_bytes[0] = (char)first_byte;
    *byte_count_out = expected_bytes;
    
    if (expected_bytes == 1) {
        /* ASCII */
        *codepoint_out = first_byte;
        return LLE_SUCCESS;
    }
    
    /* Read additional bytes */
    for (int i = 1; i < expected_bytes; i++) {
        unsigned char byte;
        ssize_t n = read(interface->terminal_fd, &byte, 1);
        
        if (n <= 0) {
            /* Incomplete sequence - use replacement character */
            *codepoint_out = 0xFFFD;
            return LLE_SUCCESS;
        }
        
        /* Validate continuation byte (10xxxxxx) */
        if ((byte & 0xC0) != 0x80) {
            *codepoint_out = 0xFFFD;
            return LLE_SUCCESS;
        }
        
        utf8_bytes[i] = (char)byte;
    }
    
    /* Decode to codepoint */
    switch (expected_bytes) {
        case 2:
            *codepoint_out = ((first_byte & 0x1F) << 6) |
                            (utf8_bytes[1] & 0x3F);
            break;
        case 3:
            *codepoint_out = ((first_byte & 0x0F) << 12) |
                            ((utf8_bytes[1] & 0x3F) << 6) |
                            (utf8_bytes[2] & 0x3F);
            break;
        case 4:
            *codepoint_out = ((first_byte & 0x07) << 18) |
                            ((utf8_bytes[1] & 0x3F) << 12) |
                            ((utf8_bytes[2] & 0x3F) << 6) |
                            (utf8_bytes[3] & 0x3F);
            break;
    }
    
    return LLE_SUCCESS;
}
```

### 5. Event Population (~20 lines)

```c
/* Populate character event */
event->type = LLE_INPUT_TYPE_CHARACTER;
event->timestamp = lle_get_current_time_microseconds();

uint32_t codepoint;
char utf8_bytes[8] = {0};
uint8_t byte_count;

decode_utf8(interface, first_byte, &codepoint, utf8_bytes, &byte_count);

event->data.character.codepoint = codepoint;
memcpy(event->data.character.utf8_bytes, utf8_bytes, byte_count);
event->data.character.byte_count = byte_count;

return LLE_SUCCESS;
```

## Testing Strategy

### Unit Tests (~300 lines)

**File**: `tests/lle/unit/test_terminal_event_reading.c`

1. **Timeout Tests** (3 tests)
   - Zero timeout (non-blocking poll)
   - Short timeout (100ms)
   - Infinite timeout (mock with signal)

2. **Character Reading Tests** (5 tests)
   - ASCII character reading
   - UTF-8 2-byte character (é)
   - UTF-8 3-byte character (€)
   - UTF-8 4-byte character (𝄞)
   - Invalid UTF-8 (replacement character)

3. **Window Resize Tests** (2 tests)
   - SIGWINCH generates resize event
   - Resize event has priority over input

4. **EOF Handling** (1 test)
   - EOF detection when stdin closed

5. **Error Handling** (2 tests)
   - NULL parameter validation
   - System call error handling

6. **Integration Tests** (2 tests)
   - Multiple events in sequence
   - Interleaved resize and character events

**Test Challenges**:
- Need to mock stdin for deterministic input
- Use pipes for simulating input streams
- Signal testing requires careful setup

### Manual Testing

Interactive test program for validation:
```c
/* tests/lle/manual/test_terminal_input.c */
int main() {
    lle_unix_interface_t *interface;
    lle_unix_interface_init(&interface);
    lle_unix_interface_enter_raw_mode(interface);
    
    printf("Type characters (Ctrl-D to exit):\n");
    
    while (1) {
        lle_input_event_t event;
        lle_result_t result = lle_unix_interface_read_event(interface, &event, 1000);
        
        if (result == LLE_SUCCESS) {
            switch (event.type) {
                case LLE_INPUT_TYPE_CHARACTER:
                    printf("Char: U+%04X (%s)\n",
                           event.data.character.codepoint,
                           event.data.character.utf8_bytes);
                    break;
                case LLE_INPUT_TYPE_WINDOW_RESIZE:
                    printf("Resize: %zux%zu\n",
                           event.data.resize.new_width,
                           event.data.resize.new_height);
                    break;
                case LLE_INPUT_TYPE_TIMEOUT:
                    printf("Timeout\n");
                    break;
                case LLE_INPUT_TYPE_EOF:
                    printf("EOF\n");
                    goto cleanup;
            }
        }
    }
    
cleanup:
    lle_unix_interface_exit_raw_mode(interface);
    lle_unix_interface_destroy(interface);
    return 0;
}
```

## Performance Targets

| Operation | Target | Measurement |
|-----------|--------|-------------|
| ASCII read | <100μs | Single byte read + decode |
| UTF-8 read | <200μs | Multi-byte read + decode |
| Timeout (no input) | <50μs | select() overhead |
| Resize event | <100μs | Flag check + ioctl |

## Error Handling

### Graceful Degradation
- Invalid UTF-8 → Replacement character (U+FFFD)
- Interrupted system calls → Retry or timeout
- EOF → Clean shutdown event
- Partial UTF-8 → Replacement character

### Error Recovery
- No state corruption on errors
- Interface remains usable after error
- Error events contain diagnostic information

## Integration with Spec 06

While Phase 3 provides basic character reading, Spec 06's input parser can **wrap** this interface:

```c
/* Spec 06 integration pattern */
lle_result_t lle_input_parser_read(lle_input_parser_system_t *parser) {
    lle_input_event_t raw_event;
    lle_result_t result = lle_unix_interface_read_event(
        parser->unix_interface, &raw_event, parser->timeout_ms);
    
    if (result == LLE_SUCCESS && raw_event.type == LLE_INPUT_TYPE_CHARACTER) {
        /* Feed character to parser for escape sequence detection */
        lle_input_stream_buffer_data(parser->stream,
                                     raw_event.data.character.utf8_bytes,
                                     raw_event.data.character.byte_count);
        /* Continue parsing... */
    }
    
    /* Handle other event types... */
}
```

This allows:
- Terminal abstraction to remain simple and focused
- Input parsing to be optional and composable
- Testing at multiple levels of abstraction

## Files to Modify

1. **src/lle/terminal_state.c**
   - Replace stub implementation of `lle_unix_interface_read_event()`
   - Add UTF-8 decoder helper functions
   - Add SIGWINCH event handling
   - **Estimated**: +250 lines

2. **tests/lle/unit/test_terminal_event_reading.c** (new)
   - Comprehensive unit tests
   - **Estimated**: ~300 lines

3. **tests/lle/manual/test_terminal_input.c** (new)
   - Interactive testing program
   - **Estimated**: ~100 lines

4. **meson.build**
   - Add new unit test
   - Add manual test (optional build target)
   - **Estimated**: +15 lines

## Zero-Tolerance Compliance Checklist

- [ ] No TODOs in implementation
- [ ] No stub functions (except future integration points)
- [ ] All error paths tested
- [ ] All memory allocations have corresponding frees
- [ ] All parameters validated
- [ ] All system calls checked for errors
- [ ] UTF-8 validation implemented
- [ ] Timeout handling complete
- [ ] Signal handling integrated
- [ ] Documentation complete

## Risk Assessment

### Low Risk
- ✅ Simple, well-understood POSIX APIs (`select()`, `read()`)
- ✅ UTF-8 decoding is straightforward
- ✅ Existing Phase 2 infrastructure solid

### Medium Risk
- ⚠️ Testing requires mocking stdin (solvable with pipes)
- ⚠️ Signal handling race conditions (mitigated by careful ordering)

### Mitigation
- Use pipes for deterministic testing
- Check SIGWINCH flag before and after `select()`
- Comprehensive error handling
- Manual testing for validation

## Success Criteria

1. ✅ All unit tests pass (100% coverage)
2. ✅ Manual test demonstrates real keyboard input
3. ✅ Window resize events generated correctly
4. ✅ UTF-8 characters decoded properly
5. ✅ Timeout behavior correct (non-blocking, short, long)
6. ✅ Clean build with no warnings
7. ✅ Performance targets met
8. ✅ Zero-tolerance compliance verified

## Estimated Effort

- **Implementation**: ~2-3 hours
- **Testing**: ~2 hours
- **Documentation**: ~1 hour
- **Total**: ~5-6 hours

## Next Steps After Phase 3

With Phase 3 complete, Spec 02 (Terminal Abstraction) will be **fully implemented**:
- ✅ Phase 1: Terminal Capability Detection
- ✅ Phase 2: Terminal State Management
- ✅ Phase 3: Event Reading

This enables:
1. **Real input testing** for Spec 06 (Input Parsing)
2. **Interactive testing tools** for LLE development
3. **Integration testing** with live terminal I/O
4. **Full terminal abstraction** for Spec 10 (Master Coordinator)

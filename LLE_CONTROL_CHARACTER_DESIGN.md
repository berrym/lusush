# LLE Control Character Handling - Technical Design Document

## Overview

This document defines the proper separation of concerns for control character handling in the Lusush Line Editor (LLE) to ensure compatibility with Unix signal conventions, terminal control, and shell functionality.

## Problem Statement

Interactive shells must handle three distinct classes of control characters:
1. **Signal-generating characters** that trigger Unix signals (SIGINT, SIGQUIT, SIGTSTP)
2. **Terminal control characters** that manage flow control and terminal state
3. **Line editing characters** that provide text manipulation within the line editor

Improper handling can lead to conflicts between the line editor, shell signal handling, and terminal driver functionality.

## Design Principles

### 1. Separation of Concerns
Each subsystem handles only the control characters appropriate to its domain:
- **Shell**: Signal-generating characters (`Ctrl+C`, `Ctrl+\`, `Ctrl+Z`)
- **Terminal Driver**: Flow control characters (`Ctrl+S`, `Ctrl+Q`)
- **Line Editor**: Text editing characters (`Ctrl+A`, `Ctrl+E`, `Ctrl+K`, etc.)

### 2. Unix Compatibility
LLE follows standard Unix/POSIX conventions for control character behavior to ensure familiar user experience and proper integration with system tools.

### 3. Graceful Degradation
When running in non-standard environments, LLE provides fallback behavior while maintaining core functionality.

## Control Character Classification

### Class 1: Signal-Generating Characters (Shell Domain)

| Character | ASCII | Signal | Default Action | LLE Behavior |
|-----------|-------|--------|----------------|--------------|
| `Ctrl+C`  | 0x03  | SIGINT | Interrupt process | Cancel line (fallback) |
| `Ctrl+\`  | 0x1C  | SIGQUIT | Quit with core dump | Ignore (let shell handle) |
| `Ctrl+Z`  | 0x1A  | SIGTSTP | Suspend process | Ignore (let shell handle) |

**Rationale**: These characters generate Unix signals that must be handled by the shell's signal handling system, not intercepted by the line editor.

### Class 2: Terminal Control Characters (Terminal Driver Domain)

| Character | ASCII | Function | Default Action | LLE Behavior |
|-----------|-------|----------|----------------|--------------|
| `Ctrl+S`  | 0x13  | XOFF | Stop terminal output | Ignore (let terminal handle) |
| `Ctrl+Q`  | 0x11  | XON | Resume terminal output | Ignore (let terminal handle) |

**Rationale**: Flow control is managed by the terminal driver and affects all terminal output, not just the line editor.

### Class 3: Line Editing Characters (LLE Domain)

| Character | ASCII | Function | LLE Action |
|-----------|-------|----------|------------|
| `Ctrl+A`  | 0x01  | Beginning of line | Move cursor to start |
| `Ctrl+E`  | 0x05  | End of line | Move cursor to end |
| `Ctrl+K`  | 0x0B  | Kill line | Delete to end of line |
| `Ctrl+U`  | 0x15  | Kill line backward | Delete to beginning |
| `Ctrl+W`  | 0x17  | Kill word backward | Delete previous word |
| `Ctrl+H`  | 0x08  | Backspace | Delete previous character |
| `Ctrl+D`  | 0x04  | Delete/EOF | Delete char or EOF if empty |
| `Ctrl+L`  | 0x0C  | Clear screen | Clear and redraw |
| `Ctrl+_`  | 0x1F  | Undo | Undo last operation |
| `Ctrl+Y`  | 0x19  | Yank | Paste from kill ring |

**Rationale**: These characters provide standard readline/emacs-style line editing functionality.

## Implementation Strategy

### 1. Raw Mode Considerations

When LLE puts the terminal in raw mode:
- Signal-generating characters lose their automatic signal behavior
- LLE must explicitly ignore these characters to allow shell handling
- Terminal flow control may need special handling

### 2. Character Processing Pipeline

```
Input Character → Key Event Processing → Character Classification → Handler Selection

Signal chars    → Ignore (pass to shell)
Terminal chars  → Ignore (pass to terminal driver) 
Editing chars   → Process in LLE
Printable chars → Insert into buffer
```

### 3. Error Handling

When signal-generating characters are received:
- Log the event for debugging
- Do not update display
- Allow normal signal processing to occur
- Provide fallback behavior if signals are blocked

### 4. Testing Strategy

Test cases must verify:
- Signal characters don't interfere with signal handling
- Terminal control characters don't disrupt flow control
- Line editing characters work correctly
- Proper behavior in both TTY and non-TTY environments

## Code Implementation

### Character Constants

```c
// Signal-generating characters (ignored by LLE)
#define LLE_ASCII_CTRL_C         0x03   // SIGINT
#define LLE_ASCII_CTRL_BACKSLASH 0x1C   // SIGQUIT  
#define LLE_ASCII_CTRL_Z         0x1A   // SIGTSTP

// Terminal control characters (ignored by LLE)
#define LLE_ASCII_CTRL_S         0x13   // XOFF
#define LLE_ASCII_CTRL_Q         0x11   // XON

// Line editing characters (handled by LLE)
#define LLE_ASCII_CTRL_UNDERSCORE 0x1F  // Undo
// ... others as needed
```

### Processing Logic

```c
case LLE_KEY_CHAR:
    if (event.character == LLE_ASCII_CTRL_UNDERSCORE) {
        // Handle undo in LLE
        handle_undo();
    }
    else if (event.character == LLE_ASCII_CTRL_BACKSLASH ||
             event.character == LLE_ASCII_CTRL_S ||
             event.character == LLE_ASCII_CTRL_Q) {
        // Ignore - let shell/terminal handle
        needs_display_update = false;
    }
    else if (event.character >= 32 && event.character <= 126) {
        // Insert printable characters
        insert_character(event.character);
    }
    break;
```

## Special Cases

### Ctrl+D Behavior

`Ctrl+D` has dual behavior in Unix:
- **EOF when buffer is empty**: Signals end of input
- **Delete character when buffer has content**: Standard editing

LLE implements both behaviors correctly:
```c
case LLE_KEY_CTRL_D:
    if (editor->buffer->length == 0) {
        line_cancelled = true;  // EOF behavior
    } else {
        cmd_result = lle_cmd_delete_char(editor->display);  // Delete behavior
    }
    break;
```

### Ctrl+C Handling

`Ctrl+C` should generate SIGINT, but in raw mode this doesn't happen automatically:
- **Primary**: Shell signal handler should process SIGINT
- **Fallback**: LLE cancels current line if signal handling fails
- **Future**: Integrate with shell signal system for proper behavior

## Testing Requirements

### Signal Integration Tests
- Verify `Ctrl+C` generates SIGINT in shell context
- Verify `Ctrl+Z` properly suspends shell process
- Verify `Ctrl+\` generates SIGQUIT when appropriate

### Terminal Integration Tests  
- Verify `Ctrl+S`/`Ctrl+Q` flow control works
- Test behavior in both canonical and raw terminal modes
- Verify terminal state restoration after line editing

### Line Editor Tests
- Verify all editing characters work correctly
- Test undo functionality with `Ctrl+_`
- Verify kill ring operations with `Ctrl+Y`

## Future Enhancements

### Signal Coordination
Implement proper coordination between LLE and shell signal handling:
- LLE registers signal handlers that coordinate with shell
- Proper cleanup when signals are received during line editing
- Graceful handling of signal delivery timing

### Terminal Mode Management
Improve terminal mode transitions:
- Minimize raw mode duration
- Proper restoration of terminal attributes
- Better handling of terminal size changes

### Configuration
Allow customization of control character behavior:
- User-configurable key bindings
- Option to disable signal character processing
- Custom signal handling integration

## Compatibility Matrix

| Environment | Signal Handling | Terminal Control | Line Editing |
|-------------|----------------|------------------|--------------|
| TTY + Shell | Shell handles signals | Terminal driver | LLE handles editing |
| Non-TTY | Fallback behavior | Limited control | LLE handles editing |
| Embedded | Configurable | Application-specific | LLE handles editing |

## References

- **POSIX.1-2017**: Terminal interface specifications
- **GNU Readline Manual**: Standard line editing behavior
- **Advanced Programming in the UNIX Environment**: Signal handling
- **The TTY Demystified**: Terminal driver behavior
- **Bash Manual**: Shell signal handling and job control

---

**Document Version**: 1.0  
**Last Updated**: December 2024  
**Status**: Implemented in LLE-039  
**Review Required**: Before LLE-040 implementation
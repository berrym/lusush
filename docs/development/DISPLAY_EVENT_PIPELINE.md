# Display Event Pipeline Architecture

This document describes the LLE (Lusush Line Editor) display system architecture,
the event pipeline that drives rendering, and critical state management considerations.

**Last Updated**: 2025-11-30 (Session 37)

---

## Overview

The LLE display system uses a layered architecture with an event-driven pipeline.
Understanding this architecture is essential for debugging display issues, as
problems often manifest as missing events, stale state, or incorrect cursor positioning.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            LLE DISPLAY PIPELINE                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  lle_readline()                                                             │
│       │                                                                     │
│       ▼                                                                     │
│  refresh_display(ctx)                                                       │
│       │                                                                     │
│       ├── render_controller  ──►  lle_render_buffer_content()              │
│       │                                  │                                  │
│       │                                  ▼                                  │
│       │                           render_output                             │
│       │                                  │                                  │
│       ▼                                  │                                  │
│  display_bridge  ◄───────────────────────┘                                  │
│       │                                                                     │
│       ▼                                                                     │
│  lle_display_bridge_send_output()                                          │
│       │                                                                     │
│       ├── command_layer_set_command()                                      │
│       │        │                                                            │
│       │        ├── [OPTIMIZATION CHECK]                                    │
│       │        │    if (!changed && !first_render) return early            │
│       │        │                                                            │
│       │        └── publish_command_event(LAYER_EVENT_REDRAW_NEEDED)        │
│       │                     │                                               │
│       │                     ▼                                               │
│       └── layer_events_process_pending()                                   │
│                     │                                                       │
│                     ▼                                                       │
│            dc_handle_redraw_needed()                                        │
│                     │                                                       │
│                     ▼                                                       │
│            [TERMINAL WRITES]                                                │
│            - Prompt (first render only)                                     │
│            - Command text                                                   │
│            - Ghost text (autosuggestion)                                    │
│            - Completion menu                                                │
│            - Cursor positioning                                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Key Components

### 1. refresh_display() - Entry Point
**Location**: `src/lle/lle_readline.c`

The main entry point for all display updates. Called whenever the buffer changes
or display needs updating.

```c
static void refresh_display(readline_context_t *ctx)
{
    // 1. Update autosuggestion (if not suppressed)
    update_autosuggestion(ctx);
    
    // 2. Get display integration and render controller
    lle_display_integration_t *display_integration = lle_display_integration_get_global();
    lle_render_controller_t *render_controller = display_integration->render_controller;
    
    // 3. Render buffer content
    lle_render_output_t *render_output = NULL;
    lle_render_buffer_content(render_controller, ctx->buffer, &ctx->buffer->cursor, &render_output);
    
    // 4. Send through display bridge
    lle_display_bridge_send_output(display_bridge, render_output, &ctx->buffer->cursor);
}
```

### 2. display_bridge - Pipeline Coordinator
**Location**: `src/lle/display_bridge.c`

Coordinates between LLE's render system and the layered display system.

```c
lle_result_t lle_display_bridge_send_output(...)
{
    // 1. Update command_layer with new text
    command_layer_set_command(cmd_layer, command_text, cursor_pos);
    
    // 2. Process pending events (triggers dc_handle_redraw_needed)
    layer_events_process_pending(bridge->layer_events, 10, 0);
}
```

### 3. command_layer - Text and Event Management
**Location**: `src/display/command_layer.c`

Manages command text, syntax highlighting, and publishes display events.

**CRITICAL OPTIMIZATION** (line ~308):
```c
bool is_first_render = (layer->update_sequence_number == 0);

if (!command_changed && !cursor_changed && !is_first_render && !menu_changed) {
    // No change - return early WITHOUT publishing event
    return COMMAND_LAYER_SUCCESS;  // <-- SILENT RETURN!
}

// ... perform updates ...

publish_command_event(layer, LAYER_EVENT_REDRAW_NEEDED);
```

**Why This Matters**: If `update_sequence_number > 0` and buffer is empty→empty,
no event is published and `dc_handle_redraw_needed()` is never called.

### 4. layer_events - Event System
**Location**: `src/display/layer_events.c`

Publisher/subscriber event system for inter-layer communication.

Key event types:
- `LAYER_EVENT_REDRAW_NEEDED` - Triggers terminal rendering
- `LAYER_EVENT_CONTENT_CHANGED` - Content updated (informational)
- `LAYER_EVENT_CURSOR_MOVED` - Cursor position changed

### 5. display_controller - Terminal Rendering
**Location**: `src/display/display_controller.c`

Subscribes to `LAYER_EVENT_REDRAW_NEEDED` and performs actual terminal I/O.

```c
static layer_events_error_t dc_handle_redraw_needed(...)
{
    // Step 1: Move to command start column
    // Step 2: Handle ghost text cleanup from previous render
    // Step 3: Clear command area (\033[J)
    // Step 4: Write command text, ghost text, menu
    // Step 5: Position cursor correctly
}
```

### 6. autosuggestions_layer - Ghost Text
**Location**: `src/display/autosuggestions_layer.c`

Manages Fish-style autosuggestions (ghost text).

**CRITICAL**: When clearing autosuggestion, must publish `LAYER_EVENT_REDRAW_NEEDED`:
```c
autosuggestions_layer_error_t autosuggestions_layer_publish_change(...)
{
    // Publish CONTENT_CHANGED (informational)
    layer_events_publish_content_changed(...);
    
    // ALSO publish REDRAW_NEEDED (triggers rendering)
    layer_events_publish_simple(layer->event_system,
                               LAYER_EVENT_REDRAW_NEEDED,
                               LAYER_ID_AUTOSUGGESTIONS,
                               0,
                               LAYER_EVENT_PRIORITY_HIGH);
}
```

---

## Critical State Variables

### prompt_rendered (display_controller.c)
```c
static bool prompt_rendered = false;
```
- `false`: Next render will write prompt to terminal
- `true`: Prompt already on screen, only update command area

### last_terminal_end_row (display_controller.c)
```c
static int last_terminal_end_row = 0;
```
Tracks where terminal cursor ended after previous render (including ghost text/menu).
Used in Step 2 to clean up wrapped ghost text.

### update_sequence_number (command_layer)
```c
layer->update_sequence_number
```
- `0`: Next command_layer_set_command() is "first render" (always publishes event)
- `>0`: Optimization check applies (may skip event if no change)

### suppress_autosuggestion (readline_context_t)
```c
ctx->suppress_autosuggestion
```
- `true`: update_autosuggestion() returns early (prevents regeneration)
- `false`: Normal autosuggestion behavior

---

## Reset Functions

### dc_reset_prompt_display_state()
**Location**: `src/display/display_controller.c:153`

Called at start of each new readline session. Resets all display state:

```c
void dc_reset_prompt_display_state(void) {
    prompt_rendered = false;
    last_terminal_end_row = 0;
    
    if (screen_buffer_initialized) {
        screen_buffer_clear(&current_screen);
        screen_buffer_clear(&desired_screen);
    }
    
    // CRITICAL: Reset command_layer sequence number
    // Without this, empty-to-empty transitions skip event publishing
    display_controller_t *dc = display_integration_get_controller();
    if (dc && dc->compositor && dc->compositor->command_layer) {
        dc->compositor->command_layer->update_sequence_number = 0;
    }
}
```

### dc_finalize_input()
**Location**: `src/display/display_controller.c:168`

Called when readline returns. Moves cursor to new line and resets state:

```c
void dc_finalize_input(void) {
    write(STDOUT_FILENO, "\n", 1);  // Move to next line
    dc_reset_prompt_display_state();
}
```

---

## Common Bugs and Solutions

### Bug: Prompt not drawn after abort
**Symptom**: Ctrl+G on empty buffer shows blank line, no prompt  
**Cause**: `update_sequence_number > 0` from previous session, optimization skips event  
**Solution**: Reset `update_sequence_number = 0` in `dc_reset_prompt_display_state()`

### Bug: Ghost text remains after clearing
**Symptom**: Ctrl+G clears autosuggestion internally but ghost text stays visible  
**Cause**: autosuggestions_layer only published CONTENT_CHANGED, not REDRAW_NEEDED  
**Solution**: Publish LAYER_EVENT_REDRAW_NEEDED when suggestion changes

### Bug: Autosuggestion immediately reappears after clearing
**Symptom**: Ctrl+G clears suggestion, refresh_display() regenerates it  
**Cause**: update_autosuggestion() called during refresh after clear  
**Solution**: Set `ctx->suppress_autosuggestion = true` before clearing

### Bug: Cursor on wrong row after prompt
**Symptom**: Prompt draws correctly but cursor is one row below  
**Cause**: `last_terminal_end_row` or `current_screen.cursor_row` has stale value  
**Solution**: Ensure proper reset in `dc_reset_prompt_display_state()`

### Bug: Double newline on abort
**Symptom**: Ctrl+G produces two blank lines before new prompt  
**Cause**: `dc_finalize_input()` called twice (in abort handler AND readline cleanup)  
**Solution**: Only call in one place (let readline cleanup handle it)

---

## Debugging Techniques

### 1. Trace Event Flow
Add fprintf to key points:
```c
// In dc_handle_redraw_needed():
fprintf(stderr, "[DC] redraw called, prompt_rendered=%d\n", prompt_rendered);

// In command_layer_set_command():
fprintf(stderr, "[CMD] is_first_render=%d, changed=%d\n", is_first_render, command_changed);

// In layer_events_process_pending():
fprintf(stderr, "[EVENTS] processed %d events\n", count);
```

### 2. Check Event Count
If `layer_events_process_pending()` returns 0 when you expect events, the
event was never published (likely early return in command_layer_set_command).

### 3. Verify State Resets
Add assertions or debug prints to verify critical state is reset:
```c
// At start of lle_readline():
assert(prompt_rendered == false);
assert(command_layer->update_sequence_number == 0);
```

---

## Rendering Steps (dc_handle_redraw_needed)

### Step 1: Position to Command Start
```c
// Move to absolute column (1-based)
snprintf(move_to_col, "\033[%dG", command_start_col + 1);
write(STDOUT_FILENO, move_to_col, ...);
```

### Step 2: Ghost Text Cleanup
If previous render had ghost text wrapping to extra rows:
```c
if (last_terminal_end_row > current_screen.cursor_row) {
    // Move DOWN to end of previous ghost text
    // Clear from there (\033[J)
    // Move back UP to command row
}
```

### Step 3: Clear Command Area
```c
write(STDOUT_FILENO, "\033[J", 3);  // Clear to end of screen
```

### Step 4: Write Content
1. Command text (with syntax highlighting)
2. Ghost text (autosuggestion) in dim color
3. Completion menu (if visible)

### Step 5: Position Cursor
```c
// Move up if needed (from ghost text/menu end to cursor row)
if (rows_to_move_up > 0) {
    snprintf(up_seq, "\033[%dA", rows_to_move_up);
    write(STDOUT_FILENO, up_seq, ...);
}

// Move to absolute column
snprintf(col_seq, "\033[%dG", cursor_col + 1);
write(STDOUT_FILENO, col_seq, ...);

// Update tracking
last_terminal_end_row = current_terminal_row;
```

---

## Layer Hierarchy

```
                    ┌─────────────────────┐
                    │  display_controller │  (Terminal I/O, event subscriber)
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │     compositor      │  (Layer coordination)
                    └──────────┬──────────┘
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
┌─────────▼─────────┐ ┌────────▼────────┐ ┌────────▼────────┐
│   prompt_layer    │ │  command_layer  │ │autosuggestions  │
│                   │ │                 │ │    _layer       │
│ - Prompt content  │ │ - Command text  │ │ - Ghost text    │
│ - Multi-line      │ │ - Syntax hl     │ │ - Suggestions   │
│ - Git status      │ │ - Cursor pos    │ │ - Cache         │
└───────────────────┘ └─────────────────┘ └─────────────────┘
```

---

## Event Types Reference

| Event Type | Publisher | Subscriber | Purpose |
|------------|-----------|------------|---------|
| REDRAW_NEEDED | command_layer, autosuggestions_layer | display_controller | Trigger terminal render |
| CONTENT_CHANGED | command_layer, autosuggestions_layer | Various | Notify content update |
| CURSOR_MOVED | command_layer | Various | Notify cursor position change |
| SIZE_CHANGED | terminal_control | All layers | Terminal resize |

---

## File Quick Reference

| File | Purpose |
|------|---------|
| `src/lle/lle_readline.c` | Main readline loop, refresh_display() |
| `src/lle/display_bridge.c` | LLE ↔ display system bridge |
| `src/display/display_controller.c` | Terminal rendering, dc_handle_redraw_needed() |
| `src/display/command_layer.c` | Command text, syntax highlighting, events |
| `src/display/autosuggestions_layer.c` | Fish-style ghost text |
| `src/display/layer_events.c` | Event pub/sub system |
| `src/display/screen_buffer.c` | Screen state tracking |
| `src/display/prompt_layer.c` | Prompt rendering |

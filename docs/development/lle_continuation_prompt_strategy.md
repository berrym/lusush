# LLE Continuation Prompt Implementation Strategy

**Document**: lle_continuation_prompt_strategy.md  
**Date**: 2025-11-08  
**Status**: Planning / Design Phase  
**Purpose**: Explore architectural approaches for adding continuation prompts to LLE multiline input

---

## Background: The Problem

### User Report
LLE currently provides no visual continuation prompts when entering multiline commands:
```bash
for i in 1 2 3; do
echo $i
done
```

Users expect visual feedback (like `loop>` or `>`) to indicate shell is still reading multiline input.

### Current Status
- ✅ Multiline editing **works functionally** (detects incomplete, inserts newlines, executes correctly)
- ✅ Shared continuation parser correctly identifies incomplete constructs
- ❌ **No visual prompts** displayed to user

### Why Simple Solutions Don't Work

**Architectural Constraint**: LLE uses single-buffer model (one `lle_readline()` call for entire multiline command), unlike GNU Readline which calls `readline()` multiple times with different prompts.

**Failed Approach**: Injecting prompts into buffer
```c
// WRONG: Breaks shell parsing
Buffer: "for i in 1; do\nloop> echo $i"
                         ^^^^^^ Invalid POSIX syntax
```

**Failed Approach**: Updating prompt_layer on each Enter
```c
// WRONG: Display model renders prompt once at start, not per-line
prompt_layer_set_content(layer, "loop>");
```

---

## Architecture Analysis: How Other Shells Handle This

### GNU Readline (Lusush default)
- **Architecture**: Multi-call model
- **Prompts**: Context-aware (`loop>`, `if>`, `quote>`, `case>`, `function>`)
- **How it works**: Each line is a separate `readline()` call with its own prompt displayed **outside** returned text

```c
while (incomplete) {
    prompt = get_context_aware_prompt();  // "loop>", "if>", etc.
    line = readline(prompt);  // NEW call for each line
    accumulate(line);
}
```

### ZSH ZLE (Observed behavior)
- **Architecture**: Likely single-buffer (based on "ZLE buffer" terminology)
- **Prompts**: Simple, fixed continuation marker (default: `∙` or PS2 value)
- **How it works**: Continuation prompt injected during display, not in buffer

```zsh
❯ for i in 1 2 3; do
∙ echo $i   
∙ done
```

**Key Insight**: ZSH uses **simple, fixed marker** instead of context-aware prompts. Much easier to implement!

### LLE (Current)
- **Architecture**: Single-buffer model
- **Prompts**: None
- **How it works**: Buffer contains `"for...\necho\ndone"` with embedded newlines

---

## Proposed Solution: ZSH-Style Simple Continuation Prompts

### Middle Ground Approach

Adopt ZSH's simpler strategy:
- Use **fixed continuation marker** (e.g., `> ` from PS2)
- Inject prompts in **display output only** (not in buffer)
- Maintain buffer/display separation
- Provide visual feedback without complexity of context-aware prompts

### Benefits
1. ✅ Much simpler than context-aware prompts
2. ✅ Fixed string, no dynamic parsing needed
3. ✅ Only requires composition engine enhancement
4. ✅ Provides visual feedback (better than nothing)
5. ✅ Maintains buffer/display separation (prompts not in buffer)

### Limitations
1. ❌ Less expressive than GNU Readline's context-aware prompts
2. ⚠️ Requires cursor position translation
3. ⚠️ Requires composition engine modification

---

## Implementation Design

### Core Challenge: Cursor Position Translation

**The Problem**: When prompts are injected into display, cursor positions diverge between buffer and display.

**Buffer (what gets executed)**:
```
"for i in 1 2 3; do\necho $i\ndone"
 ^              ^  ^          ^    ^
 0              15 16         25   30  ← Buffer byte offsets
```

**Display (what user sees with injected `> `)**:
```
for i in 1 2 3; do
> echo $i
> done
^              ^  ^^        ^^ ^    ^
0              15 16 17     26 27   32  ← Display positions
                   ↑ prompt   ↑ prompt
```

**Mismatch**: Each injected prompt shifts subsequent cursor positions in display.

### Solution: Display Position Translation Layer

**Key Insight**: Cursor continues operating in **buffer space** (no changes needed). Only rendering needs translation.

#### Translation Function
```c
/**
 * Translate buffer position to display position accounting for injected prompts
 * 
 * @param buffer_content The actual command buffer text
 * @param buffer_pos Cursor position in buffer (byte offset)
 * @param continuation_prompt The PS2 prompt string (e.g., "> ")
 * @return Display position where cursor should appear on screen
 */
size_t translate_buffer_to_display_position(
    const char *buffer_content,
    size_t buffer_pos,
    const char *continuation_prompt
) {
    size_t display_pos = 0;
    size_t prompt_len = strlen(continuation_prompt);
    
    // Walk through buffer up to cursor position
    for (size_t i = 0; i < buffer_pos; i++) {
        if (buffer_content[i] == '\n') {
            // Found newline - continuation prompt will be injected after it
            display_pos += 1;  // The newline itself
            display_pos += prompt_len;  // The injected prompt
        } else {
            display_pos += 1;  // Regular character
        }
    }
    
    return display_pos;
}
```

#### Composition Engine Changes
```c
// In compose_multiline_strategy()

const char *ps2 = "> ";  // Simple, fixed continuation prompt (from config)
char *output_ptr = output;

// Copy prompt
strcpy(output_ptr, prompt_content);
output_ptr += strlen(prompt_content);

// Process command content line by line
const char *line_start = command_content;
const char *ptr = command_content;

while (*ptr) {
    if (*ptr == '\n') {
        // Copy line up to newline
        size_t line_len = ptr - line_start;
        memcpy(output_ptr, line_start, line_len);
        output_ptr += line_len;
        
        // Add newline
        *output_ptr++ = '\n';
        
        // Inject continuation prompt
        strcpy(output_ptr, ps2);
        output_ptr += strlen(ps2);
        
        // Move to start of next line
        ptr++;
        line_start = ptr;
    } else {
        ptr++;
    }
}

// Copy final line
size_t final_len = ptr - line_start;
memcpy(output_ptr, line_start, final_len);
output_ptr += final_len;

// Null terminate
*output_ptr = '\0';

// Translate cursor position for display
size_t display_cursor_pos = translate_buffer_to_display_position(
    command_content,
    cursor_byte_offset,
    ps2
);
```

### Architectural Separation of Concerns

```
┌─────────────────────────────────────────────┐
│ Buffer Space (Source of Truth)             │
│ "for i in 1; do\necho\ndone"               │
│ Cursor at byte offset: 16                  │
│                                             │
│ - Buffer operations (insert/delete/move)   │
│ - Cursor manager                            │
│ - Keybinding actions                        │
│ - History system                            │
│ - Execution logic                           │
└─────────────────┬───────────────────────────┘
                  │
                  │ translate_buffer_to_display()
                  │ (composition engine)
                  ↓
┌─────────────────────────────────────────────┐
│ Display Space (Presentation Layer)         │
│ "for i in 1; do\n> echo\n> done"           │
│ Cursor at display position: 18             │
│                                             │
│ - Visual rendering                          │
│ - Terminal cursor positioning               │
│ - ANSI escape sequences                     │
└─────────────────────────────────────────────┘
```

### What Stays The Same
- ✅ Buffer operations (insert, delete, move cursor)
- ✅ Cursor manager logic (operates in buffer space)
- ✅ Keybinding actions (buffer operations)
- ✅ History system (stores buffer content)
- ✅ Execution logic (receives buffer content)

### What Changes
1. **Composition Engine** (Medium effort):
   - Modify `compose_multiline_strategy()` to inject prompts
   - Add `translate_buffer_to_display_position()` function
   - Return both buffer and display cursor positions

2. **Display Controller** (Low effort):
   - Use translated display position for cursor placement
   - Already separates buffer from display (just use new position)

---

## Implementation Phases

### Phase 1: Simple Fixed Prompt (Recommended Starting Point)
**Scope**: Implement basic `> ` continuation marker (like default PS2)

**Changes**:
- Add translation function to composition engine
- Modify `compose_multiline_strategy()` to inject fixed `> ` at newlines
- Update cursor positioning to use translated coordinates
- Test with various multiline constructs

**Estimated Effort**: 4-5 hours of focused work
- Composition engine changes: 1-2 hours
- Translation function: 30 minutes
- Display integration: 30 minutes
- Testing: 1-2 hours

**User Value**: Immediate visual feedback for multiline input

### Phase 2: Configurable PS2 (Enhancement)
**Scope**: Support custom PS2 from config system

**Changes**:
- Read PS2 value from config or environment
- Pass PS2 to composition engine
- Support ANSI color sequences in PS2

**Estimated Effort**: 1-2 hours

**User Value**: Customizable continuation prompts

### Phase 3: Context-Aware Prompts (Advanced)
**Scope**: GNU Readline-style prompts (`loop>`, `if>`, `quote>`, etc.)

**Changes**:
- Pass continuation parser state to composition engine
- Dynamic prompt selection based on construct type
- Update translation to handle variable-length prompts

**Estimated Effort**: 1 additional day

**User Value**: More expressive feedback (debatable if worth complexity)

---

## Open Questions & Edge Cases

### Answered Questions

**Q: How do we maintain prompt/buffer separation?**  
A: Prompts injected during composition (display layer), never touch buffer. Translation function maps between spaces.

**Q: Does cursor movement logic change?**  
A: No! Cursor continues operating in buffer space. Only rendering translates to display space.

**Q: What about editing operations?**  
A: All editing happens in buffer space. Display automatically re-renders with updated buffer content.

### Remaining Complexity Questions

#### 1. Multi-Column Cursor Positioning
**Question**: If terminal width causes line wrapping, how does cursor positioning work?

**Example**:
```
Terminal width: 20 chars
Display output:  "for i in 1 2 3 4 5|
                  6 7 8 9 10; do\n> e|
                  cho"
```

**Considerations**:
- Translation function returns linear position
- Display controller must convert to (row, column) coordinates
- Wrapping calculation already exists in display system
- Likely **not a problem** - display controller handles wrapping

**Impact**: Low - display system already handles wrapped output

#### 2. Unicode/Grapheme Clusters
**Question**: Does translation need grapheme-awareness for cursor positioning?

**Example**:
```
Buffer: "echo 👨‍👩‍👧‍👦\ntest"  (family emoji = 1 grapheme, 11 bytes)
```

**Considerations**:
- Translation counts characters, not bytes
- LLE already has grapheme-aware cursor (cursor.grapheme_index)
- May need grapheme-aware translation for proper positioning

**Current Translation**: Counts bytes/characters  
**Needed Translation**: May need to count graphemes for display positioning

**Impact**: Medium - may need grapheme-aware version of translation function

#### 3. ANSI Escape Sequences in PS2
**Question**: If user sets colorful continuation prompt, how do we count length correctly?

**Example**:
```
PS2 = "\033[1;32m> \033[0m"  (green ">" with reset)
Visual length: 2 chars
Actual length: 12 chars (including ANSI codes)
```

**Considerations**:
- Need to strip ANSI when counting prompt length for translation
- Common pattern in terminal applications
- LLE already handles ANSI in prompt rendering

**Solution**: Count only visible characters, strip ANSI codes when measuring

**Impact**: Low - standard terminal programming pattern

#### 4. Reverse Translation (Display → Buffer)
**Question**: Do we ever need to translate display position back to buffer position?

**Scenarios**:
- Mouse click positioning (if LLE supports mouse)
- Terminal reports cursor position
- Copy/paste operations

**Current Assessment**: Likely not needed for keyboard-only interaction

**Impact**: Unknown - depends on future mouse support

#### 5. Performance of Linear Scan
**Question**: Is O(n) translation acceptable for large buffers?

**Considerations**:
- Translation walks entire buffer up to cursor position
- Large multiline commands (100+ lines) could be slow
- Happens on every cursor movement

**Optimization Options**:
- Cache translation results
- Only recalculate when buffer changes
- Incremental updates

**Current Assessment**: Start simple, optimize if profiling shows issues

**Impact**: Low for typical usage, may need optimization for pathological cases

#### 6. Cursor at Newline Position
**Question**: When cursor is exactly at `\n`, is it before or after injected prompt?

**Example**:
```
Buffer: "for|\necho"  (cursor at position 3, before \n)
Display: "for|\n> echo"  (cursor displays at position 3)

Buffer: "for\n|echo"  (cursor at position 4, after \n)
Display: "for\n> |echo"  (cursor displays at position 6)
```

**Consideration**: Translation needs to handle this edge case correctly

**Impact**: Low - clear logic: count prompts for **newlines before cursor**

#### 7. Empty Lines in Multiline Input
**Question**: Should empty lines get continuation prompts?

**Example**:
```
Buffer: "echo foo\n\necho bar"
         (empty line in middle)

Display option A:
echo foo

> echo bar

Display option B:
echo foo
> 
> echo bar
```

**Considerations**:
- Option A: Skip prompt on empty lines (cleaner)
- Option B: Show prompt on all lines (consistent)
- How does GNU Readline handle this?
- How does ZSH handle this?

**Impact**: Low - behavior decision, either is acceptable

---

## Testing Strategy

### Unit Tests
1. **Translation Function Tests**:
   - Single line (no newlines) → no offset
   - One newline → offset by prompt length
   - Multiple newlines → cumulative offset
   - Cursor before first newline → no offset
   - Cursor after last newline → full offset
   - Empty buffer → no offset

2. **Composition Engine Tests**:
   - Single-line command → no injected prompts
   - Two-line command → one injected prompt
   - Multiple-line command → multiple injected prompts
   - Empty lines in multiline → verify behavior

### Integration Tests
1. **Cursor Movement**:
   - LEFT/RIGHT across newline boundaries
   - UP/DOWN navigation in multiline buffer
   - HOME/END within multiline command
   - Ctrl-A / Ctrl-E (beginning/end of line)

2. **Editing Operations**:
   - Insert character at various positions
   - Delete character across newline
   - Backspace at continuation line start
   - Yank (paste) multiline content

3. **Visual Verification**:
   - For loop with continuation prompts
   - If statement with continuation prompts
   - Function definition with continuation prompts
   - Quoted string spanning lines

### Edge Case Tests
1. Very long lines (terminal wrapping)
2. Unicode/emoji in multiline commands
3. Colorful PS2 with ANSI codes
4. Empty lines in multiline input
5. Cursor at exact newline position

---

## Alternative Approaches (For Completeness)

### Option 1: Multi-Call Architecture (GNU Readline Compatible)
**Description**: Change LLE to call `lle_readline()` multiple times like GNU Readline

**Pros**:
- Perfect GNU Readline compatibility
- Context-aware prompts naturally supported
- No cursor translation needed

**Cons**:
- ❌ Breaks single-buffer model (fundamental architecture change)
- ❌ Loses unified editing session (can't UP arrow to previous line)
- ❌ History becomes line-by-line instead of complete commands
- ❌ Major refactoring of LLE core

**Verdict**: Not recommended - breaks LLE's design philosophy

### Option 2: Virtual Text / Overlay System
**Description**: Add display overlay system for "virtual text" not in buffer

**Pros**:
- Generic system for many use cases (line numbers, hints, etc.)
- Clean separation of buffer and display concerns
- Modern editor pattern (VSCode inline hints)

**Cons**:
- Larger architectural change
- More complex than needed for just continuation prompts
- May be over-engineering the problem

**Verdict**: Interesting long-term, overkill for immediate need

### Option 3: Status Line Indicator (Minimal)
**Description**: Show continuation state in status line instead of per-line prompts

**Example**:
```
for i in 1 2 3; do
echo $i
done
────────────────────────────────
[LOOP] Line 2 of 3
```

**Pros**:
- Very simple to implement
- No cursor translation needed
- Still provides feedback

**Cons**:
- Less intuitive than per-line prompts
- Doesn't match user expectations from bash/zsh
- Status line might not be visible

**Verdict**: Fallback option if translation proves too complex

---

## Decision Matrix

| Approach | Complexity | User Value | GNU Readline Compat | Implementation Time |
|----------|-----------|------------|---------------------|-------------------|
| **ZSH-style simple prompt** | Medium | High | Partial | 4-5 hours |
| Context-aware prompts | High | Medium | Full | +1 day |
| Multi-call architecture | Very High | High | Full | Weeks |
| Virtual text system | High | High | Partial | Days |
| Status line indicator | Low | Low | None | 1-2 hours |

**Recommendation**: Start with ZSH-style simple prompt (Phase 1)

---

## Next Steps

### Before Implementation
1. ✅ Document architectural approach (this document)
2. ⏳ Investigate ZSH ZLE source code (confirm single-buffer assumption)
3. ⏳ Test edge cases manually in ZSH (empty lines, cursor positioning)
4. ⏳ Prototype translation function in isolation
5. ⏳ Get user approval on approach

### Implementation Checklist
- [ ] Add `translate_buffer_to_display_position()` to composition engine
- [ ] Modify `compose_multiline_strategy()` to inject prompts
- [ ] Update display controller to use translated cursor position
- [ ] Add configuration for PS2 value
- [ ] Write unit tests for translation function
- [ ] Write integration tests for cursor movement
- [ ] Manual testing with various multiline constructs
- [ ] Update documentation

### Future Enhancements (Post Phase 1)
- [ ] Support custom PS2 from config
- [ ] Handle ANSI escape sequences in PS2
- [ ] Optimize translation for large buffers (if needed)
- [ ] Add context-aware prompts (if user wants GNU Readline parity)
- [ ] Mouse support (requires reverse translation)

---

## Conclusion

The ZSH-style simple continuation prompt approach provides:
- ✅ **Reasonable user experience** (visual feedback for multiline input)
- ✅ **Maintainable architecture** (clean buffer/display separation)
- ✅ **Achievable implementation** (half day of work)
- ✅ **Room for enhancement** (can add context-aware prompts later)

This is the recommended path forward for LLE continuation prompt support.

---

**Document Status**: Planning complete, ready for prototyping  
**Last Updated**: 2025-11-08  
**Authors**: AI Assistant + User collaborative design

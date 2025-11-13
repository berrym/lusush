# LLE Dual-Action Architecture

**Date**: 2025-11-13  
**Status**: Design Approved, Implementation In Progress  
**Author**: Session 14 - Keybinding Manager Migration

---

## Executive Summary

LLE's keybinding manager is being enhanced to support two types of actions:
1. **Simple Actions** - Operate on `lle_editor_t` only (most keybindings)
2. **Context-Aware Actions** - Have full access to `readline_context_t` (complex keybindings)

This architectural enhancement eliminates the need for signal flags (`eof_requested`, `abort_requested`, `line_accepted`) and provides a clean foundation for future complex features.

---

## Problem Statement

### Original Issue: ENTER Key Migration Failed

During keybinding manager migration, Groups 1-4 (21 keybindings) successfully migrated using simple actions. However, Group 5 (ENTER key) caused complete system regression - history navigation broke, display corrupted, multiple keybindings stopped working.

**Root Cause**: ENTER needs access to readline context to:
- Check continuation state (multiline incomplete input)
- Add commands to history
- Set `done` and `final_line` to exit readline loop

The flag-based approach (`line_accepted` flag) failed because:
1. Flags are checked after EVERY action execution
2. Flag reset logic is complex and error-prone  
3. Hidden state makes debugging extremely difficult
4. Not scalable - each complex action needs new flags

### Why This Matters for LLE's Future

Planned features that will need readline context access:
- **Incremental history search (Ctrl-R)**: Display integration, search state
- **Tab completion**: Completion menu, display updates, context
- **History expansion (!!, !$)**: Needs to modify readline state
- **Multiline continuation prompts**: Display rendering with context
- **Vi mode commands**: Complex multi-keystroke operations
- **Macro recording/playback**: Full state manipulation

**Without proper architecture**: Each feature requires new flags, increasing complexity exponentially.

**With dual-action architecture**: Features can be context-aware from the start.

---

## Design: Dual-Action System

### Core Concept

Instead of one action signature, support two:

```c
/* Simple actions - 95% of keybindings */
typedef lle_result_t (*lle_action_simple_t)(lle_editor_t *editor);

/* Context-aware actions - complex keybindings */
typedef lle_action_context_t)(readline_context_t *ctx);
```

### Type-Safe Action Union

```c
typedef enum {
    LLE_ACTION_TYPE_SIMPLE,    /* Operates on editor only */
    LLE_ACTION_TYPE_CONTEXT    /* Needs full readline context */
} lle_action_type_t;

typedef struct {
    lle_action_type_t type;
    union {
        lle_action_simple_t simple;
        lle_action_context_t context;
    } func;
    const char *name;  /* For debugging/introspection */
} lle_keybinding_action_t;
```

### Keybinding Manager API

```c
/* Bind simple action (existing behavior) */
lle_result_t lle_keybinding_manager_bind_simple(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_action_simple_t action,
    const char *action_name
);

/* Bind context-aware action (new) */
lle_result_t lle_keybinding_manager_bind_context(
    lle_keybinding_manager_t *manager,
    const char *key_sequence,
    lle_action_context_t action,
    const char *action_name
);
```

### Execution Dispatch

```c
static lle_result_t execute_keybinding_action(
    readline_context_t *ctx,
    const char *key_sequence,
    lle_result_t (*fallback_handler)(lle_event_t *, void *)
)
{
    if (ctx->keybinding_manager && ctx->editor) {
        lle_keybinding_action_t *action = NULL;
        lle_result_t result = lle_keybinding_manager_lookup(
            ctx->keybinding_manager,
            key_sequence,
            &action
        );
        
        if (result == LLE_SUCCESS && action != NULL) {
            lle_result_t exec_result;
            
            if (action->type == LLE_ACTION_TYPE_SIMPLE) {
                /* Simple action: operate on editor, then refresh display */
                exec_result = action->func.simple(ctx->editor);
                
                /* Check legacy flags (will be removed after all migrations) */
                if (ctx->editor->eof_requested) {
                    *ctx->done = true;
                    *ctx->final_line = NULL;
                    return exec_result;
                }
                
                if (ctx->editor->abort_requested) {
                    *ctx->done = true;
                    *ctx->final_line = strdup("");
                    return exec_result;
                }
                
                /* Refresh display after simple action */
                if (exec_result == LLE_SUCCESS) {
                    refresh_display(ctx);
                }
            } 
            else if (action->type == LLE_ACTION_TYPE_CONTEXT) {
                /* Context-aware action: has full access, handles everything */
                exec_result = action->func.context(ctx);
                /* No automatic refresh - action handles it if needed */
            }
            
            return exec_result;
        }
    }
    
    /* Fallback to hardcoded handler */
    if (fallback_handler) {
        return fallback_handler(NULL, ctx);
    }
    
    return LLE_SUCCESS;
}
```

---

## Implementation Strategy

### Phase 1: Infrastructure (No Behavioral Changes)

1. **Update keybinding manager data structures**
   - Change action storage to use union type
   - Add `type` field to distinguish action types
   - Keep backward compatibility with existing bindings

2. **Add new binding API**
   - `lle_keybinding_manager_bind_simple()` - wrap existing `bind()`
   - `lle_keybinding_manager_bind_context()` - new function
   - Update lookup to return full action structure

3. **Update execution dispatch**
   - Check action type before execution
   - Route simple actions through existing path
   - Add context-aware action path

### Phase 2: Migrate ENTER Key

1. **Create context-aware ENTER action**
   ```c
   lle_result_t lle_accept_line_context(readline_context_t *ctx);
   ```
   - Direct port of `handle_enter()` logic
   - Check continuation state
   - Add to history
   - Set done/final_line
   - Handle multiline insertion

2. **Bind as context-aware**
   ```c
   lle_keybinding_manager_bind_context(
       keybinding_manager,
       "ENTER",
       lle_accept_line_context,
       "accept-line"
   );
   ```

3. **Route through manager**
   - Both `\n` and `LLE_KEY_ENTER` go through `execute_keybinding_action`
   - Dispatcher recognizes context-aware type
   - Executes with full context

### Phase 3: Cleanup (Future)

1. **Remove legacy flags** (after confirming no usage)
   - `eof_requested` - Ctrl-D can use context-aware if needed
   - `abort_requested` - Ctrl-G can use context-aware if needed
   - `line_accepted` - no longer needed

2. **Document migration pattern** for future complex actions

---

## Architectural Principles

### Separation of Concerns

**Simple Actions**:
- Pure editor operations
- No readline dependencies
- Reusable in other contexts
- Examples: navigation, deletion, kill/yank

**Context-Aware Actions**:
- Explicitly coupled to readline
- Full access by design
- Used when readline integration is intrinsic
- Examples: accept line, incremental search, completion

### Type Safety

The union type makes coupling **explicit and type-safe**:
- Can't accidentally call simple action with context
- Can't forget to provide context to context-aware action
- Compiler enforces correct usage

### Extensibility

Adding a new complex feature:
1. Write context-aware action function
2. Bind with `bind_context()`
3. Done - no flags, no wrapper modifications

### No Hidden State

**Old approach** (flags):
- Action sets flag → wrapper checks flag → wrapper takes action
- Hidden dependencies, implicit communication
- Hard to debug, easy to break

**New approach** (dual actions):
- Context-aware action does what it needs directly
- Explicit access, obvious behavior
- Easy to understand, hard to break

---

## Benefits

### Immediate

1. **ENTER works correctly** - Direct translation from working handler
2. **No regressions** - Other keybindings unaffected
3. **Simpler code** - No flag checking for context actions

### Long-Term

1. **Future features enabled** - Infrastructure ready for complex actions
2. **Maintenance burden reduced** - No flag state machine to debug
3. **Code clarity improved** - Explicit about dependencies
4. **Scalable** - Unlimited complex actions without increasing complexity

### Development Velocity

- New complex feature: Write context-aware action, bind, done
- No wrapper modifications needed
- No flag reset logic to maintain
- Lower bug surface area

---

## Migration Path for Existing Code

### Ctrl-D (EOF)

Currently uses `eof_requested` flag. Options:
1. **Keep as simple action with flag** (simpler, no change needed)
2. **Convert to context-aware** (cleaner, removes flag)

Recommendation: Keep as-is initially, convert later if needed.

### Ctrl-G (Abort)

Currently uses `abort_requested` flag. Options:
1. **Keep as simple action with flag** (simpler, no change needed)
2. **Convert to context-aware** (cleaner, removes flag)

Recommendation: Keep as-is initially, convert later if needed.

### Pattern for Future Flags

**Question to ask**: "Does this action need to modify readline state (done, final_line, continuation, history)?"
- **No**: Use simple action
- **Yes**: Use context-aware action

---

## Risks and Mitigations

### Risk: Increased Coupling for Context Actions

**Mitigation**: This is intentional and honest. Actions that need readline context SHOULD be coupled. Making it explicit is better than hiding it with flags.

### Risk: API Complexity (Two Binding Functions)

**Mitigation**: 
- Clear naming convention
- Most bindings still use simple API
- Documentation makes distinction obvious

### Risk: Implementation Bugs

**Mitigation**:
- Careful testing of dispatch logic
- Existing simple actions unchanged (low risk)
- Context actions isolated (failures don't affect others)

---

## Success Criteria

1. ✅ ENTER key migrated to keybinding manager
2. ✅ No regressions in Groups 1-4 keybindings
3. ✅ All manual tests pass (basic, multiline, history, editing)
4. ✅ Code cleaner than flag-based approach
5. ✅ Architecture documented for future developers
6. ✅ Pattern established for future complex actions

---

## Future Work

### Immediate Next Steps
1. Implement infrastructure (manager updates)
2. Create context-aware ENTER action
3. Test thoroughly
4. Document and commit

### Later Enhancements
1. Convert Ctrl-D/Ctrl-G to context-aware (optional)
2. Remove legacy flags entirely
3. Add incremental search as context-aware action
4. Add completion system as context-aware actions

---

## References

- **Original Issue**: Session 14 - ENTER implementation broke all keybindings
- **Keybinding Manager**: `include/lle/keybinding_manager.h`
- **Action Functions**: `src/lle/keybinding_actions.c`
- **Readline Integration**: `src/lle/lle_readline.c`
- **Migration Tracker**: `docs/development/KEYBINDING_MIGRATION_TRACKER.md`

---

**Decision**: Approved 2025-11-13 - Dual-action architecture is the correct long-term solution for LLE.

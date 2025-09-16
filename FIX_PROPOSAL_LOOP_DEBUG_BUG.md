# FIX PROPOSAL: Loop Debug Integration Bug

**Bug**: `DEBUG: Unhandled keyword type 46 (DONE)` - Loop debugging breaks parser state  
**Priority**: 🚨 CRITICAL - IMMEDIATE FIX REQUIRED  
**Root Cause**: Debug breakpoints corrupt execution context during loop processing  

---

## 🔍 PROBLEM ANALYSIS

### Current Broken Flow
1. ✅ Loop starts: `for i in 1 2 3; do`
2. ✅ First iteration begins with `$i = "1"`
3. 🚨 **BREAKPOINT HITS** inside loop body
4. 🚨 Debug system calls `debug_enter_interactive_mode()`
5. 🚨 **EXECUTION CONTEXT CORRUPTED** when resuming
6. ❌ Loop variable `$i` becomes empty
7. ❌ Parser encounters DONE token in wrong context
8. ❌ `parse_simple_command()` gets `TOK_DONE` (type 46) and fails

### Root Cause Identification
The issue is in the **execution handoff between normal execution and debug mode**:

1. **Context Loss**: Loop iteration state (variable values, position) not preserved
2. **Parser State**: Tokenizer/parser position becomes misaligned 
3. **Execution Resume**: Debug system doesn't cleanly resume loop execution
4. **Variable Scoping**: Loop variables lose their iteration values

---

## 🎯 PROPOSED SOLUTION

### Strategy: Context-Aware Debug Pausing

**Core Principle**: Preserve complete execution context during debug interruption, then restore it exactly when resuming.

### Phase 1: Immediate Fix (2-4 hours)

#### 1.1 Add Execution Context Preservation
**File**: `include/debug.h`
```c
// Add to debug_context_t structure
typedef struct debug_context {
    // ... existing fields ...
    
    // NEW: Execution context preservation
    struct {
        bool in_loop;
        char *loop_variable;        // Current loop variable name
        char *loop_variable_value;  // Current iteration value
        int loop_iteration;         // Current iteration number
        node_t *loop_node;         // AST node of current loop
        parser_t *saved_parser_state; // Parser state snapshot
    } execution_context;
} debug_context_t;
```

#### 1.2 Modify Breakpoint Check Integration
**File**: `src/executor.c` (around line 347)
```c
// BEFORE (current broken approach):
if (executor->in_script_execution && executor->current_script_file) {
    DEBUG_BREAKPOINT_CHECK(executor->current_script_file, executor->current_script_line);
}

// AFTER (context-aware approach):
if (executor->in_script_execution && executor->current_script_file) {
    // Save execution context BEFORE debug check
    debug_save_execution_context(g_debug_context, executor, node);
    
    if (debug_check_breakpoint(g_debug_context, executor->current_script_file, executor->current_script_line)) {
        // Breakpoint hit - context already saved
        // Debug system will handle pause/resume with context preservation
    }
    
    // Restore execution context AFTER debug (if modified)
    debug_restore_execution_context(g_debug_context, executor, node);
}
```

#### 1.3 Implement Context Save/Restore Functions
**File**: `src/debug/debug_breakpoints.c`
```c
// Save execution context before debug interruption
void debug_save_execution_context(debug_context_t *ctx, executor_t *executor, node_t *node) {
    if (!ctx || !executor || !node) return;
    
    // Detect if we're in a loop
    ctx->execution_context.in_loop = (node->type == NODE_FOR || 
                                     node->type == NODE_WHILE || 
                                     node->type == NODE_UNTIL);
    
    if (ctx->execution_context.in_loop) {
        // Save loop-specific context
        if (node->type == NODE_FOR) {
            // Extract loop variable and current value from FOR node
            ctx->execution_context.loop_variable = strdup(node->data.for_loop.variable);
            
            // Get current value from symbol table
            char *current_value = symtable_get(ctx->execution_context.loop_variable);
            ctx->execution_context.loop_variable_value = current_value ? strdup(current_value) : NULL;
        }
        
        // Save reference to loop AST node
        ctx->execution_context.loop_node = node;
    }
}

// Restore execution context after debug session
void debug_restore_execution_context(debug_context_t *ctx, executor_t *executor, node_t *node) {
    if (!ctx || !ctx->execution_context.in_loop) return;
    
    // Restore loop variable value if it was lost
    if (ctx->execution_context.loop_variable && ctx->execution_context.loop_variable_value) {
        // Check if variable was corrupted (became empty)
        char *current_value = symtable_get(ctx->execution_context.loop_variable);
        if (!current_value || strlen(current_value) == 0) {
            // Restore the saved value
            symtable_set(ctx->execution_context.loop_variable, 
                        ctx->execution_context.loop_variable_value);
        }
    }
}

// Clean up context when debug session ends
void debug_cleanup_execution_context(debug_context_t *ctx) {
    if (!ctx) return;
    
    free(ctx->execution_context.loop_variable);
    free(ctx->execution_context.loop_variable_value);
    ctx->execution_context.loop_variable = NULL;
    ctx->execution_context.loop_variable_value = NULL;
    ctx->execution_context.in_loop = false;
    ctx->execution_context.loop_node = NULL;
}
```

#### 1.4 Fix Interactive Debug Mode
**File**: `src/debug/debug_breakpoints.c` - Modify `debug_enter_interactive_mode()`
```c
void debug_enter_interactive_mode(debug_context_t *ctx) {
    if (!ctx) return;
    
    debug_printf(ctx, "\nEntering interactive debug mode. Type 'help' for commands.\n");
    
    // Show loop context if in loop
    if (ctx->execution_context.in_loop) {
        debug_printf(ctx, "Currently in loop: variable '%s' = '%s'\n", 
                    ctx->execution_context.loop_variable ?: "unknown",
                    ctx->execution_context.loop_variable_value ?: "unknown");
    }
    
    // ... rest of existing interactive code ...
    
    while (ctx->step_mode) {
        debug_printf(ctx, "(lusush-debug) ");
        fflush(ctx->debug_output);

        char input[256];
        if (fgets(input, sizeof(input), debug_input)) {
            debug_handle_user_input(ctx, input);
            
            // CRITICAL: Don't let step_mode affect loop execution
            // If user says continue, exit debug but preserve loop context
            if (!ctx->step_mode) {
                break; // Exit debug loop cleanly
            }
        } else {
            // EOF - exit debug but preserve execution context
            ctx->step_mode = false;
            break;
        }
    }
    
    // IMPORTANT: Don't cleanup execution context here
    // Let it be cleaned up after loop completes naturally
}
```

### Phase 2: Enhanced Fix (4-6 hours)

#### 2.1 Parser State Preservation
**Objective**: Ensure parser doesn't encounter DONE tokens in wrong context

**File**: `src/parser.c` - Add debug-aware parsing
```c
// Add context check before parsing commands
static node_t *parse_simple_command(parser_t *parser) {
    token_t *current = tokenizer_current(parser->tokenizer);
    if (!current) {
        return NULL;
    }
    
    // NEW: Check if we're in debug mode and got a loop token in wrong context
    if (g_debug_context && g_debug_context->enabled) {
        if (current->type == TOK_DONE || current->type == TOK_FI || 
            current->type == TOK_ESAC) {
            // These should never appear in simple command context
            // This indicates parser state corruption from debug interruption
            debug_printf(g_debug_context, 
                "Parser state corruption detected: %s token in simple command context\n",
                token_type_name(current->type));
            
            // Return NULL to stop parsing rather than show confusing error
            return NULL;
        }
    }
    
    // ... rest of existing function unchanged ...
}
```

#### 2.2 Enhanced Loop Variable Tracking
**File**: `src/executor.c` - Enhance FOR loop execution
```c
// In execute_for() function - add debug context tracking
int execute_for(executor_t *executor, node_t *node) {
    // ... existing setup code ...
    
    // NEW: Notify debug system we're starting loop execution
    if (g_debug_context && g_debug_context->enabled) {
        debug_begin_loop_execution(g_debug_context, node, var_name);
    }
    
    for (int i = 0; words[i]; i++) {
        // Set loop variable
        symtable_set(var_name, words[i]);
        
        // NEW: Update debug context with current iteration
        if (g_debug_context && g_debug_context->enabled) {
            debug_update_loop_iteration(g_debug_context, var_name, words[i], i);
        }
        
        // Execute loop body with potential debug breakpoints
        int result = execute_node_list(executor, node->data.for_loop.body);
        
        // ... rest of loop logic ...
    }
    
    // NEW: Notify debug system loop completed
    if (g_debug_context && g_debug_context->enabled) {
        debug_end_loop_execution(g_debug_context);
    }
    
    // ... existing cleanup code ...
}
```

---

## 🧪 TESTING STRATEGY

### Critical Test Cases
```bash
# Test 1: Basic for loop with breakpoint
cat > test1.sh << 'EOF'
for i in 1 2 3; do
    echo "Iteration: $i"  # Breakpoint on this line
done
EOF

# Test 2: While loop with breakpoint
cat > test2.sh << 'EOF' 
count=1
while [ $count -le 3 ]; do
    echo "Count: $count"  # Breakpoint on this line
    count=$((count + 1))
done
EOF

# Test 3: Nested loops
cat > test3.sh << 'EOF'
for outer in a b; do
    for inner in 1 2; do
        echo "$outer$inner"  # Breakpoint on this line
    done
done
EOF
```

### Verification Commands
```bash
# Each test should work without errors
echo 'debug on; debug break add test1.sh 2; source test1.sh' | lusush
echo 'debug on; debug break add test2.sh 3; source test2.sh' | lusush  
echo 'debug on; debug break add test3.sh 3; source test3.sh' | lusush

# Expected: No "Unhandled keyword type 46" errors
# Expected: Loop variables maintain correct values
# Expected: Clean debug session with proper variable display
```

---

## 📋 IMPLEMENTATION CHECKLIST

### Phase 1: Immediate Fix (Target: 4 hours)
- [ ] Add execution context structure to `debug_context_t`
- [ ] Implement `debug_save_execution_context()`
- [ ] Implement `debug_restore_execution_context()`  
- [ ] Modify `DEBUG_BREAKPOINT_CHECK()` integration
- [ ] Update `debug_enter_interactive_mode()` for clean resume
- [ ] Test basic for loop with breakpoint
- [ ] Test while loop with breakpoint
- [ ] Verify no "Unhandled keyword type 46" errors

### Phase 2: Enhanced Fix (Target: 6 hours total)
- [ ] Add parser state corruption detection
- [ ] Implement `debug_begin_loop_execution()`
- [ ] Implement `debug_update_loop_iteration()`
- [ ] Implement `debug_end_loop_execution()`
- [ ] Test nested loops with breakpoints
- [ ] Test complex loop bodies with breakpoints
- [ ] Verify all POSIX loop constructs work with debugging
- [ ] Performance testing (ensure minimal overhead)

### Phase 3: Verification (Target: 8 hours total)
- [ ] Comprehensive regression testing
- [ ] Cross-platform testing (Linux, macOS, BSD)
- [ ] Memory leak checking with valgrind
- [ ] Update documentation to remove warnings
- [ ] Create positive examples showing working loop debugging

---

## 🎯 SUCCESS CRITERIA

### Before Fix is Complete
✅ **No parser errors**: No "Unhandled keyword type 46 (DONE)" messages  
✅ **Variable preservation**: Loop variables maintain correct values during debug  
✅ **Clean execution**: Loops complete successfully after debug pause/resume  
✅ **All loop types**: for, while, until all work with breakpoints  
✅ **Nested loops**: Inner and outer loop variables both preserved  
✅ **Interactive debugging**: User can inspect variables and step through loops  

### Acceptance Test
```bash
# This EXACT command sequence must work flawlessly:
echo 'debug on; debug break add script.sh 5; source script.sh' | lusush

# Where script.sh contains:
# #!/usr/bin/env lusush
# for i in 1 2 3; do
#     echo "Value: $i"
# done

# Expected output:
# >>> BREAKPOINT HIT <<<
# Value: 1
# Value: 2  
# Value: 3

# NOT:
# Value:     (empty variable)
# DEBUG: Unhandled keyword type 46 (DONE)
```

---

## 📊 RISK ASSESSMENT

### Low Risk Changes
- Adding context preservation structures (new code, minimal impact)
- Enhancing debug output with loop information (cosmetic)

### Medium Risk Changes  
- Modifying `DEBUG_BREAKPOINT_CHECK()` integration (could affect all debugging)
- Parser state corruption detection (could change parser behavior)

### High Risk Changes
- Modifying core loop execution (`execute_for`, `execute_while`) 
- Changing debug session resume logic (could break existing functionality)

### Mitigation Strategy
1. **Incremental implementation**: Phase 1 first, test thoroughly
2. **Comprehensive testing**: All existing functionality must still work
3. **Rollback plan**: Keep original debug integration as fallback
4. **Performance monitoring**: Ensure no significant overhead added

---

## 🚀 ESTIMATED TIMELINE

**Phase 1 (Critical Fix)**: 4 hours
- Context preservation framework
- Basic loop variable fix
- Initial testing

**Phase 2 (Enhanced Fix)**: +2 hours (6 hours total)  
- Parser improvements
- Enhanced loop tracking
- Comprehensive testing

**Phase 3 (Verification)**: +2 hours (8 hours total)
- Full regression testing
- Documentation updates
- Performance validation

**Total Estimated Time**: 8 hours for complete fix with full verification

**Priority**: Can start with Phase 1 for immediate critical fix, then enhance

---

This fix addresses the root cause while preserving all existing functionality. The approach is surgical and focused on the specific execution context corruption issue.
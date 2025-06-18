# Interactive Input Enhancement Design

## Current Problem

**Two Different Input Handling Paths:**
1. **Interactive (linenoise)**: Simple `\` continuation only
2. **Non-interactive**: Enhanced `get_input_complete()` with `is_command_complete()`

**Issues:**
- Interactive mode can't handle multiline constructs properly
- Parsing behavior differs between interactive and non-interactive modes
- Users get inconsistent experience

## Proposed Solution: Unified Input Architecture

### Enhanced Interactive Input with linenoise + is_command_complete()

```c
// Enhanced interactive input that leverages is_command_complete()
char *ln_gets_complete(void) {
    static char *accumulated_input = NULL;
    static size_t accumulated_size = 0;
    char *line = NULL;
    char *prompt = NULL;
    bool first_line = true;

    while (true) {
        // Use PS1 for first line, PS2 for continuation
        if (first_line) {
            build_prompt();
            prompt = get_shell_varp("PS1", "% ");
        } else {
            prompt = get_shell_varp("PS2", "> ");
        }

        line = linenoise(prompt);
        if (!line) {
            // EOF or error - return accumulated input if any
            if (accumulated_input && *accumulated_input) {
                char *result = accumulated_input;
                accumulated_input = NULL;
                accumulated_size = 0;
                return result;
            }
            return NULL;
        }

        // Handle accumulation
        if (accumulated_input) {
            size_t line_len = strlen(line);
            size_t needed = accumulated_size + line_len + 2;
            char *tmp = realloc(accumulated_input, needed);
            if (!tmp) {
                error_syscall("error: realloc in ln_gets_complete");
                free(accumulated_input);
                accumulated_input = NULL;
                return line;
            }
            accumulated_input = tmp;
            strcat(accumulated_input, "\n");
            strcat(accumulated_input, line);
            accumulated_size = needed - 1;
        } else {
            // First line
            accumulated_size = strlen(line) + 1;
            accumulated_input = malloc(accumulated_size);
            if (!accumulated_input) {
                error_syscall("error: malloc in ln_gets_complete");
                return line;
            }
            strcpy(accumulated_input, line);
        }

        // Check if input is syntactically complete
        if (is_command_complete(accumulated_input)) {
            char *result = accumulated_input;
            accumulated_input = NULL;
            accumulated_size = 0;
            
            // Add to history
            if (*result) {
                history_add(result);
                history_save();
            }
            
            return result;
        }

        // Need more input - continue loop with PS2 prompt
        first_line = false;
        free(line); // Free the individual line since it's now in accumulated_input
    }
}
```

### Benefits of This Approach

1. **Functional Equivalence**: Interactive and non-interactive modes use identical completion logic
2. **Better UX**: Interactive users get proper multiline support for:
   ```bash
   lusush% if true; then
   >     echo "multiline works!"
   >     for i in 1 2 3; do
   >         echo $i
   >     done
   > fi
   ```
3. **Consistent Parsing**: Same tokenization and completion detection everywhere
4. **Enhanced Token System Integration**: Leverages the new pushback system seamlessly

## Implementation Plan

### Phase 1: Create Enhanced Interactive Input
- Implement `ln_gets_complete()` function
- Integrate `is_command_complete()` with linenoise
- Handle proper PS1/PS2 prompting

### Phase 2: Unify Input Interface  
- Create single `get_unified_input()` function
- Route interactive/non-interactive through same completion logic
- Maintain backward compatibility

### Phase 3: Enhanced Features
- Syntax highlighting in multiline mode
- Better error recovery in interactive mode
- Context-aware completion during multiline input
```

#include "../../include/debug.h"
#include "../../include/errors.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Add breakpoint
int debug_add_breakpoint(debug_context_t *ctx, const char *file, int line,
                         const char *condition) {
    if (!ctx || !file || line <= 0) {
        return -1;
    }

    breakpoint_t *bp = malloc(sizeof(breakpoint_t));
    if (!bp) {
        debug_printf(ctx, "ERROR: Failed to allocate breakpoint\n");
        return -1;
    }

    // Initialize breakpoint
    bp->id = ctx->next_breakpoint_id++;
    bp->file = strdup(file);
    bp->line = line;
    bp->condition = condition ? strdup(condition) : NULL;
    bp->hit_count = 0;
    bp->enabled = true;
    bp->next = ctx->breakpoints;

    // Add to list
    ctx->breakpoints = bp;

    debug_printf(ctx, "Breakpoint %d set at %s:%d\n", bp->id, file, line);
    if (condition) {
        debug_printf(ctx, "  Condition: %s\n", condition);
    }

    return bp->id;
}

// Remove breakpoint
bool debug_remove_breakpoint(debug_context_t *ctx, int id) {
    if (!ctx || id <= 0) {
        return false;
    }

    breakpoint_t **current = &ctx->breakpoints;
    while (*current) {
        if ((*current)->id == id) {
            breakpoint_t *bp = *current;
            *current = bp->next;

            debug_printf(ctx, "Breakpoint %d removed from %s:%d\n", bp->id,
                         bp->file, bp->line);

            free(bp->file);
            free(bp->condition);
            free(bp);
            return true;
        }
        current = &(*current)->next;
    }

    debug_printf(ctx, "Breakpoint %d not found\n", id);
    return false;
}

// Enable/disable breakpoint
bool debug_enable_breakpoint(debug_context_t *ctx, int id, bool enable) {
    if (!ctx || id <= 0) {
        return false;
    }

    breakpoint_t *bp = ctx->breakpoints;
    while (bp) {
        if (bp->id == id) {
            bp->enabled = enable;
            debug_printf(ctx, "Breakpoint %d %s\n", id,
                         enable ? "enabled" : "disabled");
            return true;
        }
        bp = bp->next;
    }

    debug_printf(ctx, "Breakpoint %d not found\n", id);
    return false;
}

// Check if we hit a breakpoint
bool debug_check_breakpoint(debug_context_t *ctx, const char *file, int line) {
    if (!ctx || !ctx->enabled || !file || line <= 0) {
        return false;
    }

    breakpoint_t *bp = ctx->breakpoints;
    while (bp) {
        if (bp->enabled && bp->line == line && strcmp(bp->file, file) == 0) {
            bp->hit_count++;

            debug_printf(ctx, "\n>>> BREAKPOINT HIT <<<\n");
            debug_printf(ctx, "Breakpoint %d at %s:%d (hit count: %d)\n",
                         bp->id, file, line, bp->hit_count);

            // Evaluate condition if present
            if (bp->condition) {
                bool condition_met =
                    debug_evaluate_condition(ctx, bp->condition);
                debug_printf(ctx, "  Condition: %s -> %s\n", bp->condition,
                             condition_met ? "true" : "false");
                if (!condition_met) {
                    return false; // Continue execution if condition not met
                }
            }

            // Show current context
            debug_show_context(ctx, file, line);

            // Enter interactive debugging mode
            debug_enter_interactive_mode(ctx);

            return true;
        }
        bp = bp->next;
    }

    // Check if we're in step mode
    if (ctx->step_mode) {
        debug_printf(ctx, "\n>>> STEP <<<\n");
        debug_printf(ctx, "At %s:%d\n", file, line);
        debug_show_context(ctx, file, line);
        debug_enter_interactive_mode(ctx);
        return true;
    }

    return false;
}

// List all breakpoints
void debug_list_breakpoints(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_header(ctx, "Breakpoints");

    if (!ctx->breakpoints) {
        debug_printf(ctx, "  No breakpoints set\n");
        return;
    }

    breakpoint_t *bp = ctx->breakpoints;
    while (bp) {
        debug_printf(ctx, "  %d: %s:%d %s (hits: %d)\n", bp->id, bp->file,
                     bp->line, bp->enabled ? "enabled" : "disabled",
                     bp->hit_count);

        if (bp->condition) {
            debug_printf(ctx, "      Condition: %s\n", bp->condition);
        }

        bp = bp->next;
    }
}

// Clear all breakpoints
void debug_clear_breakpoints(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    breakpoint_t *bp = ctx->breakpoints;
    while (bp) {
        breakpoint_t *next = bp->next;
        free(bp->file);
        free(bp->condition);
        free(bp);
        bp = next;
    }

    ctx->breakpoints = NULL;
    ctx->next_breakpoint_id = 1;

    debug_printf(ctx, "All breakpoints cleared\n");
}

// Step execution functions
void debug_step_into(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    ctx->mode = DEBUG_MODE_STEP;
    ctx->step_mode = true;

    debug_printf(ctx, "Stepping into...\n");
}

void debug_step_over(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    ctx->mode = DEBUG_MODE_STEP_OVER;
    ctx->step_mode = true;

    debug_printf(ctx, "Stepping over...\n");
}

void debug_step_out(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    // Step out means continue until we exit the current function
    // For now, just continue
    ctx->mode = DEBUG_MODE_CONTINUE;
    ctx->step_mode = false;

    debug_printf(ctx, "Stepping out...\n");
}

void debug_continue(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    ctx->mode = DEBUG_MODE_CONTINUE;
    ctx->step_mode = false;

    debug_printf(ctx, "Continuing execution...\n");
}

// Handle user input during debugging
void debug_handle_user_input(debug_context_t *ctx, const char *input) {
    if (!ctx || !input) {
        return;
    }

    // Remove newline and trim whitespace
    char *cmd = strdup(input);
    char *newline = strchr(cmd, '\n');
    if (newline) {
        *newline = '\0';
    }

    // Skip leading whitespace
    char *trimmed = cmd;
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }

    // Handle empty input (repeat last command)
    if (strlen(trimmed) == 0) {
        free(cmd);
        return;
    }

    // Parse and handle commands
    if (strcmp(trimmed, "c") == 0 || strcmp(trimmed, "continue") == 0) {
        debug_continue(ctx);
    } else if (strcmp(trimmed, "s") == 0 || strcmp(trimmed, "step") == 0) {
        debug_step_into(ctx);
    } else if (strcmp(trimmed, "n") == 0 || strcmp(trimmed, "next") == 0) {
        debug_step_over(ctx);
    } else if (strcmp(trimmed, "f") == 0 || strcmp(trimmed, "finish") == 0) {
        debug_step_out(ctx);
    } else if (strcmp(trimmed, "bt") == 0 ||
               strcmp(trimmed, "backtrace") == 0) {
        debug_show_stack(ctx);
    } else if (strcmp(trimmed, "up") == 0) {
        debug_stack_up(ctx);
    } else if (strcmp(trimmed, "down") == 0) {
        debug_stack_down(ctx);
    } else if (strcmp(trimmed, "l") == 0 || strcmp(trimmed, "list") == 0) {
        debug_list_breakpoints(ctx);
    } else if (strcmp(trimmed, "where") == 0) {
        debug_show_current_location(ctx);
    } else if (strncmp(trimmed, "p ", 2) == 0 ||
               strncmp(trimmed, "print ", 6) == 0) {
        // Variable inspection
        char *var = trimmed + (trimmed[1] == ' ' ? 2 : 6);
        debug_inspect_variable(ctx, var);
    } else if (strncmp(trimmed, "set ", 4) == 0) {
        // Variable assignment
        char *assignment = trimmed + 4;
        debug_set_variable(ctx, assignment);
    } else if (strncmp(trimmed, "watch ", 6) == 0) {
        // Add variable to watch list
        char *var = trimmed + 6;
        debug_watch_variable(ctx, var);
    } else if (strncmp(trimmed, "eval ", 5) == 0) {
        // Evaluate expression
        char *expr = trimmed + 5;
        debug_evaluate_expression(ctx, expr);
    } else if (strcmp(trimmed, "vars") == 0) {
        debug_inspect_all_variables(ctx);
    } else if (strcmp(trimmed, "help") == 0 || strcmp(trimmed, "h") == 0) {
        debug_print_help(ctx);
    } else if (strcmp(trimmed, "q") == 0 || strcmp(trimmed, "quit") == 0) {
        debug_printf(ctx, "Continuing execution...\n");
        ctx->step_mode = false;
        ctx->mode = DEBUG_MODE_CONTINUE;
    } else {
        debug_printf(ctx, "Unknown command: '%s' (type 'help' for commands)\n",
                     trimmed);
    }

    free(cmd);
}

// Navigate stack frames
void debug_stack_up(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    debug_printf(ctx, "Stack navigation not yet implemented\n");
    // TODO: Implement stack frame navigation
}

void debug_stack_down(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    debug_printf(ctx, "Stack navigation not yet implemented\n");
    // TODO: Implement stack frame navigation
}

// Show current location
void debug_show_current_location(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->current_frame) {
        debug_printf(ctx, "Current location: %s",
                     ctx->current_frame->function_name);
        if (ctx->current_frame->file_path) {
            debug_printf(ctx, " at %s:%d\n", ctx->current_frame->file_path,
                         ctx->current_frame->line_number);
        } else {
            debug_printf(ctx, "\n");
        }
        debug_show_stack(ctx);
    } else {
        debug_printf(ctx, "No current execution context\n");
    }
}

// Set variable value
void debug_set_variable(debug_context_t *ctx, const char *assignment) {
    if (!ctx || !assignment) {
        return;
    }

    debug_printf(ctx, "Variable assignment: %s\n", assignment);
    debug_printf(ctx, "(Variable setting not yet implemented)\n");
    // TODO: Implement variable assignment
}

// Evaluate expression
void debug_evaluate_expression(debug_context_t *ctx, const char *expression) {
    if (!ctx || !expression) {
        return;
    }

    debug_printf(ctx, "Evaluating: %s\n", expression);
    debug_printf(ctx, "(Expression evaluation not yet implemented)\n");
    // TODO: Implement expression evaluator
}

// Show context around current line
void debug_show_context(debug_context_t *ctx, const char *file, int line) {
    if (!ctx || !file) {
        return;
    }

    debug_printf(ctx, "\nContext at %s:%d:\n", file, line);

    // Try to read and show file context
    FILE *f = fopen(file, "r");
    if (f) {
        char buffer[256];
        int current_line = 1;
        int start_line = (line > 3) ? line - 2 : 1;
        int end_line = line + 2;

        while (fgets(buffer, sizeof(buffer), f) && current_line <= end_line) {
            if (current_line >= start_line) {
                char marker = (current_line == line) ? '>' : ' ';
                debug_printf(ctx, "  %c %3d: %s", marker, current_line, buffer);
            }
            current_line++;
        }
        fclose(f);
    } else {
        debug_printf(ctx, "  (source file not available)\n");
    }

    // Show current stack frame info
    if (ctx->current_frame) {
        debug_printf(ctx, "\nCurrent function: %s\n",
                     ctx->current_frame->function_name);
    }
}

// Enter interactive debugging mode
void debug_enter_interactive_mode(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    debug_printf(
        ctx, "\nEntering interactive debug mode. Type 'help' for commands.\n");

    while (ctx->step_mode) {
        debug_printf(ctx, "(lusush-debug) ");
        fflush(ctx->debug_output);

        char input[256];
        if (fgets(input, sizeof(input), stdin)) {
            debug_handle_user_input(ctx, input);
        } else {
            // EOF or error - exit debug mode
            debug_printf(ctx, "\nExiting debug mode\n");
            ctx->step_mode = false;
            ctx->enabled = false;
            break;
        }
    }
}

// Evaluate breakpoint condition
bool debug_evaluate_condition(debug_context_t *ctx, const char *condition) {
    if (!ctx || !condition) {
        return true; // No condition means always true
    }

    // Simple condition evaluation for now
    // TODO: Implement full expression evaluator

    // Handle simple variable comparisons
    if (strstr(condition, "==") || strstr(condition, "!=") ||
        strstr(condition, "<") || strstr(condition, ">")) {
        // For now, just return true for any comparison
        // In a full implementation, this would parse and evaluate the
        // expression
        return true;
    }

    // Handle variable existence checks
    if (condition[0] == '$') {
        // Check if variable exists
        // TODO: Implement variable lookup
        return true;
    }

    return true; // Default to true
}

// Print debug help
void debug_print_help(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_header(ctx, "Interactive Debug Commands");
    debug_printf(
        ctx, "  c, continue    - Continue execution until next breakpoint\n");
    debug_printf(
        ctx, "  s, step        - Step into next statement (enter functions)\n");
    debug_printf(
        ctx, "  n, next        - Step over next statement (skip functions)\n");
    debug_printf(ctx, "  f, finish      - Step out of current function\n");
    debug_printf(ctx, "  bt, backtrace  - Show call stack\n");
    debug_printf(ctx, "  up             - Move up one stack frame\n");
    debug_printf(ctx, "  down           - Move down one stack frame\n");
    debug_printf(ctx, "  l, list        - List breakpoints\n");
    debug_printf(ctx, "  p <var>        - Print variable value\n");
    debug_printf(ctx, "  set <var>=<val> - Set variable value\n");
    debug_printf(ctx, "  vars           - Show all variables in scope\n");
    debug_printf(ctx, "  watch <var>    - Add variable to watch list\n");
    debug_printf(ctx, "  eval <expr>    - Evaluate expression\n");
    debug_printf(ctx, "  where          - Show current location and context\n");
    debug_printf(ctx, "  h, help        - Show this help\n");
    debug_printf(ctx, "  q, quit        - Exit debug mode and continue\n");
    debug_printf(ctx, "\nTip: Use Tab for command completion\n");
}

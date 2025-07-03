#include "../../include/debug.h"
#include "../../include/errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

            debug_printf(ctx, "Hit breakpoint %d at %s:%d (hit count: %d)\n",
                         bp->id, file, line, bp->hit_count);

            // TODO: Evaluate condition if present
            if (bp->condition) {
                debug_printf(
                    ctx, "  Condition: %s (evaluation not yet implemented)\n",
                    bp->condition);
            }

            // Show current context
            debug_show_stack(ctx);

            // Enter step mode
            ctx->mode = DEBUG_MODE_STEP;
            ctx->step_mode = true;

            // Wait for user input
            debug_printf(ctx, "Debug> ");
            fflush(ctx->debug_output);

            char input[256];
            if (fgets(input, sizeof(input), stdin)) {
                debug_handle_user_input(ctx, input);
            }

            return true;
        }
        bp = bp->next;
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

    // Remove newline
    char *cmd = strdup(input);
    char *newline = strchr(cmd, '\n');
    if (newline) {
        *newline = '\0';
    }

    // Parse command
    if (strcmp(cmd, "c") == 0 || strcmp(cmd, "continue") == 0) {
        debug_continue(ctx);
    } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "step") == 0) {
        debug_step_into(ctx);
    } else if (strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0) {
        debug_step_over(ctx);
    } else if (strcmp(cmd, "f") == 0 || strcmp(cmd, "finish") == 0) {
        debug_step_out(ctx);
    } else if (strcmp(cmd, "bt") == 0 || strcmp(cmd, "backtrace") == 0) {
        debug_show_stack(ctx);
    } else if (strcmp(cmd, "l") == 0 || strcmp(cmd, "list") == 0) {
        debug_list_breakpoints(ctx);
    } else if (strncmp(cmd, "p ", 2) == 0 || strncmp(cmd, "print ", 6) == 0) {
        // Variable inspection
        char *var = cmd + (cmd[1] == ' ' ? 2 : 6);
        debug_inspect_variable(ctx, var);
    } else if (strcmp(cmd, "vars") == 0) {
        debug_inspect_all_variables(ctx);
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
        debug_print_help(ctx);
    } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        debug_printf(ctx, "Exiting debug mode\n");
        ctx->enabled = false;
    } else {
        debug_printf(ctx, "Unknown command: %s (type 'help' for commands)\n",
                     cmd);
    }

    free(cmd);
}

// Print debug help
void debug_print_help(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_header(ctx, "Debug Commands");
    debug_printf(ctx, "  c, continue  - Continue execution\n");
    debug_printf(ctx, "  s, step      - Step into next statement\n");
    debug_printf(ctx, "  n, next      - Step over next statement\n");
    debug_printf(ctx, "  f, finish    - Step out of current function\n");
    debug_printf(ctx, "  bt, backtrace - Show call stack\n");
    debug_printf(ctx, "  l, list      - List breakpoints\n");
    debug_printf(ctx, "  p <var>      - Print variable value\n");
    debug_printf(ctx, "  vars         - Show all variables\n");
    debug_printf(ctx, "  h, help      - Show this help\n");
    debug_printf(ctx, "  q, quit      - Exit debug mode\n");
}

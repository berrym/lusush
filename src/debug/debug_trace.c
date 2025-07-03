#include "../../include/debug.h"
#include "../../include/errors.h"
#include "../../include/node.h"
#include "../../include/symtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Trace node execution
void debug_trace_node(debug_context_t *ctx, node_t *node, const char *file,
                      int line) {
    if (!ctx || !ctx->enabled || !ctx->trace_execution || !node) {
        return;
    }

    char *desc = debug_get_node_description(node);
    debug_printf(ctx, "TRACE: %s:%d - %s\n", file ? file : "unknown", line,
                 desc);
    free(desc);

    // Show timing if enabled
    if (ctx->show_timing) {
        long current_time = debug_get_time_ns();
        char time_str[64];
        debug_format_time(current_time -
                              ctx->session_start.tv_sec * 1000000000L -
                              ctx->session_start.tv_nsec,
                          time_str, sizeof(time_str));
        debug_printf(ctx, "  Time: %s\n", time_str);
    }

    // Show variables if enabled and we have a current frame
    if (ctx->show_variables && ctx->current_frame &&
        ctx->current_frame->local_vars) {
        debug_printf(ctx, "  Variables in scope:\n");
        // TODO: Implement variable display from symtable
    }

    ctx->total_commands++;
}

// Trace command execution
void debug_trace_command(debug_context_t *ctx, const char *command, char **argv,
                         int argc) {
    if (!ctx || !ctx->enabled || !ctx->trace_execution || !command) {
        return;
    }

    debug_printf(ctx, "COMMAND: %s", command);

    // Show arguments
    if (argv && argc > 1) {
        debug_printf(ctx, " with args: ");
        for (int i = 1; i < argc; i++) {
            fprintf(ctx->debug_output, "'%s'", argv[i]);
            if (i < argc - 1) {
                fprintf(ctx->debug_output, " ");
            }
        }
        fprintf(ctx->debug_output, "\n");
    } else {
        fprintf(ctx->debug_output, "\n");
    }

    fflush(ctx->debug_output);
}

// Trace builtin command execution
void debug_trace_builtin(debug_context_t *ctx, const char *builtin, char **argv,
                         int argc) {
    if (!ctx || !ctx->enabled || !ctx->trace_execution || !builtin) {
        return;
    }

    debug_printf(ctx, "BUILTIN: %s", builtin);

    // Show arguments
    if (argv && argc > 1) {
        debug_printf(ctx, " with args: ");
        for (int i = 1; i < argc; i++) {
            fprintf(ctx->debug_output, "'%s'", argv[i]);
            if (i < argc - 1) {
                fprintf(ctx->debug_output, " ");
            }
        }
        fprintf(ctx->debug_output, "\n");
    } else {
        fprintf(ctx->debug_output, "\n");
    }

    fflush(ctx->debug_output);
}

// Trace function call
void debug_trace_function_call(debug_context_t *ctx, const char *function,
                               char **argv, int argc) {
    if (!ctx || !ctx->enabled || !ctx->trace_execution || !function) {
        return;
    }

    debug_printf(ctx, "FUNCTION: %s", function);

    // Show arguments
    if (argv && argc > 1) {
        debug_printf(ctx, " with args: ");
        for (int i = 1; i < argc; i++) {
            fprintf(ctx->debug_output, "'%s'", argv[i]);
            if (i < argc - 1) {
                fprintf(ctx->debug_output, " ");
            }
        }
        fprintf(ctx->debug_output, "\n");
    } else {
        fprintf(ctx->debug_output, "\n");
    }

    fflush(ctx->debug_output);
}

// Stack frame management
debug_frame_t *debug_push_frame(debug_context_t *ctx, const char *function,
                                const char *file, int line) {
    if (!ctx || !function) {
        return NULL;
    }

    // Check stack depth limit
    if (ctx->stack_depth >= ctx->max_stack_depth) {
        debug_printf(ctx, "WARNING: Maximum stack depth reached (%d)\n",
                     ctx->max_stack_depth);
        return NULL;
    }

    debug_frame_t *frame = malloc(sizeof(debug_frame_t));
    if (!frame) {
        debug_printf(ctx, "ERROR: Failed to allocate debug frame\n");
        return NULL;
    }

    // Initialize frame
    frame->function_name = strdup(function);
    frame->file_path = file ? strdup(file) : NULL;
    frame->line_number = line;
    frame->current_node = NULL;
    frame->local_vars = NULL;
    frame->parent = ctx->current_frame;

    // Set timing
    clock_gettime(CLOCK_MONOTONIC, &frame->start_time);
    frame->end_time.tv_sec = 0;
    frame->end_time.tv_nsec = 0;

    // Update context
    ctx->current_frame = frame;
    ctx->stack_depth++;

    if (ctx->trace_execution) {
        debug_printf(ctx, "ENTER: %s (%s:%d) [depth: %d]\n", function,
                     file ? file : "unknown", line, ctx->stack_depth);
    }

    return frame;
}

// Pop stack frame
void debug_pop_frame(debug_context_t *ctx) {
    if (!ctx || !ctx->current_frame) {
        return;
    }

    debug_frame_t *frame = ctx->current_frame;

    // Set end time
    clock_gettime(CLOCK_MONOTONIC, &frame->end_time);

    // Calculate execution time
    long duration_ns =
        (frame->end_time.tv_sec - frame->start_time.tv_sec) * 1000000000L +
        (frame->end_time.tv_nsec - frame->start_time.tv_nsec);

    if (ctx->trace_execution) {
        char time_str[64];
        debug_format_time(duration_ns, time_str, sizeof(time_str));
        debug_printf(ctx, "EXIT: %s (duration: %s) [depth: %d]\n",
                     frame->function_name, time_str, ctx->stack_depth);
    }

    // Update context
    ctx->current_frame = frame->parent;
    ctx->stack_depth--;

    // Update total time
    ctx->total_time_ns += duration_ns;

    // Clean up frame
    free(frame->function_name);
    free(frame->file_path);
    free(frame);
}

// Update current frame node
void debug_update_frame_node(debug_context_t *ctx, node_t *node) {
    if (!ctx || !ctx->current_frame) {
        return;
    }

    ctx->current_frame->current_node = node;
}

// Show current stack
void debug_show_stack(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_header(ctx, "Call Stack");

    if (!ctx->current_frame) {
        debug_printf(ctx, "  (empty)\n");
        return;
    }

    debug_frame_t *frame = ctx->current_frame;
    int depth = ctx->stack_depth;

    while (frame) {
        debug_printf(ctx, "  #%d: %s", depth, frame->function_name);

        if (frame->file_path) {
            fprintf(ctx->debug_output, " at %s:%d", frame->file_path,
                    frame->line_number);
        }

        // Show timing for current frame
        if (ctx->show_timing && frame == ctx->current_frame) {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            long duration_ns =
                (current_time.tv_sec - frame->start_time.tv_sec) * 1000000000L +
                (current_time.tv_nsec - frame->start_time.tv_nsec);
            char time_str[64];
            debug_format_time(duration_ns, time_str, sizeof(time_str));
            fprintf(ctx->debug_output, " (running: %s)", time_str);
        }

        fprintf(ctx->debug_output, "\n");

        frame = frame->parent;
        depth--;
    }
}

// Variable inspection
void debug_inspect_variable(debug_context_t *ctx, const char *name) {
    if (!ctx || !ctx->enabled || !name) {
        return;
    }

    debug_printf(ctx, "VARIABLE: %s\n", name);

    // TODO: Implement variable lookup from symtable
    // For now, show placeholder
    debug_printf(ctx, "  Value: (variable inspection not yet implemented)\n");
    debug_printf(ctx, "  Scope: %s\n",
                 ctx->current_frame ? ctx->current_frame->function_name
                                    : "global");
}

// Inspect all variables
void debug_inspect_all_variables(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_header(ctx, "Variable Inspection");

    if (!ctx->current_frame) {
        debug_printf(ctx, "  No active frame\n");
        return;
    }

    // TODO: Implement full variable inspection
    debug_printf(ctx, "  Frame: %s\n", ctx->current_frame->function_name);
    debug_printf(ctx, "  (variable inspection not yet implemented)\n");
}

// Watch variable (placeholder)
void debug_watch_variable(debug_context_t *ctx, const char *name) {
    if (!ctx || !ctx->enabled || !name) {
        return;
    }

    debug_printf(ctx, "WATCH: %s (watching not yet implemented)\n", name);
}

// Show variable changes (placeholder)
void debug_show_variable_changes(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_printf(ctx, "Variable changes: (not yet implemented)\n");
}

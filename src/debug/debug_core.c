#include "../../include/debug.h"
#include "../../include/errors.h"
#include "../../include/executor.h"
#include "../../include/lusush.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Global debug context
debug_context_t *g_debug_context = NULL;

// External reference to current executor for function introspection
extern executor_t *current_executor;

// Initialize debug context
debug_context_t *debug_init(void) {
    debug_context_t *ctx = malloc(sizeof(debug_context_t));
    if (!ctx) {
        return NULL;
    }

    // Initialize all fields to default values
    ctx->level = DEBUG_NONE;
    ctx->mode = DEBUG_MODE_NORMAL;
    ctx->enabled = false;
    ctx->step_mode = false;
    ctx->trace_execution = false;
    ctx->profile_enabled = false;
    ctx->analysis_enabled = false;

    // Execution state
    ctx->current_frame = NULL;
    ctx->stack_depth = 0;

    // Breakpoints
    ctx->breakpoints = NULL;
    ctx->next_breakpoint_id = 1;

    // Profiling
    ctx->profile_data = NULL;
    ctx->timing_enabled = false;

    // Analysis
    ctx->analysis_issues = NULL;
    ctx->issue_count = 0;

    // Output control - default to stderr for debug output
    ctx->debug_output = stderr;
    ctx->profile_output = stderr;
    ctx->analysis_output = stderr;

    // Configuration defaults
    ctx->show_variables = true;
    ctx->show_stack_trace = true;
    ctx->show_timing = false;
    ctx->highlight_syntax = true;
    ctx->max_stack_depth = 100;

    // Statistics
    ctx->total_commands = 0;
    ctx->total_time_ns = 0;
    clock_gettime(CLOCK_MONOTONIC, &ctx->session_start);

    // Set global context
    g_debug_context = ctx;

    return ctx;
}

// Cleanup debug context
void debug_cleanup(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    // Clean up stack frames
    while (ctx->current_frame) {
        debug_pop_frame(ctx);
    }

    // Clean up breakpoints
    debug_clear_breakpoints(ctx);

    // Clean up profile data
    profile_data_t *profile = ctx->profile_data;
    while (profile) {
        profile_data_t *next = profile->next;
        free(profile->function_name);
        free(profile->file_path);
        free(profile);
        profile = next;
    }

    // Clean up analysis issues
    debug_clear_analysis_issues(ctx);

    // Close output files if they're not stderr
    if (ctx->debug_output != stderr) {
        fclose(ctx->debug_output);
    }
    if (ctx->profile_output != stderr) {
        fclose(ctx->profile_output);
    }
    if (ctx->analysis_output != stderr) {
        fclose(ctx->analysis_output);
    }

    // Clear global context if this is it
    if (g_debug_context == ctx) {
        g_debug_context = NULL;
    }

    free(ctx);
}

// Set debug level
void debug_set_level(debug_context_t *ctx, debug_level_t level) {
    if (!ctx) {
        return;
    }

    ctx->level = level;

    // Enable features based on level
    switch (level) {
    case DEBUG_NONE:
        ctx->enabled = false;
        ctx->trace_execution = false;
        ctx->profile_enabled = false;
        ctx->analysis_enabled = false;
        break;

    case DEBUG_BASIC:
        ctx->enabled = true;
        ctx->trace_execution = false;
        ctx->profile_enabled = false;
        ctx->analysis_enabled = false;
        break;

    case DEBUG_VERBOSE:
        ctx->enabled = true;
        ctx->trace_execution = true;
        ctx->profile_enabled = false;
        ctx->analysis_enabled = false;
        break;

    case DEBUG_TRACE:
        ctx->enabled = true;
        ctx->trace_execution = true;
        ctx->profile_enabled = false;
        ctx->analysis_enabled = true;
        break;

    case DEBUG_PROFILE:
        ctx->enabled = true;
        ctx->trace_execution = true;
        ctx->profile_enabled = true;
        ctx->analysis_enabled = true;
        ctx->timing_enabled = true;
        break;
    }
}

// Set debug mode
void debug_set_mode(debug_context_t *ctx, debug_mode_t mode) {
    if (!ctx) {
        return;
    }

    ctx->mode = mode;

    switch (mode) {
    case DEBUG_MODE_NORMAL:
        ctx->step_mode = false;
        break;

    case DEBUG_MODE_STEP:
    case DEBUG_MODE_STEP_OVER:
        ctx->step_mode = true;
        break;

    case DEBUG_MODE_CONTINUE:
        ctx->step_mode = false;
        break;
    }
}

// Enable/disable debugging
void debug_enable(debug_context_t *ctx, bool enable) {
    if (!ctx) {
        return;
    }

    ctx->enabled = enable;

    if (enable) {
        debug_printf(ctx, "Debug mode enabled (level: %d)\n", ctx->level);
        debug_printf(ctx, "Debug session started at: %ld.%09ld\n",
                     ctx->session_start.tv_sec, ctx->session_start.tv_nsec);
    } else {
        debug_printf(ctx, "Debug mode disabled\n");
    }
}

// Get current time in nanoseconds
long debug_get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

// Format time duration
void debug_format_time(long ns, char *buffer, size_t size) {
    if (ns < 1000) {
        snprintf(buffer, size, "%ld ns", ns);
    } else if (ns < 1000000) {
        snprintf(buffer, size, "%.2f μs", ns / 1000.0);
    } else if (ns < 1000000000) {
        snprintf(buffer, size, "%.2f ms", ns / 1000000.0);
    } else {
        snprintf(buffer, size, "%.2f s", ns / 1000000000.0);
    }
}

// Debug printf with context
void debug_printf(debug_context_t *ctx, const char *format, ...) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    va_list args;
    va_start(args, format);

    // Add debug prefix
    fprintf(ctx->debug_output, "[DEBUG] ");

    // Add stack depth indication
    for (int i = 0; i < ctx->stack_depth; i++) {
        fprintf(ctx->debug_output, "  ");
    }

    // Print the actual message
    vfprintf(ctx->debug_output, format, args);
    fflush(ctx->debug_output);

    va_end(args);
}

// Print separator line
void debug_print_separator(debug_context_t *ctx) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    fprintf(ctx->debug_output, "[DEBUG] ");
    for (int i = 0; i < 60; i++) {
        fprintf(ctx->debug_output, "-");
    }
    fprintf(ctx->debug_output, "\n");
    fflush(ctx->debug_output);
}

// Print header with title
void debug_print_header(debug_context_t *ctx, const char *title) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_separator(ctx);
    debug_printf(ctx, "%s\n", title);
    debug_print_separator(ctx);
}

// Set output file for debug messages
void debug_set_output_file(debug_context_t *ctx, const char *filename) {
    if (!ctx || !filename) {
        return;
    }

    // Close existing file if it's not stderr
    if (ctx->debug_output != stderr) {
        fclose(ctx->debug_output);
    }

    // Open new file
    ctx->debug_output = fopen(filename, "w");
    if (!ctx->debug_output) {
        ctx->debug_output = stderr; // Fallback to stderr
        debug_printf(
            ctx,
            "Warning: Could not open debug output file '%s', using stderr\n",
            filename);
    } else {
        debug_printf(ctx, "Debug output redirected to: %s\n", filename);
    }
}

// Set profile output file
void debug_set_profile_output_file(debug_context_t *ctx, const char *filename) {
    if (!ctx || !filename) {
        return;
    }

    if (ctx->profile_output != stderr) {
        fclose(ctx->profile_output);
    }

    ctx->profile_output = fopen(filename, "w");
    if (!ctx->profile_output) {
        ctx->profile_output = stderr;
        debug_printf(
            ctx,
            "Warning: Could not open profile output file '%s', using stderr\n",
            filename);
    } else {
        debug_printf(ctx, "Profile output redirected to: %s\n", filename);
    }
}

// Set analysis output file
void debug_set_analysis_output_file(debug_context_t *ctx,
                                    const char *filename) {
    if (!ctx || !filename) {
        return;
    }

    if (ctx->analysis_output != stderr) {
        fclose(ctx->analysis_output);
    }

    ctx->analysis_output = fopen(filename, "w");
    if (!ctx->analysis_output) {
        ctx->analysis_output = stderr;
        debug_printf(
            ctx,
            "Warning: Could not open analysis output file '%s', using stderr\n",
            filename);
    } else {
        debug_printf(ctx, "Analysis output redirected to: %s\n", filename);
    }
}

// Get node description for debugging
char *debug_get_node_description(node_t *node) {
    if (!node) {
        return strdup("(null)");
    }

    char *desc = malloc(256);
    if (!desc) {
        return NULL;
    }

    switch (node->type) {
    case NODE_COMMAND:
        snprintf(desc, 256, "COMMAND: %s",
                 node->val.str ? node->val.str : "(empty)");
        break;
    case NODE_PIPE:
        snprintf(desc, 256, "PIPE");
        break;
    case NODE_IF:
        snprintf(desc, 256, "IF");
        break;
    case NODE_WHILE:
        snprintf(desc, 256, "WHILE");
        break;
    case NODE_FOR:
        snprintf(desc, 256, "FOR");
        break;
    case NODE_CASE:
        snprintf(desc, 256, "CASE");
        break;
    case NODE_FUNCTION:
        snprintf(desc, 256, "FUNCTION: %s",
                 node->val.str ? node->val.str : "(anonymous)");
        break;
    case NODE_BRACE_GROUP:
        snprintf(desc, 256, "BRACE_GROUP");
        break;
    case NODE_SUBSHELL:
        snprintf(desc, 256, "SUBSHELL");
        break;
    case NODE_LOGICAL_AND:
        snprintf(desc, 256, "LOGICAL_AND");
        break;
    case NODE_LOGICAL_OR:
        snprintf(desc, 256, "LOGICAL_OR");
        break;
    default:
        snprintf(desc, 256, "UNKNOWN_NODE_TYPE_%d", node->type);
        break;
    }

    return desc;
}

// Print node information
void debug_print_node(debug_context_t *ctx, node_t *node, int indent) {
    if (!ctx || !ctx->enabled || !node) {
        return;
    }

    // Print indentation
    for (int i = 0; i < indent; i++) {
        fprintf(ctx->debug_output, "  ");
    }

    char *desc = debug_get_node_description(node);
    fprintf(ctx->debug_output, "[DEBUG] %s\n", desc);
    free(desc);

    // Print children
    if (node->first_child) {
        debug_print_node(ctx, node->first_child, indent + 1);
    }

    // Print siblings
    if (node->next_sibling) {
        debug_print_node(ctx, node->next_sibling, indent);
    }
}

// Print entire AST
void debug_print_ast(debug_context_t *ctx, node_t *ast) {
    if (!ctx || !ctx->enabled) {
        return;
    }

    debug_print_header(ctx, "Abstract Syntax Tree");
    debug_print_node(ctx, ast, 0);
}

// Clear analysis issues
void debug_clear_analysis_issues(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    analysis_issue_t *issue = ctx->analysis_issues;
    while (issue) {
        analysis_issue_t *next = issue->next;
        free(issue->file_path);
        free(issue->severity);
        free(issue->category);
        free(issue->message);
        free(issue->suggestion);
        free(issue);
        issue = next;
    }

    ctx->analysis_issues = NULL;
    ctx->issue_count = 0;
}

// Function introspection implementation
void debug_list_functions(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    // Access the current executor to get function definitions
    if (!current_executor || !current_executor->functions) {
        printf("No functions defined.\n");
        return;
    }

    printf("Defined functions:\n");
    
    function_def_t *func = current_executor->functions;
    int count = 0;
    
    while (func) {
        count++;
        printf("  %d. %s\n", count, func->name);
        func = func->next;
    }
    
    if (count == 0) {
        printf("  (none)\n");
    } else {
        printf("\nTotal: %d function%s\n", count, count == 1 ? "" : "s");
        printf("Use 'debug function <name>' to see function details.\n");
    }
}

void debug_show_function(debug_context_t *ctx, const char *function_name) {
    if (!ctx || !function_name) {
        return;
    }

    // Access the current executor to find the function
    if (!current_executor) {
        printf("No executor context available.\n");
        return;
    }

    // Find the function in the executor's function list
    function_def_t *func = current_executor->functions;
    while (func) {
        if (strcmp(func->name, function_name) == 0) {
            break;
        }
        func = func->next;
    }

    if (!func) {
        printf("Function '%s' not found.\n", function_name);
        printf("Use 'debug functions' to list all defined functions.\n");
        return;
    }

    // Display function information
    printf("Function: %s\n", func->name);
    printf("========================================\n");
    
    if (!func->body) {
        printf("Body: (empty function)\n");
    } else {
        printf("Body: (AST representation)\n");
        
        // Print the AST structure for the function body
        printf("AST Structure:\n");
        debug_print_node(ctx, func->body, 2);
    }
    
    printf("========================================\n");
    printf("Usage: %s [arguments...]\n", func->name);
}

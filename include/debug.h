#ifndef DEBUG_H
#define DEBUG_H

#include "node.h"
#include "symtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

// Debug levels for different types of debugging information
typedef enum {
    DEBUG_NONE = 0,
    DEBUG_BASIC = 1,
    DEBUG_VERBOSE = 2,
    DEBUG_TRACE = 3,
    DEBUG_PROFILE = 4
} debug_level_t;

// Debug execution modes
typedef enum {
    DEBUG_MODE_NORMAL,    // Normal execution
    DEBUG_MODE_STEP,      // Step-by-step execution
    DEBUG_MODE_STEP_OVER, // Step over function calls
    DEBUG_MODE_CONTINUE   // Continue to next breakpoint
} debug_mode_t;

// Breakpoint structure
typedef struct breakpoint {
    int id;
    char *file;
    int line;
    char *condition;
    int hit_count;
    bool enabled;
    struct breakpoint *next;
} breakpoint_t;

// Debug stack frame for tracking execution context
typedef struct debug_frame {
    char *function_name;
    char *file_path;
    int line_number;
    node_t *current_node;
    symtable_t *local_vars;
    struct debug_frame *parent;
    struct timespec start_time;
    struct timespec end_time;
} debug_frame_t;

// Performance profiling data
typedef struct profile_data {
    char *function_name;
    char *file_path;
    long total_time_ns;
    int call_count;
    long min_time_ns;
    long max_time_ns;
    struct profile_data *next;
} profile_data_t;

// Script analysis issue
typedef struct analysis_issue {
    char *file_path;
    int line_number;
    char *severity; // "error", "warning", "info"
    char *category; // "syntax", "performance", "security", "style"
    char *message;
    char *suggestion;
    struct analysis_issue *next;
} analysis_issue_t;

// Main debug context
typedef struct debug_context {
    debug_level_t level;
    debug_mode_t mode;
    bool enabled;
    bool step_mode;
    bool trace_execution;
    bool profile_enabled;
    bool analysis_enabled;

    // Execution state
    debug_frame_t *current_frame;
    int stack_depth;

    // Breakpoints
    breakpoint_t *breakpoints;
    int next_breakpoint_id;

    // Profiling
    profile_data_t *profile_data;
    bool timing_enabled;

    // Analysis
    analysis_issue_t *analysis_issues;
    int issue_count;

    // Output control
    FILE *debug_output;
    FILE *profile_output;
    FILE *analysis_output;

    // Configuration
    bool show_variables;
    bool show_stack_trace;
    bool show_timing;
    bool highlight_syntax;
    int max_stack_depth;

    // Statistics
    long total_commands;
    long total_time_ns;
    struct timespec session_start;
} debug_context_t;

// Global debug context
extern debug_context_t *g_debug_context;

// Core debug system functions
debug_context_t *debug_init(void);
void debug_cleanup(debug_context_t *ctx);
void debug_set_level(debug_context_t *ctx, debug_level_t level);
void debug_set_mode(debug_context_t *ctx, debug_mode_t mode);
void debug_enable(debug_context_t *ctx, bool enable);

// Execution tracing
void debug_trace_node(debug_context_t *ctx, node_t *node, const char *file,
                      int line);
void debug_trace_command(debug_context_t *ctx, const char *command, char **argv,
                         int argc);
void debug_trace_builtin(debug_context_t *ctx, const char *builtin, char **argv,
                         int argc);
void debug_trace_function_call(debug_context_t *ctx, const char *function,
                               char **argv, int argc);

// Stack frame management
debug_frame_t *debug_push_frame(debug_context_t *ctx, const char *function,
                                const char *file, int line);
void debug_pop_frame(debug_context_t *ctx);
void debug_update_frame_node(debug_context_t *ctx, node_t *node);
void debug_show_stack(debug_context_t *ctx);

// Breakpoint management
int debug_add_breakpoint(debug_context_t *ctx, const char *file, int line,
                         const char *condition);
bool debug_remove_breakpoint(debug_context_t *ctx, int id);
bool debug_enable_breakpoint(debug_context_t *ctx, int id, bool enable);
bool debug_check_breakpoint(debug_context_t *ctx, const char *file, int line);
void debug_list_breakpoints(debug_context_t *ctx);
void debug_clear_breakpoints(debug_context_t *ctx);

// Variable inspection
void debug_inspect_variable(debug_context_t *ctx, const char *name);
void debug_inspect_all_variables(debug_context_t *ctx);
void debug_watch_variable(debug_context_t *ctx, const char *name);
void debug_show_variable_changes(debug_context_t *ctx);

// Step execution
void debug_step_into(debug_context_t *ctx);
void debug_step_over(debug_context_t *ctx);
void debug_step_out(debug_context_t *ctx);
void debug_continue(debug_context_t *ctx);

// Performance profiling
void debug_profile_start(debug_context_t *ctx);
void debug_profile_stop(debug_context_t *ctx);
void debug_profile_function_enter(debug_context_t *ctx, const char *function);
void debug_profile_function_exit(debug_context_t *ctx, const char *function);
void debug_profile_report(debug_context_t *ctx);
void debug_profile_reset(debug_context_t *ctx);

// Script analysis
void debug_analyze_script(debug_context_t *ctx, const char *script_path);
void debug_add_analysis_issue(debug_context_t *ctx, const char *file, int line,
                              const char *severity, const char *category,
                              const char *message, const char *suggestion);
void debug_show_analysis_report(debug_context_t *ctx);
void debug_clear_analysis_issues(debug_context_t *ctx);

// Utility functions
void debug_print_node(debug_context_t *ctx, node_t *node, int indent);
void debug_print_ast(debug_context_t *ctx, node_t *ast);
char *debug_get_node_description(node_t *node);
long debug_get_time_ns(void);
void debug_format_time(long ns, char *buffer, size_t size);

// Output functions
void debug_printf(debug_context_t *ctx, const char *format, ...);
void debug_print_separator(debug_context_t *ctx);
void debug_print_header(debug_context_t *ctx, const char *title);

// Configuration
void debug_set_output_file(debug_context_t *ctx, const char *filename);
void debug_set_profile_output_file(debug_context_t *ctx, const char *filename);
void debug_set_analysis_output_file(debug_context_t *ctx, const char *filename);

// Integration macros for easy debugging
#define DEBUG_TRACE_NODE(node, file, line)                                     \
    if (g_debug_context && g_debug_context->enabled) {                         \
        debug_trace_node(g_debug_context, node, file, line);                   \
    }

#define DEBUG_TRACE_COMMAND(cmd, argv, argc)                                   \
    if (g_debug_context && g_debug_context->enabled) {                         \
        debug_trace_command(g_debug_context, cmd, argv, argc);                 \
    }

#define DEBUG_PROFILE_ENTER(func)                                              \
    if (g_debug_context && g_debug_context->profile_enabled) {                 \
        debug_profile_function_enter(g_debug_context, func);                   \
    }

#define DEBUG_PROFILE_EXIT(func)                                               \
    if (g_debug_context && g_debug_context->profile_enabled) {                 \
        debug_profile_function_exit(g_debug_context, func);                    \
    }

#define DEBUG_BREAKPOINT_CHECK(file, line)                                     \
    if (g_debug_context && g_debug_context->enabled) {                         \
        debug_check_breakpoint(g_debug_context, file, line);                   \
    }

// User interaction functions
void debug_handle_user_input(debug_context_t *ctx, const char *input);
void debug_print_help(debug_context_t *ctx);

#endif // DEBUG_H

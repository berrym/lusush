/**
 * @file debug_analysis.c
 * @brief Script Analysis and Issue Detection
 *
 * Provides static analysis capabilities for shell scripts, detecting
 * syntax errors, style issues, security vulnerabilities, performance
 * anti-patterns, and portability concerns.
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (C) 2021-2026 Michael Berry
 */

#include "debug.h"
#include "errors.h"
#include "node.h"
#include "parser.h"
#include "tokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Forward declarations for static functions
static void debug_analyze_syntax(debug_context_t *ctx, const char *file,
                                 const char *content);
static void debug_analyze_style(debug_context_t *ctx, const char *file,
                                const char *content);
static void debug_analyze_performance(debug_context_t *ctx, const char *file,
                                      const char *content);
static void debug_analyze_security(debug_context_t *ctx, const char *file,
                                   const char *content);
static void debug_analyze_portability(debug_context_t *ctx, const char *file,
                                      const char *content);

/**
 * @brief Analyze a script file for various issues
 * @param ctx Debug context for output
 * @param script_path Path to the script file to analyze
 */
void debug_analyze_script(debug_context_t *ctx, const char *script_path) {
    if (!ctx || !script_path) {
        return;
    }

    debug_printf(ctx, "Analyzing script: %s\n", script_path);

    // Check if file exists
    struct stat st;
    if (stat(script_path, &st) != 0) {
        debug_printf(ctx, "ERROR: Script file not found: %s\n", script_path);
        return;
    }

    // Read script file
    FILE *file = fopen(script_path, "r");
    if (!file) {
        debug_printf(ctx, "ERROR: Cannot open script file: %s\n", script_path);
        return;
    }

    // Read entire file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *script_content = malloc(file_size + 1);
    if (!script_content) {
        fclose(file);
        debug_printf(ctx, "ERROR: Memory allocation failed\n");
        return;
    }

    fread(script_content, 1, file_size, file);
    script_content[file_size] = '\0';
    fclose(file);

    // Clear previous analysis results
    debug_clear_analysis_issues(ctx);

    // Perform various analysis checks
    debug_analyze_syntax(ctx, script_path, script_content);
    debug_analyze_style(ctx, script_path, script_content);
    debug_analyze_performance(ctx, script_path, script_content);
    debug_analyze_security(ctx, script_path, script_content);
    debug_analyze_portability(ctx, script_path, script_content);

    // Generate analysis report
    debug_show_analysis_report(ctx);

    free(script_content);
}

/**
 * @brief Add an analysis issue to the context
 * @param ctx Debug context
 * @param file File path where issue was found
 * @param line Line number of the issue
 * @param severity Severity level (error, warning, info)
 * @param category Category of issue (syntax, style, security, etc.)
 * @param message Description of the issue
 * @param suggestion Suggested fix for the issue
 */
void debug_add_analysis_issue(debug_context_t *ctx, const char *file, int line,
                              const char *severity, const char *category,
                              const char *message, const char *suggestion) {
    if (!ctx || !file || !severity || !category || !message) {
        return;
    }

    analysis_issue_t *issue = malloc(sizeof(analysis_issue_t));
    if (!issue) {
        return;
    }

    issue->file_path = strdup(file);
    issue->line_number = line;
    issue->severity = strdup(severity);
    issue->category = strdup(category);
    issue->message = strdup(message);
    issue->suggestion = suggestion ? strdup(suggestion) : NULL;
    issue->next = ctx->analysis_issues;

    ctx->analysis_issues = issue;
    ctx->issue_count++;
}

/**
 * @brief Analyze script for syntax issues
 * @param ctx Debug context
 * @param file File path being analyzed
 * @param content Script content to analyze
 */
static void debug_analyze_syntax(debug_context_t *ctx, const char *file,
                                 const char *content) {
    if (!ctx || !file || !content) {
        return;
    }

    // Try to parse the script
    parser_t *parser = parser_new(content);
    if (!parser) {
        debug_add_analysis_issue(ctx, file, 1, "error", "syntax",
                                 "Failed to create parser",
                                 "Check script syntax");
        return;
    }

    // Parse and check for errors
    node_t *ast = parser_parse(parser);
    if (!ast) {
        debug_add_analysis_issue(
            ctx, file, 1, "error", "syntax", "Syntax error in script",
            "Check parentheses, quotes, and command structure");
    } else {
        // Basic syntax validation passed
        debug_printf(ctx, "Syntax validation: PASSED\n");
    }

    parser_free(parser);
    if (ast) {
        // TODO: Implement proper AST cleanup
        // node_free(ast);
    }
}

/**
 * @brief Analyze script for style issues
 * @param ctx Debug context
 * @param file File path being analyzed
 * @param content Script content to analyze
 */
static void debug_analyze_style(debug_context_t *ctx, const char *file,
                                const char *content) {
    if (!ctx || !file || !content) {
        return;
    }

    int line_number = 1;
    const char *line_start = content;
    const char *pos = content;

    while (*pos) {
        if (*pos == '\n') {
            // Check line length
            int line_length = pos - line_start;
            if (line_length > 120) {
                debug_add_analysis_issue(ctx, file, line_number, "warning",
                                         "style", "Line too long",
                                         "Consider breaking long lines");
            }

            // Check for trailing whitespace
            if (pos > line_start && (*(pos - 1) == ' ' || *(pos - 1) == '\t')) {
                debug_add_analysis_issue(ctx, file, line_number, "info",
                                         "style", "Trailing whitespace",
                                         "Remove trailing spaces/tabs");
            }

            line_number++;
            line_start = pos + 1;
        }
        pos++;
    }

    // Check for shebang
    if (strncmp(content, "#!", 2) != 0) {
        debug_add_analysis_issue(ctx, file, 1, "warning", "style",
                                 "Missing shebang",
                                 "Add #!/bin/sh or #!/usr/bin/env lusush");
    }

    // Check for consistent indentation
    bool uses_tabs = false;
    bool uses_spaces = false;
    pos = content;
    line_number = 1;

    while (*pos) {
        if (*pos == '\n') {
            line_number++;
            pos++;
            // Check indentation at start of line
            while (*pos == ' ' || *pos == '\t') {
                if (*pos == '\t') {
                    uses_tabs = true;
                } else {
                    uses_spaces = true;
                }
                pos++;
            }
        } else {
            pos++;
        }
    }

    if (uses_tabs && uses_spaces) {
        debug_add_analysis_issue(ctx, file, 1, "warning", "style",
                                 "Mixed tabs and spaces",
                                 "Use consistent indentation");
    }
}

/**
 * @brief Analyze script for performance issues
 * @param ctx Debug context
 * @param file File path being analyzed
 * @param content Script content to analyze
 */
static void debug_analyze_performance(debug_context_t *ctx, const char *file,
                                      const char *content) {
    if (!ctx || !file || !content) {
        return;
    }

    int line_number = 1;
    const char *pos = content;

    // Look for performance anti-patterns
    while (*pos) {
        if (*pos == '\n') {
            line_number++;
        }

        // Check for inefficient patterns
        if (strncmp(pos, "cat ", 4) == 0 && strstr(pos, " | ") != NULL) {
            debug_add_analysis_issue(ctx, file, line_number, "info",
                                     "performance", "Useless use of cat",
                                     "Use input redirection instead");
        }

        if (strncmp(pos, "$(ls ", 5) == 0 || strncmp(pos, "`ls ", 4) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "warning",
                                     "performance", "Parsing ls output",
                                     "Use shell globbing instead");
        }

        if (strncmp(pos, "for ", 4) == 0 && strstr(pos, "$(seq ") != NULL) {
            debug_add_analysis_issue(ctx, file, line_number, "info",
                                     "performance", "Inefficient loop",
                                     "Use arithmetic expansion instead");
        }

        pos++;
    }
}

/**
 * @brief Analyze script for security issues
 * @param ctx Debug context
 * @param file File path being analyzed
 * @param content Script content to analyze
 */
static void debug_analyze_security(debug_context_t *ctx, const char *file,
                                   const char *content) {
    if (!ctx || !file || !content) {
        return;
    }

    int line_number = 1;
    const char *pos = content;

    // Look for security issues
    while (*pos) {
        if (*pos == '\n') {
            line_number++;
        }

        // Check for unquoted variables
        if (*pos == '$' && *(pos + 1) != '(' && *(pos + 1) != '{') {
            const char *var_start = pos + 1;
            const char *var_end = var_start;

            // Find end of variable name
            while (*var_end && (isalnum(*var_end) || *var_end == '_')) {
                var_end++;
            }

            if (var_end > var_start) {
                debug_add_analysis_issue(
                    ctx, file, line_number, "warning", "security",
                    "Unquoted variable",
                    "Quote variables to prevent word splitting");
            }
        }

        // Check for eval usage
        if (strncmp(pos, "eval ", 5) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "error",
                                     "security", "Use of eval",
                                     "Avoid eval for security reasons");
        }

        // Check for dangerous commands
        if (strncmp(pos, "rm -rf ", 7) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "warning",
                                     "security", "Dangerous rm command",
                                     "Be careful with recursive deletion");
        }

        if (strncmp(pos, "chmod 777 ", 10) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "warning",
                                     "security", "Overly permissive chmod",
                                     "Use minimal necessary permissions");
        }

        pos++;
    }
}

/**
 * @brief Analyze script for portability issues
 * @param ctx Debug context
 * @param file File path being analyzed
 * @param content Script content to analyze
 */
static void debug_analyze_portability(debug_context_t *ctx, const char *file,
                                      const char *content) {
    if (!ctx || !file || !content) {
        return;
    }

    int line_number = 1;
    const char *pos = content;

    // Look for portability issues
    while (*pos) {
        if (*pos == '\n') {
            line_number++;
        }

        // Check for bash-specific constructs
        if (strncmp(pos, "[[", 2) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "info",
                                     "portability", "Bash-specific test",
                                     "Use [ ] for POSIX compliance");
        }

        if (strncmp(pos, "function ", 9) == 0) {
            debug_add_analysis_issue(
                ctx, file, line_number, "info", "portability",
                "Bash-specific function syntax", "Use POSIX function syntax");
        }

        // Check for non-portable commands
        if (strncmp(pos, "echo -e ", 8) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "warning",
                                     "portability", "Non-portable echo option",
                                     "Use printf instead");
        }

        if (strncmp(pos, "source ", 7) == 0) {
            debug_add_analysis_issue(ctx, file, line_number, "info",
                                     "portability", "Bash-specific source",
                                     "Use . instead for POSIX compliance");
        }

        pos++;
    }
}

/**
 * @brief Display the analysis report
 * @param ctx Debug context containing analysis results
 */
void debug_show_analysis_report(debug_context_t *ctx) {
    if (!ctx) {
        return;
    }

    debug_print_header(ctx, "Script Analysis Report");

    if (ctx->issue_count == 0) {
        debug_printf(ctx, "No issues found - script looks good!\n");
        return;
    }

    // Count issues by severity
    int error_count = 0, warning_count = 0, info_count = 0;
    analysis_issue_t *issue = ctx->analysis_issues;
    while (issue) {
        if (strcmp(issue->severity, "error") == 0) {
            error_count++;
        } else if (strcmp(issue->severity, "warning") == 0) {
            warning_count++;
        } else if (strcmp(issue->severity, "info") == 0) {
            info_count++;
        }
        issue = issue->next;
    }

    debug_printf(ctx,
                 "Issues found: %d total (%d errors, %d warnings, %d info)\n\n",
                 ctx->issue_count, error_count, warning_count, info_count);

    // Show issues by category
    const char *categories[] = {"syntax", "security", "performance", "style",
                                "portability"};
    const char *category_names[] = {"Syntax", "Security", "Performance",
                                    "Style", "Portability"};

    for (int i = 0; i < 5; i++) {
        bool has_issues = false;
        issue = ctx->analysis_issues;

        // Check if we have issues in this category
        while (issue) {
            if (strcmp(issue->category, categories[i]) == 0) {
                has_issues = true;
                break;
            }
            issue = issue->next;
        }

        if (!has_issues) {
            continue;
        }

        debug_printf(ctx, "%s Issues:\n", category_names[i]);
        debug_printf(ctx, "%-8s %-4s %-60s\n", "Severity", "Line", "Message");
        debug_printf(ctx, "%-8s %-4s %-60s\n", "--------", "----", "-------");

        issue = ctx->analysis_issues;
        while (issue) {
            if (strcmp(issue->category, categories[i]) == 0) {
                debug_printf(ctx, "%-8s %-4d %s\n", issue->severity,
                             issue->line_number, issue->message);
                if (issue->suggestion) {
                    debug_printf(ctx, "         %-4s Suggestion: %s\n", "",
                                 issue->suggestion);
                }
            }
            issue = issue->next;
        }
        debug_printf(ctx, "\n");
    }

    // Summary and recommendations
    debug_printf(ctx, "Summary:\n");
    if (error_count > 0) {
        debug_printf(
            ctx, "  WARNING: %d syntax or critical errors need to be fixed\n",
            error_count);
    }
    if (warning_count > 0) {
        debug_printf(ctx, "  WARNING: %d warnings should be addressed\n",
                     warning_count);
    }
    if (info_count > 0) {
        debug_printf(ctx, "  INFO: %d informational items for improvement\n",
                     info_count);
    }

    debug_printf(ctx, "\nRecommendations:\n");
    debug_printf(ctx, "  - Fix all syntax errors before running the script\n");
    debug_printf(ctx,
                 "  - Address security warnings to prevent vulnerabilities\n");
    debug_printf(
        ctx, "  - Consider performance suggestions for better efficiency\n");
    debug_printf(ctx, "  - Follow style guidelines for maintainability\n");
    debug_printf(
        ctx,
        "  - Address portability issues for cross-platform compatibility\n");
}

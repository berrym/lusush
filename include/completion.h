/**
 * completion.h - Comprehensive Tab Completion System for lusush
 *
 * This module provides intelligent command-line completion using linenoise.
 *
 * Features:
 *
 * 1. Context-Aware Completion:
 *    - First word: Commands, builtins, aliases
 *    - Arguments: Files, directories
 *    - Variables: Environment and shell variables with $ prefix
 *
 * 2. Command Completion:
 *    - Built-in commands (echo, export, cd, etc.)
 *    - Aliases (ll, la, .., etc.)
 *    - Executable programs from PATH
 *
 * 3. File/Directory Completion:
 *    - Local files and directories
 *    - Absolute and relative paths
 *    - Automatic trailing slash for directories
 *    - Hidden files when explicitly requested (starting with .)
 *
 * 4. Variable Completion:
 *    - Environment variables ($HOME, $PATH, etc.)
 *    - Special shell variables ($?, $$, $0, $#, $1-$9)
 *    - Custom shell variables
 *
 * 5. Smart Suffix Addition:
 *    - Space after commands
 *    - Slash after directories
 *    - Context-appropriate suffixes
 *
 * 6. History Fallback:
 *    - Previous commands matching typed prefix
 *
 * Usage:
 *   The completion system is automatically initialized when the shell starts.
 *   Press TAB to trigger completion at any point in command input.
 */

#ifndef COMPLETION_H
#define COMPLETION_H

#include "linenoise_replacement.h"

// Completion callback for linenoise
void lusush_completion_callback(const char *buf, linenoiseCompletions *lc);

// Hints callback for linenoise
char *lusush_hints_callback(const char *buf, int *color, int *bold);
void lusush_free_hints_callback(void *hint);

// Individual completion functions
void complete_commands(const char *text, linenoiseCompletions *lc);
void complete_files(const char *text, linenoiseCompletions *lc);
void complete_variables(const char *text, linenoiseCompletions *lc);
void complete_builtins(const char *text, linenoiseCompletions *lc);
void complete_aliases(const char *text, linenoiseCompletions *lc);
void complete_history(const char *text, linenoiseCompletions *lc);

// Helper functions
char *get_completion_word(const char *buf, int *start_pos);
char *get_first_command(const char *buf);
int is_command_position(const char *buf, int pos);
void add_completion_with_suffix(linenoiseCompletions *lc,
                                const char *completion, const char *suffix);

// Hints system functions
char *generate_command_hint(const char *buf);
char *generate_file_hint(const char *buf);
char *generate_variable_hint(const char *buf);
char *generate_builtin_hint(const char *buf);
char *get_best_completion_match(const char *text);
int should_show_hints(const char *buf);

#endif /* COMPLETION_H */

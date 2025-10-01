/*
 * Lusush - A modern shell with GNU Readline integration
 * Copyright (C) 2021-2025  Michael Berry
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LUSUSH_H
#define LUSUSH_H

#include "node.h"
#include "version.h"

#include <fcntl.h>
#include <glob.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#define SHELL_NAME "lusush"

// Maximum line length
#ifdef MAXLINE
#undef MAXLINE
#endif
#define MAXLINE 4096

extern bool exit_flag;

// Special shell variables (exit_flag is declared separately)
extern int last_exit_status;
extern pid_t shell_pid;
extern char **shell_argv;
extern int shell_argc;
extern pid_t last_background_pid;
extern bool shell_argv_is_dynamic;

#define SOURCE_NAME get_shell_varp("0", SHELL_NAME)

#define MODE_WRITE (O_RDWR | O_CREAT | O_TRUNC)
#define MODE_APPEND (O_RDWR | O_CREAT | O_APPEND)
#define MODE_READ (O_RDONLY)

// Expansion context to track state during expansion
typedef struct expansion_context {
    bool in_double_quotes;
    bool in_single_quotes;
    bool in_var_assign;
    int var_assign_eq_count;
    bool no_field_split;
    bool no_pathname_expand;
    bool no_tilde_expand;
} exp_ctx_t;

// Expansion result type for better error handling
typedef enum {
    EXP_OK,
    EXP_ERROR,
    EXP_NO_EXPANSION,
    EXP_INVALID_VAR
} exp_result_t;

// Individual expansion function types
typedef struct {
    exp_result_t result;
    char *expanded;
    size_t len;
} expansion_t;

// String builder for efficient string manipulation
typedef struct string_builder {
    char *data;
    size_t len;
    size_t capacity;
} str_builder_t;

// parser functions
int parse_and_execute(const char *command);

// executor access functions
struct executor;
struct executor *get_global_executor(void);

// symbol table variable functions
char *get_shell_varp(char *, char *);
int get_shell_vari(char *, int);
ssize_t get_shell_varl(char *, int);
void set_shell_varp(char *, char *);
void set_shell_vari(char *, int);

// shell options
// Legacy shell options enum removed - now using config system

// POSIX shell options structure
typedef struct shell_options {
    // Command-line invocation modes
    bool command_mode;    // -c flag: execute command string
    char *command_string; // -c argument: command to execute
    bool stdin_mode;      // -s flag: read from stdin
    bool interactive;     // -i flag: force interactive mode
    bool login_shell;     // -l flag: login shell behavior

    // Shell behavior flags
    bool exit_on_error;   // -e flag: exit on command failure
    bool trace_execution; // -x flag: trace command execution
    bool syntax_check;    // -n flag: syntax check only
    bool unset_error;     // -u flag: error on unset variables
    bool verbose;         // -v flag: print input lines
    bool no_globbing;     // -f flag: disable pathname expansion
    bool hash_commands;   // -h flag: command hashing
    bool job_control;     // -m flag: enable job control
    bool allexport;       // -a flag: automatically export all variables
    bool noclobber;       // -C flag: prevent output redirection from overwriting files
    bool onecmd;          // -t flag: exit after one command
    bool notify;          // -b flag: asynchronous background job notification
    bool ignoreeof;       // ignoreeof: prevent exit on EOF (Ctrl+D)
    bool nolog;           // nolog: prevent function definitions from entering command history
    bool emacs_mode;      // emacs: enable emacs-style command line editing (default true)
    bool vi_mode;         // vi: enable vi-style command line editing
    bool posix_mode;      // posix: enable strict POSIX compliance mode
    bool pipefail_mode;   // pipefail: make pipelines fail if any command fails
    bool histexpand_mode; // histexpand: enable history expansion (!! !n !string)
    bool history_mode;    // history: enable command history recording
    bool interactive_comments_mode; // interactive-comments: enable # comments in interactive mode
    bool braceexpand_mode; // braceexpand: enable brace expansion {a,b,c}
} shell_options_t;

// Global shell options
extern shell_options_t shell_opts;

void init_shell_opts(void);
bool get_enhanced_completion(void);
bool get_no_word_expand(void);

// POSIX option management functions
void init_posix_options(void);
bool is_posix_option_set(char option);
bool should_exit_on_error(void);
bool should_trace_execution(void);
bool is_syntax_check_mode(void);
bool should_error_unset_vars(void);
bool is_verbose_mode(void);
bool is_globbing_disabled(void);
bool should_auto_export(void);
bool is_noclobber_enabled(void);
void print_command_trace(const char *command);
int builtin_set(char **args);

// struct to represent the words resulting from word expansion
typedef struct word {
    char *data;
    size_t len;
    struct word *next;
} word_t;

// word expansion functions
word_t *make_word(char *str);
void free_all_words(word_t *first);
bool is_name(const char *str);

char *substitute_str(char *s1, char *s2, size_t start, size_t end);
char *wordlist_to_str(word_t *word);

// Core expansion functions
expansion_t lusush_tilde_expand(const char *str, const exp_ctx_t *ctx);
expansion_t var_expand(const char *str, const exp_ctx_t *ctx);
expansion_t command_substitute_exp(const char *str, const exp_ctx_t *ctx);
expansion_t arithm_expand_exp(const char *str, const exp_ctx_t *ctx);

// Main word expansion pipeline
word_t *word_expand(const char *orig_word);
char *word_expand_to_str(const char *word);

// Field splitting and pathname expansion
word_t *field_split(const char *str);
word_t *pathnames_expand(word_t *words);
void remove_quotes(word_t *wordlist);

// Context management
exp_ctx_t *create_expansion_context(void);
void free_expansion_context(exp_ctx_t *ctx);
void reset_expansion_context(exp_ctx_t *ctx);

// String builder functions
str_builder_t *sb_create(size_t initial_capacity);
void sb_free(str_builder_t *sb);
bool sb_append(str_builder_t *sb, const char *str);
bool sb_append_char(str_builder_t *sb, char c);
bool sb_append_len(str_builder_t *sb, const char *str, size_t len);
char *sb_finalize(str_builder_t *sb);

char *pos_params_expand(char *tmp, bool in_double_quotes);
char *arithm_expand(const char *orig_expr);

// command execution functions
char *search_path(char *fn);
int do_exec_cmd(int argc, char **argv);
int do_basic_command(node_t *n);
int execute_pipeline_simple(char *line);
int execute_simple_pipeline(char **commands, int cmd_count);
int execute_single_command(char *command);
int execute_pipeline_commands(char ***cmd_args, int *cmd_argc, int cmd_count);
int setup_redirection(node_t *redir);
// int setup_redirections(node_t *cmd); // Conflicts with redirection.h
// int execute_node(node_t *node); // Conflicts with executor.c
int execute_new_parser_command(node_t *cmd);
int execute_new_parser_pipeline(node_t *pipe_node);
int execute_new_parser_control_structure(node_t *control_node);
int execute_new_parser_if(node_t *if_node);
int execute_new_parser_while(node_t *while_node);
int execute_new_parser_for(node_t *for_node);

#endif

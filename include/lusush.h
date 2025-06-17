#ifndef LUSUSH_H
#define LUSUSH_H

#include "node.h"
#include "scanner.h"

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
node_t *parse_command(token_t *tok);
node_t *parse_redirection(token_t *redir_tok, token_t *target_tok);
int parse_and_execute(source_t *);

// symbol table variable functions
char *get_shell_varp(char *, char *);
int get_shell_vari(char *, int);
ssize_t get_shell_varl(char *, int);
void set_shell_varp(char *, char *);
void set_shell_vari(char *, int);

// shell options
typedef enum {
    MULTILINE_EDIT,
    HISTORY_NO_DUPS,
    NO_WORD_EXPAND,
} BOOL_SHELL_OPTS;

void init_shell_opts(void);
void setopt(int argc, char **argv);

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
expansion_t tilde_expand(const char *str, const exp_ctx_t *ctx);
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
char *arithm_expand(char *orig_expr);

// pattern matching functions
bool has_glob_chars(char *p, size_t len);
size_t match_prefix(const char *pattern, char *str, bool longest);
size_t match_suffix(const char *pattern, char *str, bool longest);
char **get_filename_matches(const char *pattern, glob_t *matches);

// command execution functions
char *search_path(char *fn);
int do_exec_cmd(int argc, char **argv);
int do_basic_command(node_t *n);
int execute_pipeline_simple(char *line);
int execute_simple_pipeline(char **commands, int cmd_count);
int execute_single_command(char *command);
int execute_pipeline_commands(char ***cmd_args, int *cmd_argc, int cmd_count);
int setup_redirection(node_t *redir);
int setup_redirections(node_t *cmd);
int execute_node(node_t *node);

#endif

#ifndef LUSUSH_H
#define LUSUSH_H

#include "node.h"
#include "scanner.h"

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
extern bool parsing_alias;

#define SOURCE_NAME get_shell_varp("0", SHELL_NAME)

#define MODE_WRITE (O_RDWR | O_CREAT | O_TRUNC)
#define MODE_APPEND (ORDWR | O_CREAT | O_APPEND)
#define MODE_READ (O_RDONLY)

// parser functions
node_t *parse_command(token_t *tok);
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

char *substitute_str(char *s1, char *s2, size_t start, size_t end);
char *wordlist_to_str(word_t *word);

word_t *word_expand(char *orig_word);
char *word_expand_to_str(char *word);
char *tilde_expand(char *s);
char *command_substitute(char *orig_cmd);
char *var_expand(char *orig_var_name);
char *pos_params_expand(char *tmp, bool in_double_quotes);
word_t *pathnames_expand(word_t *words);
word_t *field_split(char *str);
void remove_quotes(word_t *wordlist);

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

#endif

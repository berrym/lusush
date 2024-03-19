#ifndef LUSUSH_H
#define LUSUSH_H

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
extern bool no_word_expand;

#define SOURCE_NAME get_shell_varp("0", SHELL_NAME)

#define MODE_WRITE (O_RDWR | O_CREAT | O_TRUNC)
#define MODE_APPEND (ORDWR | O_CREAT | O_APPEND)
#define MODE_READ (O_RDONLY)

char *get_shell_varp(char *, char *);
int get_shell_vari(char *, int);
ssize_t get_shell_varl(char *, int);
void set_shell_varp(char *, char *);
void set_shell_vari(char *, int);

extern int64_t optionsx;

#define OPTION_INTERACTIVE_COMMENTS 0x01l   /* (1 << 0 ) */
#define OPTION_LAST_PIPE 0x02l              /* (1 << 1 ) */
#define OPTION_NOCASE_MATCH 0x04l           /* (1 << 2 ) */
#define OPTION_NULL_GLOB 0x08l              /* (1 << 3 ) */
#define OPTION_FAIL_GLOB 0x10l              /* (1 << 4 ) */
#define OPTION_NOCASE_GLOB 0x20l            /* (1 << 5 ) */
#define OPTION_DOT_GLOB 0x40l               /* (1 << 6 ) */
#define OPTION_EXT_GLOB 0x80l               /* (1 << 7 ) */
#define OPTION_HUP_ON_EXIT 0x100l           /* (1 << 8 ) */
#define OPTION_EXEC_FAIL 0x200l             /* (1 << 9 ) */
#define OPTION_AUTO_CD 0x400l               /* (1 << 10) */
#define OPTION_CDABLE_VARS 0x800l           /* (1 << 11) */
#define OPTION_CHECK_HASH 0x1000l           /* (1 << 12) */
#define OPTION_CHECK_JOBS 0x2000l           /* (1 << 13) */
#define OPTION_CHECK_WINSIZE 0x4000l        /* (1 << 14) */
#define OPTION_CMD_HIST 0x8000l             /* (1 << 15) */
#define OPTION_LIT_HIST 0x10000l            /* (1 << 16) */
#define OPTION_COMPLETE_FULL_QUOTE 0x20000l /* (1 << 17) */
#define OPTION_EXPAND_ALIASES 0x40000l      /* (1 << 18) */
#define OPTION_FORCE_FIGNORE 0x80000l       /* (1 << 19) */
#define OPTION_GLOB_ASCII_RANGES 0x100000l  /* (1 << 20) */
#define OPTION_HIST_APPEND 0x200000l        /* (1 << 21) */
#define OPTION_HOST_COMPLETE 0x400000l      /* (1 << 22) */
#define OPTION_INHERIT_ERREXIT 0x800000l    /* (1 << 23) */
#define OPTION_LOCAL_VAR_INHERIT 0x1000000l /* (1 << 24) */
#define OPTION_LOCAL_VAR_UNSET 0x2000000l   /* (1 << 25) */
#define OPTION_LOGIN_SHELL 0x4000000l       /* (1 << 26) */
#define OPTION_MAIL_WARN 0x8000000l         /* (1 << 27) */
#define OPTION_PROMPT_VARS 0x10000000l      /* (1 << 28) */
#define OPTION_RESTRICTED_SHELL 0x20000000l /* (1 << 29) */
#define OPTION_SHIFT_VERBOSE 0x40000000l    /* (1 << 30) */
#define OPTION_SOURCE_PATH 0x80000000l      /* (1 << 31) */
#define OPTION_XPG_ECHO 0x100000000l        /* (1 << 32) */
#define OPTION_USER_COMPLETE 0x200000000l   /* (1 << 33) */
#define OPTION_CLEAR_SCREEN 0x400000000l    /* (1 << 34) */
#define OPTION_HIST_RE_EDIT 0x800000000l    /* (1 << 35) */
#define OPTION_HIST_VERIFY 0x1000000000l    /* (1 << 36) */
#define OPTION_LIST_JOBS_LONG                                                  \
    0x2000000000l                      /* (1 << 37) -- tcsh-like extension */
#define OPTION_LIST_JOBS 0x4000000000l /* (1 << 38) -- tcsh-like extension */
#define OPTION_PUSHD_TO_HOME                                                   \
    0x8000000000l                      /* (1 << 39) -- tcsh-like extension */
#define OPTION_DUNIQUE 0x10000000000l  /* (1 << 40) -- tcsh-like extension */
#define OPTION_DEXTRACT 0x20000000000l /* (1 << 41) -- tcsh-like extension */
#define OPTION_ADD_SUFFIX                                                      \
    0x40000000000l /* (1 << 42) -- tcsh-like extension                         \
                    */
#define OPTION_PRINT_EXIT_VALUE                                                \
    0x80000000000l /* (1 << 43) -- tcsh-like extension */
#define OPTION_RECOGNIZE_ONLY_EXE                                              \
    0x100000000000l /* (1 << 44) -- tcsh-like extension */
#define OPTION_SAVE_DIRS                                                       \
    0x200000000000l /* (1 << 45) -- tcsh-like extension                        \
                     */
#define OPTION_SAVE_HIST                                                       \
    0x400000000000l /* (1 << 46) -- tcsh-like extension                        \
                     */
#define OPTION_PROMPT_BANG                                                     \
    0x800000000000l /* (1 << 47) -- zsh-like extension                         \
                     */
#define OPTION_PROMPT_PERCENT                                                  \
    0x1000000000000l /* (1 << 48) -- zsh-like extension */
#define OPTION_CALLER_VERBOSE 0x2000000000000l /* (1 << 49) */

#define optionx_set(o) ((((optionsx) & (o)) == (o)) ? 1 : 0)

int set_optionx(int64_t, int);
int64_t optionx_index(char *);
int setx_builtin(int, char **);
void purge_xoptions(char, int);
void disable_extended_options(void);

/* for compatibility with bash */
int shopt_builtin(int, char **);

int set_optionx(int64_t, int);
int64_t optionx_index(char *);
int parse_and_execute(source_s *);

// struct to represent the words resulting from word expansion
struct word_s {
    char *data;
    size_t len;
    struct word_s *next;
};

// word expansion functions
struct word_s *make_word(char *str);
void free_all_words(struct word_s *first);

size_t find_closing_quote(char *data);
size_t find_closing_brace(char *data);
void delete_char_at(char *str, size_t index);
char *substitute_str(char *s1, char *s2, size_t start, size_t end);
char *wordlist_to_str(struct word_s *word);

struct word_s *word_expand(char *orig_word);
char *word_expand_to_str(char *word);
char *tilde_expand(char *s);
char *command_substitute(char *orig_cmd);
char *var_expand(char *orig_var_name);
char *pos_params_expand(char *tmp, bool in_double_quotes);
struct word_s *pathnames_expand(struct word_s *words);
struct word_s *field_split(char *str);
void remove_quotes(struct word_s *wordlist);

char *arithm_expand(char *orig_expr);

// some string manipulation functions
char *strchr_any(char *string, char *chars);
char *quote_val(char *val, bool add_quotes);
bool check_buffer_bounds(const size_t *count, size_t *len, char ***buf);
void free_buffer(size_t len, char **buf);

// pattern matching functions
bool has_glob_chars(char *p, size_t len);
size_t match_prefix(const char *pattern, char *str, bool longest);
size_t match_suffix(const char *pattern, char *str, bool longest);
char **get_filename_matches(const char *pattern, glob_t *matches);

#endif

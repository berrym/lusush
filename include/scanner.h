#include <sys/types.h>

#ifndef SCANNER_H
#define SCANNER_H

#define INIT_SRC_POS -2
#define ERRCHAR 0

struct source
{
    char *buf;
    size_t bufsize;
    size_t curline;
    size_t curchar;
    size_t curlinestart;
    ssize_t pos;
    ssize_t pos_old;
    size_t wstart;
};

enum token_type {
    TOKEN_EMPTY,
    TOKEN_UNKNOWN,
    TOKEN_COMMENT,
    TOKEN_EOF,
    TOKEN_WORD,
    TOKEN_ASSIGNMENT_WORD,
    TOKEN_NAME,
    TOKEN_NEWLINE,
    TOKEN_IO_NUMBER,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_PIPE,
    TOKEN_LESS,
    TOKEN_GREAT,
    TOKEN_SEMI,
    TOKEN_AND,
    TOKEN_INTEGER,
    TOKEN_KEYWORD_LBRACE,
    TOKEN_KEYWORD_RBRACE,
    TOKEN_KEYWORD_BANG,
    TOKEN_AND_IF,
    TOKEN_OR_IF,
    TOKEN_DSEMI,
    TOKEN_DLESS,
    TOKEN_DGREAT,
    TOKEN_LESSAND,
    TOKEN_GREATAND,
    TOKEN_LESSGREAT,
    TOKEN_DLESSDASH,
    TOKEN_CLOBBER,
    TOKEN_SEMI_AND,
    TOKEN_SEMI_OR,
    TOKEN_PIPE_AND,
    TOKEN_ANDGREAT,
    TOKEN_AND_GREAT_GREAT,
    TOKEN_SEMI_SEMI_AND,
    TOKEN_TRIPLELESS,
};

struct token {
    enum token_type type;
    size_t lineno;
    size_t charno;
    size_t linestart;
    struct source *src;
    size_t text_len;
    char *text;
};

extern struct token eof_token;

struct token *tokenize(struct source *);
void free_token(struct token *);
void set_token_type(struct token *);

char *get_token_description(enum token_type);
void set_token_type(struct token *);
struct token *tokenize(struct source *);
struct token *get_current_token(void);
struct token *get_previous_token(void);
struct token *dup_token(struct token *);
void set_current_token(struct token *);
void set_previous_token(struct token *);
void free_token(struct token *);
int is_token_of_type(struct token *, enum token_type);
int is_keyword(char *);
int is_separator_tok(enum token_type);
char next_char(struct source *);
char prev_char(struct source *);
void unget_char(struct source *);
char peek_char(struct source *);
void skip_whitespace(struct source *);
void free_tok_buf(void);

#endif

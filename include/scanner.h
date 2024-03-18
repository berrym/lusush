#ifndef SCANNER_H
#define SCANNER_H

#include <stddef.h>
#include <sys/types.h>

#define INIT_SRC_POS -2
#define ERRCHAR 0

typedef struct source_s {
    char *buf;
    size_t bufsize;
    size_t curline;
    size_t curchar;
    size_t curlinestart;
    ssize_t pos;
    ssize_t pos_old;
    size_t wstart;
} source_s;

typedef enum token_type_e {
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
} token_type_e;

typedef struct token_s {
    token_type_e type;
    size_t lineno;
    size_t charno;
    size_t linestart;
    source_s *src;
    size_t text_len;
    char *text;
} token_s;

extern token_s eof_token;

token_s *tokenize(source_s *);
void free_token(token_s *);
void set_token_type(token_s *);

char *get_token_description(token_type_e);
void set_token_type(token_s *);
token_s *tokenize(source_s *);
token_s *get_current_token(void);
token_s *get_previous_token(void);
token_s *dup_token(token_s *);
void set_current_token(token_s *);
void set_previous_token(token_s *);
void free_token(token_s *);
int is_token_of_type(token_s *, token_type_e);
int is_keyword(char *);
int is_separator_tok(token_type_e);
char next_char(source_s *);
char prev_char(source_s *);
void unget_char(source_s *);
char peek_char(source_s *);
void skip_whitespace(source_s *);
void free_tok_buf(void);

#endif

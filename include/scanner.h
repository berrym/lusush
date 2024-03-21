#ifndef SCANNER_H
#define SCANNER_H

#include <stdbool.h>
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
    // general type tokes
    TOKEN_EMPTY,
    TOKEN_UNKNOWN,
    TOKEN_COMMENT,
    TOKEN_EOF,
    TOKEN_WORD,
    TOKEN_ASSIGNMENT_WORD,
    TOKEN_NAME,
    TOKEN_NEWLINE,
    TOKEN_IO_NUMBER, // number preceding an I/O operator
    // POSIX Operators
    TOKEN_AND_IF,    // '&&'
    TOKEN_OR_IF,     // '||'
    TOKEN_DSEMI,     // ';;'
    TOKEN_DLESS,     // '<<'
    TOKEN_DGREAT,    // '>>'
    TOKEN_LESSAND,   // '<&'
    TOKEN_GREATAND,  // '>&'
    TOKEN_LESSGREAT, // '<>'
    TOKEN_DLESSDASH, // '<<-'
    TOKEN_CLOBBER,   // '>|'
    // POSIX Shell Keywords
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_THEN,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_FI,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_DONE,
    TOKEN_KEYWORD_CASE,
    TOKEN_KEYWORD_ESAC,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_UNTIL,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_LBRACE, // '{' (yes, that's a POSIX keyword)
    TOKEN_KEYWORD_RBRACE, // '}' (ditto)
    TOKEN_KEYWORD_BANG,   // '!' (ditto)
    TOKEN_KEYWORD_IN,
    // non-POSIX Shell Keywords and Operators
    TOKEN_KEYWORD_SELECT,   // the 'select' keyword
    TOKEN_KEYWORD_FUNCTION, // the 'function' keyword
    TOKEN_KEYWORD_TIME,     // the 'time' keyword
    TOKEN_KEYWORD_COPROC,   // the 'coproc' keyword
    TOKEN_SEMI_AND,         // ';&'
    TOKEN_SEMI_SEMI_AND,    // ';;&'
    TOKEN_SEMI_OR,          // ';|', equivalent in function to ';;&'
    TOKEN_PIPE_AND,         // '|&'
    TOKEN_TRIPLELESS,       // '<<<'
    TOKEN_ANDGREAT,         // '&>'
    TOKEN_AND_GREAT_GREAT,  // '&>>'
    // others
    TOKEN_LEFT_PAREN,  // '('
    TOKEN_RIGHT_PAREN, // ')'
    TOKEN_PIPE,        // '|'
    TOKEN_LESS,        // '<'
    TOKEN_GREAT,       // '>'
    TOKEN_SEMI,        // ';'
    TOKEN_AND,         // '&'
    TOKEN_INTEGER,
    // special case for ElIf-Else-Fi keywords, used by the parser
    TOKEN_KEYWORDS_ELIF_ELSE_FI,
    // special case for Esac/;; keywords, used by the parser in POSIX mode
    TOKEN_DSEMI_ESAC,
    // special case for Esac/;;/;&/;| keywords, used by the parser in non-POSIX
    // mode
    TOKEN_DSEMI_ESAC_SEMIAND_SEMIOR,
    // unknown keyword
    TOKEN_KEYWORD_NA,
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

token_s *tokenize(source_s *src);
void free_token(token_s *src);
void set_token_type(token_s *src);
void set_token_type(token_s *tok);
token_s *tokenize(source_s *src);
token_s *get_current_token(void);
token_s *get_previous_token(void);
token_s *dup_token(token_s *tok);
void set_current_token(token_s *tok);
void set_previous_token(token_s *tok);
void free_token(token_s *tok);
int is_token_of_type(token_s *tok, token_type_e type);
bool is_separator_tok(token_type_e type);
int is_keyword(char *str);
enum token_type_e get_keyword_toktype(int index);
char next_char(source_s *src);
char prev_char(source_s *src);
void unget_char(source_s *src);
char peek_char(source_s *src);
void skip_whitespace(source_s *src);
void free_tok_buf(void);

#endif

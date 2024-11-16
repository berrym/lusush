#include "../include/scanner.h"

#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/strings.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// the shell command language keywords
char *keywords[] = {
    // POSIX keywords
    "if",
    "then",
    "else",
    "elif",
    "fi",
    "do",
    "done",
    "case",
    "esac",
    "while",
    "until",
    "for",
    "{",
    "}",
    "!",
    "in",
    // non-POSIX keywords
    "select",
    "function",
    "time",
    "coproc",
};

// keyword count */
size_t keyword_count = sizeof(keywords) / sizeof(char *);

/* shell command language operators */
char *operators[] = {
    // standard operators
    "&&",
    "||",
    ";;",
    "<<",
    ">>",
    "<&",
    ">&",
    "<>",
    ">|",
    // zsh extension, equivalent to >|
    ">!",
};

static char *tok_buf = NULL;
static size_t tok_bufsize = 0;
static ssize_t tok_bufindex = -1;

token_t eof_token = {
    .type = TOKEN_EOF,
    .lineno = 0,
    .charno = 0,
    .text_len = 0,
};

static token_t *cur_tok = NULL;
static token_t *prev_tok = NULL;

bool is_seperator_tok(token_type_t type) {
    switch (type) {
    case TOKEN_LEFT_PAREN:
    case TOKEN_RIGHT_PAREN:
    case TOKEN_PIPE:
    case TOKEN_PIPE_AND:
    case TOKEN_AND:
    case TOKEN_NEWLINE:
    case TOKEN_SEMI:
    case TOKEN_SEMI_AND:
    case TOKEN_SEMI_OR:
    case TOKEN_SEMI_SEMI_AND:
    case TOKEN_DSEMI:
    case TOKEN_AND_IF:
    case TOKEN_OR_IF:
    case TOKEN_ANDGREAT:
    case TOKEN_GREATAND:
    case TOKEN_AND_GREAT_GREAT:
    case TOKEN_COMMENT:
    case TOKEN_EOF:
    case TOKEN_EMPTY:
        return true;
    default:
        return false;
    }
}

/*
 * Check if the given str is a shell keyword.
 *
 * Returns the index of str in the keywords array, which is 0 to
 * one less than keyword_count, or -1 if the str is not a keyword.
 */
int is_keyword(char *str) {
    // sanity check
    if (str == NULL || !*str) {
        return -1;
    }

    for (size_t i = 0; i < keyword_count; i++) {
        // check the string in the keywords array
        if (strcmp(keywords[i], str) == 0) {
            return i;
        }
    }

    // string is not a keyowrd
    return -1;
}

/*
 * Set the type field of the given token, according to the value of
 * the token's text field.
 */
void set_token_type(token_t *tok) {
    token_type_t t = TOKEN_EMPTY;

    if (tok->text_len == 1) { // one-char tokens
        switch (tok->text[0]) {
        case '(':
            t = TOKEN_LEFT_PAREN;
            break;
        case ')':
            t = TOKEN_RIGHT_PAREN;
            break;
        case '{':
            t = TOKEN_KEYWORD_LBRACE;
            break;
        case '}':
            t = TOKEN_KEYWORD_RBRACE;
            break;
        case '!':
            t = TOKEN_KEYWORD_BANG;
            break;
        case '|':
            t = TOKEN_PIPE;
            break;
        case '<':
            t = TOKEN_LESS;
            break;
        case '>':
            t = TOKEN_GREAT;
            break;
        case '&':
            t = TOKEN_AND;
            break;
        case '\n':
            t = TOKEN_NEWLINE;
            break;
        case ';':
            t = TOKEN_SEMI;
            break;
        default:
            if (isdigit(tok->text[0])) {
                t = TOKEN_INTEGER;
            } else {
                t = TOKEN_WORD;
            }
            break;
        }
    } else if (tok->text_len == 2) { // two-char tokens
        switch (tok->text[0]) {
        case '&':
            if (tok->text[1] == '>') { // &>
                t = TOKEN_ANDGREAT;
            } else { // &&
                t = TOKEN_AND_IF;
            }
            break;
        case '|':
            if (tok->text[1] == '&') { // |&
                t = TOKEN_PIPE_AND;
            } else { // ||
                t = TOKEN_OR_IF;
            }
            break;
        case ';':
            if (tok->text[1] == '&') { // ;&
                t = TOKEN_SEMI_AND;
            } else if (tok->text[1] == '|') { // ;|
                t = TOKEN_SEMI_OR;
            } else { // ;;
                t = TOKEN_DSEMI;
            }
            break;
        case '>':
            if (tok->text[1] == '>') { // >>
                t = TOKEN_DGREAT;
            } else if (tok->text[1] == '&') { // >&
                t = TOKEN_GREATAND;
            } else { // >|
                t = TOKEN_CLOBBER;
            }
            break;
        case '<':
            if (tok->text[1] == '<') { // <<
                t = TOKEN_DLESS;
            } else if (tok->text[1] == '&') { // <&
                t = TOKEN_LESSAND;
            } else { // <>
                t = TOKEN_LESSGREAT;
            }
            break;
        default:
            // two digit number
            if (isdigit(tok->text[0]) && isdigit(tok->text[1])) {
                t = TOKEN_INTEGER;
                break;
            }

            // check if its a two-letter keyword
            int index = -1;
            if ((index = is_keyword(tok->text)) >= 0) {
                t = get_keyword_toktype(index);
            } else if (isalpha(tok->text[0]) && tok->text[1] == '=') {
                // one-letter variable and '='
                t = TOKEN_ASSIGNMENT_WORD;
            } else {
                // just a normal word
                t = TOKEN_WORD;
            }
            break;
        }
    } else { // multi-char tokens
        if (tok->text[0] == '#') {
            // comment token
            t = TOKEN_COMMENT;
        } else if (isdigit(tok->text[0])) {
            // number token
            t = TOKEN_INTEGER;
            char *p = tok->text;

            do {
                // if it contains any non-digit chars, its a word, not a number
                if (*p < '0' || *p > '9') {
                    t = TOKEN_WORD;
                    break;
                }
            } while (*++p);
        } else if (isalpha(tok->text[0])) {
            // check if its a keyword
            int index = -1;

            if ((index = is_keyword(tok->text)) >= 0) {
                t = get_keyword_toktype(index);
            } else {
                // if it contains =, check if its an assignment word
                if (strchr(tok->text, '=')) {
                    t = TOKEN_ASSIGNMENT_WORD;
                    char *p = tok->text;

                    // if assignment word, chars before the '=' must be
                    // alphanumeric or '_', as this refers to the variable name
                    // to which we're assigning a value.
                    while (*++p != '=') {
                        if (!isalnum(*p) && *p != '_') {
                            // this is included to support bash's extended
                            // assignment by using +=
                            if (*p == '+' && p[1] == '=') {
                                continue;
                            }

                            // non-alphanumeric or '_' chars before '=' means
                            // its not an assignment word
                            t = TOKEN_WORD;
                            break;
                        }
                    }
                } else {
                    t = TOKEN_WORD;
                }
            }
        } else if (strcmp(tok->text, "<<<") == 0) {
            t = TOKEN_TRIPLELESS;
        } else if (strcmp(tok->text, ";;&") == 0) {
            t = TOKEN_SEMI_SEMI_AND;
        } else if (strcmp(tok->text, "&>>") == 0) {
            t = TOKEN_AND_GREAT_GREAT;
        } else {
            t = TOKEN_WORD;
        }
    }

    // if we couldn't assign a type to this token, give it a TOKEN_UNKNOWN type
    if (t == TOKEN_EMPTY) {
        t = TOKEN_UNKNOWN;
    }

    tok->type = t;
}

/*
 * Return the token type that describes one of the shell's keywords.
 * The keywords are stored in an array (scanner.c) and the
 * index field gives the index of an item in the array. The value we
 * return is the token type describing the indexed keyword. If the
 * index is out of the keywords array bounds, we return TOKEN_KEYWORD_NA.
 */
token_type_t get_keyword_toktype(int index) {
    switch (index) {
    case 0:
        return TOKEN_KEYWORD_IF;
        break;
    case 1:
        return TOKEN_KEYWORD_THEN;
        break;
    case 2:
        return TOKEN_KEYWORD_ELSE;
        break;
    case 3:
        return TOKEN_KEYWORD_ELIF;
        break;
    case 4:
        return TOKEN_KEYWORD_FI;
        break;
    case 5:
        return TOKEN_KEYWORD_DO;
        break;
    case 6:
        return TOKEN_KEYWORD_DONE;
        break;
    case 7:
        return TOKEN_KEYWORD_CASE;
        break;
    case 8:
        return TOKEN_KEYWORD_ESAC;
        break;
    case 9:
        return TOKEN_KEYWORD_WHILE;
        break;
    case 10:
        return TOKEN_KEYWORD_UNTIL;
        break;
    case 11:
        return TOKEN_KEYWORD_FOR;
        break;
    case 12:
        return TOKEN_KEYWORD_LBRACE;
        break;
    case 13:
        return TOKEN_KEYWORD_RBRACE;
        break;
    case 14:
        return TOKEN_KEYWORD_BANG;
        break;
    case 15:
        return TOKEN_KEYWORD_IN;
        break;
    case 16:
        return TOKEN_KEYWORD_SELECT;
        break;
    case 17:
        return TOKEN_KEYWORD_FUNCTION;
        break;
    case 18:
        return TOKEN_KEYWORD_TIME;
        break;
    case 19:
        return TOKEN_KEYWORD_COPROC;
        break;
    }
    return TOKEN_KEYWORD_NA;
}

token_t *dup_token(token_t *tok) {
    token_t *new_tok = NULL;

    if (tok == NULL) {
        return NULL;
    }

    new_tok = calloc(1, sizeof(token_t));
    if (new_tok == NULL) {
        error_return("error: `dup_token`");
        return NULL;
    }
    memcpy(new_tok, tok, sizeof(token_t));

    if (tok->text) {
        tok->text_len = strlen(tok->text);
        new_tok->text = get_alloced_str(tok->text);
    }
    new_tok->text_len = tok->text_len;

    return new_tok;
}

void unget_char(source_t *src) {
    if (!src->pos) {
        return;
    }

    src->pos--;
}

char next_char(source_t *src) {
    if (src->pos == INIT_SRC_POS) {
        src->pos = -1;
    }

    if ((size_t)++src->pos >= src->bufsize) {
        src->pos = src->bufsize;
        return EOF;
    }

    return src->buf[src->pos];
}

char peek_char(source_t *src) {
    ssize_t pos = src->pos;

    if (pos == INIT_SRC_POS) {
        pos++;
    }

    pos++;

    if ((size_t)pos >= src->bufsize) {
        return EOF;
    }

    return src->buf[pos];
}

void skip_whitespace(source_t *src) {
    char c;

    while (((c = peek_char(src)) != EOF) && isspace((int)c)) {
        next_char(src);
    }
}

void add_to_buf(char c) {
    char *tmp = NULL;

    if (tok_buf == NULL) {
        return;
    }

    tok_buf[tok_bufindex++] = c;
    if ((size_t)tok_bufindex >= tok_bufsize) {
        tmp = realloc(tok_buf, tok_bufsize * 2);
        if (tmp == NULL) {
            error_return("error: `add_to_buf`");
            return;
        };
        tok_buf = tmp;
        tok_bufsize *= 2;
    }
}

token_t *create_token(char *s) {
    token_t *tok = NULL;
    char *buf = NULL, *tmp = NULL;

    tok = calloc(1, sizeof(token_t));
    if (tok == NULL) {
        error_return("error: `create_token`");
        return NULL;
    }

    tmp = get_alloced_str(s); // strdup(s);
    if (tmp == NULL) {
        free_token(tok);
        error_return("error: `create token`");
        return NULL;
    }

    buf = str_strip_whitespace(tmp);

    tok->text_len = strlen(buf);
    tok->text = get_alloced_str(buf); // strdup(buf);

    if (tok->text == NULL) {
        if (tok) {
            free(tok);
        }
        error_return("error: `create_token`");
        return NULL;
    }

    free_alloced_str(tmp);

    return tok;
}

void free_token(token_t *tok) {
    // don't attempt to free the EOF token
    if (tok == NULL || tok == &eof_token) {
        return;
    }

    free_alloced_str(tok->text);
    free(tok);

    // update the current token struct pointer
    if (cur_tok == tok) {
        cur_tok = NULL;
    }

    // update the previous token struct pointer
    if (prev_tok == tok) {
        prev_tok = NULL;
    }
}

token_t *tokenize(source_t *src) {
    bool loop = true;

    if (src == NULL || src->buf == NULL || !src->bufsize) {
        return &eof_token;
    }

    if (tok_buf == NULL) {
        tok_bufsize = MAXLINE + 1;
        tok_buf = alloc_str(tok_bufsize, false);
        if (tok_buf == NULL) {
            return &eof_token;
        }
    }

    tok_bufindex = 0;
    tok_buf[0] = '\0';

    if (cur_tok) {
        if (prev_tok) {
            free_token(prev_tok);
        }
        prev_tok = cur_tok;
        cur_tok = NULL;
    }

    ssize_t linestart, line, chr;

    /* init position indexes */
    src->pos_old = src->pos + 1;
    if (src->pos < 0) {
        linestart = 0;
        line = 1;
        chr = 1;
    } else {
        linestart = src->curlinestart;
        line = src->curline;
        chr = src->curchar;
    }

    char nc2;
    size_t i;

    char nc = next_char(src);
    if (nc == ERRCHAR || nc == EOF) {
        eof_token.lineno = src->curline;
        eof_token.charno = src->curchar;
        eof_token.linestart = src->curlinestart;
        eof_token.src = src;
        cur_tok = &eof_token;
        return &eof_token;
    }

    do {
        switch (nc) {
        case '"':
        case '\'':
        case '`':
            add_to_buf(nc);
            i = find_closing_quote(src->buf + src->pos);
            if (!i) {
                src->pos = src->bufsize;
                error_message("error: `tokenize`: missing closing quote '%c'",
                              nc);
                return &eof_token;
            }
            while (i--) {
                add_to_buf(next_char(src));
            }
            break;
        case '\\':
            nc2 = next_char(src);
            if (nc2 == '\n') {
                break;
            }
            add_to_buf(nc);
            if (nc2 > 0) {
                add_to_buf(nc2);
            }
            break;
        case '$':
            add_to_buf(nc);
            nc = peek_char(src);
            if (nc == '{' || nc == '(') {
                i = find_closing_brace(src->buf + src->pos + 1);
                if (!i) {
                    src->pos = src->bufsize;
                    error_message(
                        "error: `tokenize`: missing closing brace '%c'", nc);
                    return &eof_token;
                }
                while (i--) {
                    add_to_buf(next_char(src));
                }
            } else if (isalnum(nc)) {
                add_to_buf(next_char(src));
            } else {
                switch (nc) {
                case '*':
                case '@':
                case '#':
                case '!':
                case '?':
                case '$':
                    add_to_buf(next_char(src));
                default:;
                }
            }
            break;
        case '>':
        case '<':
        case '|':
            // if an '>', '<', or '|' operator delimits the current token,
            // delimit it
            if (tok_bufindex > 0) {
                unget_char(src);
                loop = false;
                break;
            }

            // add the operator to buffer
            add_to_buf(nc);
            loop = false;
            break;
        case '&':
        case ';':
            // if an '&' or ';' operator delimits the current token, delimit it
            if (tok_bufindex > 0) {
                unget_char(src);
                loop = false;
                break;
            }

            // add the operator to buffer
            add_to_buf(nc);
            loop = false;
            break;
        case ' ':
        case '\t':
            if (tok_bufindex > 0) {
                loop = false;
            }
            break;
        case '\n':
            if (tok_bufindex > 0) {
                unget_char(src);
            } else {
                add_to_buf(nc);
            }
            loop = false;
            break;
        default:
            add_to_buf(nc);
            break;
        }

        if (!loop) {
            break;
        }
    } while ((nc = next_char(src)) != EOF);

    eof_token.lineno = src->curline;
    eof_token.charno = src->curchar;
    eof_token.linestart = src->curlinestart;
    eof_token.src = src;

    if (tok_bufindex == 0) {
        return &eof_token;
    }

    if ((size_t)tok_bufindex >= tok_bufsize) {
        tok_bufindex--;
    }

    tok_buf[tok_bufindex] = '\0';

    token_t *tok = create_token(tok_buf);
    if (tok == NULL) {
        error_message("error: `tokenize`: failed to create new token");
        return &eof_token;
    }

    set_token_type(tok);

    tok->lineno = line;
    tok->charno = chr;
    tok->src = src;
    tok->linestart = linestart;

    cur_tok = tok;

    return tok;
}

token_t *get_current_token(void) { return cur_tok ? cur_tok : &eof_token; }

token_t *get_previous_token(void) { return prev_tok; }

void set_current_token(token_t *tok) { cur_tok = tok; }

void set_previous_token(token_t *tok) { prev_tok = tok; }

void free_tok_buf(void) { free_alloced_str(tok_buf); }

#include "../include/scanner.h"
#include "../include/errors.h"
#include "../include/lusush.h"
#include "../include/strings.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *tok_buf = NULL;
static size_t tok_bufsize = 0;
static ssize_t tok_bufindex = -1;

token_s eof_token = {
    .type = TOKEN_EOF,
    .lineno = 0,
    .charno = 0,
    .text_len = 0,
};

static token_s *cur_tok = NULL;
static token_s *prev_tok = NULL;

bool is_seperator_tok(token_type_e type) {
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

token_s *dup_token(token_s *tok) {
    token_s *new_tok = NULL;

    if (tok == NULL)
        return NULL;

    new_tok = calloc(1, sizeof(token_s));
    if (new_tok == NULL) {
        error_return("dup_token");
        return NULL;
    }
    memcpy(new_tok, tok, sizeof(token_s));

    if (tok->text) {
        tok->text_len = strlen(tok->text);
        new_tok->text = get_alloced_str(tok->text);
    }
    new_tok->text_len = tok->text_len;

    return new_tok;
}

void unget_char(source_s *src) {
    if (!src->pos)
        return;

    src->pos--;
}

char next_char(source_s *src) {
    if (src->pos == INIT_SRC_POS)
        src->pos = -1;

    if (++src->pos >= src->bufsize) {
        src->pos = src->bufsize;
        return EOF;
    }

    return src->buf[src->pos];
}

char peek_char(source_s *src) {
    ssize_t pos = src->pos;

    if (pos == INIT_SRC_POS)
        pos++;

    pos++;

    if (pos >= src->bufsize)
        return EOF;

    return src->buf[pos];
}

void skip_whitespace(source_s *src) {
    char c;

    while (((c = peek_char(src)) != EOF) && isspace((int)c))
        next_char(src);
}

void add_to_buf(char c) {
    char *tmp = NULL;

    tok_buf[tok_bufindex++] = c;
    if (tok_bufindex > tok_bufsize) {
        tmp = realloc(tok_buf, tok_bufsize * 2);
        if (tmp == NULL) {
            error_return("add_to_buf");
            return;
        };
        tok_buf = tmp;
        tok_bufsize *= 2;
    }
}

token_s *create_token(char *s) {
    token_s *tok = NULL;
    char *buf = NULL, *tmp = NULL;

    tok = calloc(1, sizeof(token_s));
    if (tok == NULL) {
        error_return("create_token");
        return NULL;
    }

    tmp = strdup(s);
    if (tmp == NULL) {
        error_return("create token");
        return NULL;
    }

    buf = str_strip_whitespace(tmp);

    tok->text_len = strlen(buf);
    tok->text = strdup(buf);
    ;
    if (tok->text == NULL) {
        error_return("create_token");
        return NULL;
    }

    free_str(tmp);
    buf = NULL;

    return tok;
}

void free_token(token_s *tok) {
    if (tok->text)
        free_str(tok->text);
    free(tok);
    tok = NULL;
}

token_s *tokenize(source_s *src) {
    bool loop = true;

    if (src == NULL || src->buf == NULL || !src->bufsize)
        return &eof_token;

    if (tok_buf == NULL) {
        tok_bufsize = MAXLINE;
        tok_buf = alloc_str(tok_bufsize, false);
        if (tok_buf == NULL)
            return &eof_token;
    }

    tok_bufindex = 0;
    tok_buf[0] = '\0';

    char nc = next_char(src);
    char nc2;
    size_t i;

    if (nc == ERRCHAR || nc == EOF)
        return &eof_token;

    do {
        switch (nc) {
        case '"':
        case '\'':
        case '`':
            add_to_buf(nc);
            i = find_closing_quote(src->buf + src->pos);
            if (!i) {
                src->pos = src->bufsize;
                fprintf(stderr, "error: missing closing quote '%c'\n", nc);
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
                    fprintf(stderr, "error: missing closing brace '%c'\n", nc);
                    return &eof_token;
                }
                while (i--) {
                    add_to_buf(next_char(src));
                }
            } else if (isalnum(nc) || nc == '*' || nc == '@' || nc == '#' ||
                       nc == '!' || nc == '?' || nc == '$') {
                add_to_buf(next_char(src));
            }
            break;
        case ' ':
        case '\t':
            if (tok_bufindex > 0)
                loop = false;
            break;
        case '\n':
            if (tok_bufindex > 0)
                unget_char(src);
            else
                add_to_buf(nc);
            loop = false;
            break;
        default:
            add_to_buf(nc);
            break;
        }

        if (!loop)
            break;
    } while ((nc = next_char(src)) != EOF);

    if (tok_bufindex == 0)
        return &eof_token;

    if (tok_bufindex >= tok_bufsize)
        tok_bufindex--;

    tok_buf[tok_bufindex] = '\0';

    token_s *tok = create_token(tok_buf);
    if (tok == NULL) {
        error_message("tokenize: failed to create new token");
        return &eof_token;
    }
    tok->src = src;

    return tok;
}

token_s *get_current_token(void) { return cur_tok ?: &eof_token; }

token_s *get_previous_token(void) { return prev_tok; }

void set_current_token(token_s *tok) { cur_tok = tok; }

void set_previous_token(token_s *tok) { prev_tok = tok; }

void free_tok_buf(void) {
    if (tok_buf)
        free_str(tok_buf);
}

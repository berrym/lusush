#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include "errors.h"
#include "lusush.h"
#include "scanner.h"

static char *tok_buf = NULL;
static size_t tok_bufsize = 0;
static ssize_t tok_bufindex = -1;

struct token eof_token = {
    .type = TOKEN_EOF,
    .lineno = 0,
    .charno = 0,
    .text_len = 0,
};

static struct token *cur_tok  = NULL;
static struct token *prev_tok = NULL;

bool is_seperator_tok(enum token_type type)
{
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

struct token *dup_token(struct token *token)
{
    if (!token)
        return NULL;

    struct token *token2 = NULL;
    if ((token2 = calloc(1, sizeof(struct token))) == NULL) {
        error_syscall("dup_token");
        return NULL;
    }
    memcpy(token2, token, sizeof(struct token));

    if (token->text) {
        token->text_len = strnlen(token->text, strnlen(token->text, MAXLINE));
        token2->text = get_malloced_str(token->text);
    }
    token2->text_len = token->text_len;

    return token2;
}

void unget_char(struct source *src)
{
    if (!src->pos)
        return;

    src->pos--;
}

char next_char(struct source *src)
{
    if (src->pos == INIT_SRC_POS)
        src->pos = -1;

    if (++src->pos >= src->bufsize) {
        src->pos = src->bufsize;
        return EOF;
    }

    return src->buf[src->pos];
}

char peek_char(struct source *src)
{
    ssize_t pos = src->pos;

    if (pos == INIT_SRC_POS)
        pos++;

    pos++;

    if (pos >= src->bufsize)
        return EOF;

    return src->buf[pos];
}

void skip_whitespace(struct source *src)
{
    char c;

    while (((c = peek_char(src)) != EOF) && isspace((int)c))
        next_char(src);
}

void add_to_buf(char c)
{
    tok_buf[tok_bufindex++] = c;
    if (tok_bufindex > tok_bufsize) {
        char *tmp = NULL;
        if ((tmp = realloc(tok_buf, tok_bufsize * 2)) == NULL) {
            error_syscall("add_to_buf: realloc");
            return;
        }
        tok_buf = tmp;
        tok_bufsize *= 2;
    }
}

struct token *create_token(char *s)
{
    struct token *tok = NULL;
    char *buf = NULL;

    if ((tok = calloc(1, sizeof(struct token))) == NULL) {
        error_syscall("create_token: calloc");
        return NULL;
    }

    tok->text_len = strnlen(s, MAXLINE);

    if ((buf = calloc(strnlen(s, MAXLINE) + 1, sizeof(char))) == NULL) {
        error_syscall("create_token: calloc");
        return NULL;
    }

    strncpy(buf, s, strnlen(s, MAXLINE));
    tok->text = buf;

    return tok;
}

void free_token(struct token *tok)
{
    if (tok->text)
        free(tok->text);
    free(tok);
    tok = NULL;
}

struct token *tokenize(struct source *src)
{
    bool loop = true;

    if (!src || !src->buf || !src->bufsize)
        return &eof_token;

    if (!tok_buf) {
        tok_bufsize = MAXLINE + 1;
        if ((tok_buf = calloc(MAXLINE + 1, sizeof(char))) == NULL) {
            error_syscall("tokenize: calloc");
            return &eof_token;
        }
    }

    tok_bufindex = 0;
    tok_buf[0] = '\0';

    char nc = next_char(src);

    if (nc == ERRCHAR || nc == EOF)
        return &eof_token;

    do {
        switch (nc) {
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

    struct token *tok = create_token(tok_buf);
    if (!tok) {
        fprintf(stderr, "error: failed to alloc buffer: %s\n",
                strerror(errno));
        return &eof_token;
    }
    tok->src = src;

    return tok;
}

struct token *get_current_token(void)
{
    return cur_tok ? : &eof_token;
}


struct token *get_previous_token(void)
{
    return prev_tok;
}

void set_current_token(struct token *tok)
{
    cur_tok = tok;
}

void set_previous_token(struct token *tok)
{
    prev_tok = tok;
}

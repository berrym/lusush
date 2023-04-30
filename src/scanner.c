#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include "errors.h"
#include "lusush.h"
#include "scanner.h"
#include "strings.h"

static char *tok_buf = NULL;
static size_t tok_bufsize = 0;
static ssize_t tok_bufindex = -1;

token_s eof_token = {
    .type = TOKEN_EOF,
    .lineno = 0,
    .charno = 0,
    .text_len = 0,
};

static token_s *cur_tok  = NULL;
static token_s *prev_tok = NULL;

bool is_seperator_tok(token_type_e type)
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

token_s *dup_token(token_s *tok)
{
    token_s *new_tok = NULL;

    if (!tok)
        return NULL;

    new_tok = calloc(1, sizeof(token_s));
    if (!new_tok) {
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

void unget_char(source_s *src)
{
    if (!src->pos)
        return;

    src->pos--;
}

char next_char(source_s *src)
{
    if (src->pos == INIT_SRC_POS)
        src->pos = -1;

    if (++src->pos >= src->bufsize) {
        src->pos = src->bufsize;
        return EOF;
    }

    return src->buf[src->pos];
}

char peek_char(source_s *src)
{
    ssize_t pos = src->pos;

    if (pos == INIT_SRC_POS)
        pos++;

    pos++;

    if (pos >= src->bufsize)
        return EOF;

    return src->buf[pos];
}

void skip_whitespace(source_s *src)
{
    char c;

    while (((c = peek_char(src)) != EOF) && isspace((int)c))
        next_char(src);
}

void add_to_buf(char c)
{
    char *tmp = NULL;

    tok_buf[tok_bufindex++] = c;
    if (tok_bufindex > tok_bufsize) {
        tmp = realloc(tok_buf, tok_bufsize * 2);
        if (!tmp) {
            error_return("add_to_buf");
            return;
        }
        tok_buf = tmp;
        tok_bufsize *= 2;
    }
}

token_s *create_token(char *s)
{
    token_s *tok = NULL;
    char *buf = NULL, *tmp = NULL;

    tok = calloc(1, sizeof(token_s));
    if (!tok) {
        error_return("create_token");
        return NULL;
    }

    tmp = strdup(s);
    if (!tmp) {
        error_return("create token");
        return NULL;
    }

    buf = str_strip_whitespace(tmp);

    tok->text_len = strlen(buf);
    tok->text = strdup(buf);
    if (!tok->text) {
        error_return("create_token");
        return NULL;
    }

    free_str(tmp);
    buf = NULL;

    return tok;
}

void free_token(token_s *tok)
{
    if (tok->text)
        free_str(tok->text);
    free(tok);
    tok = NULL;
}

token_s *tokenize(source_s *src)
{
    bool loop = true;

    if (!src || !src->buf || !src->bufsize)
        return &eof_token;

    if (!tok_buf) {
        tok_bufsize = MAXLINE;
        if (!(tok_buf = alloc_str(tok_bufsize, false)))
            return &eof_token;
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

    token_s *tok = create_token(tok_buf);
    if (!tok) {
        error_message("tokenize: failed to create new token");
        return &eof_token;
    }
    tok->src = src;

    return tok;
}

token_s *get_current_token(void)
{
    return cur_tok ? : &eof_token;
}


token_s *get_previous_token(void)
{
    return prev_tok;
}

void set_current_token(token_s *tok)
{
    cur_tok = tok;
}

void set_previous_token(token_s *tok)
{
    prev_tok = tok;
}

void free_tok_buf(void) {
    if (tok_buf)
        free_str(tok_buf);
}

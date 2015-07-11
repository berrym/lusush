/**
 * parse.c - command parser
 *
 * Copyright (c) 2009-2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lusush.h"
#include "parse.h"

#define PARSER_ERROR_ABORT -1
#define PARSER_ERROR_BREAK 0
#define PARSER_CONTINUE_ON 1

// character classifications
enum {
    IS_MAGIC,                   /* magic character */
    IS_WHSPC,                   /* whitespace */
    IS_NCHAR                    /* normal character */
};

// loop and counter variables
static unsigned int i = 0;
static unsigned int j = 0;
static unsigned int wpos = 0;
static unsigned int cpos = 0;
static unsigned int qcnt = 0;

// command flags effecting parser behavior
static bool iredir = false;
static bool oredir = false;
static bool readreg = false;
static bool inquote = false;

// input buffers
static char *line = NULL;
static struct command *cmd = NULL;

/**
 * char_type:
 *      Identify the classification of a character, one of (magic,
 *      whitespace,  or normal), returning it's enumerated value.
 */
static int char_type(char c)
{
    static int chtype = 0;

    switch (c) {
    case '#':
    case '&':
    case '<':
    case '>':
    case '"':
    case '~':
        chtype = IS_MAGIC;
        break;
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
    case '\v':
        chtype = IS_WHSPC;
        break;
    default:
        chtype = IS_NCHAR;
        break;
    }

    return chtype;
}

/**
 * do_pound:
 *      Process the pound (comment) character.
 */
static int do_pound(void)
{
    if (!inquote) {
        if (iredir)
            cmd->ifname[cpos] = '\0';
        else if (oredir)
            cmd->ofname[cpos] = '\0';
        else
            cmd->argv[wpos][cpos] = '\0';

        line[i] = '\0';

        return PARSER_CONTINUE_ON;
    }

    if (iredir || oredir) {
        fprintf(stderr, "lusush: parse error near character '#'\n");
        return PARSER_ERROR_BREAK;
    }

    cmd->argv[wpos][cpos] = '#';
    cpos++;

    return PARSER_CONTINUE_ON;
}

/**
 * do_ampersand:
 *      Process the ampersand character which signals that a command
 *      should be executed as a background process.
 */
static int do_ampersand(void)
{
    if (!inquote) {
        if (line[i + 1] && line[i + 1] == '&') {
            fprintf(stderr, "lusush: parse error near '&&': " \
                    "invalid operator.\n");
            return PARSER_ERROR_BREAK;
        }

        cmd->background = true;

        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = '\0';

        if (wpos)
            wpos--;

        return PARSER_CONTINUE_ON;
    }

    if (iredir || oredir) {
        fprintf(stderr, "lusush: parse error near '&'\n");
        return PARSER_ERROR_BREAK;
    }

    cmd->argv[wpos][cpos] = '&';
    cpos++;

    return PARSER_CONTINUE_ON;
}

/**
 * do_lessthan:
 *      Process the character '<' which denotes input redirection. 
 */
static int do_lessthan(void)
{
    if (!inquote) {
        iredir = cmd->iredir = true;

        if (line[i + 1] && line[i + 1] == '<') {
            fprintf(stderr, "lusush: parse error near '<<': " \
                    "invalid operator\n");
            return PARSER_ERROR_BREAK;
        }

        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = '\0';
    }
    else {
        cmd->argv[wpos][cpos] = '<';
        cpos++;
    }

    return PARSER_CONTINUE_ON;
}

/**
 * do_greaterthan():
 *      Process the '>' character which denotes output redirection.
 */
static int do_greaterthan(void)
{
    if (!inquote) {
        oredir = cmd->oredir = true;

        if (line[i + 1] && line[i + 1] == '>') {
            cmd->oredir_append = true;
            i++;
        }

        if (cmd->oredir_append) {
            if (line[i + 1] && line[i + 1] == '>') {
                fprintf(stderr, "lusush: parse error near '>>>': "  \
                        "invalid operator\n");
                return PARSER_ERROR_BREAK;
            }
        }

        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = '\0';
    }
    else {
        cmd->argv[wpos][cpos] = '>';
        cpos++;
    }

    return PARSER_CONTINUE_ON;
}

/**
 * do_doublequote:
 *      Process the double quote character that delimits strings.
 */
static int do_doublequote(void)
{
    if (inquote)
        inquote = false;
    else
        inquote = true;

    return PARSER_CONTINUE_ON;
}

/**
 * do_tilde:
 *      Process the tilde character which expands to user's home directory.
 */
static int do_tilde(void)
{
    char *home = NULL;

    if (!(home = getenv("HOME"))) {
        cmd->argv[wpos][cpos] = '~';
        cpos++;
    }
    else {
        strncat(cmd->argv[wpos], home, strlen(home));
        cpos += strlen(home);
    }

    home = NULL;

    return PARSER_CONTINUE_ON;
}

/**
 * do_magic:
 *      Process characters with special syntatic significance.
 */
static int do_magic(char c)
{
    int ret;

    switch (c) {
    case '#':
        ret = do_pound();
        break;
    case '&':
        ret = do_ampersand();
        break;
    case '<':
        ret = do_lessthan();
        break;
    case '>':
        ret = do_greaterthan();
        break;
    case '"':
        ret = do_doublequote();
        break;
    case '~':
        ret = do_tilde();
    default:
        break;
    }

    switch (ret) {
    case PARSER_ERROR_ABORT:
    case PARSER_ERROR_BREAK:
        break;
    default:
        readreg = false;
        cmd->argv[wpos] = NULL;
        cmd->argc = wpos;
        break;
    }

    return ret;
}

/**
 * do_whspc:
 *      Process whitespace.
 */
static int do_whspc(char c)
{
    if (inquote && !iredir && !oredir) {
        cmd->argv[wpos][cpos] = c;
        cpos++;

        return PARSER_CONTINUE_ON;
    }

    while (isspace((int)c)) {
        i++;
        c = line[i];
    }
    i--;

    if (!wpos && !readreg)
        return PARSER_ERROR_BREAK;

    if (iredir) {
        cmd->ifname[cpos] = '\0';
        iredir = false;
    }
    else if (oredir) {
        cmd->ofname[cpos] = '\0';
        oredir = false;
    }
    else {
        cmd->argv[wpos][cpos] = '\0';
    }
    wpos++;
    cpos = 0;

    if ((cmd->argv[wpos] = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: parse.c: do_whspc: calloc");

        for (j = wpos - 1; ; j--) {
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }

        return PARSER_ERROR_ABORT;
    }

    cmd->argv[wpos][cpos] = '\0';
    cmd->argc = wpos + 1;

    return PARSER_CONTINUE_ON;
}

/**
 * do_nchar:
 *      Process normal character.
 */
static int do_nchar(char c)
{
    if (!readreg)
        readreg = true;

    if (iredir)
        cmd->ifname[cpos] = c;
    else if (oredir)
        cmd->ofname[cpos] = c;
    else
        cmd->argv[wpos][cpos] = c;

    cpos++;

    return PARSER_CONTINUE_ON;
}

/**
 * parse_cmd:
 *      Parse a string one character at a time, determine what class of
 *      character it is, one of (magic, whitespace or normal), then call
 *      the appropriate function to process the character that fills
 *      relevant data into the fields of a struct command for execution.
 */
int parse_cmd(struct command *cmd_ptr, char *const line_ptr)
{
    int ret;
    char c;

    line = line_ptr;
    cmd = cmd_ptr;

    if (!line)
        return PARSER_ERROR_ABORT;

    if (!*line)
        return PARSER_ERROR_BREAK;

    i = j = wpos = cpos = 0;
    iredir = oredir = readreg = inquote = false;
    cmd->argc = 1;

    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        switch (char_type(c)) {
        case IS_MAGIC:
            if (!(ret = do_magic(c)))
                return ret;
            break;
        case IS_WHSPC:
            if (!(ret = do_whspc(c)))
                return ret;
            break;
        case IS_NCHAR:
            if (!(ret = do_nchar(c)))
                return ret;
        default:
            break;
        }
    }

    return ret;
}

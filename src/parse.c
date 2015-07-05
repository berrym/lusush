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
int char_type(char c)
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
 * do_magic:
 *      Process characters with special syntatic significance.
 */
int do_magic(char c)
{
    char *home = NULL;
    readreg = false;

    switch (c) {
    case '#':
        if (!inquote) {
            if (iredir)
                cmd->ifname[cpos] = '\0';
            else if (oredir)
                cmd->ofname[cpos] = '\0';
            else
                cmd->argv[wpos][cpos] = '\0';

            goto done;
        }

        if (iredir || oredir) {
            fprintf(stderr, "lusush: error near character " \
                    "%u --> '%c'\n", i, c);
            return -1;
        }

        cmd->argv[wpos][cpos] = c;
        cpos++;
        break;
    case '&':
        if (!inquote) {
            cmd->background = true;
            cmd->argv[wpos][cpos] = '\0';
            wpos--;
            goto done;
        }

        if (iredir || oredir) {
            fprintf(stderr, "lusush: parse error near character at " \
                    "%u --> '%c'\n", i, c);

            return 0;
        }

        cmd->argv[wpos][cpos] = c;
        cpos++;
        break;
    case '<':
        if (!inquote) {
            cmd->iredir = true;
            iredir = true;

            if (line[i + 1] && line[i + 1] == '<') {
                fprintf(stderr, "lusush: parse error near '<<': " \
                        "invalid operator\n");

                return 0;
            }

            cmd->argv[wpos][cpos] = '\0';
        }
        else {
            cmd->argv[wpos][cpos] = c;
            cpos++;
        }
        break;
    case '>':
        if (!inquote) {
            cmd->oredir = true;
            oredir = true;

            if (line[i + 1] && line[i + 1] == '>') {
                cmd->oredir_append = true;
                i++;
            }

            if (line[i + 1] && line[i + 1] == '>') {
                fprintf(stderr, "lusush: parse error near '>>>': " \
                        "invalid operator\n");

                return 0;
            }

            cmd->argv[wpos][cpos] = '\0';
        }
        else {
            cmd->argv[wpos][cpos] = c;
            cpos++;
        }
        break;
    case '"':
        if (inquote)
            inquote = false;
        else
            inquote = true;

        return wpos;
    case '~':
        if (!(home = getenv("HOME"))) {
            cmd->argv[wpos][cpos] = c;
            cpos++;
        }
        else {
            strncat(cmd->argv[wpos], home, strlen(home));
            cpos += strlen(home);
        }

        home = NULL;
        break;
    }

done:
    cmd->argv[wpos] = NULL;
    cmd->argc = wpos;

    return wpos;
}

/**
 * do_whspc
 *      Process whitespace.
 */
int do_whspc(char c)
{
    if (inquote && !iredir && !oredir) {
        cmd->argv[wpos][cpos] = c;
        cpos++;

        return c;
    }

    while (isspace((int)c)) {
        i++;
        c = line[i];
    }
    i--;

    if (!wpos && !readreg)
        return 0;

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
        perror("lusush: calloc");

        for (j = wpos - 1; ; j--) {
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }

        return -1;
    }

    cmd->argv[wpos][cpos] = '\0';
    cmd->argc = wpos + 1;

    return c;
}

/**
 * do_nchar:
 *      Process normal character.
 */
int do_nchar(char c)
{
    if (!readreg)
        readreg = true;

    if (cmd->iredir && iredir)
        cmd->ifname[cpos] = c;
    else if (cmd->oredir && oredir)
        cmd->ofname[cpos] = c;
    else
        cmd->argv[wpos][cpos] = c;

    cpos++;

    return c;
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
    int ret = 0;
    char c;

    line = line_ptr;
    cmd = cmd_ptr;

    if (!line)
        return -1;

    if (!*line)
        return 0;

    i = j = wpos = cpos = 0;
    iredir = oredir = readreg = inquote = false;
    cmd->argc = 1;

    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        switch (char_type(c)) {
        case IS_MAGIC:
            if ((ret = do_magic(c)) == -1)
                return ret;
            break;
        case IS_WHSPC:
            if ((ret = do_whspc(c)) == -1)
                return ret;
            break;
        case IS_NCHAR:
            if ((ret = do_nchar(c)) == -1)
                return ret;
        default:
            break;
        }
    }

    return ret;
}

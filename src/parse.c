/**
 * parse.c - command parser
 *
 * Copyright (c) 2009-2014 Michael Berry <trismegustis@gmail.com>
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

// CMD flags effecting parser behavior
static bool iredir = false;
static bool oredir = false;
static bool readreg = false;
static bool inquote = false;

// input buffers
static char *line = NULL;
static CMD *cmd = NULL;

/**
 * char_type:
 *       Identify what type of character we are dealing with.
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
 *       Handle magic characters.
 */
int do_magic(char c)
{
    char *home = NULL;
    readreg = false;

    switch (c) {
    case '#':
        if (!inquote) {
            if (iredir) {
                cmd->ifname[cpos] = '\0';
            }
            else if (oredir) {
                cmd->ofname[cpos] = '\0';
            }
            else {
                cmd->argv[wpos][cpos] = '\0';
            }
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
            fprintf(stderr, "lusush: error near character " \
                    "%u --> '%c'\n", i, c);
            return -1;
        }

        cmd->argv[wpos][cpos] = c;
        cpos++;
        break;
    case '<':
        if (!inquote) {
            cmd->iredir = true;
            iredir = true;
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
            if (line[i+1] && line[i+1] == '>') {
                cmd->oredir_append = true;
                i++;
            }
            cmd->argv[wpos][cpos] = '\0';
        }
        else {
            cmd->argv[wpos][cpos] = c;
            cpos++;
        }
        break;
    case '"':
        if (inquote) {
            inquote = false;
	}
        else {
            inquote = true;
	}
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
 *       Process whitespace.
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
        
    cmd->argv[wpos] = calloc(BUFSIZ, sizeof(char));
    if (cmd->argv[wpos] == NULL) {
	perror("lusush: calloc");
	for (j = wpos - 1; ; j--) {
	    free(cmd->argv[j]);
	    cmd->argv[j] = NULL;
	}
	return -1;
    }
    cmd->argv[wpos][cpos] = '\0';
    cmd->argc = wpos+1;

    return c;
}

/**
 * do_nchar:
 *       Process normal character.
 */
int do_nchar(char c)
{
    if (!readreg) readreg = true;

    if (cmd->iredir && iredir) {
        cmd->ifname[cpos] = c;
    }
    else if (cmd->oredir && oredir) {
        cmd->ofname[cpos] = c;
    }
    else {
	cmd->argv[wpos][cpos] = c;
    }
    cpos++;

    return c;
}

/**
 * parse_cmd:
 *       Given a string of input parse_cmd will seperate words by whitespace
 *       and place each individual word into it's own string inside of a pointer
 *       to pointer char, called argv, which should already be initialized.
 *       parse_cmd __DOES NOT__ alloc cmd->argv, only additional strings.
 *       Special characters like &, <, and > have special cases and are dealt
 *       with according to their meaning, setting appropriate flags and filling
 *       appropriate buffers with information.  Individual words are also
 *       checked for expansions such as alias expansion.
 */
int parse_cmd(CMD *cmd_ptr, char *const line_ptr)
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

    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        switch (char_type(c)) {
        case IS_MAGIC:
            if ((ret = do_magic(c)) == -1) {
                return ret;
            }
            break;
        case IS_WHSPC:
            if ((ret = do_whspc(c)) == -1) {
                return ret;
            }
            break;
        case IS_NCHAR:
            if ((ret = do_nchar(c)) == -1) {
                return ret;
            }
            break;
        default:
            break;
        }
    }

    return ret;
}

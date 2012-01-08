/*
 * parse.c - command parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldefs.h"
#include "ltypes.h"
#include "parse.h"

static int ret_level = 0;
static unsigned int i = 0;
static unsigned int j = 0;
static unsigned int lpos = 0;
static unsigned int wpos = 0;

static bool in_redirect = false;
static bool out_redirect = false;
static bool read_reg = false;
static bool in_quote = false;

static char *line = NULL;
static CMD *cmd = NULL;

/*
 * Identify what type of character we are dealing with.
 */
int char_type(char c)
{
    int chtype = 0;

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
        chtype = IS_WHSPC;
        break;
    default:
        chtype = IS_NCHAR;
        break;
    }

    return chtype;
}

/*
 * Handle magic characters.
 */
int do_magic(char c)
{
    char *home = NULL;

    switch (c) {
    case '#':
        if (!in_quote) {
            if (in_redirect) {
                cmd->in_filename[wpos] = '\0';
            }
            else if (out_redirect) {
                cmd->out_filename[wpos] = '\0';
            }
            else {
                cmd->argv[lpos][wpos] = '\0';
            }
            goto done;
        }

        if (in_redirect || out_redirect) {
            fprintf(stderr, "lusush: error near character " \
                    "%u --> '%c'\n", i, c);
            return -1;
        }

        cmd->argv[lpos][wpos] = c;
        wpos++;
        break;

    case '&':
        if (!in_quote) {
            cmd->background = true;
            cmd->argv[lpos][wpos] = '\0';
            lpos--;
            goto done;
        }

        if (in_redirect || out_redirect) {
            fprintf(stderr, "lusush: error near character " \
                    "%u --> '%c'\n", i, c);
            return -1;
        }

        cmd->argv[lpos][wpos] = c;
        wpos++;
        break;

    case '<':
        if (!in_quote) {
            cmd->in_redirect = true;
            in_redirect = true;
            cmd->argv[lpos][wpos] = '\0';
        }
        else {
            cmd->argv[lpos][wpos] = c;
            wpos++;
        }
        break;

    case '>':
        if (!in_quote) {
            cmd->out_redirect = true;
            out_redirect = true;
            if (line[i+1] && line[i+1] == '>') {
                cmd->oredir_append = true;
                i++;
            }
            cmd->argv[lpos][wpos] = '\0';
        }
        else {
            cmd->argv[lpos][wpos] = c;
            wpos++;
        }
        break;
    case '"':
        if (in_quote)
            in_quote = false;
        else
            in_quote = true;
        break;
    case '~':
        if (!(home = getenv("HOME"))) {
            cmd->argv[lpos][wpos] = c;
            wpos++;
        }
        else {
            strncat(cmd->argv[lpos], home, strlen(home));
            wpos += strlen(home);
        }
        home = NULL;
        break;
    }
 done:
    lpos++;
    cmd->argv[lpos] = NULL;
    cmd->argc = lpos;

    return lpos;  
}

/*
 * Process whitespace.
 */
int do_whspc(char c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        if (in_quote && !in_redirect && !out_redirect) {
            cmd->argv[lpos][wpos] = c;
            wpos++;
            break;
        }

        while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            i++;
            c = line[i];
        }
        i--;

        if (!lpos && !read_reg)
            break;

        if (in_redirect) {
            cmd->in_filename[wpos] = '\0';
            in_redirect = false;
        }
        else if (out_redirect) {
            cmd->out_filename[wpos] = '\0';
            out_redirect = false;
        }
        else {
            cmd->argv[lpos][wpos] = '\0';
        }
        lpos++;
        wpos = 0;

        cmd->argv[lpos] = calloc(MAXLINE, sizeof(char));
        if (cmd->argv[lpos] == NULL) {
            perror("lusush: calloc");
            for (j = lpos - 1; j >= 0; j--) {
                free(cmd->argv[j]);
                cmd->argv[j] = NULL;
            }
            return -1;
        }
        cmd->argv[lpos][wpos] = '\0';
        cmd->argc++;
        break;
    }
}

/*
 * Process normal character.
 */
int do_nchar(char c)
{
    if (!read_reg)
        read_reg = true;

    if (cmd->in_redirect && in_redirect) {
        cmd->in_filename[wpos] = c;
    }
    else if (cmd->out_redirect && out_redirect) {
        cmd->out_filename[wpos] = c;
    }
    else {
        cmd->argv[lpos][wpos] = c;
    }
    wpos++;
}

/*
 * Given a string of input parse_cmd will seperate words by whitespace
 * and place each individual word into it's own string inside of a pointer
 * to pointer char, called argv, which should already be initialized.
 * parse_cmd __DOES NOT__ alloc cmd->argv, only additional strings.
 * Special characters like &, <, and > have special cases and are dealt
 * with according to their meaning, setting appropriate flags and filling
 * appropriate buffers with information.  Individual words are also
 * checked for expansions such as alias expansion.
 */
int parse_cmd(CMD *cmd_ptr, char *line_ptr)
{
    int ret = 0;
    char c;

    line = line_ptr;
    cmd = cmd_ptr;

    if (!line)
        return -1;
    if (!*line)
        return 0;

    i = j = lpos = wpos = 0;
    in_redirect = out_redirect = read_reg = in_quote = false;

    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        switch (char_type(c)) {
        case IS_MAGIC:
            ret = do_magic(c);
            break;
        case IS_WHSPC:
            ret = do_whspc(c);
            break;
        case IS_NCHAR:
            ret  = do_nchar(c);
            break;
        default:
            break;
        }
    }

    return ret;
}

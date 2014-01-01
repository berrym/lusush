/**
 * parse.c - command parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lusush.h"
#include "parse.h"

// loop and counter variables
static unsigned int i = 0;
static unsigned int j = 0;
static unsigned int lpos = 0;
static unsigned int wpos = 0;
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
                cmd->ifname[wpos] = '\0';
            }
            else if (oredir) {
                cmd->ofname[wpos] = '\0';
            }
            else {
                cmd->argv[lpos][wpos] = '\0';
            }
            goto done;
        }

        if (iredir || oredir) {
            fprintf(stderr, "lusush: error near character " \
                    "%u --> '%c'\n", i, c);
            return -1;
        }

        cmd->argv[lpos][wpos] = c;
        wpos++;
        break;
    case '&':
        if (!inquote) {
            cmd->background = true;
            cmd->argv[lpos][wpos] = '\0';
            lpos--;
            goto done;
        }

        if (iredir || oredir) {
            fprintf(stderr, "lusush: error near character " \
                    "%u --> '%c'\n", i, c);
            return -1;
        }

        cmd->argv[lpos][wpos] = c;
        wpos++;
        break;
    case '<':
        if (!inquote) {
            cmd->iredir = true;
            iredir = true;
            cmd->argv[lpos][wpos] = '\0';
        }
        else {
            cmd->argv[lpos][wpos] = c;
            wpos++;
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
            cmd->argv[lpos][wpos] = '\0';
        }
        else {
            cmd->argv[lpos][wpos] = c;
            wpos++;
        }
        break;
    case '"':
        if (inquote) {
            inquote = false;
	}
        else {
            inquote = true;
	}
	return lpos;
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
    cmd->argv[lpos] = NULL;
    cmd->argc = lpos;

    return lpos;  
}

/**
 * do_whspc
 *       Process whitespace.
 */
int do_whspc(char c)
{
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        if (inquote && !iredir && !oredir) {
            cmd->argv[lpos][wpos] = c;
            wpos++;
            break;
        }

        while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            i++;
            c = line[i];
        }
        i--;

        if (!lpos && !readreg)
            break;

        if (iredir) {
            cmd->ifname[wpos] = '\0';
            iredir = false;
        }
        else if (oredir) {
            cmd->ofname[wpos] = '\0';
            oredir = false;
        }
        else {
            cmd->argv[lpos][wpos] = '\0';
        }
        lpos++;
        wpos = 0;
        
        cmd->argv[lpos] = calloc(BUFSIZ, sizeof(char));
        if (cmd->argv[lpos] == NULL) {
            perror("lusush: calloc");
            for (j = lpos - 1; ; j--) {
                free(cmd->argv[j]);
                cmd->argv[j] = NULL;
            }
            return -1;
        }
        cmd->argv[lpos][wpos] = '\0';
        cmd->argc = lpos+1;
        break;
    }

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
        cmd->ifname[wpos] = c;
    }
    else if (cmd->oredir && oredir) {
        cmd->ofname[wpos] = c;
    }
    else {
	cmd->argv[lpos][wpos] = c;
    }
    wpos++;

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

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
#include "cmdlist.h"
#include "misc.h"

#define PARSER_ERROR_ABORT -1   // major error, shell should terminate
#define PARSER_ERROR_BREAK 0    // quit parsing line, not serious
#define PARSER_CONTINUE_ON 1    // keep going everything is ok

// character classifications
enum {
    IS_MAGIC,                   // magic character
    IS_WHSPC,                   // whitespace
    IS_NCHAR                    // normal character
};

// loop and counter variables
static unsigned int i;
static unsigned int j;
static unsigned int wpos;
static unsigned int cpos;

// command flags effecting parser behavior
static bool iredir;
static bool oredir;
static bool readreg;
static bool inquote;
static bool escaping;

// input buffers
static char *line = NULL;
static struct command *cmd = NULL;

/**
 * strip_trailing_whspc:
 *      Remove whitespace at the end of a string.
 */
static void strip_trailing_whspc(char *s)
{
    while (strlen(s) && isspace((int)s[strlen(s) - 1]))
        s[strlen(s) - 1] = '\0';
}

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
    case '\\':
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
    // Keep the character, do not terminate line
    if (escaping || inquote) {
        if (iredir || oredir) {
            fprintf(stderr, "lusush: parse error near '#'\n");
            return PARSER_ERROR_BREAK;
        }

        cmd->argv[wpos][cpos] = '#';
        cpos++;

        if (escaping)
            escaping = false;

        return PARSER_CONTINUE_ON;
    }

    // Null terminate buffers
    if (iredir)
        cmd->ifname[cpos] = '\0';
    else if (oredir)
        cmd->ofname[cpos] = '\0';
    else
        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = '\0';

    line[i] = '\0';

    return PARSER_CONTINUE_ON;
}

/**
 * do_ampersand:
 *      Process the ampersand character which signals that a command
 *      should be executed as a background process.
 */
static int do_ampersand(void)
{
    // Not keeping the character
    if (!escaping && !inquote) {
        // && not supported
        if (line[i + 1] && line[i + 1] == '&') {
            fprintf(stderr, "lusush: parse error near '&&': " \
                    "invalid operator.\n");
            return PARSER_ERROR_BREAK;
        }

        // Set background flag
        cmd->background = true;

        // Null terminate line
        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = '\0';

        // If wpos is not zero we need to decrease it's value by one
        // or it will index an unallocated string
        if (wpos)
            wpos--;

        return PARSER_CONTINUE_ON;
    }

    if (iredir || oredir) {
        fprintf(stderr, "lusush: parse error near '&'\n");
        return PARSER_ERROR_BREAK;
    }

    // Keep the character
    cmd->argv[wpos][cpos] = '&';
    cpos++;

    // Unset the escaping flag if set
    if (escaping)
        escaping = false;

    return PARSER_CONTINUE_ON;
}

/**
 * do_lessthan:
 *      Process the character '<' which denotes input redirection. 
 */
static int do_lessthan(void)
{
    // Keep the character
    if (escaping || inquote) {
        cmd->argv[wpos][cpos] = '<';
        cpos++;
        if (escaping)
            escaping = false;
        return PARSER_CONTINUE_ON;
    }

    // Set the input redirection flags
    iredir = cmd->iredir = true;

    // << is unsupported syntax
    if (line[i + 1] && line[i + 1] == '<') {
        fprintf(stderr, "lusush: parse error near '<<': " \
                "invalid operator\n");
        return PARSER_ERROR_BREAK;
    }

    // Null terminate the line
    if (cmd->argv[wpos])
        cmd->argv[wpos][cpos] = '\0';

    return PARSER_CONTINUE_ON;
}

/**
 * do_greaterthan():
 *      Process the '>' character which denotes output redirection.
 */
static int do_greaterthan(void)
{
    // Keep the character
    if (escaping || inquote) {
        cmd->argv[wpos][cpos] = '>';
        cpos++;
        if (escaping)
            escaping = false;
        return PARSER_CONTINUE_ON;
    }

    // Set output redirection flags
    oredir = cmd->oredir = true;

    // Check if using appended file writes
    if (line[i + 1] && line[i + 1] == '>') {
        cmd->oredir_append = true;
        i++;
    }

    // A third > is unsupported syntax
    if (cmd->oredir_append) {
        if (line[i + 1] && line[i + 1] == '>') {
            fprintf(stderr, "lusush: parse error near '>>>': "  \
                    "invalid operator\n");
            return PARSER_ERROR_BREAK;
        }
    }

    // Null terminate line
    if (cmd->argv[wpos])
        cmd->argv[wpos][cpos] = '\0';

    wpos++;                     // increase wpos
    cpos = 0;                   // reset cpos

    return PARSER_CONTINUE_ON;
}

/**
 * do_doublequote:
 *      Process the double quote character that delimits strings.
 */
static int do_doublequote(void)
{
    // Keep the double quote
    if (escaping) {
        cmd->argv[wpos][cpos] = '"';
        cpos++;
        escaping = false;
        return PARSER_CONTINUE_ON;
    }

    // Flip the flag state
    inquote ^= 1;

    return PARSER_CONTINUE_ON;
}

/**
 * do_tilde:
 *      Process the tilde character which expands to user's home directory.
 */
static int do_tilde(void)
{
    char *home = NULL;

    // Keep the character
    if (escaping || inquote) {
        cmd->argv[wpos][cpos] = '~';
        cpos++;
        if (escaping)
            escaping = false;
        return PARSER_CONTINUE_ON;
    }

    // Check that the string has memory allocated
    if (!cmd->argv[wpos])
        return PARSER_ERROR_BREAK;

    // Get the user's home directory
    if (!(home = getenv("HOME")))
        return PARSER_ERROR_BREAK;

    // Expand the tilde to the user's home
    strncat(cmd->argv[wpos], home, strlen(home) + 1);
    cpos += strlen(home);

    home = NULL;

    return PARSER_CONTINUE_ON;
}

/**
 * do_backslash:
 *      Process the backslash character as an escape character.
 */
static int do_backslash(void)
{
    // Keep the character
    if (escaping) {
        cmd->argv[wpos][cpos] = '\\';
        cpos++;
    }

    // Flip the flag state
    escaping ^= 1;

    return PARSER_CONTINUE_ON;
}

/**
 * do_magic:
 *      Process characters with special syntatic significance.
 */
static int do_magic(char c)
{
    int err = PARSER_CONTINUE_ON;

    switch (c) {
    case '#':
        err = do_pound();
        break;
    case '&':
        err = do_ampersand();
        break;
    case '<':
        err = do_lessthan();
        break;
    case '>':
        err = do_greaterthan();
        break;
    case '"':
        err = do_doublequote();
        break;
    case '~':
        err = do_tilde();
        break;
    case '\\':
        err = do_backslash();
    default:
        break;
    }

    return err;
}

/**
 * do_whspc:
 *      Process whitespace, unless inside a quotation ignore it.
 */
static int do_whspc(char c)
{
    // Keep the whitespace
    if (inquote && !iredir && !oredir) {
        cmd->argv[wpos][cpos] = c;
        cpos++;

        return PARSER_CONTINUE_ON;
    }

    // Iterate over line until a non whitespace character is found
    while (isspace((int)c)) {
        i++;
        c = line[i];
    }
    i--;

    // No valid input was ever read, quit parsing
    if (!wpos && !readreg)
        return PARSER_ERROR_BREAK;

    if (iredir && cpos) {       // terminate input file name
        cmd->ifname[cpos] = '\0';
        iredir = false;
    }
    else if (oredir && cpos) {  // terminate output file name
        cmd->ofname[cpos] = '\0';
        oredir = false;
    }
    else {                      // terminate current word
        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = '\0';
    }
    wpos++;                     // increase wpos
    cpos = 0;                   // reset cpos

    // Allocate next string
    if ((cmd->argv[wpos] = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: parse.c: do_whspc: calloc");

        for (j = wpos - 1; ; j--) {
            free(cmd->argv[j]);
            cmd->argv[j] = NULL;
        }

        return PARSER_ERROR_ABORT;
    }

    // Null terminate line
    cmd->argv[wpos][cpos] = '\0';
    cmd->argc = wpos + 1;       // set argument count

    return PARSER_CONTINUE_ON;
}

/**
 * do_nchar:
 *      Process normal character.
 */
static int do_nchar(char c)
{
    // Set readreg flag
    if (!readreg)
        readreg = true;

    // Do string interpolation
    if (escaping && inquote) {
        switch(c) {
        case 't':
            c = '\t';
            escaping = false;
            break;
        case 'n':
            c = '\n';
            escaping = false;
            break;
        case 'v':
            c = '\v';
            escaping = false;
            break;
        case 'f':
            c = '\f';
            escaping = false;
            break;
        default:
            cmd->argv[wpos][cpos] = '\\';
            cpos++;
            escaping = false;
            break;
        }
    }

    // Put c into the proper buffer
    if (iredir)
        cmd->ifname[cpos] = c;
    else if (oredir)
        cmd->ofname[cpos] = c;
    else
        if (cmd->argv[wpos])
            cmd->argv[wpos][cpos] = c;

    cpos++;                     // increase cpos

    return PARSER_CONTINUE_ON;
}

/**
 * do_token:
 *      Parse a token one character at a time, determine what class of
 *      character it is, one of (magic, whitespace or normal), then call
 *      the appropriate function to process the character that fills
 *      relevant data into the fields of a struct command for execution.
 */
static int do_token(char *tok, struct command *cmdp)
{
    int err;
    char c;

    line = tok;
    cmd = cmdp;

    // Set state variables to starting values
    i = j = wpos = cpos = 0;
    iredir = oredir = readreg = inquote = escaping = false;
    cmd->argc = 1;

    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        switch (char_type(c)) {
        case IS_MAGIC:
            if (!(err = do_magic(c)))
                return err;
            break;
        case IS_WHSPC:
            if (!(err = do_whspc(c)))
                return err;
            break;
        case IS_NCHAR:
            if (!(err = do_nchar(c)))
                return err;
        default:
            break;
        }
    }

    // Check that something was parsed and actually stored
    if (!readreg)
        return PARSER_ERROR_BREAK;

    return PARSER_CONTINUE_ON;
}

/**
 * parse_command:
 *      Break a line into tokens that are parsed by do_token.
 */
int parse_command(const char *linep, struct command *cmdp)
{
    int err;                    // error code
    unsigned int k, l;          // loop iterators
    int count = 0;              // number of commands parsed
    bool pipe = false;          // pipe flag

    // Storage for first tier of tokens (";")
    char *tok = NULL, *ptr1 = NULL, *savep1 = NULL;
    // Storage for secondary tier of tokens ("|")
    char *subtok = NULL, *ptr2 = NULL, *savep2 = NULL;
    // Buffer for a copy of linep to mangle with strtok_r
    char *tmp = NULL;

    // Line not allocated or inaccessible, terminate
    if (!linep)
        return PARSER_ERROR_ABORT;

    // Line is empty or unitialized, quit parsing
    if (!*linep)
        return PARSER_ERROR_BREAK;

    if ((tmp = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: input.c: do_line: calloc");
        return PARSER_ERROR_ABORT;
    }

    strncpy(tmp, linep, MAXLINE); // make a copy of linep to mangle

    // Break line into major tokens seperated by a semicolon
    for (k = 0, ptr1 = tmp; ; k++, ptr1 = NULL) {
        if (!(tok = strtok_r(ptr1, ";", &savep1)))
            break;

        strip_trailing_whspc(tok);

        // Break token into smaller tokens seperated by |
        for (l = 0, ptr2 = tok; ; l++, ptr2 = NULL) {
            if (!(subtok = strtok_r(ptr2, "|", &savep2))) {
                pipe = false;
                break;
            }

            strip_trailing_whspc(subtok);

            if (cmdalloc(cmdp) < 0) {
                err = PARSER_ERROR_ABORT;
                goto cleanup;
            }

            // Check if we are at the head of a pipe chain
            if (l == 1) {
                vprint("**** Do pipe %s\n", subtok);
                cmdp->prev->pipe = true;
                cmdp->prev->pipe_head = true;
                pipe = true;
            }

            if (pipe)
                cmdp->pipe = true;

            // Parse a token into a command struct
            switch (err = do_token(subtok, cmdp)) {
            case PARSER_ERROR_ABORT:
            case PARSER_ERROR_BREAK:
                goto cleanup;
            default:
                cmdp->next->prev = cmdp;
                cmdp = cmdp->next;
                cmdp->next = NULL;
                count++;
            }
        }
    }

cleanup:
    if (tmp)
        free(tmp);

    tmp = NULL;

    if (k && ptr1)
        free(ptr1);

    ptr1 = NULL;

    if (l && ptr2)
        free(ptr2);

    ptr2 = NULL;

    switch (err) {
    case PARSER_ERROR_ABORT:
    case PARSER_ERROR_BREAK:
        return err;
    default:
        break;
    }

    return count;               // return number of command's parsed
}

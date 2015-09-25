/**
 * parse.c - command parser
 *
 * Copyright (c) 2015 Michael Berry <trismegustis@gmail.com>
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

#include "parse.h"
#include "cmdlist.h"
#include "opts.h"

#include "alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DBGSTR "DEBUG: parse.c: "

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
static size_t i;
static size_t j;
static size_t wpos;
static size_t cpos;

// state flags effecting parser behavior
static bool iredir;             // input redirection flag
static bool oredir;             // output redirection flag
static bool readreg;            // read normal (regular) character flag
static bool inquote;            // inside quotation flag
static bool escaping;           // in an escape sequence flag

// input buffers
static char *line = NULL;
static struct command *cmd = NULL;

/**
 * strip_leading_whspc:
 *      Remove whitespace at the beginning of a string.
 */
static inline void strip_leading_whspc(char *s)
{
    char buf[MAXLINE] = { '\0' }; // buffer to store modified string
    size_t k, l;                  // loop counters

    // Iterate over leading whitespace ignoring it
    for (k = 0; k < MAXLINE && isspace((int)s[k]); k++);

    if (!k)
        return;

    // Copy the rest of the string into buf
    for (l = 0; s[k]; k++, l++)
        buf[l] = s[k];

    // If buf differs from s overwrite s with buf
    if (strncmp(buf, s, MAXLINE) == 0)
        return;

    memset(s, '\0', strnlen(s, MAXLINE));
    strncpy(s, buf, strnlen(buf, MAXLINE));
}

/**
 * strip_trailing_whspc:
 *      Remove whitespace at the end of a string.
 */
static inline void strip_trailing_whspc(char *s)
{
    while (strnlen(s, MAXLINE) && isspace((int)s[strnlen(s, MAXLINE) - 1]))
        s[strnlen(s, MAXLINE) - 1] = '\0';
}

/**
 * tokenize:
 *      Break a string into tokens, delimited by a ';' or a '|'.
 *      The token is split off the front of the original string.
 */
static char *tokenize(char **s, struct command *cmdp)
{
    size_t k;                     // loop counter
    bool esc = false, iq = false; // in an escape and in a quote flags
    char *c = NULL;               // pointer to iterate over s
    char *tok = NULL;             // storage for the token

    // Check that s is a valid string
    if (!s || !*s)
        return NULL;

    // Allocate memory for the token
    if ((tok = calloc(strnlen(*s, MAXLINE), sizeof(char *))) == NULL)
        error_syscall("lusush: parse.c: tokenize: calloc");

    // Iterate over s and delimit on ';' or '|' unless escaping or in a quote
    for (k = 0, c = *s; *c; k++, *c++) {
        switch (*c) {
        case '\\':              // escape, keep ; or |, interpolate later
            tok[k] = *c;
            esc ^= 1;
            break;
        case '"':               // dquote, keep ; or |, interpolate later
            tok[k] = *c;
            if (esc)
                esc = false;
            else
                iq ^= 1;
            break;
        case ';':               // finish parsing current command
            if (iq || esc) {
                tok[k] = *c;
                if (esc)
                    esc = false;
                break;
            }
            else {
                tok[k] = '\0';
            }
            *c++;
            goto mangle;
            break;
        case '|':               // finsish parsing token, flag a pipe
            if (iq || esc) {
                tok[k] = *c;
                if (esc)
                    esc = false;
                break;
            }
            else {
                tok[k] = '\0';
                cmdp->pipe = true;
                if (!cmdp->prev)
                    cmdp->pipe_head = true;
            }
            *c++;
            goto mangle;
            break;
        default:
            tok[k] = *c;
            break;
        }
    }

mangle:
    // Mangle s then return the token
    if (!*tok) {
        free(tok);
        tok = NULL;
        return NULL;
    }

    *s = c;

    return tok;
}

/**
 * expand_token:
 *      Perform alias expansion on a token.
 */
static inline void expand_token(char *tok, char delim, char *buf)
{
    // Loop iterator
    size_t k;
    // Storage for the call to strtok_r
    char *subtok = NULL, *ptr = NULL, *savep = NULL;
    // A buffer for an expanded alias
    char *ea = NULL;

    strip_leading_whspc(tok);

    // Tokenize the first word of the token delimited by whitespace
    if (!(subtok = strtok_r(tok, "\t\n\r\f\v ", &savep)))
        return;

    // Check if subtok has an alias expansion
    ea = expand_alias(subtok);

    if (!ea)
        return;

    // Copy the expanded alias into buf
    strncat(buf, ea, strnlen(ea, MAXLINE));

    vputs("EA ==>\t(%s)\n", ea);
    // Concatenate any data after subtok into buf
    if (strncmp(ea, subtok, MAXLINE) != 0) {
        for (k = strnlen(buf, MAXLINE); k < strnlen(subtok, MAXLINE); k++)
            buf[strnlen(buf, MAXLINE)] = subtok[k];
        buf[strnlen(buf, MAXLINE)] = delim;
    }
    vputs("BUF ==>\t(%s)\n", buf);
}

/**
 * expand_line:
 *      Perform alias substitutions on a string.
 */
static void expand_line(char *s)
{
    // Storage for first tier of tokens (";")
    char *tok = NULL, *ptr1 = NULL, *savep1 = NULL;
    // Storage for secondary tier of tokens ("|")
    char *subtok = NULL, *ptr2 = NULL, *savep2 = NULL;
    // Buffer for a copy of s to mangle with strtok_r
    char tmp[MAXLINE] = { '\0' };
    // A buffer for the newly expanded line
    char buf[MAXLINE] = { '\0' };

    // Make a copy of s
    strncpy(tmp, s, strnlen(s, MAXLINE));

    // Break line into major tokens seperated by a semicolon
    for (ptr1 = tmp; ; ptr1 = NULL) {
        if (!(tok = strtok_r(ptr1, ";", &savep1)))
            break;

        strip_trailing_whspc(tok);
        expand_token(tok, ';', buf);

        // Break token into smaller tokens seperated by |
        for (ptr2 = tok; ; ptr2 = NULL) {
            if (!(subtok = strtok_r(ptr2, "|", &savep2)))
                goto substitute;

            if (strncmp(tok, subtok, MAXLINE) == 0)
                break;

            strip_trailing_whspc(subtok);
            expand_token(subtok, '|', buf);
        }
    }

substitute:
    // If buf is empty no substitutions were made
    if (!*buf)
        return;

    // If buf is the same as s no substitutions were made
    if (strncmp(buf, s, MAXLINE) == 0)
        return;

    strip_trailing_whspc(buf);

    // Copy new buf to s
    memset(s, 0, strnlen(s, MAXLINE));
    strncpy(s, buf, strnlen(buf, MAXLINE) + 1);
}

/**
 * char_type:
 *      Identify the classification of a character, one of magic,
 *      whitespace, or normal, returning it's enumerated value.
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
            error_message("lusush: parse error near '#'\n");
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
            error_message("lusush: parse error near '&&': "
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
        error_message("lusush: parse error near '&'\n");
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
        error_message("lusush: parse error near '<<': "
                      "invalid operator\n");
        return PARSER_ERROR_BREAK;
    }

    // Null terminate the line
    if (cmd->argv[wpos])
        cmd->argv[wpos][cpos] = '\0';

    return PARSER_CONTINUE_ON;
}

/**
 * do_greaterthan:
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
            error_message("lusush: parse error near '>>>': "
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
    strncat(cmd->argv[wpos], home, strnlen(home, MAXLINE) + 1);
    cpos += strnlen(home, MAXLINE);

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
    // Iterate over line until a non whitespace character is found
    while (isspace((int)c)) {
        // Keep the whitespace if inside a double quotation
        if (inquote) {
            cmd->argv[wpos][cpos] = c;
            cpos++;
        }
        i++;
        c = line[i];
    }
    i--;

    // No valid input has been parsed yet, return
    if (!wpos && !readreg)
        return PARSER_CONTINUE_ON;

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
    if ((cmd->argv[wpos] = calloc(MAXLINE, sizeof(char))) == NULL)
        error_syscall("lusush: parse.c: do_whspc: calloc");

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

    // Do string interpolation on escape sequences
    if (escaping && inquote) {
        switch(c) {
        case 't':
            c = '\t';
            break;
        case 'n':
            c = '\n';
            break;
        case 'v':
            c = '\v';
            break;
        case 'f':
            c = '\f';
        default:
            break;
        }
    }

    // Can't be in an escape still
    if (escaping)
        escaping = false;

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
    int err;                    // error code
    char c;                     // current charater

    // Point line and cmd to current token and command being processed
    line = tok;
    cmd = cmdp;

    // Set state variables to starting values
    i = j = wpos = cpos = 0;
    iredir = oredir = readreg = inquote = escaping = false;
    cmd->argc = 1;

    for (i = 0; i < strnlen(line, MAXLINE); i++) {
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

    // Print out command structure if VERBOSE_PRINT is enabled
    if (opt_is_set(VERBOSE_PRINT))
        display_command(cmd);

    return PARSER_CONTINUE_ON;
}

/**
 * parse_command:
 *      Break a line into tokens that are parsed by do_token.
 */
int parse_command(const char *linep, struct command *cmdp)
{
    int err;                    // error code
    size_t count = 0;           // number of commands parsed
    char *tok = NULL, *tmp = NULL, *savep = NULL;
    bool pipe = false;

    // Line not allocated or inaccessible, terminate
    if (!linep)
        return PARSER_ERROR_ABORT;

    // Line is empty or uninitialized, quit parsing
    if (!*linep)
        return PARSER_ERROR_BREAK;

    // Make a copy of linep for mangling
    if (!(tmp = calloc(MAXLINE, sizeof(char))))
        error_syscall("lusush: parse.c: parse_command: calloc");

    strncpy(tmp, linep, MAXLINE);

    // Alias expansions
    expand_line(tmp);
    vputs("EXPANDED LINE ==> (%s)\n", tmp);
    savep = tmp;                // save the original pointer offset

    while (*tmp) {
        if (cmdp->prev && cmdp->prev->pipe)
            cmdp->pipe = true;

        if (!(tok = tokenize(&tmp, cmdp)))
            break;

        strip_trailing_whspc(tok);

        switch (err = do_token(tok, cmdp)) {
        case PARSER_ERROR_ABORT:
        case PARSER_ERROR_BREAK:
            return err;
        default:
            cmdp->next = alloc_command(cmdp);
            cmdp->next->prev = cmdp;
            cmdp = cmdp->next;
            cmdp->next = NULL;
            count++;
            if (tok)
                free(tok);
            tok = NULL;
        }
        vputs("TMP ==> %s\n", tmp);
        pipe = false;
    }

    if (savep)
        free(savep);

    savep = NULL;
    tmp = NULL;

    return count;               // Return number of command's parsed
}

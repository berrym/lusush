/*
 * parse.c - command parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldefs.h"
#include "ltypes.h"
#include "parse.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// TODO: The parsing routine has gotten ridiculus. REFACTOR REFACTOR REFACTOR //
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


/*
 * parse_cmd:
 *      Given a string of input parse_cmd will seperate words by whitespace
 *      and place each individual word into it's own string inside of a pointer
 *      to pointer char, called argv, which should already be initialized.
 *      parse_cmd __DOES NOT__ alloc cmd->argv, only additional strings.
 *      Special characters like &, <, and > have special cases and are dealt
 *      with according to their meaning, setting appropriate flags and filling
 *      appropriate buffers with information.  Individual words are also
 *      checked for expansions such as alias expansion.
 */
int parse_cmd(CMD *cmd, char *line)
{
    unsigned int i, j, lpos, wpos;
    bool in_redirect, out_redirect, read_reg, in_quote;
    char c;
    char *home = NULL;

    // Make sure our line is not empty
    if (!line)
        return -1;
    if (!*line)
        return 0;

    // initialize our integral variables to zero
    i = j = lpos = wpos = 0;
    in_redirect = out_redirect = read_reg = in_quote = false;

    ///////////////////////////////////////////////////////////////////////////
    // Loop through line character at a time and place words, seperated by
    // whitespace, into individual elements in an array of strings.
    ///////////////////////////////////////////////////////////////////////////
    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        switch (c) {

            /////////////////////////////////////////////////
            // Special Characters
            /////////////////////////////////////////////////

        case '#':                           // comment, ignore rest of line
            // Make sure we are not inside quotations
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

            // Make sure we are not redirecting
            if (in_redirect || out_redirect) {
                fprintf(stderr, "lusush: error near character "\
                        "%u --> '%c'\n", i, c);
                return -1;
            }

            // Copy the character
            cmd->argv[lpos][wpos] = c;
            wpos++;
            break;
        case '&':
            // Make sure we are not inside quatations
            if (!in_quote) {
                cmd->background = true;     // flag as background process
                cmd->argv[lpos][wpos]='\0';
                lpos--;                     // fix count
                goto done;
            }

            // Make sure we are not redirecting
            if (in_redirect || out_redirect) {
                fprintf(stderr, "lusush: error near character " \
                        "%u --> '%c'\n", i, c);
                return -1;
            }

            // Copy the character
            cmd->argv[lpos][wpos] = c;
            wpos++;
            break;
        case '<':
            if (!in_quote) {
                cmd->in_redirect = true;    // flag input redirection
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
                cmd->out_redirect = true;   // flag output redirection
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
        case '"':                           // quotaion
            if (in_quote)                   // outside
                in_quote = false;           // turn off flag.
            else                            // inside
                in_quote = true;            // turn on flag
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
            
            /////////////////////////////////////////////////
            // Whitespace characters
            /////////////////////////////////////////////////

        case ' ':
        case '\t':
        case '\n':
        case '\r':
            // If inside a quote we do not ignore spaces
            if (in_quote && !in_redirect && !out_redirect) {
                cmd->argv[lpos][wpos] = c;
                wpos++;
                break;
            }

        // Ignore all characters untill first non-whitespace
        while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            i++;
            c = line[i];
        }
        i--;        // decrement i to not skip non-space character

        if (!lpos && !read_reg)
            break;

        // Write to appropriate buffer
        if (in_redirect) {
            cmd->in_filename[wpos] = '\0';
            in_redirect = false;
        }
        else if (out_redirect) {
            cmd->out_filename[wpos] = '\0';
            out_redirect = false;
        }
        else {
            cmd->argv[lpos][wpos] = '\0';   // terminate string
        }

        lpos++;                             // increment line offset
        wpos = 0;                           // reset character offset

        // Allocate room on the heap for the next string
        cmd->argv[lpos] = calloc(MAXLINE, sizeof(char));
        if (cmd->argv[lpos] == NULL) {
            perror("lusush: calloc");
            for (j = lpos - 1; j >= 0; j--) {
                free(cmd->argv[j]);
                cmd->argv[j] = NULL;
            }
            return -1;
        }
        cmd->argv[lpos][wpos] = '\0';       // initialize with 0
        cmd->argc++;
        break;

        /////////////////////////////////////////////////
        // Normal characters
        /////////////////////////////////////////////////

        default:
            if (!read_reg)
                read_reg = true;            // normal char read in

            // copy the character
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
            break;
        }
    }

 done:
    lpos++;
    cmd->argv[lpos] = NULL;             // Make sure last element is 0
    cmd->argc = lpos;                   // set cmd->argc equal to lpos

    return lpos;                        // return number of lines parsed
}


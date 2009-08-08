#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ldefs.h"
#include "ltypes.h"
#include "parse.h"

/**
 * parse_cmd:
 *      Given a string of input parse_cmd will seperate words by whitespace
 *      and place each individual word into it's own string inside of a pointer
 *      to pointer char, called argv, which should already be initialized.
 *      parse_cmd __DOES NOT__ alloc cmd->argv, only additional strings.
 *      Special characters like &, <, and > have special cases and are dealt
 *      with according to their meaning, setting appropriate flags and filling
 *      appropriate buffers with information.
 */
int parse_cmd(CMD *cmd, const char *line)
{
    unsigned int i, j;
    unsigned int lpos, wpos;
    bool in_redirect;
    bool out_redirect;
    char c;

    if (!line || !*line)
        return -1;

    // initialize our integral variables to zero
    i = j = 0;
    lpos = wpos = 0;
    in_redirect = out_redirect = false;

    /*
     * Loop through line character at a time and place words, seperated by
     * whitespace, into individual elements in an array of strings.
     */
    for (i = 0; i < strlen(line); i++) {
        c = line[i];
 
        // c is a special character
        if (c == '&') {
            cmd->background = true;         // flag as background process
            cmd->argv[lpos][wpos]='\0';
            break;
        }
        else if (c == '<' && !cmd->in_redirect) {
            cmd->in_redirect = true;        // flag input redirection
            in_redirect = true;
            cmd->argv[lpos][wpos] = '\0';
        }
        else if (c == '>' && !cmd->out_redirect) {
            cmd->out_redirect = true;       // flag output redirection
            out_redirect = true;
            cmd->argv[lpos][wpos] = '\0';
        }
        // c is not whitespeace
        else if (c != ' ' && c != '\t' && c != '\n') {
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
        }
        // c is whitespace
        else {
            do {
                i++;
                c = line[i];
            }
            while (c == ' ' || c == '\t' || c == '\n');
            i--;// decrement i to not skip the next non-whitespace character

            if (in_redirect) {
                cmd->in_filename[wpos] = '\0';
                in_redirect = false;
            }
            else if (out_redirect) {
                cmd->out_filename[wpos] = '\0';
                out_redirect = false;
            }
            else {
                cmd->argv[lpos][wpos] = '\0';   // place NULL at end of string
            }
            lpos++;                         // increment line index
            wpos = 0;                       // set word character index to 0
            // Allocate room on the heap for the next string
            if (line[i+1] != '\0') {
                cmd->argv[lpos] = (char *)calloc(MAXLINE, sizeof(char));
                if (cmd->argv[lpos] == NULL) {
                    perror("lusush: calloc");
                    for (j = lpos - 1; j >= 0; j--) {
                        free(cmd->argv[j]);
                        cmd->argv[j] = NULL;
                    }
                    return -1;
                }
                cmd->argv[lpos][wpos] = '\0';   // initialize with NULL
            }
            cmd->argc++;
        }
    }

    for (i = 0; cmd->argv[i]; i++) {
        if (isspace(*cmd->argv[i])) {
            free(cmd->argv[i]);
            cmd->argv[i] = (char *)NULL;
        }
    }

    lpos++;
    cmd->argv[lpos] = (char *)NULL;
    cmd->argc = lpos;

    return lpos;
}


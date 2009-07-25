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
 *      with according to their meaning, setting appropriate flags.
 */
int parse_cmd(CMD *cmd, const char *line)
{
    unsigned int i, j;
    unsigned int lpos, wpos;
    char c;

    if (line == NULL)
        return -1;

    i = j = 0;
    lpos = wpos = 0;                // intialize our integers to zero

    /*
     * Loop through line character at a time and place words, seperated by
     * whitespace, into individual elements in an array of strings.
     * Special characters
     */
    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        if (c == '&') {
            cmd->background = true;         // flag as background process
            cmd->argv[lpos][wpos]='\0';
            break;
        }
        else if (c == '<') {
        }
        else if (c == '>') {
        }
        else if (c == '|') {
        }
        // c is not whitespeace
        else if (!isspace(c)) {
            cmd->argv[lpos][wpos] = c;       // copy the character
            wpos++;
        }
        // c is whitespace
        else {
            cmd->argv[lpos][wpos] = '\0';    // place NULL at end of string
            lpos++;                         // increment line index
            wpos = 0;                       // set word character index to 0

            // Allocate room on the heap for the next string
            cmd->argv[lpos] = (char *)calloc(MAXLINE, sizeof(char));
            if (cmd->argv[lpos] == NULL) {
                perror("lusush: calloc");
                for (j = lpos - 1; j >= 0; j--) {
                    free(cmd->argv[j]);
                    cmd->argv[j] = NULL;
                }
                return -1;
            }
            cmd->argv[lpos][wpos] = '\0';    // initialize new word with NULL

            do {
                i++;
                c = line[i];
                if (c != ' ' && c != '\t')
                    break;
            } while (1);
            i--;// decrement i to not skip the next non-whitespace character
        }
    }

    lpos++;
    cmd->argv[lpos] = NULL;
    cmd->argc = lpos;

    return lpos;
}


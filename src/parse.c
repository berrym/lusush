#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldefs.h"
#include "parse.h"

/**
 * parse_cmd:
 *      Given a string of input parse_cmd will seperate words by whitespace
 *      and place each individual word into it's own string inside of a pointer
 *      to pointer char, called wordlist, which should already be initialized.
 *      parse_cmd __DOES NOT__ alloc wordlist, only additional strings.
 */
int parse_cmd(char **wordlist, const char *line)
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
     */
    for (i = 0; i < strlen(line); i++) {
        c = line[i];

        // c is not whitespeace
        if (c != ' ' && c != '\t') {
            wordlist[lpos][wpos] = c;       // copy the character
            wpos++;
        }
        else {
            wordlist[lpos][wpos] = '\0';    // place NULL at end of string
            lpos++;                         // increment line index
            wpos = 0;                       // set word character index to 0

            // Allocate room on the heap for the next string
            wordlist[lpos] = (char *)calloc(MAXLINE, sizeof(char));
            if (wordlist[lpos] == NULL) {
                perror("lusush: calloc");
                for (j = lpos - 1; j >= 0; j--) {
                    free(wordlist[j]);
                    wordlist[j] = NULL;
                }
                return -1;
            }
            wordlist[lpos][wpos] = '\0';    // initialize new word with NULL

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
    wordlist[lpos] = NULL;

    return lpos;
}


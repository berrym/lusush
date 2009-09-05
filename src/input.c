/**
 * input.c - input routines
 */

// include statements {{{

#include <stdio.h>                  // Needed for readline history to compile
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ldefs.h"
#include "input.h"
#include "cmdlist.h"
#include "env.h"
#include "parse.h"
#include "history.h"

// end of include statements }}}

// macros/defines/globals {{{

#ifdef USING_READLINE
static char *line_read = (char *)NULL;      // storage for readline
#endif

// end of macros/define/globals }}}

// function rl_gets {{{

#if defined( USING_READLINE )
/**
 * rl_gets:
 *      Read a string, and return a pointer to it.  Returns NULL on EOF.
 */
char *rl_gets(const char *prompt)
{
    // If the buffer has already been allocated,
    // return the memory to the free pool
    if (line_read) {
        free(line_read);
        line_read = (char *)NULL;
    }

    // Get a line from the user
    line_read = readline(prompt);

    // If the line has any text in it, save it on the history
    if (line_read && *line_read)
        add_history(line_read);

    return (line_read);
}
#endif

// end of rl_gets }}}

// function get_input {{{

/**
 * get_input:
 *      return a pointer to a line of user input, store line in history
 */
char *get_input(FILE *in, const char *prompt)
{
    char *buf = (char *)NULL;          // input buffer

#ifdef USING_READLINE
    if (SHELL_TYPE != NORMAL_SHELL) {
        if ((buf = rl_gets(prompt)) == (char *)NULL) {
            return (char *)NULL;
        }
    }
    else {
        if ((buf = (char *)calloc(MAXLINE, sizeof(char))) == (char *)NULL) {
            perror("lusush: calloc");
            return (char *)NULL;
        }

        if (fgets(buf, MAXLINE, in) == (char *)NULL)
            return (char *)NULL;

        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
    }
#else
    if ((buf = (char *)calloc(MAXLINE, sizeof(char))) == (char *)NULL) {
        perror("lusush: calloc");
        return (char *)NULL;
    }

    printf("%s", prompt);

    if (fgets(buf, MAXLINE, in) == (char *)NULL)
        return (char *)NULL;

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';

    strcpy(hist_list[hist_size], buf);
    hist_size++;
#endif

    return buf;
}

// end get_input }}}

// function do_line {{{

/**
 * do_line:
 *      (line) is parsed and the information is stored in doubly linked list
 *      of commands, that is a CMDLIST of CMD's. (see ltypes.h)
 */
int do_line(char *line, CMDLIST *cmdl, CMD *cmd)
{
    unsigned int cnt = 0;               // Number of commands parsed
    int ret = 0;                        // Storage for return values
    int i = 0, j = 0;                   // loop variables
    bool pipe = false;                  // pipe chain flag

    // Storage for first tier of tokens (";")
    char *tok = (char *)NULL, *ptr1 = (char *)NULL, *savep1 = (char *)NULL;
    // Storage for secondary tier of tokens ("|")
    char *subtok = (char *)NULL, *ptr2 = (char *)NULL, *savep2 = (char *)NULL;

    char tmp[MAXLINE] = { '\0' };       // copy of line to mangle with strtok_r

    if (!line)
        return -1;
    if (!*line)
        return 0;

    strncpy(tmp, line, MAXLINE);        // copy string
    cmdl->size++;                       // increase cmdl size counter

    for (i = 0, ptr1 = tmp ;; i++, ptr1 = NULL) {
        if (!(tok = strtok_r(ptr1, ";", &savep1))) {
            break;
        }

        // Remove trailing whitespace
        if (strlen(tok) >= 1 && isspace((int)tok[strlen(tok) - 1])) {
            while (strlen(tok) >= 1 && isspace((int)tok[strlen(tok) - 1])) {
                tok[strlen(tok) - 1] = '\0';
            }
        }

        for (j = 0, ptr2 = tok ;; j++, ptr2 = NULL) {
            if (!(subtok = strtok_r(ptr2, "|", &savep2))) {
                pipe = false;
                break;
            }
            // Remove trailing whitespace
            if (strlen(subtok) >= 1 &&
                    isspace((int)subtok[strlen(subtok) - 1])) {
                while (strlen(subtok) >= 1 &&
                        isspace((int)subtok[strlen(subtok) - 1])) {
                    subtok[strlen(subtok) - 1] = '\0';
                }
            }

            strcpy(cmd->buf, subtok);           // Copy the string
            timestamp_cmd(cmd);                 // date it

            if (cmdalloc(cmd) < 0) {
                return -1;
            }

            if (j && j % 2 && j < 3) {
                print_debug("****do pipe %s\n", subtok);
                cmd->prev->pipe = true;
                cmd->prev->pchain_master = true;
                pipe = true;
            }

            if (pipe) {
                cmd->pipe = true;
            }

            switch (ret = parse_cmd(cmd, subtok)) {
                case -1:
                case 0:
                    return ret;
                default:
                    cmd->next->prev = cmd;
                    cmd = cmd->next;
                    cmd->next = (CMD *)NULL;
                    cnt++;
            }
        }
    }

    return cnt;
}

// end of do_line }}}


// vim:filetype=c foldmethod=marker autoindent expandtab shiftwidth=4

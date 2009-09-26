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
#include "misc.h"

// end of include statements }}}

// macros/defines/globals {{{

#ifdef USING_READLINE
static char *line_read = (char *)0;      // storage for readline
#endif

// end of macros/define/globals }}}

// function rl_gets {{{

#if defined( USING_READLINE )
/**
 * rl_gets:
 *      Read a string, and return a pointer to it.  Returns 0 on EOF.
 */
char *rl_gets(const char *prompt)
{
    // If the buffer has already been allocated,
    // return the memory to the free pool
    if (line_read) {
        free(line_read);
        line_read = (char *)0;
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
    char *buf = (char *)0;          // input buffer

#ifdef USING_READLINE
    if (SHELL_TYPE != NORMAL_SHELL) {
        if ((buf = rl_gets(prompt)) == (char *)0) {
            return (char *)0;
        }
    }
    else {
        if ((buf = (char *)calloc(MAXLINE, sizeof(char))) == (char *)0) {
            perror("lusush: calloc");
            return (char *)0;
        }

        if (fgets(buf, MAXLINE, in) == (char *)0)
            return (char *)0;

        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
    }
#else
    if ((buf = (char *)calloc(MAXLINE, sizeof(char))) == (char *)0) {
        perror("lusush: calloc");
        return (char *)0;
    }

    printf("%s", prompt);

    if (fgets(buf, MAXLINE, in) == (char *)0)
        return (char *)0;

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';

    strncpy(hist_list[hist_size], buf, MAXLINE);
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
 *
 * TODO: currently using strtok_r to split line into seperate commands when
 *       processing a chained commands.  This is a  bad thing.  Write
 *       do_line so that the strtok family of functions is not used.
 */
int do_line(char *line, CMDLIST *cmdl, CMD *cmd)
{
    unsigned int cnt = 0;               // Number of commands parsed
    int ret = 0;                        // Storage for return values
    int i = 0, j = 0;                   // loop variables
    bool pipe = false;                  // pipe chain flag

    // Storage for first tier of tokens (";")
    char *tok = (char *)0, *ptr1 = (char *)0, *savep1 = (char *)0;
    // Storage for secondary tier of tokens ("|")
    char *subtok = (char *)0, *ptr2 = (char *)0, *savep2 = (char *)0;

    char tmp[MAXLINE] = { '\0' };       // copy of line to mangle with strtok_r

    if (!line)
        return -1;
    if (!*line)
        return 0;

    strncpy(tmp, line, MAXLINE);        // copy string
    cmdl->size++;                       // increase cmdl size counter

    for (i = 0, ptr1 = tmp ;; i++, ptr1 = 0) {
        if (!(tok = strtok_r(ptr1, ";", &savep1))) {
            break;
        }

        // Remove trailing whitespace
        if (strlen(tok) >= 1 && isspace((int)tok[strlen(tok) - 1])) {
            while (strlen(tok) >= 1 && isspace((int)tok[strlen(tok) - 1])) {
                tok[strlen(tok) - 1] = '\0';
            }
        }

        for (j = 0, ptr2 = tok ;; j++, ptr2 = 0) {
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

            strncpy(cmd->buf, subtok, MAXLINE);   // Copy the string
            timestamp_cmd(cmd);                     // date it

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
                    cmd->next = (CMD *)0;
                    cnt++;
            }
        }
    }

    return cnt;
}

// end of do_line }}}


// vim:filetype=c foldmethod=marker autoindent expandtab shiftwidth=4

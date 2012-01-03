/*
 * input.c - input routines
 */
#ifdef USING_READLINE
#include <stdio.h>                  // Needed for readline history to compile
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ldefs.h"
#include "input.h"
#include "expand.h"
#include "cmdlist.h"
#include "init.h"
#include "parse.h"
#include "history.h"
#include "misc.h"

#define DBGSTR "DEBUG: input.c: "

static char *line_read = NULL;      // storage for readline and fgets

#ifdef USING_READLINE
/*
 * rl_gets:
 *      Read a string, and return a pointer to it.  Returns 0 on EOF.
 */
char *rl_gets(const char *prompt)
{
    // If the buffer has already been allocated,
    // return the memory to the free pool
    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    // Get a line from the user
    line_read = readline(prompt);

    // If the line has any text in it, save it on the history
    if (line_read && *line_read)
        add_history(line_read);

    return line_read;
}
#endif

/*
 * get_input:
 *      return a pointer to a line of user input, store line in history
 */
char *get_input(FILE *in, const char *prompt)
{
    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

#ifdef USING_READLINE
    char *tmp = calloc(MAXLINE, sizeof(char*));
    
    if (SHELL_TYPE != NORMAL_SHELL) {
        if ((line_read = rl_gets(prompt)) == NULL)
            return NULL;
    }
    else {
        if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL) {
            perror("lusush: calloc");
            return NULL;
        }

        if (fgets(line_read, MAXLINE, in) == NULL)
            return NULL;

        if (line_read[strlen(line_read) - 1] == '\n')
            line_read[strlen(line_read) - 1] = '\0';
    }

    strncpy(tmp, line_read, MAXLINE);
    expand(tmp);
    print_v("%sexpanded_line=%s\n", DBGSTR, tmp);
    if (strcmp(tmp, line_read) != 0) {
        free(line_read);
        if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL) {
            perror("lusush: calloc");
            return NULL;
        }
        strncpy(line_read, tmp, MAXLINE);
    }

    if (tmp) {
        memset(tmp, '\0', MAXLINE);
        free(tmp);
        tmp = NULL;
    }
#else
    if ((line_read = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: calloc");
        return NULL;
    }

    if (SHELL_TYPE != NORMAL_SHELL)
        printf("%s", prompt);

    if (fgets(line_read, MAXLINE, in) == NULL)
        return NULL;

    if (line_read[strlen(line_read) - 1] == '\n')
        line_read[strlen(line_read) - 1] = '\0';

    strncpy(hist_list[hist_size], line_read, MAXLINE);
    hist_size++;

    expand(line_read);
    print_v("%sexpanded_line=%s\n", DBGSTR, line_read);
#endif

    return line_read;
}

/*
 * do_line:
 *      (line) is parsed and the information is stored in doubly linked list
 *      of commands, that is a CMDLIST of CMDs. (see ltypes.h)
 *
 * TODO: currently using strtok_r to split line into seperate commands when
 *       processing a chained commands.  This is a  bad thing.  Write
 *       do_line so that the strtok family of functions is not used.
 */
int do_line(char *line, CMD *cmd)
{
    size_t cnt = 0;                     // Number of commands parsed
    int ret = 0;                        // Storage for return values
    int i = 0, j = 0;                   // loop variables
    bool pipe = false;                  // pipe chain flag

    // Storage for first tier of tokens (";")
    char *tok = NULL, *ptr1 = NULL, *savep1 = NULL;
    // Storage for secondary tier of tokens ("|")
    char *subtok = NULL, *ptr2 = NULL, *savep2 = NULL;

    //char tmp[MAXLINE] = { '\0' };       // copy of line to mangle with strtok_r
    char *tmp = calloc(MAXLINE, sizeof(char*));

    if (!line)
        return -1;
    if (!*line)
        return 0;

    strncpy(tmp, line, MAXLINE);        // copy string

    for (i = 0, ptr1 = tmp ;; i++, ptr1 = 0) {
        if (!(tok = strtok_r(ptr1, ";", &savep1)))
            break;

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

            if (cmdalloc(cmd) < 0)
                return -1;

            strncpy(cmd->buf, subtok, MAXLINE);     // Copy the string
            timestamp_cmd(cmd);                     // date it

            if (j == 1) {
                print_v("****do pipe %s\n", subtok);
                cmd->prev->pipe = true;
                cmd->prev->pchain_master = true;
                pipe = true;
            }

            if (pipe)
                cmd->pipe = true;

            switch (ret = parse_cmd(cmd, subtok)) {
            case -1:
            case 0:
                return ret;
            default:
                cmd->next->prev = cmd;
                cmd = cmd->next;
                cmd->next = NULL;
                cnt++;
            }
        }
    }

    if (tmp) {
        memset(tmp, '\0', MAXLINE);
        free(tmp);
        tmp = NULL;
    }

    return cnt;
}

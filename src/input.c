#include <stdio.h>                  // Needed for readline history to compile
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ldefs.h"

#if defined( USING_READLINE )
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "input.h"
#include "cmdlist.h"
#include "env.h"
#include "parse.h"

#if defined( USING_READLINE )
static char *line_read = (char *)NULL;

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

/**
 * get_input:
 *      reads input one line at a time from the stream in.  The line
 *      is parsed and the information is stored in doubly linked listed
 *      of commands, that is a CMDLIST of CMD's.
 */
int get_input(FILE *in, CMDLIST *cmdl, CMD *cmd)
{
    unsigned int cnt = 0;
    int ret = 0, i = 0, j = 0;
    bool pipe = false;
    char *tok = (char *)NULL, *ptr1 = (char *)NULL, *savep1 = (char *)NULL;
    char *subtok = (char *)NULL, *ptr2 = (char *)NULL, *savep2 = (char *)NULL;
    char tmp[MAXLINE] = { '\0' };
#if defined( USING_READLINE )
    char *buf = (char *)NULL;
#else
    char buf[MAXLINE] = { '\0' };
#endif

#if defined( USING_READLINE )
    if ((buf = rl_gets((ENV_PROMPT = getenv("PROMPT"))
                        ? ENV_PROMPT : "% ")) == (char *)NULL)
        return -1;
#else
    if (fgets(buf, MAXLINE, in) == (char *)NULL)
        return -1;

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';
#endif

    strncpy(tmp, buf, MAXLINE);
    cmdl->size++;

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

            if (j && (j % 2) && j < 3) {
                print_debug("****do pipe %s\n", subtok);
                cmd->prev->pipe = true;
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

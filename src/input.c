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
    int ret;
#if defined( USING_READLINE )
    char *buf;
    //const char *prompt = getenv("PROMPT");
#else
    char buf[MAXLINE];
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

    // Remove trailing whitespace
    if (strlen(buf) > 1 && isspace(buf[strlen(buf) - 1])) {
        do {
            buf[strlen(buf) - 1] = '\0';
        }
        while (buf[strlen(buf) - 1] && isspace(buf[strlen(buf) - 1]));
    }
    strcpy(cmd->buf, buf);              // Copy the string
    timestamp_cmd(cmd);                 // date it

    if (cmdalloc(cmd) < 0) {
        return -1;
    }
    if ((ret = parse_cmd(cmd, buf)) < 0) {
        return -1;
    }
    else if (ret == 0)
    {
        return 0;
    }

    cmd->next->prev = cmd;
    cmd = cmd->next;
    cmd->next = (CMD *)NULL;

    cmdl->size++;

    return ret;
}

#include <stdio.h>                  // Needed for readline history to compile
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "input.h"
#include "cmdlist.h"
#include "env.h"
#include "parse.h"

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

/**
 * get_input:
 *      reads input one line at a time from the stream in.  The line
 *      is parsed and the information is stored in doubly linked listed
 *      of commands, that is a CMDLIST of CMD's.
 */
int get_input(FILE *in, CMDLIST *cmdl, CMD *cmd)
{
    int ret;
    char *buf;
    const char *prompt = getenv("PROMPT");
    //char buf[MAXLINE];
    /*
    if (fgets(buf, MAXLINE, in) == NULL)
        return -1;

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';
    */
    if ((buf = rl_gets(prompt)) == (char *)NULL)
        return 1;

    strcpy(cmd->buf, buf);
    timestamp_cmd(cmd);

    if (cmdalloc(cmd) == -1) {
        return -1;
    }
    if ((ret = parse_cmd(cmd, buf)) < 0) {
        return -1;
    }

    cmd->next->prev = cmd;
    cmd = cmd->next;
    cmd->next = NULL;

    cmdl->size++;

    return cmdl->size;
}

#include <stdlib.h>
#include <string.h>
#include "input.h"
#include "cmdlist.h"
#include "parse.h"

/**
 * readline
 */
int get_input(FILE *in, CMDLIST *cmdl, CMD *cmd)
{
    char buf[MAXLINE];

    if (fgets(buf, MAXLINE, in) == NULL)
        return -1;

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';

    strcpy(cmd->buf, buf);
    timestamp_cmd(cmd);

    if (cmdalloc(cmd) == -1) {
        return -1;
    }
    if ((cmd->argc = parse_cmd(cmd->argv, buf)) < 0) {
        return -1;
    }

    cmd->next->prev = cmd;
    cmd = cmd->next;
    cmd->next = NULL;

    cmdl->size++;

    return cmdl->size;
}

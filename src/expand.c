/*
 * expand.c - input expansion routines for lusush
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldefs.h"
#include "ltypes.h"
#include "expand.h"
#include "alias.h"
#include "builtins.h"
#include "misc.h"

#define DBGSTR "DEBUG: expand.c: "

static char *expanded = NULL;

void expand(char *line)
{
    char tmp[MAXLINE] = { '\0' };
    char prv[MAXLINE] = { '\0' };
    char *tok = NULL;
    char *ea = NULL;

    if (!line || !*line)
        return;

    if (expanded) {
        free(expanded);
    }

    if ((expanded = calloc(MAXLINE, sizeof(char))) == NULL) {
        perror("lusush: expand.c: expand");
        return;
    }
    *expanded = '\0';

    strncpy(tmp, line, MAXLINE);

    if (!(tok = strtok(tmp, " ")))
        return;

    if (strcmp(tok, builtins[BUILTIN_CMD_UNALIAS]) == 0)
        return;

    while (tok) {
        if (*prv && (strncmp(prv, "unalias", 8) == 0)) {
            ;
        }
        ea = expand_alias(tok);
        print_debug("%sexpand: tok=%s ea=%s\n", DBGSTR, tok, ea);
        strncat(expanded, ea ? ea : tok, MAXLINE);
        strncat(expanded, " ", 2);
        strncpy(prv, tok, MAXLINE);
        tok = strtok(NULL, " ");
    }

    print_debug("%sexpand: expanded=%s\n", DBGSTR, expanded);

    strncpy(line, expanded, MAXLINE);
}

/**
 * expand.c - input expansion routines for lusush
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lusush.h"
#include "expand.h"
#include "alias.h"
#include "builtins.h"
#include "misc.h"

#define DBGSTR "DEBUG: expand.c: "

static char *expanded = NULL;

/**
 * expand:
 *      perform word expansion
 */
void expand(char *line)
{
    char tmp[BUFSIZ] = { '\0' };
    char prv[BUFSIZ] = { '\0' };
    char *tok = NULL;
    char *ea = NULL;

    if (!line || !*line)
        return;

    if (expanded) {
        free(expanded);
    }

    if ((expanded = calloc(BUFSIZ, sizeof(char))) == NULL) {
        perror("lusush: expand.c: expand");
        return;
    }
    *expanded = '\0';

    strncpy(tmp, line, BUFSIZ);

    if (!(tok = strtok(tmp, " ")))
        return;

    if (strcmp(tok, builtins[BUILTIN_CMD_UNALIAS]) == 0)
        return;

    while (tok) {
        if (*prv && (strncmp(prv, "unalias", 8) == 0)) {
            ;
        } else {
            ea = expand_alias(tok);
            vprint("%sexpand: tok=%s ea=%s\n", DBGSTR, tok, ea);
            strncat(expanded, ea ? ea : tok, BUFSIZ);
            strncat(expanded, " ", 2);
            strncpy(prv, tok, BUFSIZ);
        };
        tok = strtok(NULL, " ");
    }

    vprint("%sexpand: expanded=%s\n", DBGSTR, expanded);

    strncpy(line, expanded, BUFSIZ);
}

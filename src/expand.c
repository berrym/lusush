/**
 * expand.c - input expansion routines for lusush
 *
 * Copyright (c) 2009-2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
void expand(char *const line)
{
    char tmp[BUFFSIZE] = { '\0' };
    char prv[BUFFSIZE] = { '\0' };
    char *tok = NULL;
    char *ea = NULL;

    if (!line || !*line)
        return;

    if (expanded)
        free(expanded);

    if ((expanded = calloc(BUFFSIZE, sizeof(char))) == NULL) {
        perror("lusush: expand.c: expand: calloc");
        return;
    }
    *expanded = '\0';

    strncpy(tmp, line, BUFFSIZE);

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
            strncat(expanded, ea ? ea : tok, BUFFSIZE);
            strncat(expanded, " ", 2);
            strncpy(prv, tok, BUFFSIZE);
        };
        tok = strtok(NULL, " ");
    }

    vprint("%sexpand: expanded=%s\n", DBGSTR, expanded);

    strncpy(line, expanded, BUFFSIZE);
}

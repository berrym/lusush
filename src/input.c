/**
 * input.c - input routines
 */
#ifdef HAVE_LIBREADLINE
#include <stdio.h>                  // Needed for readline history to compile
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lusush.h"
#include "input.h"
#include "expand.h"
#include "cmdlist.h"
#include "init.h"
#include "parse.h"
#include "history.h"
#include "misc.h"

#define DBGSTR "DEBUG: input.c: "

static char *line_read = NULL;      // storage for readline and fgets

#ifdef HAVE_LIBREADLINE
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

/**
 * get_input:
 *      return a pointer to a line of user input, store line in history
 */
char *get_input(FILE *in, const char *prompt)
{
    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

#ifdef HAVE_LIBREADLINE
    char *tmp = calloc(BUFSIZ, sizeof(char));
    
    if (SHELL_TYPE != NORMAL_SHELL) {
        if ((line_read = rl_gets(prompt)) == NULL) {
            if (tmp) {
                memset(tmp, '\0', BUFSIZ);
                free(tmp);
                tmp = NULL;
            }
            return NULL;
        }
    }
    else {
        if ((line_read = calloc(BUFSIZ, sizeof(char))) == NULL) {
            perror("lusush: calloc");
            if (tmp) {
                memset(tmp, '\0', BUFSIZ);
                free(tmp);
                tmp = NULL;
            }
            return NULL;
        }

        if (fgets(line_read, BUFSIZ, in) == NULL) {            
            if (tmp) {
                memset(tmp, '\0', BUFSIZ);
                free(tmp);
                tmp = NULL;
            }
            return NULL;
        }
        
        if (line_read[strlen(line_read) - 1] == '\n')
            line_read[strlen(line_read) - 1] = '\0';
    }

    strncpy(tmp, line_read, BUFSIZ);
    expand(tmp);
    vprint("%sexpanded_line=%s\n", DBGSTR, tmp);
    if (strcmp(tmp, line_read) != 0) {
        free(line_read);
        if ((line_read = calloc(BUFSIZ, sizeof(char))) == NULL) {
            perror("lusush: calloc");
            return NULL;
        }
        strncpy(line_read, tmp, BUFSIZ);
    }

    if (tmp) {
        memset(tmp, '\0', BUFSIZ);
        free(tmp);
        tmp = NULL;
    }
#else
    if ((line_read = calloc(BUFSIZ, sizeof(char))) == NULL) {
        perror("lusush: calloc");
        return NULL;
    }

    if (SHELL_TYPE != NORMAL_SHELL)
        printf("%s", prompt);

    if (fgets(line_read, BUFSIZ, in) == NULL)
        return NULL;

    if (line_read[strlen(line_read) - 1] == '\n')
        line_read[strlen(line_read) - 1] = '\0';

    strncpy(hist_list[hist_size], line_read, BUFSIZ);
    hist_size++;

    expand(line_read);
    vprint("%sexpanded_line=%s\n", DBGSTR, line_read);
#endif

    return line_read;
}

/**
 * do_line:
 *      (line) is parsed and the information is stored in a doubly linked list
 *      of commands, that is a CMDLIST of CMDs. (see ltypes.h)
 */
int do_line(char *restrict line, CMD *restrict cmd)
{
    size_t cnt = 0;                     // Number of commands parsed
    int err = 0;                        // error code
    int i = 0, j = 0;                   // loop variables
    bool pipe = false;                  // pipe chain flag

    // Storage for first tier of tokens (";")
    char *tok = NULL, *ptr1 = NULL, *savep1 = NULL;
    // Storage for secondary tier of tokens ("|")
    char *subtok = NULL, *ptr2 = NULL, *savep2 = NULL;
    // buffer for a copy of line to mangle with strtok_r
    char *tmp = calloc(BUFSIZ, sizeof(char));

    if (!line) {
        err = -1;
        goto cleanup;
    }
    
    if (!*line) {
        err = 0;
        goto cleanup;
    }

    strncpy(tmp, line, BUFSIZ);        // copy string

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

            if (cmdalloc(cmd) < 0) {
                err = -1;
                goto cleanup;
            }

            strncpy(cmd->buf, subtok, strlen(cmd->buf));     // Copy the string

            if (j == 1) {
                vprint("****do pipe %s\n", subtok);
                cmd->prev->pipe = true;
                cmd->prev->pipe_head = true;
                pipe = true;
            }

            if (pipe)
                cmd->pipe = true;

            switch (err = parse_cmd(cmd, subtok)) {
            case -1:
            case 0:
                goto cleanup;
            default:
                cmd->next->prev = cmd;
                cmd = cmd->next;
                cmd->next = NULL;
                cnt++;
            }
        }
    }

 cleanup:
    if (tmp) {
        memset(tmp, '\0', BUFSIZ);
        free(tmp);
        tmp = NULL;
    }

    /* if (ptr1) { */
    /*     memset(ptr1, '\0', BUFSIZ); */
    /*     free(ptr1); */
    /*     ptr1 = NULL; */
    /* } */

    if (ptr2) {
        memset(ptr2, '\0', BUFSIZ);
        free(ptr2);
        ptr2 = NULL;
    }

    switch (err) {
    case -1:
    case 0:
        return err;
    default:
        return cnt;
    }
}

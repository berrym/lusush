/*
 * history.c - routines to work with basic history
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ldefs.h"
#include "ltypes.h"
#include "history.h"
#include "env.h"

static bool HIST_INITIALIZED = false;

char histfile[MAXLINE] = { '\0' };
FILE *histfp = (FILE *)0;

#ifndef USING_READLINE
long hist_size = 0;
char hist_list[MAXHIST][MAXLINE] = { "\0" };
#endif

#ifdef USING_READLINE
HIST_ENTRY **hist_list;
#endif

int read_histfile(const char *histfile)
{
#ifdef USING_READLINE
    if (read_history(histfile) != 0) {
        perror("lusush: read_hist");
        return errno;
    }
#else
    int i = 0;

    if ((histfp = fopen(histfile, "r")) == (FILE *)0) {
        if (errno != ENOENT)
            perror("lusush: fopen");
        return -1;
    }

    for (i = 0; i < MAXHIST && hist_list[i]; i++) {
        if (fgets(hist_list[i], MAXLINE, histfp) == NULL) {
            break;
        }

        if (hist_list[i][strlen(hist_list[i]) - 1] == '\n') {
            hist_list[i][strlen(hist_list[i]) - 1] = '\0';
        }
    }

    fclose(histfp);
#endif

    return 0;
}

void init_history(void)
{
    if (HIST_INITIALIZED) {
        fprintf(stderr,"lusush: init_history: history already initialized.\n");
        return;
    }

    // Ugly non buffered I/O and readline history stuff {{{

#ifndef USING_READLINE
    // Set stdout, stdin, and stderr to a non-buffered state
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#else
    using_history();
    stifle_history(MAXHIST);
#endif

    // end of ungliness }}}

    if (!*histfile) {
        strncpy(histfile, ENV_HOME, MAXLINE);
        strncat(histfile, "/.lusushist", 12);
    }

    if (read_histfile(histfile) != 0) {
        return;
    }

    HIST_INITIALIZED = true;
}

void write_histfile(const char *histfile)
{
#ifdef USING_READLINE
    write_history(histfile);
#else
    int i = 0;

    if ((histfp = fopen(histfile, "a")) == (FILE *)0) {
        perror("lusush: fopen");
        return;
    }

    for (i = 0; hist_list[i] && *hist_list[i]; i++) {
        fprintf(histfp, "%s\n", hist_list[i]);
    }

    fclose(histfp);
#endif
}


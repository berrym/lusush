#include <stdio.h>
#include "ldefs.h"
#include "ltypes.h"

#ifdef USING_READLINE
#include <readline/history.h>
#endif

#ifndef __HISTORY_H__
#define __HISTORY_H__

#ifdef USING_READLINE
extern HIST_ENTRY **hist_list;
extern char *rl_gets(const char *);
#endif

#ifndef USING_READLINE
extern long hist_size;
extern char hist_list[MAXHIST][MAXLINE];
#endif

extern char histfile[MAXLINE];
extern FILE *histfp;

extern int read_histfile(const char *);
extern void init_history(void);
extern void write_histfile(const char *);

#endif

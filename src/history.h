#include <stdio.h>
#include "lusush.h"

#ifdef USING_READLINE
#include <readline/history.h>
#endif

#ifndef __HISTORY_H__
#define __HISTORY_H__

#ifdef USING_READLINE
extern HIST_ENTRY **hist_list;
#else
extern long hist_size;
extern char hist_list[MAXHIST][BUFSIZ];
#endif

extern int read_histfile(const char *);
extern void init_history(void);
extern void write_histfile(const char *);
extern char *histfilename(void);
#endif

#include "ltypes.h"

#ifndef __CMDLIST_H__
#define __CMDLIST_H__

extern int cmdalloc(CMD *);
extern void cmdfree(CMD *);
extern void free_cmdlist(CMDLIST *);
extern void display_cmdlist(CMDLIST *);
extern void display_cmd(CMD *);

#endif

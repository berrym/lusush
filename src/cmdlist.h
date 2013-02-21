#include "lusush.h"

#ifndef __CMDLIST_H__
#define __CMDLIST_H__

extern int cmdalloc(CMD *);
extern void cmdfree(CMD *);
extern void free_cmdlist(CMD *);
extern void display_cmd(CMD *);

#endif

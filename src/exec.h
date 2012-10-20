#include "lusush.h"

#ifndef __EXEC_H__
#define __EXEC_H__

extern int exec_cmd(CMD *, int);
extern int exec_external_cmd(CMD *);
extern void exec_builtin_cmd(int, CMD *);

#endif

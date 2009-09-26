#include "ltypes.h"

#ifndef __EXEC_H__
#define __EXEC_H__

extern int exec_cmd(CMD *, int);
extern int exec_external_cmd(CMD *, char **);
extern void exec_builtin_cmd(int, CMD *);
extern char *path_to_cmd(char *);

#endif

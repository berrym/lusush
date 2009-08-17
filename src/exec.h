#include "ltypes.h"

#ifndef __EXEC_H__
#define __EXEC_H__

extern void exec_cmd(CMD *);
extern void exec_external_cmd(CMD *, char **);
extern void exec_builtin_cmd(int, CMD *);
extern int is_builtin_cmd(const char *);
extern char *path_to_cmd(char *);

#endif

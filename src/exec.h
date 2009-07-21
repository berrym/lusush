#include "ltypes.h"

#ifndef __EXEC_H__
#define __EXEC_H__

extern void exec_cmdl(CMDLIST *, char **);
extern void exec_external_cmd(CMD *, char **);
extern void exec_builtin_cmd(CMD *);
extern bool is_builtin_cmd(const char *);
extern char *path_to_cmd(char *);

#endif

#include "ldefs.h"
#include "ltypes.h"

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

/**
 * Builtin commands
 *      exit        exit program
 *      help        print help
 *      cd          change directory
 *      pwd         print working directory
 *      history     print command history
 *      setenv      set environment variable
 *      unsetenv    unset environment variable
 *      alias       set an alias
 *      unalias     unset an alias
 *      setopt      turn on an option
 *      unsetopt    turn off an option
 *      setprompt   set prompt colors
 */

enum {
        BUILTIN_CMD_EXIT=0,
        BUILTIN_CMD_HELP=2,
        BUILTIN_CMD_CD=4,
        BUILTIN_CMD_PWD=6,
        BUILTIN_CMD_HISTORY=8,
        BUILTIN_CMD_SETENV=10,
        BUILTIN_CMD_UNSETENV=12,
        BUILTIN_CMD_ALIAS=14,
        BUILTIN_CMD_UNALIAS=16,
        BUILTIN_CMD_SETOPT=18,
        BUILTIN_CMD_UNSETOPT=20,
        BUILTIN_CMD_SETPROMPT=22,
        BUILTIN_CMD_CNT=24
};

extern const char *builtins[BUILTIN_CMD_CNT];

extern int is_builtin_cmd(const char *);
extern void help(const char *);
extern void cd(const char *);
extern void pwd(void);

extern void history(void);

#endif

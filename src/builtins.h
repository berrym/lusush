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
 */

enum {  BUILTIN_CMD_EXIT=0,
        BUILTIN_CMD_HELP=2,
        BUILTIN_CMD_CD=4,
        BUILTIN_CMD_PWD=6,
        BUILTIN_CMD_HISTORY=8,
        BUILTIN_CMD_SETENV=10,
        BUILTIN_CMD_UNSETENV=12,
        BUILTIN_CMD_CNT=14
};

extern const char *builtins[BUILTIN_CMD_CNT];

extern void help(const char *);
extern void cd(const char *);
extern void pwd(void);

extern void history(void);

#endif

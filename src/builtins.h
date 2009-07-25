#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#ifdef BUILTIN_CMD_CNT
#undef BUILTIN_CMD_CNT
#endif
#define BUILTIN_CMD_CNT 3

#ifdef BUILTIN_CMD_CD
#undef BUILTIN_CMD_CD
#endif
#define BUILTIN_CMD_CD 1

#ifdef BUILTIN_CMD_PWD
#undef BUILTIN_CMD_PWD
#endif
#define BUILTIN_CMD_PWD 2

/**
 * Builtin commands
 *      exit    exit program
 *      cd      change directory
 *      pwd     print working directory
 */
extern const char *builtin_cmds[BUILTIN_CMD_CNT];

extern void cd(const char *);
extern void pwd(void);

#endif

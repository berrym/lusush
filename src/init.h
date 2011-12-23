#ifndef __INIT_H__
#define __INIT_H__

extern int SHELL_TYPE;

enum {
    LOGIN_SHELL=1,
    INTERACTIVE_SHELL,
    NORMAL_SHELL
};

extern int init(int, char **);
extern void sig_int(int);
extern void sig_seg(int);

#endif

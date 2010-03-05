#ifndef __INIT_H__
#define __INIT_H__

extern int init(int, char **);
extern void sig_int(int);                      // interrupt handler
extern void sig_seg(int);                      // segfault handler

#endif

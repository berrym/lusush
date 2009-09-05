#include "../config.h"

#ifndef __LDEFS_H__
#define __LDEFS_H__

#ifdef MAXLINE
#undef MAXLINE
#endif
#define MAXLINE 4096        // maximum string length
#define MAXHIST 10000       // maximum length of history

//#define PRINT_DEBUG         // print debugging statements
//#define PRINT_DEBUG_CRAZY

#ifdef HAVE_LIBREADLINE
#define USING_READLINE      // use GNU Readline
#endif

#endif

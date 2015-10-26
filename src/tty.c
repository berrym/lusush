/**
 * tty.c - routines to work with the controlling terminal
 *
 * Copyright (c) 2015 Michael Berry <trismegustis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tty.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

int ttyfd = -1;                 // tty file descriptor
bool tty_devtty;                // flag that we have a tty
struct termios ts;              // tty attributes

/**
 * tty_init:
 *       Initialize tty_fd.  Used for saving/reseting tty modes upon
 *       foreground job completion and for setting up tty process group.
 */
void tty_init(bool init_ts)
{
    bool do_close = true;
    int fd;

    // Close old tty descriptor
    tty_close();
    tty_devtty = true;          // we have access to a tty

    // Open the controlling tty, usually /dev/tty
    if ((fd = open(ctermid(NULL), O_RDWR, 0)) < 0) {
        error_return("lusush: init.c: tty_init: open");
        tty_devtty = false;
        do_close = false;
        // Keep looking for a tty if one wasn't found
        if (isatty(0)) {
            fd = 0;
        }
        else if (isatty(2)) {
            fd = 2;
        }
        else {
            error_message("lusush: init.c: tty_init: "
                          "Can't find tty file descriptor");
            return;
        }
    }

    // Make a duplicate tty file descriptor with close on exec bit set
    if ((ttyfd = fcntl(fd, F_DUPFD_CLOEXEC, FDBASE)) < 0)
        error_return("lusush: init.c: tty_init: fcntl");
    else if (init_ts)
        if (tcgetattr(ttyfd, &ts) < 0)
            error_return("lusush: tty.c: tty_init: tcgetattr");

    // Close the tty
    if (do_close)
        if (close(fd) < 0)
            error_return("lusush: tty.c: tty_init: close");
}

/**
 * tty_close:
 *      Close controlling terminal.
 */
void tty_close(void)
{
    if (ttyfd >= 0) {
        if (close(ttyfd) < 0)
            error_return("lusush: tty.c: tty_close: close");
        ttyfd = -1;
    }
}

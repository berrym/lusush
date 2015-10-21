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

#include "lusush.h"
#include "tty.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

int tty_fd = -1;
int tty_devtty;
struct termios tty_state;

/**
 * tty_init:
 *       Initialize tty_fd.  Used for saving/reseting tty modes upon
 *       foreground job completion and for setting up tty process group.
 */
void tty_init(int init_ttystate)
{
    bool do_close = true;
    FILE *fp = NULL;

    if (tty_fd >= 0) {
        if (close(tty_fd) < 0)
            error_return("lusush: tty.c: tty_init: close");
        tty_fd = -1;
    }

    tty_devtty = 1;

    if ((fp = fopen(ctermid(NULL), "r+")) == NULL) {
        error_return("lusush: init.c: tty_init: fopen");
        tty_devtty = 0;
    }

    if (fileno(fp) < 0) {
        do_close = false;

        if (isatty(0)) {
            fp = fdopen(0, "r+");
        }
        else if (isatty(2)) {
            fp = fdopen(2, "r+");
        }
        else {
            error_message("lusush: init.c: tty_init: "
                          "Can't find tty file descriptor");
            return;
        }
    }

    if ((tty_fd = fcntl(fileno(fp), F_DUPFD_CLOEXEC, FDBASE)) < 0)
        error_return("lusush: init.c: tty_init: fcntl");
    else if (init_ttystate)
        if (tcgetattr(tty_fd, &tty_state) < 0)
            error_return("lusush: tty.c: tty_init: tcgetattr");

    if (do_close)
        if (fclose(fp) == EOF)
            error_return("lusush: tty.c: tty_init: fclose");
}

/**
 * tty_close:
 *      Close controlling terminal.
 */
void tty_close(void)
{
    if (tty_fd >= 0) {
        if (close(tty_fd) < 0)
            error_return("lusush: tty.c: tty_close: close");
        tty_fd = -1;
    }
}

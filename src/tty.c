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

int tty_fd = -1;                // tty file descriptor
bool tty_devtty;                // flag that we have a tty
struct termios tty_state;       // tty attributes

/**
 * tty_init:
 *       Initialize tty_fd.  Used for saving/reseting tty modes upon
 *       foreground job completion and for setting up tty process group.
 */
void tty_init(int init_ttystate)
{
    bool do_close = true;
    FILE *fp = NULL;

    // Close old tty descriptor
    tty_close();
    tty_devtty = true;          // we have access to a tty

    // Open the controlling tty, usually /dev/tty
    if ((fp = fopen(ctermid(NULL), "r+")) == NULL) {
        error_return("lusush: init.c: tty_init: fopen");
        tty_devtty = false;
    }

    // Keep looking for a tty if one wasn't found
    if (fileno(fp) < 0) {
        do_close = false;

        if (isatty(STDIN_FILENO)) {
            if ((fp = fdopen(STDIN_FILENO, "r+")) == NULL)
                error_return("lusush: tty.c: tty_init: fdopen");
        }
        else if (isatty(STDERR_FILENO)) {
            if ((fp = fdopen(STDERR_FILENO, "r+")) == NULL)
                error_return("lusush: tty.c: tty_init: fdopen");
        }
        else {
            error_message("lusush: init.c: tty_init: "
                          "Can't find tty file descriptor");
            return;
        }
    }

    // Make a duplicate tty file descriptor with close on exec bit set
    if ((tty_fd = fcntl(fileno(fp), F_DUPFD_CLOEXEC, FDBASE)) < 0)
        error_return("lusush: init.c: tty_init: fcntl");
    else if (init_ttystate)
        if (tcgetattr(tty_fd, &tty_state) < 0)
            error_return("lusush: tty.c: tty_init: tcgetattr");

    // Close the tty stream
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

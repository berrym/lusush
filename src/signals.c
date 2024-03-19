#include <unistd.h>
#include <signal.h>
#include "../include/errors.h"
#include "../include/signals.h"

void init_signal_handlers(void) {
    set_signal_handler(SIGINT, SIG_IGN);
    set_signal_handler(SIGSEGV, sigsegv_handler);
}

int set_signal_handler(int signo, void(handler)(int))
{
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = handler;
    return sigaction(signo, &sigact, NULL);
}

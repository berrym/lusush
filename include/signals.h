#ifndef SIGNALS_H
#define SIGNALS_H

int set_signal_handler(int, void (*)(int));
void init_signal_handlers(void);
void set_sigint_handler(void);
void set_sigsegv_handler(void);

#endif

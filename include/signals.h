#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

// Trap entry structure for signal handling
typedef struct trap_entry {
    int signal;
    char *command;
    struct trap_entry *next;
} trap_entry_t;

// Global trap list
extern trap_entry_t *trap_list;

// Original signal handler functions
int set_signal_handler(int, void (*)(int));
void init_signal_handlers(void);
void set_sigint_handler(void);
void set_sigsegv_handler(void);

// Child process management for signal handling
void set_current_child_pid(pid_t pid);
void clear_current_child_pid(void);

// Trap management functions
int set_trap(int signal, const char *command);
int remove_trap(int signal);
void list_traps(void);
int get_signal_number(const char *signame);
void execute_exit_traps(void);

#endif

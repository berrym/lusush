#include "../include/signals.h"

#include "../include/errors.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Global trap list
trap_entry_t *trap_list = NULL;

// Global variable to track if we're running a child process
static pid_t current_child_pid = 0;

// SIGINT handler that properly manages shell vs child process behavior
static void sigint_handler(int signo) {
    (void)signo; // Suppress unused parameter warning

    if (current_child_pid > 0) {
        // We have an active child process - send SIGINT to it
        kill(current_child_pid, SIGINT);
    } else {
        // No active child process - just print newline and return to prompt
        // The shell will handle returning to the prompt naturally
        printf("\n");
        fflush(stdout);
    }
}

void init_signal_handlers(void) {
    set_signal_handler(SIGINT, sigint_handler);
    set_signal_handler(SIGSEGV, sigsegv_handler);
}

// Function to set the current child PID (called when forking)
void set_current_child_pid(pid_t pid) { current_child_pid = pid; }

// Function to clear the current child PID (called when child exits)
void clear_current_child_pid(void) { current_child_pid = 0; }

int set_signal_handler(int signo, void(handler)(int)) {
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = handler;
    return sigaction(signo, &sigact, NULL);
}

// Find trap entry for given signal
static trap_entry_t *find_trap(int signal) {
    trap_entry_t *current = trap_list;
    while (current) {
        if (current->signal == signal) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Signal handler that executes trap commands
static void trap_signal_handler(int signo) {
    trap_entry_t *trap = find_trap(signo);
    if (trap && trap->command) {
        // Execute the trap command
        // For now, we'll use system() - this could be improved to use the
        // shell's executor
        system(trap->command);
    }
}

// Set a trap for a signal
int set_trap(int signal, const char *command) {
    // Remove existing trap for this signal
    remove_trap(signal);

    if (!command || strlen(command) == 0) {
        // Empty command means remove trap (already done above)
        return 0;
    }

    // Create new trap entry
    trap_entry_t *new_trap = malloc(sizeof(trap_entry_t));
    if (!new_trap) {
        return -1;
    }

    new_trap->signal = signal;
    new_trap->command = strdup(command);
    if (!new_trap->command) {
        free(new_trap);
        return -1;
    }

    // Add to list
    new_trap->next = trap_list;
    trap_list = new_trap;

    // Set the signal handler
    if (signal == SIGINT || signal == SIGTERM || signal == SIGQUIT ||
        signal == SIGHUP || signal == SIGUSR1 || signal == SIGUSR2) {
        set_signal_handler(signal, trap_signal_handler);
    }

    return 0;
}

// Remove a trap for a signal
int remove_trap(int signal) {
    trap_entry_t *current = trap_list;
    trap_entry_t *prev = NULL;

    while (current) {
        if (current->signal == signal) {
            // Remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                trap_list = current->next;
            }

            // Free memory
            free(current->command);
            free(current);

            // Reset signal handler to default (but not for EXIT trap which is
            // special)
            if (signal != 0) {
                set_signal_handler(signal, SIG_DFL);
            }

            return 0;
        }
        prev = current;
        current = current->next;
    }

    return -1; // Trap not found
}

// List all traps
void list_traps(void) {
    trap_entry_t *current = trap_list;
    while (current) {
        printf("trap -- '%s' %d\n", current->command, current->signal);
        current = current->next;
    }
}

// Get signal number from name
int get_signal_number(const char *signame) {
    if (!signame) {
        return -1;
    }

    // Handle numeric signals
    if (signame[0] >= '0' && signame[0] <= '9') {
        return atoi(signame);
    }

    // Handle signal names (with or without SIG prefix)
    if (strcmp(signame, "INT") == 0 || strcmp(signame, "SIGINT") == 0) {
        return SIGINT;
    }
    if (strcmp(signame, "TERM") == 0 || strcmp(signame, "SIGTERM") == 0) {
        return SIGTERM;
    }
    if (strcmp(signame, "QUIT") == 0 || strcmp(signame, "SIGQUIT") == 0) {
        return SIGQUIT;
    }
    if (strcmp(signame, "HUP") == 0 || strcmp(signame, "SIGHUP") == 0) {
        return SIGHUP;
    }
    if (strcmp(signame, "USR1") == 0 || strcmp(signame, "SIGUSR1") == 0) {
        return SIGUSR1;
    }
    if (strcmp(signame, "USR2") == 0 || strcmp(signame, "SIGUSR2") == 0) {
        return SIGUSR2;
    }
    if (strcmp(signame, "EXIT") == 0) {
        return 0; // Special case for EXIT trap
    }

    return -1; // Unknown signal
}

// Execute EXIT traps (signal 0)
void execute_exit_traps(void) {
    trap_entry_t *trap = find_trap(0); // EXIT is signal 0
    if (trap && trap->command) {
        // Execute the EXIT trap command
        // For now, we'll use system() - this could be improved to use the
        // shell's executor
        system(trap->command);
    }
}

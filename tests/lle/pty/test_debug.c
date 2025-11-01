#include "pty_test_harness.h"
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    // Redirect stderr to a file so we can see debug output
    int stderr_fd = open("/tmp/lle_debug.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (stderr_fd < 0) {
        perror("open");
        return 1;
    }
    
    pty_session_t *session = NULL;
    if (pty_session_create(&session, 80, 24) != 0) {
        printf("Failed to create PTY\n");
        return 1;
    }
    
    // Fork and redirect child's stderr
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        dup2(stderr_fd, STDERR_FILENO);
        close(stderr_fd);
        
        // Spawn lusush
        if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
            fprintf(stderr, "Failed to spawn lusush\n");
            exit(1);
        }
        exit(0);
    }
    
    close(stderr_fd);
    
    // Parent - wait for child and read output
    sleep(1);
    pty_session_read_output(session, 1000);
    
    printf("Output length: %zu\n", session->output_length);
    if (session->output_length > 0) {
        printf("Output: %.*s\n", (int)session->output_length, session->output_buffer);
    }
    
    int status;
    waitpid(pid, &status, 0);
    
    pty_session_close(session);
    
    printf("\nCheck /tmp/lle_debug.log for debug output\n");
    return 0;
}

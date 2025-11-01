#include "pty_test_harness.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    pty_session_t *session = NULL;
    if (pty_session_create(&session, 80, 24) != 0) {
        printf("Failed to create PTY\n");
        return 1;
    }
    
    if (pty_session_spawn_lusush(session, NULL, "xterm-256color") != 0) {
        printf("Failed to spawn lusush\n");
        pty_session_close(session);
        return 1;
    }
    
    // Send a character to trigger output
    sleep(1);
    pty_session_send_input(session, "a", 0);
    sleep(1);
    pty_session_read_output(session, 500);
    
    printf("Captured %zu bytes\n", session->output_length);
    printf("Buffer contains '$ ': %s\n", strstr(session->output_buffer, "$ ") ? "YES" : "NO");
    printf("Buffer contains '> ': %s\n", strstr(session->output_buffer, "> ") ? "YES" : "NO");
    
    // Show where "$ " appears
    char *pos = strstr(session->output_buffer, "$ ");
    if (pos) {
        size_t offset = pos - session->output_buffer;
        printf("Found '$ ' at offset %zu\n", offset);
        printf("Context: ...%.*s...\n", 20, pos > session->output_buffer + 10 ? pos - 10 : session->output_buffer);
    }
    
    pty_session_close(session);
    return 0;
}

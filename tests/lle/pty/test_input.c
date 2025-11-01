#include "pty_test_harness.h"
#include <stdio.h>
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
    
    printf("Waiting for initial output...\n");
    sleep(2);
    pty_session_read_output(session, 1000);
    printf("Initial read: %zu bytes\n", session->output_length);
    
    // Try sending a character
    printf("Sending 'a'...\n");
    pty_session_send_input(session, "a", 0);
    sleep(1);
    pty_session_read_output(session, 500);
    printf("After 'a': %zu bytes\n", session->output_length);
    if (session->output_length > 0) {
        printf("Output (first 200 bytes): %.*s\n", (int)(session->output_length < 200 ? session->output_length : 200), session->output_buffer);
    }
    
    pty_session_close(session);
    return 0;
}

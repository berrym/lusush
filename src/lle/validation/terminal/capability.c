// src/lle/validation/terminal/capability.c

#define _POSIX_C_SOURCE 199309L

#include "state.h"
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// Query terminal with timeout
static int query_terminal(int fd, const char *query, char *response,
                          size_t max_len, uint32_t timeout_ms) {
    // Write query
    ssize_t written = write(fd, query, strlen(query));
    if (written != (ssize_t)strlen(query)) {
        return -1;
    }
    
    // Wait for response with timeout
    fd_set readfds;
    struct timeval timeout = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000
    };
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret <= 0) {
        return -1;  // Timeout or error
    }
    
    // Read response
    ssize_t n = read(fd, response, max_len - 1);
    if (n > 0) {
        response[n] = '\0';
        return (int)n;
    }
    
    return -1;
}

int lle_terminal_detect_capabilities(lle_terminal_validation_t *term,
                                      uint32_t timeout_ms) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    char response[256];
    
    // Detect color support (query with DA1 - Device Attributes)
    if (query_terminal(term->output_fd, "\x1b[c", response,
                       sizeof(response), timeout_ms) > 0) {
        // Parse response for capabilities
        term->caps.has_color = (strstr(response, "4;") != NULL);
        term->caps.detection_successful = true;
    }
    
    // Detect 256 color (check TERM environment)
    const char *term_env = getenv("TERM");
    if (term_env) {
        term->caps.has_256_color = (strstr(term_env, "256color") != NULL);
        term->caps.has_true_color = (strstr(term_env, "truecolor") != NULL);
        
        // Detect terminal type
        if (strstr(term_env, "xterm")) {
            term->caps.type = TERM_XTERM;
            if (strstr(term_env, "256")) {
                term->caps.type = TERM_XTERM_256;
            }
        } else if (strstr(term_env, "rxvt")) {
            term->caps.type = TERM_RXVT;
        } else if (strstr(term_env, "screen")) {
            term->caps.type = TERM_SCREEN;
        } else if (strstr(term_env, "tmux")) {
            term->caps.type = TERM_TMUX;
        } else if (strstr(term_env, "vt100")) {
            term->caps.type = TERM_VT100;
        } else {
            term->caps.type = TERM_UNKNOWN;
        }
    }
    
    // Detect Unicode support (check LANG/LC_ALL)
    const char *lang = getenv("LANG");
    if (lang && strstr(lang, "UTF-8")) {
        term->caps.has_unicode = true;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    term->caps.detection_time_ms = 
        (uint32_t)((end.tv_sec - start.tv_sec) * 1000 +
        (end.tv_nsec - start.tv_nsec) / 1000000);
    
    return 0;
}

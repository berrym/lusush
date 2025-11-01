/*
 * terminal_unix_interface.c - Unix Terminal Interface (Spec 02 Subsystem 6)
 * 
 * Minimal Unix terminal interface abstraction providing:
 * - Raw mode setup and teardown
 * - Terminal attribute saving and restoration
 * - Signal handling (SIGWINCH, SIGTSTP, SIGCONT, SIGINT, SIGTERM)
 * - Non-blocking input with timeout support
 * - UTF-8 character decoding
 * - Window resize event generation
 * - EOF and error detection
 * 
 * Critical Principles:
 * - Always restore terminal state on exit
 * - Thread-safe state transitions
 * - Async-signal-safe signal handlers
 * - Idempotent operations (safe to call multiple times)
 * 
 * Spec 02: Terminal Abstraction - Subsystem 6
 */

#include "lle/terminal_abstraction.h"
#include "lle/input_parsing.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

/* ============================================================================
 * GLOBAL STATE FOR SIGNAL HANDLERS
 * ============================================================================
 * 
 * Unfortunately, POSIX signal handlers require global state. We use a single
 * global pointer protected by mutex where possible.
 */

static lle_unix_interface_t *g_signal_interface = NULL;

/* ============================================================================
 * SIGNAL HANDLERS
 * ============================================================================
 * 
 * Signal handlers must be async-signal-safe. Only a limited set of functions
 * are allowed: tcsetattr, signal, raise, write, etc. NO malloc, printf, etc.
 */

/*
 * SIGWINCH handler - window size changed
 * 
 * This is called when the terminal window is resized. We don't do much here
 * because we can't safely call complex functions in signal context.
 */
static void handle_sigwinch(int sig) {
    (void)sig;
    
    /* Set flag to be checked in event loop (async-signal-safe) */
    if (g_signal_interface) {
        g_signal_interface->sigwinch_received = true;
    }
}

/*
 * SIGTSTP handler - suspend (Ctrl-Z)
 * 
 * Before suspending, we must restore the terminal to its original state
 * so the user gets a normal shell prompt when backgrounded.
 */
static void handle_sigtstp(int sig) {
    if (!g_signal_interface) return;
    
    /* Exit raw mode before suspending (async-signal-safe) */
    if (g_signal_interface->raw_mode_active) {
        tcsetattr(g_signal_interface->terminal_fd, TCSAFLUSH,
                  &g_signal_interface->original_termios);
    }
    
    /* Re-raise signal with default handler to actually suspend */
    signal(sig, SIG_DFL);
    raise(sig);
}

/*
 * SIGCONT handler - resume after suspend
 * 
 * When resumed from background, re-enter raw mode if we were in it.
 */
static void handle_sigcont(int sig) {
    (void)sig;
    
    if (!g_signal_interface) return;
    
    /* Re-enter raw mode if we were in it (async-signal-safe) */
    if (g_signal_interface->raw_mode_active) {
        tcsetattr(g_signal_interface->terminal_fd, TCSAFLUSH,
                  &g_signal_interface->raw_termios);
    }
    
    /* Re-install SIGTSTP handler (it was reset to default) */
    signal(SIGTSTP, handle_sigtstp);
}

/*
 * SIGINT/SIGTERM handler - clean exit
 * 
 * Ensure terminal is restored before exiting.
 */
static void handle_exit_signal(int sig) {
    if (!g_signal_interface) return;
    
    /* Exit raw mode before terminating (async-signal-safe) */
    if (g_signal_interface->raw_mode_active) {
        tcsetattr(g_signal_interface->terminal_fd, TCSAFLUSH,
                  &g_signal_interface->original_termios);
    }
    
    /* Re-raise with default handler to actually exit */
    signal(sig, SIG_DFL);
    raise(sig);
}

/* ============================================================================
 * CLEANUP ON EXIT
 * ============================================================================
 */

/*
 * atexit handler - ensure terminal is restored even on abnormal exit
 */
static void cleanup_on_exit(void) {
    if (g_signal_interface && g_signal_interface->raw_mode_active) {
        /* This is safe in atexit context */
        tcsetattr(g_signal_interface->terminal_fd, TCSAFLUSH,
                  &g_signal_interface->original_termios);
    }
}

/*
 * Register atexit cleanup handler (called once)
 */
static void register_cleanup(void) {
    static bool registered = false;
    
    if (!registered) {
        atexit(cleanup_on_exit);
        registered = true;
    }
}

/* ============================================================================
 * SIGNAL HANDLER INSTALLATION
 * ============================================================================
 */

/* Storage for original signal handlers (static lifetime) */
static struct sigaction original_sigwinch;
static struct sigaction original_sigtstp;
static struct sigaction original_sigcont;
static struct sigaction original_sigint;
static struct sigaction original_sigterm;
static bool signals_installed = false;

/*
 * Install all signal handlers
 */
static lle_result_t install_signal_handlers(lle_unix_interface_t *interface) {
    if (signals_installed) {
        return LLE_SUCCESS;  /* Already installed */
    }
    
    struct sigaction sa;
    
    /* SIGWINCH - window resize */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  /* Restart interrupted system calls */
    
    if (sigaction(SIGWINCH, &sa, &original_sigwinch) != 0) {
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    /* SIGTSTP - suspend (Ctrl-Z) */
    sa.sa_handler = handle_sigtstp;
    if (sigaction(SIGTSTP, &sa, &original_sigtstp) != 0) {
        sigaction(SIGWINCH, &original_sigwinch, NULL);
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    /* SIGCONT - resume */
    sa.sa_handler = handle_sigcont;
    if (sigaction(SIGCONT, &sa, &original_sigcont) != 0) {
        sigaction(SIGWINCH, &original_sigwinch, NULL);
        sigaction(SIGTSTP, &original_sigtstp, NULL);
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    /* SIGINT - Ctrl-C */
    sa.sa_handler = handle_exit_signal;
    if (sigaction(SIGINT, &sa, &original_sigint) != 0) {
        sigaction(SIGWINCH, &original_sigwinch, NULL);
        sigaction(SIGTSTP, &original_sigtstp, NULL);
        sigaction(SIGCONT, &original_sigcont, NULL);
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    /* SIGTERM - termination */
    if (sigaction(SIGTERM, &sa, &original_sigterm) != 0) {
        sigaction(SIGWINCH, &original_sigwinch, NULL);
        sigaction(SIGTSTP, &original_sigtstp, NULL);
        sigaction(SIGCONT, &original_sigcont, NULL);
        sigaction(SIGINT, &original_sigint, NULL);
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    /* Set global pointer for handlers */
    g_signal_interface = interface;
    signals_installed = true;
    
    return LLE_SUCCESS;
}

/*
 * Restore original signal handlers
 */
static void restore_signal_handlers(lle_unix_interface_t *interface) {
    if (!signals_installed) {
        return;
    }
    
    /* Only restore if this was the interface that installed them */
    if (g_signal_interface != interface) {
        return;
    }
    
    /* Restore all original signal handlers */
    sigaction(SIGWINCH, &original_sigwinch, NULL);
    sigaction(SIGTSTP, &original_sigtstp, NULL);
    sigaction(SIGCONT, &original_sigcont, NULL);
    sigaction(SIGINT, &original_sigint, NULL);
    sigaction(SIGTERM, &original_sigterm, NULL);
    
    /* Clear global pointer */
    g_signal_interface = NULL;
    signals_installed = false;
}

/* ============================================================================
 * PUBLIC API FUNCTIONS
 * ============================================================================
 */

/*
 * Initialize Unix terminal interface
 */
lle_result_t lle_unix_interface_init(lle_unix_interface_t **interface) {
    if (!interface) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate interface structure */
    lle_unix_interface_t *iface = calloc(1, sizeof(lle_unix_interface_t));
    if (!iface) {
        return LLE_ERROR_OUT_OF_MEMORY;
    }
    
    /* Set file descriptor (using STDIN for terminal operations) */
    iface->terminal_fd = STDIN_FILENO;
    
    /* Save original terminal state (if this is a TTY) */
    /* In non-TTY environments (tests, pipes), tcgetattr will fail - that's OK */
    if (tcgetattr(iface->terminal_fd, &iface->original_termios) != 0) {
        /* Not a TTY - initialize with empty termios */
        memset(&iface->original_termios, 0, sizeof(struct termios));
    }
    
    /* Initialize state */
    iface->raw_mode_active = false;
    iface->size_changed = false;
    iface->sigwinch_received = false;
    iface->last_error = LLE_SUCCESS;
    
    /* Get initial window size */
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        iface->current_width = ws.ws_col;
        iface->current_height = ws.ws_row;
    } else {
        iface->current_width = 80;
        iface->current_height = 24;
    }
    
    /* Install signal handlers */
    lle_result_t result = install_signal_handlers(iface);
    if (result != LLE_SUCCESS) {
        free(iface);
        return result;
    }
    
    /* Register atexit cleanup */
    register_cleanup();
    
    /* Initialize parser-related fields to NULL (will be set up later) */
    iface->sequence_parser = NULL;
    iface->capabilities = NULL;
    iface->memory_pool = NULL;
    
    *interface = iface;
    return LLE_SUCCESS;
}

/*
 * Initialize sequence parser for escape sequence processing
 * 
 * This must be called after lle_unix_interface_init() and after
 * terminal capabilities have been detected.
 */
lle_result_t lle_unix_interface_init_sequence_parser(
    lle_unix_interface_t *interface,
    lle_terminal_capabilities_t *capabilities,
    lle_memory_pool_t *memory_pool
) {
    if (!interface || !capabilities || !memory_pool) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Store references for later use */
    interface->capabilities = capabilities;
    interface->memory_pool = memory_pool;
    
    /* Initialize sequence parser */
    lle_result_t result = lle_sequence_parser_init(
        &interface->sequence_parser,
        capabilities,
        memory_pool
    );
    
    if (result != LLE_SUCCESS) {
        interface->capabilities = NULL;
        interface->memory_pool = NULL;
        return result;
    }
    
    return LLE_SUCCESS;
}

/*
 * Destroy Unix terminal interface
 */
void lle_unix_interface_destroy(lle_unix_interface_t *interface) {
    if (!interface) {
        return;
    }
    
    /* Ensure we exit raw mode before cleanup */
    if (interface->raw_mode_active) {
        lle_unix_interface_exit_raw_mode(interface);
    }
    
    /* Clean up sequence parser if initialized */
    if (interface->sequence_parser) {
        lle_sequence_parser_destroy(interface->sequence_parser);
        interface->sequence_parser = NULL;
    }
    
    /* Clear references (we don't own capabilities or memory_pool) */
    interface->capabilities = NULL;
    interface->memory_pool = NULL;
    
    /* Restore original signal handlers */
    restore_signal_handlers(interface);
    
    /* Free structure */
    free(interface);
}

/*
 * Enter raw (non-canonical) mode
 */
lle_result_t lle_unix_interface_enter_raw_mode(lle_unix_interface_t *interface) {
    if (!interface) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Already in raw mode? (idempotent) */
    if (interface->raw_mode_active) {
        return LLE_SUCCESS;
    }
    
    /* Copy original settings */
    interface->raw_termios = interface->original_termios;
    
    /* Modify for raw mode */
    struct termios *raw = &interface->raw_termios;
    
    /* Input flags - disable special processing */
    raw->c_iflag &= ~(BRKINT |  /* No break signal */
                      ICRNL |   /* Don't translate CR to NL */
                      INPCK |   /* Disable parity checking */
                      ISTRIP |  /* Don't strip 8th bit */
                      IXON);    /* Disable XON/XOFF flow control */
    
    /* Output flags - KEEP output processing for proper display */
    /* NOTE: Disabling OPOST causes display corruption - \n won't return to column 0 */
    /* We need raw INPUT mode, but output should remain processed for display */
    
    /* Control flags - 8-bit characters */
    raw->c_cflag |= (CS8);      /* 8 bits per byte */
    
    /* Local flags - disable canonical mode, echo, and signals */
    raw->c_lflag &= ~(ECHO |    /* No echo */
                      ICANON |  /* Non-canonical mode */
                      IEXTEN |  /* Disable extended input processing */
                      ISIG);    /* Disable signals (Ctrl-C, Ctrl-Z) */
    
    /* Control characters - non-blocking read */
    raw->c_cc[VMIN] = 0;        /* Non-blocking: return immediately */
    raw->c_cc[VTIME] = 0;       /* No timeout */
    
    /* Apply settings - TCSAFLUSH discards unread input */
    if (tcsetattr(interface->terminal_fd, TCSAFLUSH, raw) != 0) {
        interface->last_error = LLE_ERROR_SYSTEM_CALL;
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    interface->raw_mode_active = true;
    return LLE_SUCCESS;
}

/*
 * Exit raw mode and restore original terminal state
 */
lle_result_t lle_unix_interface_exit_raw_mode(lle_unix_interface_t *interface) {
    if (!interface) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Not in raw mode? (idempotent) */
    if (!interface->raw_mode_active) {
        return LLE_SUCCESS;
    }
    
    /* Restore original settings */
    if (tcsetattr(interface->terminal_fd, TCSAFLUSH, &interface->original_termios) != 0) {
        interface->last_error = LLE_ERROR_SYSTEM_CALL;
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    interface->raw_mode_active = false;
    return LLE_SUCCESS;
}

/*
 * Get current window size
 */
lle_result_t lle_unix_interface_get_window_size(lle_unix_interface_t *interface,
                                                size_t *width,
                                                size_t *height) {
    if (!interface || !width || !height) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    struct winsize ws;
    
    /* Try ioctl first */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0 && ws.ws_row > 0) {
        *width = ws.ws_col;
        *height = ws.ws_row;
        
        /* Update cached size */
        interface->current_width = ws.ws_col;
        interface->current_height = ws.ws_row;
        
        return LLE_SUCCESS;
    }
    
    /* Fallback to environment variables */
    const char *env_cols = getenv("COLUMNS");
    const char *env_lines = getenv("LINES");
    
    *width = (env_cols && *env_cols) ? (size_t)atoi(env_cols) : 80;
    *height = (env_lines && *env_lines) ? (size_t)atoi(env_lines) : 24;
    
    /* Update cached size */
    interface->current_width = *width;
    interface->current_height = *height;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * CONVERSION HELPERS - PARSED INPUT TO INPUT EVENT
 * ============================================================================ */

/*
 * Convert lle_key_info_t keycode to lle_special_key_t
 */
static lle_special_key_t convert_key_code(uint32_t keycode, lle_key_type_t key_type) {
    /* For cursor keys and editing keys, keycode typically maps directly */
    switch (key_type) {
        case LLE_KEY_TYPE_CURSOR:
            /* Cursor keys: assume keycode maps to arrow keys */
            if (keycode >= 1 && keycode <= 4) {
                return (lle_special_key_t)(LLE_KEY_UP + keycode - 1);
            }
            break;
        case LLE_KEY_TYPE_FUNCTION:
            /* Function keys: F1-F12 */
            if (keycode >= 1 && keycode <= 12) {
                return (lle_special_key_t)(LLE_KEY_F1 + keycode - 1);
            }
            break;
        case LLE_KEY_TYPE_EDITING:
            /* Editing keys: map common codes */
            switch (keycode) {
                case 1: return LLE_KEY_HOME;
                case 2: return LLE_KEY_INSERT;
                case 3: return LLE_KEY_DELETE;
                case 4: return LLE_KEY_END;
                case 5: return LLE_KEY_PAGE_UP;
                case 6: return LLE_KEY_PAGE_DOWN;
                default: break;
            }
            break;
        case LLE_KEY_TYPE_SPECIAL:
            /* Special keys */
            switch (keycode) {
                case 9: return LLE_KEY_TAB;
                case 13: case 10: return LLE_KEY_ENTER;
                case 27: return LLE_KEY_ESCAPE;
                case 127: return LLE_KEY_BACKSPACE;
                default: break;
            }
            break;
        default:
            break;
    }
    return LLE_KEY_UNKNOWN;
}

/*
 * Convert lle_key_modifiers_t to lle_key_modifier_t
 */
static lle_key_modifier_t convert_modifiers(lle_key_modifiers_t parser_mods) {
    lle_key_modifier_t result = LLE_MOD_NONE;
    
    if (parser_mods & LLE_KEY_MOD_SHIFT) result |= LLE_MOD_SHIFT;
    if (parser_mods & LLE_KEY_MOD_ALT) result |= LLE_MOD_ALT;
    if (parser_mods & LLE_KEY_MOD_CTRL) result |= LLE_MOD_CTRL;
    if (parser_mods & LLE_KEY_MOD_META) result |= LLE_MOD_META;
    
    return result;
}

/*
 * Convert lle_parsed_input_t to lle_input_event_t
 * 
 * This function bridges the comprehensive sequence parser output with the
 * terminal abstraction input event format.
 */
static lle_result_t convert_parsed_input_to_event(
    const lle_parsed_input_t *parsed,
    lle_input_event_t *event
) {
    if (!parsed || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    memset(event, 0, sizeof(lle_input_event_t));
    event->timestamp = parsed->data.text_info.timestamp;
    event->sequence_number = 0;  /* Will be set by caller if needed */
    
    switch (parsed->type) {
        case LLE_PARSED_INPUT_TYPE_TEXT:
            /* Regular text input */
            event->type = LLE_INPUT_TYPE_CHARACTER;
            event->data.character.codepoint = parsed->data.text_info.codepoint;
            memcpy(event->data.character.utf8_bytes,
                   parsed->data.text_info.utf8_bytes,
                   parsed->data.text_info.utf8_length);
            event->data.character.byte_count = (uint8_t)parsed->data.text_info.utf8_length;
            break;
            
        case LLE_PARSED_INPUT_TYPE_KEY:
            /* Key press or combination */
            event->type = LLE_INPUT_TYPE_SPECIAL_KEY;
            event->data.special_key.key = convert_key_code(
                parsed->data.key_info.keycode,
                parsed->data.key_info.type
            );
            event->data.special_key.modifiers = convert_modifiers(
                parsed->data.key_info.modifiers
            );
            event->timestamp = parsed->data.key_info.timestamp;
            break;
            
        case LLE_PARSED_INPUT_TYPE_MOUSE:
            /* Mouse events are not directly supported by lle_input_event_t */
            /* We'll need to handle these through a different mechanism later */
            /* For now, treat as unknown/error */
            event->type = LLE_INPUT_TYPE_ERROR;
            event->data.error.error_code = LLE_ERROR_FEATURE_NOT_AVAILABLE;
            snprintf(event->data.error.error_message,
                    sizeof(event->data.error.error_message),
                    "Mouse events not yet supported");
            break;
            
        case LLE_PARSED_INPUT_TYPE_SEQUENCE:
            /* Terminal sequence - treat as error for now */
            event->type = LLE_INPUT_TYPE_ERROR;
            event->data.error.error_code = LLE_ERROR_FEATURE_NOT_AVAILABLE;
            snprintf(event->data.error.error_message,
                    sizeof(event->data.error.error_message),
                    "Terminal sequence events not yet supported");
            break;
            
        case LLE_PARSED_INPUT_TYPE_PASTE:
            /* Bracketed paste - treat as error for now */
            event->type = LLE_INPUT_TYPE_ERROR;
            event->data.error.error_code = LLE_ERROR_FEATURE_NOT_AVAILABLE;
            snprintf(event->data.error.error_message,
                    sizeof(event->data.error.error_message),
                    "Paste events not yet supported");
            break;
            
        case LLE_PARSED_INPUT_TYPE_FOCUS:
            /* Focus events - treat as error for now */
            event->type = LLE_INPUT_TYPE_ERROR;
            event->data.error.error_code = LLE_ERROR_FEATURE_NOT_AVAILABLE;
            snprintf(event->data.error.error_message,
                    sizeof(event->data.error.error_message),
                    "Focus events not yet supported");
            break;
            
        case LLE_PARSED_INPUT_TYPE_UNKNOWN:
        default:
            /* Unknown input type */
            event->type = LLE_INPUT_TYPE_ERROR;
            event->data.error.error_code = LLE_ERROR_INPUT_PARSING;
            snprintf(event->data.error.error_message,
                    sizeof(event->data.error.error_message),
                    "Unknown input type from parser");
            break;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * UTF-8 DECODING HELPERS (Phase 3)
 * ============================================================================ */

/*
 * Determine UTF-8 sequence length from first byte
 */
static int get_utf8_length(unsigned char first_byte) {
    if ((first_byte & 0x80) == 0x00) return 1;  /* 0xxxxxxx - ASCII */
    if ((first_byte & 0xE0) == 0xC0) return 2;  /* 110xxxxx - 2 bytes */
    if ((first_byte & 0xF0) == 0xE0) return 3;  /* 1110xxxx - 3 bytes */
    if ((first_byte & 0xF8) == 0xF0) return 4;  /* 11110xxx - 4 bytes */
    return -1;  /* Invalid first byte */
}

/*
 * Decode UTF-8 sequence to Unicode codepoint
 * 
 * This decoder:
 * - Validates basic structure (continuation bytes)
 * - Returns replacement character (U+FFFD) for invalid sequences
 * - Does not detect overlong sequences (acceptable tradeoff)
 */
static lle_result_t decode_utf8(lle_unix_interface_t *interface,
                                unsigned char first_byte,
                                uint32_t *codepoint_out,
                                char *utf8_bytes,
                                uint8_t *byte_count_out) {
    int expected_bytes = get_utf8_length(first_byte);
    
    if (expected_bytes < 0) {
        /* Invalid first byte - use replacement character */
        *codepoint_out = 0xFFFD;
        utf8_bytes[0] = (char)first_byte;
        *byte_count_out = 1;
        return LLE_SUCCESS;
    }
    
    utf8_bytes[0] = (char)first_byte;
    *byte_count_out = (uint8_t)expected_bytes;
    
    if (expected_bytes == 1) {
        /* ASCII - fast path */
        *codepoint_out = first_byte;
        return LLE_SUCCESS;
    }
    
    /* Read additional bytes for multi-byte sequence */
    for (int i = 1; i < expected_bytes; i++) {
        unsigned char byte;
        ssize_t n = read(interface->terminal_fd, &byte, 1);
        
        if (n <= 0) {
            /* Incomplete sequence - use replacement character */
            *codepoint_out = 0xFFFD;
            return LLE_SUCCESS;
        }
        
        /* Validate continuation byte (10xxxxxx) */
        if ((byte & 0xC0) != 0x80) {
            /* Invalid continuation - use replacement character */
            *codepoint_out = 0xFFFD;
            return LLE_SUCCESS;
        }
        
        utf8_bytes[i] = (char)byte;
    }
    
    /* Decode to Unicode codepoint */
    switch (expected_bytes) {
        case 2:
            *codepoint_out = ((first_byte & 0x1F) << 6) |
                            ((unsigned char)utf8_bytes[1] & 0x3F);
            break;
        case 3:
            *codepoint_out = ((first_byte & 0x0F) << 12) |
                            (((unsigned char)utf8_bytes[1] & 0x3F) << 6) |
                            ((unsigned char)utf8_bytes[2] & 0x3F);
            break;
        case 4:
            *codepoint_out = ((first_byte & 0x07) << 18) |
                            (((unsigned char)utf8_bytes[1] & 0x3F) << 12) |
                            (((unsigned char)utf8_bytes[2] & 0x3F) << 6) |
                            ((unsigned char)utf8_bytes[3] & 0x3F);
            break;
        default:
            *codepoint_out = 0xFFFD;
            break;
    }
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * EVENT READING (Phase 3)
 * ============================================================================ */

/*
 * Read input event from terminal with timeout support
 * 
 * This implementation provides:
 * - Non-blocking input with configurable timeout
 * - UTF-8 character decoding
 * - Window resize event generation (from SIGWINCH)
 * - EOF detection
 * - Timeout events
 * 
 * Higher-level parsing (escape sequences, special keys) is handled by
 * Spec 06 Input Parsing, which wraps this primitive interface.
 */
lle_result_t lle_unix_interface_read_event(lle_unix_interface_t *interface,
                                           lle_input_event_t *event,
                                           uint32_t timeout_ms) {
    if (!interface || !event) {
        return LLE_ERROR_INVALID_PARAMETER;
    }
    
    /* Clear event structure */
    memset(event, 0, sizeof(lle_input_event_t));
    
    /* Check for pending SIGWINCH (resize event has priority) */
    if (interface->sigwinch_received) {
        interface->sigwinch_received = false;
        
        size_t width, height;
        lle_result_t result = lle_unix_interface_get_window_size(interface, &width, &height);
        if (result != LLE_SUCCESS) {
            event->type = LLE_INPUT_TYPE_ERROR;
            event->timestamp = lle_get_current_time_microseconds();
            event->data.error.error_code = result;
            snprintf(event->data.error.error_message, 
                    sizeof(event->data.error.error_message),
                    "Failed to get window size after SIGWINCH");
            return result;
        }
        
        event->type = LLE_INPUT_TYPE_WINDOW_RESIZE;
        event->timestamp = lle_get_current_time_microseconds();
        event->data.resize.new_width = width;
        event->data.resize.new_height = height;
        interface->size_changed = true;
        
        return LLE_SUCCESS;
    }
    
    /* Use select() for timeout support */
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(interface->terminal_fd, &readfds);
    
    struct timeval tv;
    struct timeval *tv_ptr;
    
    if (timeout_ms == UINT32_MAX) {
        /* Infinite timeout - pass NULL to select() */
        tv_ptr = NULL;
    } else {
        tv.tv_sec = (time_t)(timeout_ms / 1000);
        tv.tv_usec = (suseconds_t)((timeout_ms % 1000) * 1000);
        tv_ptr = &tv;
    }
    
    int ready = select(interface->terminal_fd + 1, &readfds, NULL, NULL, tv_ptr);
    
    if (ready == -1) {
        if (errno == EINTR) {
            /* Interrupted by signal - check for resize */
            if (interface->sigwinch_received) {
                /* Recursively handle resize event */
                return lle_unix_interface_read_event(interface, event, timeout_ms);
            }
            /* Other signal - return timeout */
            event->type = LLE_INPUT_TYPE_TIMEOUT;
            event->timestamp = lle_get_current_time_microseconds();
            return LLE_SUCCESS;
        }
        /* System call error */
        event->type = LLE_INPUT_TYPE_ERROR;
        event->timestamp = lle_get_current_time_microseconds();
        event->data.error.error_code = LLE_ERROR_SYSTEM_CALL;
        snprintf(event->data.error.error_message, 
                sizeof(event->data.error.error_message),
                "select() failed: %s", strerror(errno));
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    if (ready == 0) {
        /* Timeout - no data available */
        event->type = LLE_INPUT_TYPE_TIMEOUT;
        event->timestamp = lle_get_current_time_microseconds();
        return LLE_SUCCESS;
    }
    
    /* Data available - read first byte */
    unsigned char first_byte;
    ssize_t bytes_read = read(interface->terminal_fd, &first_byte, 1);
    
    if (bytes_read == -1) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            /* Non-blocking read interrupted - treat as timeout */
            event->type = LLE_INPUT_TYPE_TIMEOUT;
            event->timestamp = lle_get_current_time_microseconds();
            return LLE_SUCCESS;
        }
        /* Read error */
        event->type = LLE_INPUT_TYPE_ERROR;
        event->timestamp = lle_get_current_time_microseconds();
        event->data.error.error_code = LLE_ERROR_SYSTEM_CALL;
        snprintf(event->data.error.error_message, 
                sizeof(event->data.error.error_message),
                "read() failed: %s", strerror(errno));
        return LLE_ERROR_SYSTEM_CALL;
    }
    
    if (bytes_read == 0) {
        /* EOF - stdin closed */
        event->type = LLE_INPUT_TYPE_EOF;
        event->timestamp = lle_get_current_time_microseconds();
        return LLE_SUCCESS;
    }
    
    /* Use comprehensive sequence parser for escape sequences and control chars if available */
    if (interface->sequence_parser && (first_byte == 0x1B || first_byte < 0x20)) {
        /* ESC or control character - feed to comprehensive parser */
        lle_parsed_input_t *parsed_input = NULL;
        char byte_buffer[1] = {(char)first_byte};
        
        lle_result_t parse_result = lle_sequence_parser_process_data(
            interface->sequence_parser,
            byte_buffer,
            1,
            &parsed_input
        );
        
        if (parse_result != LLE_SUCCESS) {
            event->type = LLE_INPUT_TYPE_ERROR;
            event->timestamp = lle_get_current_time_microseconds();
            event->data.error.error_code = parse_result;
            snprintf(event->data.error.error_message,
                    sizeof(event->data.error.error_message),
                    "Parser failed to process input");
            return parse_result;
        }
        
        if (parsed_input) {
            /* Parser returned a complete sequence - convert to event */
            lle_result_t convert_result = convert_parsed_input_to_event(parsed_input, event);
            lle_pool_free(parsed_input);
            return convert_result;
        }
        
        /* Parser is accumulating a sequence - return timeout so caller will call again */
        /* This allows the parser to accumulate the full escape sequence across multiple calls */
        event->type = LLE_INPUT_TYPE_TIMEOUT;
        event->timestamp = lle_get_current_time_microseconds();
        return LLE_SUCCESS;
    }
    
    /* Check for escape sequences (ESC = 0x1B = 27) */
    if (first_byte == 0x1B) {
        /* Read next byte with short timeout to detect escape sequences */
        unsigned char second_byte;
        fd_set read_fds;
        struct timeval escape_timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(interface->terminal_fd, &read_fds);
        escape_timeout.tv_sec = 0;
        escape_timeout.tv_usec = 10000;  /* 10ms timeout for escape sequence */
        
        int ready = select(interface->terminal_fd + 1, &read_fds, NULL, NULL, &escape_timeout);
        
        if (ready > 0) {
            ssize_t read2 = read(interface->terminal_fd, &second_byte, 1);
            
            if (read2 == 1 && second_byte == '[') {
                /* CSI sequence - read the final byte */
                unsigned char final_byte;
                ssize_t read3 = read(interface->terminal_fd, &final_byte, 1);
                
                if (read3 == 1) {
                    /* Detect common arrow key sequences: ESC [ A/B/C/D */
                    event->type = LLE_INPUT_TYPE_SPECIAL_KEY;
                    event->timestamp = lle_get_current_time_microseconds();
                    event->data.special_key.modifiers = 0;
                    
                    switch (final_byte) {
                        case 'A':
                            event->data.special_key.key = LLE_KEY_UP;
                            return LLE_SUCCESS;
                        case 'B':
                            event->data.special_key.key = LLE_KEY_DOWN;
                            return LLE_SUCCESS;
                        case 'C':
                            event->data.special_key.key = LLE_KEY_RIGHT;
                            return LLE_SUCCESS;
                        case 'D':
                            event->data.special_key.key = LLE_KEY_LEFT;
                            return LLE_SUCCESS;
                        case 'H':
                            event->data.special_key.key = LLE_KEY_HOME;
                            return LLE_SUCCESS;
                        case 'F':
                            event->data.special_key.key = LLE_KEY_END;
                            return LLE_SUCCESS;
                        case '3':
                            /* Delete key: ESC [ 3 ~ - need to read the ~ */
                            {
                                unsigned char tilde;
                                ssize_t read4 = read(interface->terminal_fd, &tilde, 1);
                                if (read4 == 1 && tilde == '~') {
                                    event->data.special_key.key = LLE_KEY_DELETE;
                                    return LLE_SUCCESS;
                                }
                            }
                            break;
                        default:
                            /* Unknown CSI sequence - fall through to return as character */
                            break;
                    }
                }
            } else if (read2 == 1 && second_byte == 'O') {
                /* SS3 sequence - alternate function keys */
                unsigned char final_byte;
                ssize_t read3 = read(interface->terminal_fd, &final_byte, 1);
                
                if (read3 == 1) {
                    event->type = LLE_INPUT_TYPE_SPECIAL_KEY;
                    event->timestamp = lle_get_current_time_microseconds();
                    event->data.special_key.modifiers = 0;
                    
                    switch (final_byte) {
                        case 'H':
                            event->data.special_key.key = LLE_KEY_HOME;
                            return LLE_SUCCESS;
                        case 'F':
                            event->data.special_key.key = LLE_KEY_END;
                            return LLE_SUCCESS;
                        default:
                            /* Unknown SS3 sequence */
                            break;
                    }
                }
            }
        }
        
        /* If we get here, it's just a plain ESC key or unrecognized sequence */
        /* Return ESC as a regular character */
    }
    
    /* Decode UTF-8 character */
    uint32_t codepoint;
    char utf8_bytes[8] = {0};
    uint8_t byte_count;
    
    lle_result_t decode_result = decode_utf8(interface, first_byte, 
                                             &codepoint, utf8_bytes, &byte_count);
    if (decode_result != LLE_SUCCESS) {
        event->type = LLE_INPUT_TYPE_ERROR;
        event->timestamp = lle_get_current_time_microseconds();
        event->data.error.error_code = decode_result;
        snprintf(event->data.error.error_message, 
                sizeof(event->data.error.error_message),
                "UTF-8 decoding failed");
        return decode_result;
    }
    
    /* Populate character event */
    event->type = LLE_INPUT_TYPE_CHARACTER;
    event->timestamp = lle_get_current_time_microseconds();
    event->data.character.codepoint = codepoint;
    memcpy(event->data.character.utf8_bytes, utf8_bytes, byte_count);
    event->data.character.byte_count = byte_count;
    
    return LLE_SUCCESS;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/*
 * Get current time in microseconds
 * 
 * Uses CLOCK_MONOTONIC for reliable timing (not affected by system time changes)
 */
uint64_t lle_get_current_time_microseconds(void) {
    struct timespec ts;
    
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    }
    
    /* Fallback to gettimeofday if CLOCK_MONOTONIC fails */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

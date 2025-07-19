/**
 * termcap_internal.h - Internal definitions for enhanced termcap implementation
 *
 * This header contains internal structures, constants, and function declarations
 * that are not exposed in the public API but are shared between termcap modules.
 */

#ifndef TERMCAP_INTERNAL_H
#define TERMCAP_INTERNAL_H

#include "termcap.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= Internal Constants ======================= */

/* Maximum sizes */
#define TERMCAP_MAX_ESCAPE_SEQUENCE 128
#define TERMCAP_MAX_TERMINAL_NAME 64
#define TERMCAP_MAX_TERMINAL_VERSION 32
#define TERMCAP_MAX_QUERY_RESPONSE 256
#define TERMCAP_MAX_CAPABILITY_TESTS 32

/* Timeouts and performance */
#define TERMCAP_FAST_QUERY_THRESHOLD_MS 10
#define TERMCAP_SLOW_QUERY_THRESHOLD_MS 100
#define TERMCAP_MAX_QUERY_ATTEMPTS 3
#define TERMCAP_CAPABILITY_CACHE_SIZE 64

/* Terminal identification patterns */
#define TERMCAP_TERM_PATTERN_MAX 16
#define TERMCAP_TERM_RESPONSE_TIMEOUT_MS 200

/* ======================= Internal Structures ======================= */

/* Pre-compiled escape sequences for performance */
typedef struct {
    char sequence[TERMCAP_MAX_ESCAPE_SEQUENCE];
    size_t length;
    bool cached;
} escape_sequence_t;

/* Terminal database entry */
typedef struct {
    const char *name;
    const char *pattern;            /* Pattern for TERM matching */
    const char *version_pattern;    /* Pattern for version detection */
    termcap_capabilities_t caps;
    int priority;                   /* Higher priority = preferred match */
} terminal_db_entry_t;

/* Capability test function */
typedef struct {
    const char *name;
    bool (*test_func)(void);
    const char *query_sequence;
    const char *expected_response_pattern;
    int timeout_ms;
} capability_test_t;

/* Internal state management */
typedef struct {
    bool initialized;
    bool capabilities_detected;
    bool batch_mode;
    int batch_fd;
    FILE *batch_stream;
    
    /* Performance tracking */
    int cursor_query_count;
    int cursor_query_total_time_ms;
    int cursor_query_timeout_ms;
    bool fast_queries_detected;
    
    /* Capability cache */
    struct {
        bool valid;
        termcap_capabilities_t caps;
        char terminal_name[TERMCAP_MAX_TERMINAL_NAME];
        char terminal_version[TERMCAP_MAX_TERMINAL_VERSION];
    } capability_cache;
    
    /* Output batching */
    termcap_output_buffer_t *current_buffer;
    
    /* Linenoise integration */
    termcap_color_t linenoise_prompt_color;
    termcap_color_t linenoise_input_color;
    termcap_color_t linenoise_completion_color;
    int linenoise_completion_style;
    bool linenoise_mouse_enabled;
    
} termcap_internal_state_t;

/* ======================= Pre-compiled Escape Sequences ======================= */

/* Common escape sequences - pre-compiled for performance */
extern const escape_sequence_t ESC_RESET_ALL;
extern const escape_sequence_t ESC_CLEAR_SCREEN;
extern const escape_sequence_t ESC_CLEAR_LINE;
extern const escape_sequence_t ESC_CLEAR_TO_EOL;
extern const escape_sequence_t ESC_CLEAR_TO_BOL;
extern const escape_sequence_t ESC_CLEAR_TO_EOS;
extern const escape_sequence_t ESC_CURSOR_HOME;
extern const escape_sequence_t ESC_HIDE_CURSOR;
extern const escape_sequence_t ESC_SHOW_CURSOR;
extern const escape_sequence_t ESC_SAVE_CURSOR;
extern const escape_sequence_t ESC_RESTORE_CURSOR;
extern const escape_sequence_t ESC_CURSOR_QUERY;
extern const escape_sequence_t ESC_DEVICE_ATTRIBUTES;
extern const escape_sequence_t ESC_ALTERNATE_SCREEN_ENTER;
extern const escape_sequence_t ESC_ALTERNATE_SCREEN_EXIT;
extern const escape_sequence_t ESC_MOUSE_ENABLE_ALL;
extern const escape_sequence_t ESC_MOUSE_DISABLE;
extern const escape_sequence_t ESC_BRACKETED_PASTE_ENABLE;
extern const escape_sequence_t ESC_BRACKETED_PASTE_DISABLE;
extern const escape_sequence_t ESC_FOCUS_EVENTS_ENABLE;
extern const escape_sequence_t ESC_FOCUS_EVENTS_DISABLE;
extern const escape_sequence_t ESC_SYNCHRONIZED_OUTPUT_BEGIN;
extern const escape_sequence_t ESC_SYNCHRONIZED_OUTPUT_END;

/* ======================= Internal Function Declarations ======================= */

/* Core internal functions */
termcap_internal_state_t *termcap_get_internal_state(void);

/* Sequence generation */
int termcap_build_color_sequence(char *buffer, size_t size, termcap_color_t fg, termcap_color_t bg);
int termcap_build_256_color_sequence(char *buffer, size_t size, int fg, int bg);
int termcap_build_rgb_color_sequence(char *buffer, size_t size, int r, int g, int b, bool foreground);
int termcap_build_cursor_move_sequence(char *buffer, size_t buffer_size, int cols, bool left);
int termcap_build_attribute_sequence(char *buffer, size_t size, termcap_attr_t attr);

/* Terminal detection and querying */
int termcap_send_query(const char *query, char *response, size_t response_size, int timeout_ms);
int termcap_parse_device_attributes(const char *response, char *terminal_name, char *version);
int termcap_detect_terminal_from_env(char *name, char *version);

/* Capability testing functions */
bool termcap_test_basic_colors(void);
bool termcap_test_256_colors(void);
bool termcap_test_truecolor(void);
bool termcap_test_mouse_support(void);
bool termcap_test_bracketed_paste(void);
bool termcap_test_focus_events(void);
bool termcap_test_alternate_screen(void);
bool termcap_test_title_setting(void);
bool termcap_test_cursor_styling(void);
bool termcap_test_synchronized_output(void);
bool termcap_test_unicode_support(void);

/* Database functions */
int termcap_load_capabilities_from_db(const char *terminal_name, termcap_capabilities_t *caps);

/* Performance optimization */
int termcap_optimize_sequences(void);
int termcap_benchmark_terminal_performance(void);
void termcap_cache_capabilities(const termcap_capabilities_t *caps, const char *name, const char *version);
bool termcap_get_cached_capabilities(termcap_capabilities_t *caps, char *name, char *version);

/* Output management */
int termcap_flush_output(void);
int termcap_setup_batch_mode(void);
int termcap_cleanup_batch_mode(void);

/* Error handling and logging */
void termcap_log_error(const char *function, const char *message);
void termcap_log_warning(const char *function, const char *message);
void termcap_log_debug(const char *function, const char *message);

/* Utility functions */
bool termcap_is_valid_color(termcap_color_t color);
bool termcap_is_valid_256_color(int color);
bool termcap_is_valid_rgb_component(int component);
int termcap_clamp_coordinate(int value, int max_value);
size_t termcap_safe_strncpy(char *dest, const char *src, size_t size);

/* Pattern matching and parsing */
bool termcap_parse_color_response(const char *response, int *r, int *g, int *b);
bool termcap_parse_cursor_position(const char *response, int *row, int *col);
bool termcap_parse_terminal_size(const char *response, int *rows, int *cols);

/* ======================= Built-in Terminal Database ======================= */

/* ======================= Capability Test Registry ======================= */

extern const capability_test_t termcap_capability_tests[];
extern const size_t termcap_capability_tests_count;

/* ======================= Global State ======================= */

/* ======================= Debugging and Testing ======================= */

#ifdef TERMCAP_DEBUG
#define TERMCAP_DEBUG_LOG(fmt, ...) \
    fprintf(stderr, "[TERMCAP DEBUG] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define TERMCAP_DEBUG_LOG(fmt, ...) do { } while(0)
#endif

/* Test and validation functions */
int termcap_validate_state(void);
int termcap_run_capability_tests(void);
int termcap_benchmark_all_operations(void);
void termcap_dump_state(FILE *stream);
void termcap_dump_capabilities(void);

/* ======================= Platform-Specific Helpers ======================= */

#ifdef __linux__
#define TERMCAP_PLATFORM_LINUX 1
#endif

#ifdef __APPLE__
#define TERMCAP_PLATFORM_MACOS 1
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define TERMCAP_PLATFORM_BSD 1
#endif

/* Platform-specific optimizations */
int termcap_platform_init(void);
int termcap_platform_cleanup(void);
int termcap_platform_get_terminal_size(int *rows, int *cols);
bool termcap_platform_supports_feature(const char *feature);

/* ======================= Memory Management ======================= */

void *termcap_malloc(size_t size);
void *termcap_realloc(void *ptr, size_t size);
void termcap_free(void *ptr);
char *termcap_strdup(const char *str);

/* ======================= Thread Safety (Future) ======================= */

#ifdef TERMCAP_THREAD_SAFE
typedef struct termcap_mutex termcap_mutex_t;
int termcap_mutex_init(termcap_mutex_t *mutex);
int termcap_mutex_lock(termcap_mutex_t *mutex);
int termcap_mutex_unlock(termcap_mutex_t *mutex);
int termcap_mutex_destroy(termcap_mutex_t *mutex);
#endif

#ifdef __cplusplus
}
#endif

#endif /* TERMCAP_INTERNAL_H */
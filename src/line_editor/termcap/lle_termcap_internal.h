/**
 * lle_termcap_internal.h - Internal definitions for enhanced LLE termcap implementation
 *
 * This header contains internal structures, constants, and function declarations
 * that are not exposed in the public API but are shared between LLE termcap modules.
 */

#ifndef LLE_TERMCAP_INTERNAL_H
#define LLE_TERMCAP_INTERNAL_H

#include "lle_termcap.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= Internal Constants ======================= */

/* Maximum sizes */
#define LLE_TERMCAP_MAX_ESCAPE_SEQUENCE 128
#define LLE_TERMCAP_MAX_TERMINAL_NAME 64
#define LLE_TERMCAP_MAX_TERMINAL_VERSION 32
#define LLE_TERMCAP_MAX_QUERY_RESPONSE 256
#define LLE_TERMCAP_MAX_CAPABILITY_TESTS 32

/* Timeouts and performance */
#define LLE_TERMCAP_FAST_QUERY_THRESHOLD_MS 10
#define LLE_TERMCAP_SLOW_QUERY_THRESHOLD_MS 100
#define LLE_TERMCAP_MAX_QUERY_ATTEMPTS 3
#define LLE_TERMCAP_CAPABILITY_CACHE_SIZE 64

/* Terminal identification patterns */
#define LLE_TERMCAP_TERM_PATTERN_MAX 16
#define LLE_TERMCAP_TERM_RESPONSE_TIMEOUT_MS 200

/* ======================= Internal Structures ======================= */

/* Pre-compiled escape sequences for performance */
typedef struct {
    char sequence[LLE_TERMCAP_MAX_ESCAPE_SEQUENCE];
    size_t length;
    bool cached;
} lle_escape_sequence_t;

/* Terminal database entry */
typedef struct {
    const char *name;
    const char *pattern;            /* Pattern for TERM matching */
    const char *version_pattern;    /* Pattern for version detection */
    lle_termcap_capabilities_t caps;
    int priority;                   /* Higher priority = preferred match */
} lle_terminal_db_entry_t;

/* Capability test function */
typedef struct {
    const char *name;
    bool (*test_func)(void);
    const char *query_sequence;
    const char *expected_response_pattern;
    int timeout_ms;
} lle_capability_test_t;

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
        lle_termcap_capabilities_t caps;
        char terminal_name[LLE_TERMCAP_MAX_TERMINAL_NAME];
        char terminal_version[LLE_TERMCAP_MAX_TERMINAL_VERSION];
    } capability_cache;
    
    /* Output batching */
    lle_termcap_output_buffer_t *current_buffer;
    
    /* Line editor integration */
    lle_termcap_color_t line_editor_prompt_color;
    lle_termcap_color_t line_editor_input_color;
    lle_termcap_color_t line_editor_completion_color;
    int line_editor_completion_style;
    bool line_editor_mouse_enabled;
    
} lle_termcap_internal_state_t;

/* ======================= Pre-compiled Escape Sequences ======================= */

/* Common escape sequences - pre-compiled for performance */
extern const lle_escape_sequence_t LLE_ESC_RESET_ALL;
extern const lle_escape_sequence_t LLE_ESC_CLEAR_SCREEN;
extern const lle_escape_sequence_t LLE_ESC_CLEAR_LINE;
extern const lle_escape_sequence_t LLE_ESC_CLEAR_TO_EOL;
extern const lle_escape_sequence_t LLE_ESC_CLEAR_TO_BOL;
extern const lle_escape_sequence_t LLE_ESC_CLEAR_TO_EOS;
extern const lle_escape_sequence_t LLE_ESC_CURSOR_HOME;
extern const lle_escape_sequence_t LLE_ESC_HIDE_CURSOR;
extern const lle_escape_sequence_t LLE_ESC_SHOW_CURSOR;
extern const lle_escape_sequence_t LLE_ESC_SAVE_CURSOR;
extern const lle_escape_sequence_t LLE_ESC_RESTORE_CURSOR;
extern const lle_escape_sequence_t LLE_ESC_CURSOR_QUERY;
extern const lle_escape_sequence_t LLE_ESC_DEVICE_ATTRIBUTES;
extern const lle_escape_sequence_t LLE_ESC_ALTERNATE_SCREEN_ENTER;
extern const lle_escape_sequence_t LLE_ESC_ALTERNATE_SCREEN_EXIT;
extern const lle_escape_sequence_t LLE_ESC_MOUSE_ENABLE_ALL;
extern const lle_escape_sequence_t LLE_ESC_MOUSE_DISABLE;
extern const lle_escape_sequence_t LLE_ESC_BRACKETED_PASTE_ENABLE;
extern const lle_escape_sequence_t LLE_ESC_BRACKETED_PASTE_DISABLE;
extern const lle_escape_sequence_t LLE_ESC_FOCUS_EVENTS_ENABLE;
extern const lle_escape_sequence_t LLE_ESC_FOCUS_EVENTS_DISABLE;
extern const lle_escape_sequence_t LLE_ESC_SYNCHRONIZED_OUTPUT_BEGIN;
extern const lle_escape_sequence_t LLE_ESC_SYNCHRONIZED_OUTPUT_END;

/* ======================= Internal Function Declarations ======================= */

/* Core internal functions */
lle_termcap_internal_state_t *lle_termcap_get_internal_state(void);

/* Sequence generation */
int lle_termcap_build_color_sequence(char *buffer, size_t size, lle_termcap_color_t fg, lle_termcap_color_t bg);
int lle_termcap_build_256_color_sequence(char *buffer, size_t size, int fg, int bg);
int lle_termcap_build_rgb_color_sequence(char *buffer, size_t size, int r, int g, int b, bool foreground);
int lle_termcap_build_cursor_move_sequence(char *buffer, size_t buffer_size, int cols, bool left);
int lle_termcap_build_attribute_sequence(char *buffer, size_t size, lle_termcap_attr_t attr);

/* Terminal detection and querying */
int lle_termcap_send_query(const char *query, char *response, size_t response_size, int timeout_ms);
int lle_termcap_parse_device_attributes(const char *response, char *terminal_name, char *version);
int lle_termcap_detect_terminal_from_env(char *name, char *version);

/* Capability testing functions */
bool lle_termcap_test_basic_colors(void);
bool lle_termcap_test_256_colors(void);
bool lle_termcap_test_truecolor(void);
bool lle_termcap_test_mouse_support(void);
bool lle_termcap_test_bracketed_paste(void);
bool lle_termcap_test_focus_events(void);
bool lle_termcap_test_alternate_screen(void);
bool lle_termcap_test_title_setting(void);
bool lle_termcap_test_cursor_styling(void);
bool lle_termcap_test_synchronized_output(void);
bool lle_termcap_test_unicode_support(void);

/* Database functions */
int lle_termcap_load_capabilities_from_db(const char *terminal_name, lle_termcap_capabilities_t *caps);

/* Performance optimization */
int lle_termcap_optimize_sequences(void);
int lle_termcap_benchmark_terminal_performance(void);
void lle_termcap_cache_capabilities(const lle_termcap_capabilities_t *caps, const char *name, const char *version);
bool lle_termcap_get_cached_capabilities(lle_termcap_capabilities_t *caps, char *name, char *version);

/* Output management */
int lle_termcap_flush_output(void);
int lle_termcap_setup_batch_mode(void);
int lle_termcap_cleanup_batch_mode(void);

/* Error handling and logging */
void lle_termcap_log_error(const char *function, const char *message);
void lle_termcap_log_warning(const char *function, const char *message);
void lle_termcap_log_debug(const char *function, const char *message);

/* Utility functions */
bool lle_termcap_is_valid_color(lle_termcap_color_t color);
bool lle_termcap_is_valid_256_color(int color);
bool lle_termcap_is_valid_rgb_component(int component);
int lle_termcap_clamp_coordinate(int value, int max_value);
size_t lle_termcap_safe_strncpy(char *dest, const char *src, size_t size);

/* Pattern matching and parsing */
bool lle_termcap_parse_color_response(const char *response, int *r, int *g, int *b);
bool lle_termcap_parse_cursor_position(const char *response, int *row, int *col);
bool lle_termcap_parse_terminal_size(const char *response, int *rows, int *cols);

/* ======================= Built-in Terminal Database ======================= */

/* ======================= Capability Test Registry ======================= */

extern const lle_capability_test_t lle_termcap_capability_tests[];
extern const size_t lle_termcap_capability_tests_count;

/* ======================= Global State ======================= */

/* ======================= Debugging and Testing ======================= */

#ifdef LLE_TERMCAP_DEBUG
#define LLE_TERMCAP_DEBUG_LOG(fmt, ...) \
    fprintf(stderr, "[LLE_TERMCAP DEBUG] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define LLE_TERMCAP_DEBUG_LOG(fmt, ...) do { } while(0)
#endif

/* Test and validation functions */
int lle_termcap_validate_state(void);
int lle_termcap_run_capability_tests(void);
int lle_termcap_benchmark_all_operations(void);
void lle_termcap_dump_state(FILE *stream);
void lle_termcap_dump_capabilities(void);

/* ======================= Platform-Specific Helpers ======================= */

#ifdef __linux__
#define LLE_TERMCAP_PLATFORM_LINUX 1
#endif

#ifdef __APPLE__
#define LLE_TERMCAP_PLATFORM_MACOS 1
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define LLE_TERMCAP_PLATFORM_BSD 1
#endif

/* Platform-specific optimizations */
int lle_termcap_platform_init(void);
int lle_termcap_platform_cleanup(void);
int lle_termcap_platform_get_terminal_size(int *rows, int *cols);
bool lle_termcap_platform_supports_feature(const char *feature);

/* ======================= Memory Management ======================= */

void *lle_termcap_malloc(size_t size);
void *lle_termcap_realloc(void *ptr, size_t size);
void lle_termcap_free(void *ptr);
char *lle_termcap_strdup(const char *str);

/* ======================= Thread Safety (Future) ======================= */

#ifdef LLE_TERMCAP_THREAD_SAFE
typedef struct lle_termcap_mutex lle_termcap_mutex_t;
int lle_termcap_mutex_init(lle_termcap_mutex_t *mutex);
int lle_termcap_mutex_lock(lle_termcap_mutex_t *mutex);
int lle_termcap_mutex_unlock(lle_termcap_mutex_t *mutex);
int lle_termcap_mutex_destroy(lle_termcap_mutex_t *mutex);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LLE_TERMCAP_INTERNAL_H */

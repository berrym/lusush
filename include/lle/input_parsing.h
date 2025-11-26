/**
 * LLE Input Parsing System
 * 
 * Comprehensive, high-performance parsing of terminal input streams, converting
 * raw terminal data into structured events for the LLE event system.
 * 
 * Specification: docs/lle_specification/06_input_parsing_complete.md
 * Version: 2.0.0
 * 
 * Features:
 * - Universal terminal support (all major terminal types)
 * - Complete Unicode/UTF-8 processing with grapheme awareness
 * - Sub-millisecond parsing (<250μs target)
 * - Intelligent sequence detection and disambiguation
 * - Comprehensive error recovery
 * - Event system integration
 * - Keybinding integration (<10μs lookup)
 * - Widget hook triggers
 * - Adaptive terminal integration
 */

#ifndef LLE_INPUT_PARSING_H
#define LLE_INPUT_PARSING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "error_handling.h"
#include "memory_management.h"
#include "performance.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants and Configuration
 * ============================================================================ */

/* Buffer sizes */
#define LLE_INPUT_BUFFER_SIZE                 4096     /* Input stream buffer */
#define LLE_INPUT_PARSE_BUFFER_SIZE           2048     /* Parse buffer size */
#define LLE_MAX_SEQUENCE_LENGTH               256      /* Max escape sequence */
#define LLE_MAX_KEY_SEQUENCE_LENGTH           32       /* Max key sequence */
#define LLE_MAX_CSI_PARAMETERS                16       /* Max CSI parameters */

/* Performance limits */
#define LLE_MAX_PARSE_TIME_US                 250      /* 250μs max parse time */
#define LLE_TARGET_THROUGHPUT_CHARS_PER_SEC   100000   /* 100K chars/second */
#define LLE_TARGET_EVENT_GENERATION_TIME_US   50       /* 50μs per event */
#define LLE_MAX_PARSER_MEMORY_MB              16       /* 16MB memory limit */
#define LLE_KEYBINDING_LOOKUP_TARGET_US       10       /* 10μs keybinding lookup */

/* Timeouts */
#define LLE_MAX_SEQUENCE_TIMEOUT_US           100000   /* 100ms sequence timeout */
#define LLE_KEY_SEQUENCE_TIMEOUT_US           100000   /* 100ms key timeout */
#define LLE_MOUSE_DOUBLE_CLICK_TIME_MS        500      /* 500ms double-click */
#define LLE_MOUSE_TRIPLE_CLICK_TIME_MS        750      /* 750ms triple-click */

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

/* Core system structures */
typedef struct lle_input_parser_system lle_input_parser_system_t;
typedef struct lle_input_stream lle_input_stream_t;
typedef struct lle_input_buffer lle_input_buffer_t;
typedef struct lle_sequence_parser lle_sequence_parser_t;
typedef struct lle_utf8_processor lle_utf8_processor_t;
typedef struct lle_key_detector lle_key_detector_t;
typedef struct lle_mouse_parser lle_mouse_parser_t;
typedef struct lle_parser_state_machine lle_parser_state_machine_t;

/* Integration structures */
typedef struct lle_keybinding_integration lle_keybinding_integration_t;
typedef struct lle_widget_hook_triggers lle_widget_hook_triggers_t;
typedef struct lle_adaptive_terminal_parser lle_adaptive_terminal_parser_t;
typedef struct lle_input_coordinator lle_input_coordinator_t;
typedef struct lle_input_conflict_resolver lle_input_conflict_resolver_t;

/* Data structures */
typedef struct lle_parsed_input lle_parsed_input_t;
typedef struct lle_key_info lle_key_info_t;
typedef struct lle_mouse_event_info lle_mouse_event_info_t;
typedef struct lle_text_input_info lle_text_input_info_t;

/* External system references (from other specs) */
typedef struct lle_terminal_system lle_terminal_system_t;
typedef struct lle_terminal_capabilities lle_terminal_capabilities_t;
typedef struct lle_keybinding_engine lle_keybinding_engine_t;
typedef struct lle_widget_hooks_manager lle_widget_hooks_manager_t;
typedef struct lle_adaptive_terminal_integration lle_adaptive_terminal_integration_t;
/* Note: lle_event_system_t is already defined in performance.h and testing.h */

/* ============================================================================
 * Enumerations
 * ============================================================================ */

/**
 * Terminal escape sequence types
 */
typedef enum {
    LLE_SEQ_TYPE_UNKNOWN,                      /* Unknown sequence */
    LLE_SEQ_TYPE_CONTROL_CHAR,                 /* Control character (0x00-0x1F) */
    LLE_SEQ_TYPE_CSI,                          /* Control Sequence Introducer (ESC [) */
    LLE_SEQ_TYPE_OSC,                          /* Operating System Command (ESC ]) */
    LLE_SEQ_TYPE_DCS,                          /* Device Control String (ESC P) */
    LLE_SEQ_TYPE_APC,                          /* Application Program Command (ESC _) */
    LLE_SEQ_TYPE_PM,                           /* Privacy Message (ESC ^) */
    LLE_SEQ_TYPE_SS2,                          /* Single Shift Two (ESC N) */
    LLE_SEQ_TYPE_SS3,                          /* Single Shift Three (ESC O) */
    LLE_SEQ_TYPE_FUNCTION_KEY,                 /* Function key sequence */
    LLE_SEQ_TYPE_CURSOR_KEY,                   /* Cursor movement key */
    LLE_SEQ_TYPE_MODIFIER_KEY,                 /* Modified key combination */
    LLE_SEQ_TYPE_MOUSE_EVENT,                  /* Mouse event sequence */
    LLE_SEQ_TYPE_BRACKETED_PASTE,              /* Bracketed paste mode */
    LLE_SEQ_TYPE_FOCUS_EVENT,                  /* Focus in/out event */
    LLE_SEQ_TYPE_TERMINAL_QUERY_RESPONSE,      /* Terminal capability response */
} lle_sequence_type_t;

/**
 * Parser state machine states
 */
typedef enum {
    LLE_PARSER_STATE_NORMAL,                   /* Normal text input */
    LLE_PARSER_STATE_ESCAPE,                   /* Escape sequence started */
    LLE_PARSER_STATE_CSI,                      /* CSI sequence in progress */
    LLE_PARSER_STATE_OSC,                      /* OSC sequence in progress */
    LLE_PARSER_STATE_DCS,                      /* DCS sequence in progress */
    LLE_PARSER_STATE_UTF8_MULTIBYTE,           /* UTF-8 multibyte sequence */
    LLE_PARSER_STATE_KEY_SEQUENCE,             /* Key sequence detection */
    LLE_PARSER_STATE_MOUSE,                    /* Mouse event parsing */
    LLE_PARSER_STATE_ERROR_RECOVERY,           /* Error recovery */
} lle_parser_state_t;

/**
 * Parsed input types (distinct from lle_input_type_t in terminal_abstraction.h)
 */
typedef enum {
    LLE_PARSED_INPUT_TYPE_UNKNOWN,             /* Unknown input */
    LLE_PARSED_INPUT_TYPE_TEXT,                /* Regular text input */
    LLE_PARSED_INPUT_TYPE_KEY,                 /* Key press/combination */
    LLE_PARSED_INPUT_TYPE_MOUSE,               /* Mouse event */
    LLE_PARSED_INPUT_TYPE_SEQUENCE,            /* Terminal sequence */
    LLE_PARSED_INPUT_TYPE_PASTE,               /* Bracketed paste */
    LLE_PARSED_INPUT_TYPE_FOCUS,               /* Focus event */
} lle_parsed_input_type_t;

/**
 * Key types
 */
typedef enum {
    LLE_KEY_TYPE_UNKNOWN,                      /* Unknown key */
    LLE_KEY_TYPE_REGULAR,                      /* Regular character key */
    LLE_KEY_TYPE_CONTROL,                      /* Control character */
    LLE_KEY_TYPE_FUNCTION,                     /* Function key (F1-F24) */
    LLE_KEY_TYPE_CURSOR,                       /* Cursor movement key */
    LLE_KEY_TYPE_EDITING,                      /* Editing key (Insert, Delete, etc.) */
    LLE_KEY_TYPE_MODIFIER,                     /* Modifier key (Ctrl, Alt, Shift) */
    LLE_KEY_TYPE_SPECIAL,                      /* Special key (Tab, Enter, Esc) */
} lle_key_type_t;

/**
 * Key modifier flags
 */
typedef enum {
    LLE_KEY_MOD_NONE    = 0,                   /* No modifiers */
    LLE_KEY_MOD_SHIFT   = (1 << 0),            /* Shift key */
    LLE_KEY_MOD_ALT     = (1 << 1),            /* Alt key */
    LLE_KEY_MOD_CTRL    = (1 << 2),            /* Control key */
    LLE_KEY_MOD_META    = (1 << 3),            /* Meta/Super key */
} lle_key_modifiers_t;

/**
 * Mouse event types
 */
typedef enum {
    LLE_MOUSE_EVENT_PRESS,                     /* Mouse button press */
    LLE_MOUSE_EVENT_RELEASE,                   /* Mouse button release */
    LLE_MOUSE_EVENT_MOVE,                      /* Mouse movement */
    LLE_MOUSE_EVENT_WHEEL,                     /* Mouse wheel */
    LLE_MOUSE_EVENT_DRAG,                      /* Mouse drag */
} lle_mouse_event_type_t;

/**
 * Mouse buttons
 */
typedef enum {
    LLE_MOUSE_BUTTON_NONE = 0,                 /* No button */
    LLE_MOUSE_BUTTON_LEFT = 1,                 /* Left button */
    LLE_MOUSE_BUTTON_MIDDLE = 2,               /* Middle button */
    LLE_MOUSE_BUTTON_RIGHT = 3,                /* Right button */
    LLE_MOUSE_BUTTON_WHEEL_UP = 4,             /* Wheel up */
    LLE_MOUSE_BUTTON_WHEEL_DOWN = 5,           /* Wheel down */
} lle_mouse_button_t;

/**
 * Mouse tracking modes
 */
typedef enum {
    LLE_MOUSE_TRACKING_NONE,                   /* No mouse tracking */
    LLE_MOUSE_TRACKING_X10,                    /* X10 compatible */
    LLE_MOUSE_TRACKING_VT200,                  /* VT200 mouse */
    LLE_MOUSE_TRACKING_BTN_EVENT,              /* Button event tracking */
    LLE_MOUSE_TRACKING_ANY_EVENT,              /* Any event tracking */
    LLE_MOUSE_TRACKING_FOCUS_EVENT,            /* Focus event tracking */
} lle_mouse_tracking_mode_t;

/**
 * Key sequence match results
 */
typedef enum {
    LLE_MATCH_NONE,                            /* No match */
    LLE_MATCH_EXACT,                           /* Exact match found */
    LLE_MATCH_PREFIX,                          /* Prefix match (need more data) */
    LLE_MATCH_AMBIGUOUS,                       /* Multiple possible matches */
} lle_key_sequence_match_type_t;

/**
 * UTF-8 validation results
 */
typedef enum {
    LLE_UTF8_VALID,                            /* Valid UTF-8 sequence */
    LLE_UTF8_INCOMPLETE,                       /* Incomplete sequence */
    LLE_UTF8_INVALID,                          /* Invalid sequence */
    LLE_UTF8_OVERLONG,                         /* Overlong encoding */
    LLE_UTF8_SURROGATE,                        /* Surrogate pair (invalid) */
} lle_utf8_validation_result_t;

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * Text input information
 */
typedef struct lle_text_input_info {
    char utf8_bytes[8];                        /* UTF-8 byte sequence */
    size_t utf8_length;                        /* Number of bytes */
    uint32_t codepoint;                        /* Unicode codepoint */
    bool is_grapheme_start;                    /* Grapheme cluster start */
    uint32_t display_width;                    /* Display width (columns) */
    uint64_t timestamp;                        /* Input timestamp */
} lle_text_input_info_t;

/**
 * Detected key information
 */
struct lle_key_info {
    lle_key_type_t type;                       /* Key type */
    uint32_t keycode;                          /* Key code */
    lle_key_modifiers_t modifiers;             /* Modifier flags */
    char key_name[32];                         /* Key name */
    char sequence[LLE_MAX_KEY_SEQUENCE_LENGTH]; /* Original sequence */
    size_t sequence_length;                    /* Sequence length */
    bool is_repeat;                            /* Key repeat */
    uint64_t timestamp;                        /* Detection timestamp */
};

/**
 * Parsed mouse event information
 */
struct lle_mouse_event_info {
    lle_mouse_event_type_t type;               /* Event type */
    lle_mouse_button_t button;                 /* Button involved */
    uint16_t x, y;                             /* Mouse coordinates */
    lle_key_modifiers_t modifiers;             /* Modifier keys */
    int16_t wheel_delta;                       /* Wheel delta */
    bool double_click;                         /* Double-click detected */
    bool triple_click;                         /* Triple-click detected */
    uint64_t timestamp;                        /* Event timestamp */
};

/**
 * Parsed input result
 */
struct lle_parsed_input {
    lle_parsed_input_type_t type;              /* Parsed input type */
    
    union {
        lle_text_input_info_t text_info;       /* Text input */
        lle_key_info_t key_info;               /* Key information */
        lle_mouse_event_info_t mouse_info;     /* Mouse information */
    } data;
    
    bool handled;                              /* Input handled flag */
    uint64_t parse_time_us;                    /* Parse time in microseconds */
};

/**
 * Key sequence match result
 */
typedef struct {
    lle_key_sequence_match_type_t type;        /* Match type */
    lle_key_info_t key_info;                   /* Key information if matched */
    size_t matched_length;                     /* Bytes matched */
    bool needs_timeout;                        /* Needs timeout resolution */
} lle_key_sequence_match_result_t;

/**
 * Input stream management
 */
struct lle_input_stream {
    int terminal_fd;                           /* Terminal file descriptor */
    lle_terminal_capabilities_t *terminal_caps; /* Terminal capabilities */
    
    /* Buffering */
    char *buffer;                              /* Input buffer */
    size_t buffer_size;                        /* Buffer size */
    size_t buffer_used;                        /* Bytes in buffer */
    size_t buffer_pos;                         /* Current read position */
    
    /* Flow control */
    bool flow_control_enabled;                 /* Flow control active */
    bool blocking_mode;                        /* Blocking reads */
    
    /* Statistics */
    uint64_t bytes_read;                       /* Total bytes read */
    uint64_t read_operations;                  /* Read operation count */
    uint64_t buffer_overflows;                 /* Overflow count */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
};

/**
 * Input buffer management
 */
struct lle_input_buffer {
    char *data;                                /* Buffer data */
    size_t capacity;                           /* Buffer capacity */
    size_t used;                               /* Bytes used */
    size_t read_pos;                           /* Read position */
    size_t write_pos;                          /* Write position */
    
    /* Buffer management */
    bool circular;                             /* Circular buffer mode */
    size_t high_water_mark;                    /* High water mark */
    size_t low_water_mark;                     /* Low water mark */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
};

/**
 * Sequence parser structure
 */
struct lle_sequence_parser {
    lle_sequence_type_t type;                  /* Current sequence type */
    lle_parser_state_t state;                  /* Current parsing state */
    char buffer[LLE_MAX_SEQUENCE_LENGTH];      /* Sequence buffer */
    size_t buffer_pos;                         /* Current buffer position */
    size_t buffer_capacity;                    /* Buffer capacity */
    uint64_t sequence_start_time;              /* Sequence start timestamp */
    lle_terminal_capabilities_t *terminal_caps; /* Terminal capabilities */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
    
    /* Parsing context */
    uint32_t parameters[LLE_MAX_CSI_PARAMETERS]; /* CSI parameters */
    uint8_t parameter_count;                   /* Number of parameters */
    char intermediate_chars[8];                /* Intermediate characters */
    uint8_t intermediate_count;                /* Intermediate character count */
    char final_char;                           /* Final character */
    
    /* Error tracking */
    uint32_t malformed_sequences;              /* Malformed sequence count */
    uint32_t timeout_sequences;                /* Timed-out sequence count */
    uint64_t last_error_time;                  /* Last error timestamp */
};

/**
 * UTF-8 processor structure
 */
struct lle_utf8_processor {
    /* Current UTF-8 sequence */
    char utf8_buffer[8];                       /* UTF-8 sequence buffer */
    size_t utf8_pos;                           /* Current position */
    size_t expected_bytes;                     /* Expected byte count */
    uint32_t current_codepoint;                /* Decoded codepoint */
    
    /* Grapheme cluster detection */
    uint32_t previous_codepoint;               /* Previous codepoint */
    bool in_grapheme_cluster;                  /* In cluster */
    
    /* Statistics */
    uint64_t codepoints_processed;             /* Total codepoints */
    uint64_t grapheme_clusters_detected;       /* Total graphemes */
    uint64_t invalid_sequences_handled;        /* Invalid sequences */
};

/**
 * UTF-8 processor statistics
 */
typedef struct {
    uint64_t codepoints_processed;             /* Total codepoints processed */
    uint64_t grapheme_clusters_detected;       /* Total grapheme clusters */
    uint64_t invalid_sequences_handled;        /* Invalid sequences handled */
    size_t partial_sequence_length;            /* Current partial sequence length */
} lle_utf8_processor_stats_t;

/**
 * Codepoint information with grapheme boundary data
 */
typedef struct {
    uint32_t codepoint;                        /* Unicode codepoint */
    bool is_grapheme_boundary;                 /* Starts new grapheme cluster */
    int width;                                 /* Display width (0, 1, or 2) */
} lle_codepoint_info_t;

/**
 * Key sequence mapping entry
 */
typedef struct {
    char sequence[LLE_MAX_KEY_SEQUENCE_LENGTH]; /* Key sequence */
    size_t sequence_length;                    /* Sequence length */
    lle_key_info_t key_info;                   /* Key information */
} lle_key_sequence_map_entry_t;

/**
 * Key sequence map (hash table would be used in implementation)
 */
typedef struct {
    lle_key_sequence_map_entry_t *entries;     /* Map entries */
    size_t entry_count;                        /* Number of entries */
    size_t capacity;                           /* Map capacity */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
} lle_key_sequence_map_t;

/**
 * Key sequence detector
 */
struct lle_key_detector {
    lle_terminal_capabilities_t *terminal_caps; /* Terminal capabilities */
    lle_key_sequence_map_t *sequence_map;      /* Sequence to key mapping */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
    
    /* Current sequence detection */
    char sequence_buffer[LLE_MAX_KEY_SEQUENCE_LENGTH]; /* Sequence buffer */
    size_t sequence_pos;                       /* Current position */
    uint64_t sequence_start_time;              /* Sequence start time */
    bool ambiguous_sequence;                   /* Ambiguous flag */
    
    /* Statistics */
    uint64_t sequences_detected;               /* Sequences detected */
    uint64_t sequences_resolved;               /* Sequences resolved */
    uint64_t ambiguous_timeouts;               /* Ambiguous sequence timeouts */
};

/**
 * Mouse parser structure
 */
struct lle_mouse_parser {
    lle_terminal_capabilities_t *terminal_caps; /* Terminal capabilities */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
    
    /* Mouse tracking state */
    bool mouse_tracking_enabled;               /* Mouse tracking active */
    lle_mouse_tracking_mode_t tracking_mode;   /* Tracking mode */
    bool button_event_tracking;                /* Button event tracking */
    bool motion_event_tracking;                /* Motion event tracking */
    bool focus_event_tracking;                 /* Focus event tracking */
    
    /* Current mouse state */
    uint16_t last_x, last_y;                   /* Last mouse position */
    lle_mouse_button_t pressed_buttons;        /* Currently pressed buttons */
    lle_key_modifiers_t modifiers;             /* Modifier keys */
    uint64_t last_click_time;                  /* Last click timestamp */
    uint32_t click_count;                      /* Click count for multi-click */
    
    /* Performance statistics */
    uint64_t mouse_events_parsed;              /* Total mouse events */
    uint64_t invalid_mouse_sequences;          /* Invalid sequences */
};

/**
 * Parser state machine
 */
struct lle_parser_state_machine {
    lle_parser_state_t current_state;          /* Current state */
    lle_parser_state_t previous_state;         /* Previous state */
    
    /* State transition tracking */
    uint64_t state_transitions;                /* Total transitions */
    uint64_t state_change_time;                /* Last state change time */
    
    /* Error handling */
    lle_error_context_t *error_ctx;            /* Error context */
    uint32_t error_recoveries;                 /* Error recovery count */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
};

/**
 * Input coordinator - coordinates input processing across systems
 */
struct lle_input_coordinator {
    /* Coordination state */
    bool coordination_active;                  /* Active flag */
    uint64_t coordination_operations;          /* Operation count */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
};

/**
 * Input conflict resolver - resolves conflicts between input processing paths
 */
struct lle_input_conflict_resolver {
    /* Conflict resolution state */
    uint32_t conflicts_detected;               /* Conflicts detected */
    uint32_t conflicts_resolved;               /* Conflicts resolved */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
};

/**
 * Keybinding integration for input parsing
 */
struct lle_keybinding_integration {
    lle_keybinding_engine_t *keybinding_engine; /* Keybinding engine reference */
    
    /* Lookup cache (would use hash table in implementation) */
    void *lookup_cache;                        /* Fast keybinding lookup cache */
    
    /* Multi-key sequence buffering */
    char sequence_buffer[LLE_MAX_KEY_SEQUENCE_LENGTH]; /* Sequence buffer */
    size_t sequence_length;                    /* Current sequence length */
    uint64_t sequence_start_time;              /* Sequence start time */
    
    /* Timeout management */
    uint64_t sequence_timeout_us;              /* Sequence timeout */
    
    /* Performance metrics */
    uint64_t lookups_performed;                /* Total lookups */
    uint64_t lookup_hits;                      /* Cache hits */
    uint64_t lookup_misses;                    /* Cache misses */
    uint64_t total_lookup_time_us;             /* Total lookup time */
    uint64_t max_lookup_time_us;               /* Maximum lookup time */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
    
    /* State management */
    bool sequence_in_progress;                 /* Multi-key sequence active */
};

/**
 * Widget hook trigger system for input parsing
 */
struct lle_widget_hook_triggers {
    lle_widget_hooks_manager_t *hooks_manager; /* Widget hooks manager reference */
    
    /* Hook trigger mapping (would use more complex structure in implementation) */
    void *trigger_map;                         /* Input event to hook mapping */
    void *execution_queue;                     /* Hook execution queue */
    
    /* Hook trigger conditions */
    void *condition_engine;                    /* Hook trigger condition evaluation */
    void *hook_filters;                        /* Hook execution filters */
    
    /* Performance optimization */
    void *hook_cache;                          /* Hook execution cache */
    
    /* Performance metrics */
    uint64_t hooks_triggered;                  /* Total hooks triggered */
    uint64_t hooks_executed;                   /* Total hooks executed */
    uint64_t total_execution_time_us;          /* Total execution time */
    uint64_t max_execution_time_us;            /* Maximum execution time */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
    
    /* State management */
    bool hook_execution_enabled;               /* Hook execution state */
};

/**
 * Adaptive terminal parser - adapts to terminal capabilities
 */
struct lle_adaptive_terminal_parser {
    lle_adaptive_terminal_integration_t *adaptive_terminal; /* Adaptive terminal reference */
    
    /* Terminal-specific optimization */
    void *terminal_specific_parsers;           /* Terminal-specific parsers */
    
    /* Capability adaptation */
    bool capability_detected;                  /* Capabilities detected */
    uint32_t supported_features;               /* Feature flags */
    
    /* Performance metrics */
    uint64_t adaptations_performed;            /* Adaptations performed */
    
    /* Memory management */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
};

/**
 * Input parser performance monitoring
 */
typedef struct {
    uint64_t total_chars_processed;            /* Total characters processed */
    uint64_t total_processing_time_us;         /* Total processing time */
    uint64_t max_processing_time_us;           /* Maximum processing time */
    uint64_t events_generated;                 /* Total events generated */
    uint32_t chars_per_second;                 /* Current throughput */
    uint32_t peak_chars_per_second;            /* Peak throughput */
    uint32_t parse_errors;                     /* Total parsing errors */
    uint32_t recovery_operations;              /* Error recoveries */
    uint64_t last_performance_check;           /* Last performance check */
} lle_input_parser_performance_t;

/**
 * Input cache for performance optimization
 */
typedef struct {
    void *cache_data;                          /* Cache implementation */
    size_t cache_size;                         /* Cache size */
    uint64_t cache_hits;                       /* Cache hits */
    uint64_t cache_misses;                     /* Cache misses */
    lle_memory_pool_t *memory_pool;            /* Memory pool */
} lle_input_cache_t;

/**
 * Main input parser system structure
 */
struct lle_input_parser_system {
    /* Core parsing components */
    lle_input_stream_t *stream;                /* Input stream management */
    lle_sequence_parser_t *sequence_parser;    /* Escape sequence parser */
    lle_utf8_processor_t *utf8_processor;      /* UTF-8 Unicode processor */
    lle_key_detector_t *key_detector;          /* Key sequence detector */
    lle_mouse_parser_t *mouse_parser;          /* Mouse event parser */
    lle_parser_state_machine_t *state_machine; /* Parser state machine */
    lle_input_buffer_t *input_buffer;          /* Input buffering system */
    
    /* Critical system integrations */
    lle_keybinding_integration_t *keybinding_integration;   /* Keybinding system integration */
    lle_widget_hook_triggers_t *widget_hook_triggers;       /* Widget hook trigger system */
    lle_adaptive_terminal_parser_t *adaptive_terminal;      /* Adaptive terminal integration */
    
    /* System coordination */
    lle_event_system_t *event_system;          /* Event system integration */
    lle_input_coordinator_t *coordinator;      /* Cross-system input coordination */
    lle_input_conflict_resolver_t *conflict_resolver; /* Input processing conflict resolution */
    
    /* Performance and optimization */
    lle_error_context_t *error_ctx;            /* Error handling context */
    lle_performance_monitor_t *perf_monitor;   /* Performance monitoring */
    lle_input_cache_t *input_cache;            /* Input processing cache */
    lle_memory_pool_t *memory_pool;            /* Memory management */
    
    /* Synchronization and state */
    pthread_mutex_t parser_mutex;              /* Thread synchronization */
    bool active;                               /* Parser active state */
    uint64_t bytes_processed;                  /* Total bytes processed */
    uint64_t keybinding_lookups;               /* Keybinding lookup count */
    uint64_t widget_hooks_triggered;           /* Widget hooks triggered count */
};

/* ============================================================================
 * Function Declarations - Input Parser System
 * ============================================================================ */

/**
 * Initialize input parser system
 * 
 * Creates and initializes the complete input parsing system with all subsystems
 * and integrations.
 * 
 * @param system Output pointer for created system
 * @param terminal Terminal system reference
 * @param event_system Event system reference
 * @param keybinding_engine Keybinding engine reference
 * @param widget_hooks Widget hooks manager reference
 * @param adaptive_terminal Adaptive terminal integration reference
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_input_parser_system_init(lle_input_parser_system_t **system,
                                          lle_terminal_system_t *terminal,
                                          lle_event_system_t *event_system,
                                          lle_keybinding_engine_t *keybinding_engine,
                                          lle_widget_hooks_manager_t *widget_hooks,
                                          lle_adaptive_terminal_integration_t *adaptive_terminal,
                                          lle_memory_pool_t *memory_pool);

/**
 * Destroy input parser system
 * 
 * Cleans up and destroys the input parser system, freeing all resources.
 * 
 * @param system Parser system to destroy
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_input_parser_system_destroy(lle_input_parser_system_t *system);

/**
 * Process input data
 * 
 * Main entry point for processing raw terminal input data. Parses input and
 * generates appropriate events.
 * 
 * @param system Parser system
 * @param data Raw input data
 * @param data_len Data length in bytes
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_input_parser_process_data(lle_input_parser_system_t *system,
                                           const char *data,
                                           size_t data_len);

/**
 * Reset parser system state
 * 
 * Resets all parser components to initial state. Used for error recovery.
 * 
 * @param system Parser system
 * @return LLE_SUCCESS on success, error code on failure
 */
lle_result_t lle_input_parser_system_reset(lle_input_parser_system_t *system);

/* ============================================================================
 * Function Declarations - Input Stream Management
 * ============================================================================ */

lle_result_t lle_input_stream_init(lle_input_stream_t **stream,
                                   lle_terminal_system_t *terminal,
                                   lle_memory_pool_t *memory_pool);

lle_result_t lle_input_stream_destroy(lle_input_stream_t *stream);

lle_result_t lle_input_stream_read(lle_input_stream_t *stream,
                                   char *buffer,
                                   size_t buffer_size,
                                   size_t *bytes_read);

lle_result_t lle_input_stream_buffer_data(lle_input_stream_t *stream,
                                          const char *data,
                                          size_t data_len);

lle_result_t lle_input_stream_get_buffered(lle_input_stream_t *stream,
                                           const char **data,
                                           size_t *data_len);

lle_result_t lle_input_stream_consume(lle_input_stream_t *stream,
                                      size_t bytes);

lle_result_t lle_input_stream_peek(lle_input_stream_t *stream,
                                   size_t offset,
                                   char *byte);

lle_result_t lle_input_stream_set_blocking(lle_input_stream_t *stream,
                                           bool blocking);

lle_result_t lle_input_stream_set_flow_control(lle_input_stream_t *stream,
                                               bool enabled);

lle_result_t lle_input_stream_get_statistics(lle_input_stream_t *stream,
                                             uint64_t *bytes_read,
                                             uint64_t *read_operations,
                                             uint64_t *buffer_overflows);

lle_result_t lle_input_stream_reset(lle_input_stream_t *stream);

lle_result_t lle_input_stream_get_available(lle_input_stream_t *stream,
                                            size_t *available);

/* ============================================================================
 * Function Declarations - Input Buffer Management
 * ============================================================================ */

lle_result_t lle_input_buffer_init(lle_input_buffer_t **buffer,
                                   size_t capacity,
                                   lle_memory_pool_t *memory_pool);

lle_result_t lle_input_buffer_destroy(lle_input_buffer_t *buffer);

lle_result_t lle_input_buffer_write(lle_input_buffer_t *buffer,
                                    const char *data,
                                    size_t data_len);

lle_result_t lle_input_buffer_read(lle_input_buffer_t *buffer,
                                   char *data,
                                   size_t data_len,
                                   size_t *bytes_read);

lle_result_t lle_input_buffer_clear(lle_input_buffer_t *buffer);

/* ============================================================================
 * Function Declarations - Sequence Parser
 * ============================================================================ */

lle_result_t lle_sequence_parser_init(lle_sequence_parser_t **parser,
                                      lle_terminal_capabilities_t *terminal_caps,
                                      lle_memory_pool_t *memory_pool);

lle_result_t lle_sequence_parser_destroy(lle_sequence_parser_t *parser);

lle_result_t lle_sequence_parser_process_data(lle_sequence_parser_t *parser,
                                              const char *data,
                                              size_t data_len,
                                              lle_parsed_input_t **parsed_input);

lle_result_t lle_sequence_parser_reset_state(lle_sequence_parser_t *parser);

lle_parser_state_t lle_sequence_parser_get_state(const lle_sequence_parser_t *parser);

lle_sequence_type_t lle_sequence_parser_get_type(const lle_sequence_parser_t *parser);

lle_result_t lle_sequence_parser_get_buffer(const lle_sequence_parser_t *parser,
                                            const char **buffer,
                                            size_t *buffer_len);

lle_result_t lle_sequence_parser_get_csi_params(const lle_sequence_parser_t *parser,
                                                const uint32_t **params,
                                                uint8_t *param_count);

lle_result_t lle_sequence_parser_get_stats(const lle_sequence_parser_t *parser,
                                           uint32_t *malformed,
                                           uint32_t *timeout);

/**
 * Check if parser has timed out waiting for sequence completion.
 * If in ESCAPE state and timeout exceeded, returns the ESC as a standalone key.
 * 
 * @param parser The sequence parser
 * @param timeout_us Timeout in microseconds (typically 50000-100000 for ESC)
 * @param parsed_input Output: If timeout occurred, contains ESC key event
 * @return LLE_SUCCESS if timeout handled, LLE_ERROR_NOT_FOUND if no timeout
 */
lle_result_t lle_sequence_parser_check_timeout(lle_sequence_parser_t *parser,
                                                uint64_t timeout_us,
                                                lle_parsed_input_t **parsed_input);

/* ============================================================================
 * Function Declarations - UTF-8 Processor
 * ============================================================================ */

lle_result_t lle_input_utf8_processor_init(lle_utf8_processor_t **processor,
                                           lle_memory_pool_t *memory_pool);

void lle_input_utf8_processor_destroy(lle_utf8_processor_t *processor);

lle_result_t lle_input_utf8_processor_reset(lle_utf8_processor_t *processor);

bool lle_input_utf8_processor_has_partial(const lle_utf8_processor_t *processor);

size_t lle_input_utf8_processor_bytes_needed(const lle_utf8_processor_t *processor);

lle_result_t lle_input_utf8_processor_process_byte(lle_utf8_processor_t *processor,
                                                   unsigned char byte,
                                                   uint32_t *codepoint_out,
                                                   bool *is_grapheme_boundary);

lle_result_t lle_input_utf8_processor_process_buffer(lle_utf8_processor_t *processor,
                                                     const char *buffer,
                                                     size_t buffer_len,
                                                     lle_codepoint_info_t *codepoints,
                                                     size_t max_codepoints,
                                                     size_t *codepoints_decoded,
                                                     size_t *bytes_consumed);

lle_result_t lle_input_utf8_processor_get_stats(const lle_utf8_processor_t *processor,
                                                lle_utf8_processor_stats_t *stats);

/* Convenience wrappers for complete string processing */
bool lle_input_utf8_validate_string(const char *text, size_t length);

size_t lle_input_utf8_count_codepoints(const char *text, size_t length);

size_t lle_input_utf8_count_graphemes(const char *text, size_t length);

size_t lle_input_utf8_get_display_width(const char *text, size_t length);

/* ============================================================================
 * Function Declarations - Key Detector
 * ============================================================================ */

lle_result_t lle_key_detector_init(lle_key_detector_t **detector,
                                   lle_terminal_capabilities_t *terminal_caps,
                                   lle_memory_pool_t *memory_pool);

lle_result_t lle_key_detector_destroy(lle_key_detector_t *detector);

lle_result_t lle_key_detector_process_sequence(lle_key_detector_t *detector,
                                               const char *sequence,
                                               size_t sequence_len,
                                               lle_key_info_t **key_info);

lle_result_t lle_key_detector_handle_ambiguous_sequence(lle_key_detector_t *detector,
                                                        lle_key_info_t **key_info);

lle_result_t lle_key_detector_force_resolution(lle_key_detector_t *detector);

lle_result_t lle_key_detector_get_stats(const lle_key_detector_t *detector,
                                        uint64_t *sequences_detected,
                                        uint64_t *sequences_resolved,
                                        uint64_t *ambiguous_timeouts);

lle_result_t lle_key_detector_reset(lle_key_detector_t *detector);

bool lle_key_detector_is_waiting(const lle_key_detector_t *detector);

/* Key sequence mapping */
lle_result_t lle_key_sequence_map_init(lle_key_sequence_map_t **map,
                                       lle_memory_pool_t *memory_pool);

lle_result_t lle_key_sequence_map_destroy(lle_key_sequence_map_t *map);

lle_result_t lle_key_sequence_map_match(lle_key_sequence_map_t *map,
                                        const char *sequence,
                                        size_t sequence_len,
                                        lle_key_sequence_match_result_t *result);

/* ============================================================================
 * Function Declarations - Mouse Parser
 * ============================================================================ */

lle_result_t lle_mouse_parser_init(lle_mouse_parser_t **parser,
                                   lle_terminal_capabilities_t *terminal_caps,
                                   lle_memory_pool_t *memory_pool);

lle_result_t lle_mouse_parser_destroy(lle_mouse_parser_t *parser);

lle_result_t lle_mouse_parser_parse_sequence(lle_mouse_parser_t *parser,
                                             const char *sequence,
                                             size_t sequence_len,
                                             lle_mouse_event_info_t **mouse_info);

lle_result_t lle_mouse_parser_parse_standard_sequence(lle_mouse_parser_t *parser,
                                                      const char *sequence,
                                                      size_t sequence_len,
                                                      lle_mouse_event_info_t **mouse_info);

lle_result_t lle_mouse_parser_parse_sgr_sequence(lle_mouse_parser_t *parser,
                                                 const char *sequence,
                                                 size_t sequence_len,
                                                 lle_mouse_event_info_t **mouse_info);

lle_result_t lle_mouse_parser_set_tracking(lle_mouse_parser_t *parser,
                                           bool enabled,
                                           lle_mouse_tracking_mode_t mode);

lle_result_t lle_mouse_parser_get_stats(const lle_mouse_parser_t *parser,
                                        uint64_t *events_parsed,
                                        uint64_t *invalid_sequences);

lle_result_t lle_mouse_parser_reset(lle_mouse_parser_t *parser);

lle_result_t lle_mouse_parser_get_state(const lle_mouse_parser_t *parser,
                                        uint16_t *x,
                                        uint16_t *y,
                                        lle_mouse_button_t *pressed_buttons);

/* ============================================================================
 * Function Declarations - Parser State Machine
 * ============================================================================ */

lle_result_t lle_parser_state_machine_init(lle_parser_state_machine_t **state_machine,
                                           lle_error_context_t *error_ctx,
                                           lle_memory_pool_t *memory_pool);

lle_result_t lle_parser_state_machine_destroy(lle_parser_state_machine_t *state_machine);

lle_result_t lle_parser_state_machine_process(lle_parser_state_machine_t *state_machine,
                                              lle_input_parser_system_t *parser_sys,
                                              const char *data,
                                              size_t data_len);

lle_parser_state_t lle_parser_state_machine_get_state(const lle_parser_state_machine_t *state_machine);

uint64_t lle_parser_state_machine_get_transitions(const lle_parser_state_machine_t *state_machine);

uint32_t lle_parser_state_machine_get_error_recoveries(const lle_parser_state_machine_t *state_machine);

lle_result_t lle_parser_state_machine_reset(lle_parser_state_machine_t *state_machine);

lle_result_t lle_parser_state_machine_transition(lle_parser_state_machine_t *state_machine,
                                                 lle_parser_state_t new_state);

lle_parser_state_t lle_parser_state_machine_get_previous_state(const lle_parser_state_machine_t *state_machine);

uint64_t lle_parser_state_machine_time_in_state(const lle_parser_state_machine_t *state_machine);

bool lle_parser_state_machine_is_error_state(const lle_parser_state_machine_t *state_machine);

/* ============================================================================
 * Function Declarations - Event Generation
 * ============================================================================ */

lle_result_t lle_input_parser_generate_events(lle_input_parser_system_t *parser_sys,
                                              lle_parsed_input_t *parsed_input);

lle_result_t lle_input_parser_generate_text_events(lle_input_parser_system_t *parser_sys,
                                                   lle_parsed_input_t *parsed_input);

lle_result_t lle_input_parser_generate_key_events(lle_input_parser_system_t *parser_sys,
                                                  lle_parsed_input_t *parsed_input);

lle_result_t lle_input_parser_generate_mouse_events(lle_input_parser_system_t *parser_sys,
                                                    lle_parsed_input_t *parsed_input);

/* ============================================================================
 * Function Declarations - Keybinding Integration
 * ============================================================================ */

lle_result_t lle_keybinding_integration_init(lle_keybinding_integration_t **integration,
                                             lle_keybinding_engine_t *keybinding_engine,
                                             lle_memory_pool_t *memory_pool);

lle_result_t lle_keybinding_integration_destroy(lle_keybinding_integration_t *integration);

lle_result_t lle_input_process_with_keybinding_lookup(lle_input_parser_system_t *parser,
                                                      lle_parsed_input_t *input);

/* ============================================================================
 * Function Declarations - Widget Hook Triggers
 * ============================================================================ */

lle_result_t lle_widget_hook_triggers_init(lle_widget_hook_triggers_t **triggers,
                                           lle_widget_hooks_manager_t *hooks_manager,
                                           lle_memory_pool_t *memory_pool);

lle_result_t lle_widget_hook_triggers_destroy(lle_widget_hook_triggers_t *triggers);

lle_result_t lle_input_trigger_widget_hooks(lle_input_parser_system_t *parser,
                                            lle_parsed_input_t *input);

/* ============================================================================
 * Function Declarations - Adaptive Terminal Parser
 * ============================================================================ */

lle_result_t lle_adaptive_terminal_parser_init(lle_adaptive_terminal_parser_t **parser,
                                               lle_adaptive_terminal_integration_t *adaptive_terminal,
                                               lle_memory_pool_t *memory_pool);

lle_result_t lle_adaptive_terminal_parser_destroy(lle_adaptive_terminal_parser_t *parser);

/* ============================================================================
 * Function Declarations - Input Coordinator
 * ============================================================================ */

lle_result_t lle_input_coordinator_init(lle_input_coordinator_t **coordinator,
                                        lle_memory_pool_t *memory_pool);

lle_result_t lle_input_coordinator_destroy(lle_input_coordinator_t *coordinator);

/* ============================================================================
 * Function Declarations - Error Handling and Recovery
 * ============================================================================ */

lle_result_t lle_input_parser_recover_from_error(lle_input_parser_system_t *parser_sys,
                                                 lle_result_t error_code,
                                                 const char *problematic_data,
                                                 size_t data_len);

lle_result_t lle_input_parser_insert_replacement_character(lle_input_parser_system_t *parser_sys);

lle_result_t lle_input_parser_process_as_text(lle_input_parser_system_t *parser_sys,
                                              const char *data,
                                              size_t data_len);

lle_result_t lle_input_parser_validate_utf8(const char *data, 
                                            size_t len, 
                                            size_t *valid_len);

bool lle_input_parser_check_sequence_timeout(lle_input_parser_system_t *parser_sys,
                                             uint64_t current_time_us);

lle_result_t lle_input_parser_handle_timeout(lle_input_parser_system_t *parser_sys);

lle_result_t lle_input_parser_get_error_stats(lle_input_parser_system_t *parser_sys,
                                              uint64_t *utf8_errors,
                                              uint64_t *mouse_errors,
                                              uint64_t *sequence_errors);

/* ============================================================================
 * Function Declarations - Performance and Testing
 * ============================================================================ */

lle_result_t lle_input_parser_get_performance_metrics(lle_input_parser_system_t *parser_sys,
                                                      lle_input_parser_performance_t *metrics);

lle_result_t lle_input_parser_test_comprehensive(lle_input_parser_system_t *parser_sys);

#ifdef __cplusplus
}
#endif

#endif /* LLE_INPUT_PARSING_H */

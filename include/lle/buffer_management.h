/**
 * @file buffer_management.h
 * @brief LLE Buffer Management System - Type Definitions and Function Declarations
 * 
 * Specification: Spec 03 - Buffer Management Complete Specification
 * Version: 1.0.0
 * 
 * This header contains ALL type definitions and function declarations for the
 * LLE buffer management system. NO implementations are included here.
 * 
 * Implementation Status: PHASE 1 - Core Buffer Structure
 * - Phase 1: Core buffer structure and lifecycle (THIS PHASE)
 * - Phase 2: UTF-8 processing subsystem (FUTURE)
 * - Phase 3: Line structure management (FUTURE)
 * - Phase 4: Cursor management (FUTURE)
 * - Phase 5: Change tracking and undo/redo (FUTURE)
 * - Phase 6: Buffer operations (insert, delete, replace) (FUTURE)
 * - Phase 7: Multiline support (FUTURE)
 * 
 * Layer 0: Type Definitions Only
 * Layer 1: Implementations in src/lle/buffer_management.c (separate file)
 */

#ifndef LLE_BUFFER_MANAGEMENT_H
#define LLE_BUFFER_MANAGEMENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Include LLE dependencies */
#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/performance.h"

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================
 */

/* Buffer Size and Capacity Limits */
#define LLE_BUFFER_NAME_MAX 256                    /* Maximum buffer name length */
#define LLE_BUFFER_DEFAULT_CAPACITY 4096           /* Default 4KB buffer */
#define LLE_BUFFER_MIN_CAPACITY 256                /* Minimum 256 bytes */
#define LLE_BUFFER_MAX_CAPACITY (1024 * 1024)      /* Maximum 1MB per buffer */
#define LLE_BUFFER_GROWTH_FACTOR 2                 /* Capacity growth factor */

/* Memory Alignment */
#define LLE_BUFFER_MEMORY_ALIGNMENT 16             /* 16-byte alignment */

/* Line Structure Limits */
#define LLE_BUFFER_MAX_LINES 10000                 /* Maximum lines per buffer */
#define LLE_BUFFER_DEFAULT_LINE_CAPACITY 100       /* Default line array capacity */

/* UTF-8 Index Configuration */
#define LLE_UTF8_INDEX_GRANULARITY 64              /* Index every 64 bytes */
#define LLE_UTF8_INDEX_MAX_ENTRIES 1024            /* Maximum index entries */

/* Change Tracking Limits */
#define LLE_BUFFER_MAX_UNDO_LEVELS 1000            /* Maximum undo history */
#define LLE_BUFFER_MAX_REDO_LEVELS 1000            /* Maximum redo history */
#define LLE_BUFFER_CHANGE_SEQUENCE_LIMIT 100       /* Max operations per sequence */

/* Cache Configuration */
#define LLE_BUFFER_CACHE_SIZE 256                  /* Cache entry count */
#define LLE_BUFFER_CACHE_LINE_TTL 1000             /* Cache entry TTL (ms) */

/* Performance Targets (nanoseconds) */
#define LLE_BUFFER_PERF_INSERT_MAX_NS 500000ULL    /* 0.5ms insert target */
#define LLE_BUFFER_PERF_DELETE_MAX_NS 500000ULL    /* 0.5ms delete target */
#define LLE_BUFFER_PERF_UTF8_CALC_MAX_NS 100000ULL /* 0.1ms UTF-8 calc target */

/* Buffer Flags */
#define LLE_BUFFER_FLAG_READONLY 0x0001            /* Buffer is read-only */
#define LLE_BUFFER_FLAG_MODIFIED 0x0002            /* Buffer has been modified */
#define LLE_BUFFER_FLAG_MULTILINE 0x0004           /* Multiline mode active */
#define LLE_BUFFER_FLAG_UTF8_DIRTY 0x0008          /* UTF-8 index needs rebuild */
#define LLE_BUFFER_FLAG_LINE_DIRTY 0x0010          /* Line structure needs rebuild */
#define LLE_BUFFER_FLAG_CACHE_DIRTY 0x0020         /* Cache needs refresh */
#define LLE_BUFFER_FLAG_VALIDATION_FAILED 0x0040   /* Validation failed */

/* Line Flags */
#define LLE_LINE_FLAG_CONTINUATION 0x01            /* Line is a continuation */
#define LLE_LINE_FLAG_NEEDS_REVALIDATION 0x02      /* Line needs revalidation */
#define LLE_LINE_FLAG_CACHED 0x04                  /* Line render is cached */

/* Cache Flags */
#define LLE_CACHE_LINE_STRUCTURE 0x01              /* Cache line structure */
#define LLE_CACHE_RENDER 0x02                      /* Cache render data */
#define LLE_CACHE_LAYOUT 0x04                      /* Cache layout data */

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================
 */

/**
 * @brief Buffer change operation types
 * Spec Reference: Line 1014-1023
 */
typedef enum {
    LLE_CHANGE_TYPE_INSERT = 1,                    /* Insert text operation */
    LLE_CHANGE_TYPE_DELETE,                        /* Delete text operation */
    LLE_CHANGE_TYPE_REPLACE,                       /* Replace text operation */
    LLE_CHANGE_TYPE_CURSOR_MOVE,                   /* Cursor movement */
    LLE_CHANGE_TYPE_SELECTION,                     /* Selection change */
    LLE_CHANGE_TYPE_COMPOSITE                      /* Composite operation */
} lle_change_type_t;

/**
 * @brief Line types for multiline context
 */
typedef enum {
    LLE_LINE_TYPE_COMMAND,                         /* Regular command line */
    LLE_LINE_TYPE_CONTINUATION,                    /* Continuation line */
    LLE_LINE_TYPE_HEREDOC,                         /* Here-document content */
    LLE_LINE_TYPE_QUOTED,                          /* Inside quoted string */
    LLE_LINE_TYPE_COMMENT                          /* Comment line */
} lle_line_type_t;

/**
 * @brief Multiline parser states
 */
typedef enum {
    LLE_MULTILINE_STATE_NONE,                      /* No multiline context */
    LLE_MULTILINE_STATE_QUOTE_SINGLE,              /* Inside single quote */
    LLE_MULTILINE_STATE_QUOTE_DOUBLE,              /* Inside double quote */
    LLE_MULTILINE_STATE_QUOTE_BACKTICK,            /* Inside backtick */
    LLE_MULTILINE_STATE_HEREDOC,                   /* Inside heredoc */
    LLE_MULTILINE_STATE_PAREN,                     /* Inside parentheses */
    LLE_MULTILINE_STATE_BRACE,                     /* Inside braces */
    LLE_MULTILINE_STATE_BRACKET,                   /* Inside brackets */
    LLE_MULTILINE_STATE_PIPE,                      /* After pipe */
    LLE_MULTILINE_STATE_BACKSLASH                  /* After backslash */
} lle_multiline_state_t;

/* ============================================================================
 * TYPE DEFINITIONS - FORWARD DECLARATIONS
 * ============================================================================
 */

/* Forward declarations for circular dependencies */
typedef struct lle_buffer_t lle_buffer_t;
typedef struct lle_buffer_system_t lle_buffer_system_t;
typedef struct lle_buffer_pool_t lle_buffer_pool_t;
typedef struct lle_line_info_t lle_line_info_t;
typedef struct lle_utf8_processor_t lle_utf8_processor_t;
typedef struct lle_utf8_index_t lle_utf8_index_t;
typedef struct lle_multiline_context_t lle_multiline_context_t;
typedef struct lle_multiline_manager_t lle_multiline_manager_t;
typedef struct lle_cursor_position_t lle_cursor_position_t;
typedef struct lle_cursor_manager_t lle_cursor_manager_t;
typedef struct lle_change_operation_t lle_change_operation_t;
typedef struct lle_change_sequence_t lle_change_sequence_t;
typedef struct lle_change_tracker_t lle_change_tracker_t;
typedef struct lle_buffer_validator_t lle_buffer_validator_t;
typedef struct lle_buffer_cache_t lle_buffer_cache_t;
typedef struct lle_selection_range_t lle_selection_range_t;
typedef struct lle_buffer_performance_metrics_t lle_buffer_performance_metrics_t;

/* Type aliases for flag bitfields */
typedef uint16_t lle_buffer_flags_t;
typedef uint8_t lle_line_flags_t;
typedef uint8_t lle_cache_flags_t;

/* ============================================================================
 * STRUCTURE DEFINITIONS - PHASE 1: CORE BUFFER
 * ============================================================================
 */

/**
 * @brief Cursor position structure
 * Spec Reference: Line 867-891
 * 
 * Complete cursor position structure with ALL fields from specification.
 * Phase 4 will implement cursor management.
 */
struct lle_cursor_position_t {
    /* Logical position */
    size_t byte_offset;                            /* Byte offset in buffer */
    size_t codepoint_offset;                       /* Codepoint offset */
    size_t grapheme_offset;                        /* Grapheme cluster offset */
    size_t line_number;                            /* Line number (0-based) */
    size_t column;                                 /* Column within line */
    
    /* Visual position */
    size_t visual_column;                          /* Visual column (display) */
    size_t visual_row;                             /* Visual row (display) */
    
    /* Validation */
    bool valid;                                    /* Position validity flag */
    uint64_t timestamp;                            /* Position update timestamp */
};

/**
 * @brief Selection range structure
 * 
 * Represents a selected range of text in the buffer.
 * Phase 6 will implement selection operations.
 */
struct lle_selection_range_t {
    lle_cursor_position_t start;                   /* Selection start position */
    lle_cursor_position_t end;                     /* Selection end position */
    bool active;                                   /* Selection is active */
    bool visual_mode;                              /* Visual selection mode */
};

/**
 * @brief Core buffer structure
 * Spec Reference: Line 212-269
 * 
 * Complete buffer structure with ALL fields from specification.
 * Phase 1: Only basic lifecycle functions implemented.
 * Future phases will implement subsystem-specific operations.
 */
struct lle_buffer_t {
    /* Buffer metadata */
    uint32_t buffer_id;                            /* Unique buffer identifier */
    char name[LLE_BUFFER_NAME_MAX];                /* Buffer name/description */
    uint64_t creation_time;                        /* Buffer creation timestamp */
    uint64_t last_modified_time;                   /* Last modification timestamp */
    uint32_t modification_count;                   /* Total modifications counter */
    
    /* Buffer content storage */
    char *data;                                    /* UTF-8 encoded buffer data */
    size_t capacity;                               /* Allocated buffer capacity */
    size_t length;                                 /* Current buffer length (bytes) */
    size_t used;                                   /* Actually used buffer space */
    
    /* UTF-8 and Unicode metadata */
    size_t codepoint_count;                        /* Number of Unicode codepoints */
    size_t grapheme_count;                         /* Number of grapheme clusters */
    lle_utf8_index_t *utf8_index;                  /* Fast UTF-8 position index */
    bool utf8_index_valid;                         /* UTF-8 index validity flag */
    
    /* Line structure information */
    lle_line_info_t *lines;                        /* Line structure array */
    size_t line_count;                             /* Number of logical lines */
    size_t line_capacity;                          /* Allocated line array capacity */
    bool multiline_active;                         /* Multiline mode status */
    lle_multiline_context_t *multiline_ctx;        /* Multiline parsing context */
    
    /* Cursor and selection */
    lle_cursor_position_t cursor;                  /* Current cursor position */
    lle_selection_range_t *selection;              /* Current selection range */
    bool selection_active;                         /* Selection status flag */
    
    /* Change tracking integration */
    lle_change_sequence_t *current_sequence;       /* Active change sequence */
    uint32_t sequence_number;                      /* Current sequence number */
    bool change_tracking_enabled;                  /* Change tracking status */
    
    /* Performance optimization */
    lle_buffer_cache_t *cache;                     /* Rendering and operation cache */
    uint32_t cache_version;                        /* Cache version counter */
    bool cache_dirty;                              /* Cache dirty flag */
    
    /* Validation and integrity */
    uint32_t checksum;                             /* Buffer content checksum */
    bool integrity_valid;                          /* Integrity validation status */
    lle_buffer_flags_t flags;                      /* Buffer status flags */
    
    /* Memory management */
    lle_buffer_pool_t *pool;                       /* Associated buffer pool */
    lusush_memory_pool_t *memory_pool;             /* Lusush memory pool reference */
};

/**
 * @brief Line information structure
 * Spec Reference: Line 269-294
 * 
 * Complete line structure with ALL fields from specification.
 * Phase 3 will implement line structure management.
 */
struct lle_line_info_t {
    size_t start_offset;                           /* Line start byte offset */
    size_t end_offset;                             /* Line end byte offset */
    size_t length;                                 /* Line length in bytes */
    size_t codepoint_count;                        /* Number of codepoints in line */
    size_t grapheme_count;                         /* Number of grapheme clusters */
    size_t visual_width;                           /* Visual display width */
    
    /* Line type and characteristics */
    lle_line_type_t type;                          /* Line type (command, continuation, etc.) */
    lle_line_flags_t flags;                        /* Line status flags */
    uint8_t indent_level;                          /* Indentation level */
    
    /* Multiline context */
    lle_multiline_state_t ml_state;                /* Multiline parser state */
    char *ml_context;                              /* Multiline context string */
    
    /* Performance optimization */
    uint32_t render_cache_key;                     /* Render cache key */
    bool needs_revalidation;                       /* Revalidation required flag */
};

/* ============================================================================
 * FUNCTION DECLARATIONS - PHASE 1: CORE BUFFER LIFECYCLE
 * ============================================================================
 */

/**
 * @brief Create a new buffer
 * 
 * Allocates and initializes a new buffer with the specified initial capacity.
 * All buffer fields are initialized to safe default values.
 * 
 * @param buffer Pointer to receive created buffer
 * @param memory_pool Lusush memory pool for allocations
 * @param initial_capacity Initial buffer capacity (0 = use default)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_buffer_create(lle_buffer_t **buffer,
                               lusush_memory_pool_t *memory_pool,
                               size_t initial_capacity);

/**
 * @brief Destroy a buffer
 * 
 * Frees all resources associated with the buffer including data array,
 * line structures, and any cached data.
 * 
 * @param buffer Buffer to destroy
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_buffer_destroy(lle_buffer_t *buffer);

/**
 * @brief Initialize buffer to empty state
 * 
 * Resets buffer to empty state while preserving allocated capacity.
 * Clears all content and resets all counters.
 * 
 * @param buffer Buffer to clear
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_buffer_clear(lle_buffer_t *buffer);

/**
 * @brief Validate buffer integrity
 * 
 * Performs complete buffer validation checking:
 * - Memory bounds
 * - UTF-8 validity
 * - Line structure consistency
 * - Checksum verification
 * 
 * @param buffer Buffer to validate
 * @return LLE_SUCCESS if valid, error code if validation fails
 */
lle_result_t lle_buffer_validate(lle_buffer_t *buffer);

#endif /* LLE_BUFFER_MANAGEMENT_H */

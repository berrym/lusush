/**
 * @file history.h
 * @brief LLE History System - Public API
 * 
 * Specification: Spec 09 - History System Complete Specification
 * Version: 1.0.0
 * Status: Phase 1 Day 1 - Core Structures and Lifecycle
 * 
 * Provides comprehensive command history management with forensic-grade
 * capabilities, seamless Lusush integration, and advanced features including
 * intelligent search, deduplication, and multiline command support.
 * 
 * IMPLEMENTATION PHASES:
 * - Phase 1 (Days 1-4): Core engine, indexing, persistence
 * - Phase 2 (Days 5-7): Lusush integration, event system
 * - Phase 3 (Days 8-10): Search and navigation
 * - Phase 4 (Days 11-14): Advanced features (forensics, dedup, multiline)
 */

#ifndef LLE_HISTORY_H
#define LLE_HISTORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>

#include "lle/error_handling.h"
#include "lle/memory_management.h"
#include "lle/performance.h"
#include "lle/hashtable.h"

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/* Core types */
typedef struct lle_history_entry lle_history_entry_t;
typedef struct lle_history_core lle_history_core_t;
typedef struct lle_history_config lle_history_config_t;
typedef struct lle_history_stats lle_history_stats_t;

/* Advanced types (Phase 2+) */
typedef struct lle_history_search_engine lle_history_search_engine_t;
typedef struct lle_history_dedup_engine lle_history_dedup_engine_t;
/* Note: lle_history_system_t already defined in performance.h - no redefinition */

/* ============================================================================
 * CONSTANTS AND CONFIGURATION
 * ============================================================================ */

/* History size limits */
#define LLE_HISTORY_DEFAULT_CAPACITY        10000    /* Default max entries */
#define LLE_HISTORY_INITIAL_CAPACITY        1000     /* Initial allocation */
#define LLE_HISTORY_MAX_CAPACITY            100000   /* Absolute maximum */
#define LLE_HISTORY_MIN_CAPACITY            100      /* Minimum entries */

/* Command size limits */
#define LLE_HISTORY_MAX_COMMAND_LENGTH      32768    /* 32KB max command */
#define LLE_HISTORY_MAX_PATH_LENGTH         4096     /* Max path length */

/* Performance targets */
#define LLE_HISTORY_ADD_TARGET_US           100      /* 100μs target for add */
#define LLE_HISTORY_RETRIEVE_TARGET_US      50       /* 50μs target for retrieve */
#define LLE_HISTORY_SEARCH_TARGET_MS        10       /* 10ms target for search */

/* File format */
#define LLE_HISTORY_FILE_MAGIC              "LLE_HISTORY_V1"
#define LLE_HISTORY_FILE_VERSION            1
#define LLE_HISTORY_DEFAULT_FILE            ".lle_history"

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================ */

/**
 * History entry state
 */
typedef enum lle_history_entry_state {
    LLE_HISTORY_STATE_ACTIVE = 0,     /* Active entry */
    LLE_HISTORY_STATE_DELETED,        /* Soft deleted (for undo) */
    LLE_HISTORY_STATE_ARCHIVED,       /* Archived (moved to old storage) */
    LLE_HISTORY_STATE_CORRUPTED       /* Corrupted entry */
} lle_history_entry_state_t;

/**
 * History operation type (for performance monitoring)
 */
typedef enum lle_history_operation {
    LLE_HISTORY_OP_ADD = 0,           /* Add entry */
    LLE_HISTORY_OP_RETRIEVE,          /* Retrieve entry */
    LLE_HISTORY_OP_SEARCH,            /* Search entries */
    LLE_HISTORY_OP_SAVE,              /* Save to disk */
    LLE_HISTORY_OP_LOAD,              /* Load from disk */
    LLE_HISTORY_OP_DELETE,            /* Delete entry */
    LLE_HISTORY_OP_COUNT              /* Number of operation types */
} lle_history_operation_t;

/* ============================================================================
 * CORE STRUCTURES - PHASE 1
 * ============================================================================ */

/**
 * History entry - stores a single command with metadata
 * 
 * Phase 1: Basic fields (command, timestamp, ID, exit code)
 * Phase 4: Advanced fields (forensics, multiline, etc.)
 */
struct lle_history_entry {
    /* Core entry data - Phase 1 */
    uint64_t entry_id;                  /* Unique entry identifier */
    char *command;                      /* Command text */
    size_t command_length;              /* Command length in bytes */
    uint64_t timestamp;                 /* Unix timestamp (seconds) */
    int exit_code;                      /* Command exit status */
    
    /* Basic metadata - Phase 1 */
    char *working_directory;            /* Working directory when executed */
    lle_history_entry_state_t state;    /* Entry state */
    
    /* Phase 4: Advanced features (initialized to NULL/0 in Phase 1) */
    char *original_multiline;           /* Original multiline format (Phase 4) */
    bool is_multiline;                  /* Multiline flag (Phase 4) */
    uint32_t duration_ms;               /* Execution duration (Phase 4) */
    uint32_t edit_count;                /* Edit count (Phase 4) */
    
    /* Internal management */
    struct lle_history_entry *next;     /* Linked list next */
    struct lle_history_entry *prev;     /* Linked list prev */
};

/**
 * History statistics
 */
struct lle_history_stats {
    /* Entry counts */
    size_t total_entries;               /* Total entries added */
    size_t active_entries;              /* Active entries */
    size_t deleted_entries;             /* Deleted entries */
    
    /* Operation counts */
    uint64_t add_count;                 /* Number of adds */
    uint64_t retrieve_count;            /* Number of retrieves */
    uint64_t search_count;              /* Number of searches */
    uint64_t save_count;                /* Number of saves */
    uint64_t load_count;                /* Number of loads */
    
    /* Performance metrics (microseconds) */
    uint64_t total_add_time_us;         /* Total add time */
    uint64_t total_retrieve_time_us;    /* Total retrieve time */
    uint64_t total_search_time_us;      /* Total search time */
    
    /* Memory usage */
    size_t memory_used_bytes;           /* Memory currently used */
    size_t peak_memory_bytes;           /* Peak memory usage */
    
    /* File statistics */
    size_t file_size_bytes;             /* History file size */
    time_t last_save_time;              /* Last save timestamp */
    time_t last_load_time;              /* Last load timestamp */
};

/**
 * History configuration
 */
struct lle_history_config {
    /* Capacity settings */
    size_t max_entries;                 /* Maximum entries to keep */
    size_t max_command_length;          /* Maximum command length */
    
    /* File settings */
    char *history_file_path;            /* Path to history file */
    bool auto_save;                     /* Auto-save on add */
    bool load_on_init;                  /* Load file on initialization */
    
    /* Behavior settings */
    bool ignore_duplicates;             /* Ignore duplicate commands */
    bool ignore_space_prefix;           /* Ignore commands starting with space */
    bool save_timestamps;               /* Save timestamp metadata */
    bool save_working_dir;              /* Save working directory */
    bool save_exit_codes;               /* Save exit codes */
    
    /* Performance settings */
    size_t initial_capacity;            /* Initial array capacity */
    bool use_indexing;                  /* Use hashtable indexing */
};

/**
 * History core engine - central management
 */
struct lle_history_core {
    /* Entry storage - Phase 1 */
    lle_history_entry_t **entries;      /* Dynamic array of entry pointers */
    size_t entry_count;                 /* Current number of entries */
    size_t entry_capacity;              /* Current array capacity */
    uint64_t next_entry_id;             /* Next unique ID to assign */
    
    /* Linked list pointers (for efficient iteration) */
    lle_history_entry_t *first_entry;   /* First entry in list */
    lle_history_entry_t *last_entry;    /* Last entry in list */
    
    /* Indexing - Phase 2 */
    lle_hashtable_t *entry_lookup;      /* ID -> entry hashtable (Phase 2) */
    
    /* Configuration and statistics */
    lle_history_config_t *config;       /* Configuration */
    lle_history_stats_t stats;          /* Statistics */
    
    /* Resource management */
    lle_memory_pool_t *memory_pool;     /* Memory pool */
    lle_performance_monitor_t *perf_monitor;  /* Performance monitor */
    
    /* Thread safety */
    pthread_rwlock_t lock;              /* Read-write lock */
    bool initialized;                   /* Initialization flag */
};

/* ============================================================================
 * CORE API - PHASE 1 DAY 1
 * ============================================================================ */

/**
 * Create and initialize history core engine
 * 
 * @param core Output pointer to created core
 * @param memory_pool Memory pool for allocations
 * @param config Configuration (NULL for defaults)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_core_create(
    lle_history_core_t **core,
    lle_memory_pool_t *memory_pool,
    const lle_history_config_t *config
);

/**
 * Destroy history core and free all resources
 * 
 * @param core Core to destroy
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_core_destroy(lle_history_core_t *core);

/**
 * Create default configuration
 * 
 * @param config Output pointer to configuration
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_config_create_default(
    lle_history_config_t **config,
    lle_memory_pool_t *memory_pool
);

/**
 * Destroy configuration
 * 
 * @param config Configuration to destroy
 * @param memory_pool Memory pool used for allocation
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_config_destroy(
    lle_history_config_t *config,
    lle_memory_pool_t *memory_pool
);

/**
 * Create history entry
 * 
 * @param entry Output pointer to created entry
 * @param command Command text (will be copied)
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_entry_create(
    lle_history_entry_t **entry,
    const char *command,
    lle_memory_pool_t *memory_pool
);

/**
 * Destroy history entry
 * 
 * @param entry Entry to destroy
 * @param memory_pool Memory pool used for allocation
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_entry_destroy(
    lle_history_entry_t *entry,
    lle_memory_pool_t *memory_pool
);

/**
 * Add command to history
 * 
 * @param core History core
 * @param command Command text
 * @param exit_code Command exit code (or -1 if unknown)
 * @param entry_id Output pointer for assigned entry ID (can be NULL)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_add_entry(
    lle_history_core_t *core,
    const char *command,
    int exit_code,
    uint64_t *entry_id
);

/**
 * Get entry by ID
 * 
 * @param core History core
 * @param entry_id Entry ID to retrieve
 * @param entry Output pointer to entry (do not free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_get_entry_by_id(
    lle_history_core_t *core,
    uint64_t entry_id,
    lle_history_entry_t **entry
);

/**
 * Get entry by index (0 = oldest, count-1 = newest)
 * 
 * @param core History core
 * @param index Entry index
 * @param entry Output pointer to entry (do not free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_get_entry_by_index(
    lle_history_core_t *core,
    size_t index,
    lle_history_entry_t **entry
);

/**
 * Get total number of entries
 * 
 * @param core History core
 * @param count Output pointer for count
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_get_entry_count(
    lle_history_core_t *core,
    size_t *count
);

/**
 * Clear all history entries
 * 
 * @param core History core
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_clear(lle_history_core_t *core);

/**
 * Get statistics
 * 
 * @param core History core
 * @param stats Output pointer for statistics (do not free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_get_stats(
    lle_history_core_t *core,
    const lle_history_stats_t **stats
);

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * Expand entry array capacity (internal use only)
 * 
 * @param core History core
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expand_capacity(lle_history_core_t *core);

/**
 * Validate entry (internal use only)
 * 
 * @param entry Entry to validate
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_validate_entry(const lle_history_entry_t *entry);

/**
 * Get current working directory (helper)
 * 
 * @param buffer Buffer for path
 * @param size Buffer size
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_get_cwd(char *buffer, size_t size);

#endif /* LLE_HISTORY_H */

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
#include "ht.h"  /* libhashtable */

/* Forward declaration for event system integration */
struct lle_event_system;
struct lle_event;

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
 * INDEXING AND FAST LOOKUP (Phase 1 Day 2)
 * ============================================================================ */

/**
 * Create hashtable index for fast ID lookup
 */
lle_result_t lle_history_index_create(
    lle_hashtable_t **index,
    size_t initial_capacity
);

/**
 * Destroy hashtable index
 */
void lle_history_index_destroy(lle_hashtable_t *index);

/**
 * Insert entry into index
 */
lle_result_t lle_history_index_insert(
    lle_hashtable_t *index,
    uint64_t entry_id,
    lle_history_entry_t *entry
);

/**
 * Lookup entry by ID in index
 */
lle_result_t lle_history_index_lookup(
    lle_hashtable_t *index,
    uint64_t entry_id,
    lle_history_entry_t **entry
);

/**
 * Remove entry from index
 */
lle_result_t lle_history_index_remove(
    lle_hashtable_t *index,
    uint64_t entry_id
);

/**
 * Clear all entries from index
 */
lle_result_t lle_history_index_clear(lle_hashtable_t *index);

/**
 * Get index size
 */
lle_result_t lle_history_index_get_size(
    lle_hashtable_t *index,
    size_t *size
);

/**
 * Rebuild index from core entries
 */
lle_result_t lle_history_rebuild_index(lle_history_core_t *core);

/**
 * Get last N entries (most recent)
 */
lle_result_t lle_history_get_last_n_entries(
    lle_history_core_t *core,
    size_t n,
    lle_history_entry_t **entries,
    size_t *count
);

/**
 * Get entry by reverse index (0 = newest)
 */
lle_result_t lle_history_get_entry_by_reverse_index(
    lle_history_core_t *core,
    size_t reverse_index,
    lle_history_entry_t **entry
);

/* ============================================================================
 * PERSISTENCE AND FILE STORAGE (Phase 1 Day 3)
 * ============================================================================ */

/**
 * Save all history entries to file
 */
lle_result_t lle_history_save_to_file(
    lle_history_core_t *core,
    const char *file_path
);

/**
 * Load history entries from file
 */
lle_result_t lle_history_load_from_file(
    lle_history_core_t *core,
    const char *file_path
);

/**
 * Append single entry to history file (incremental save)
 */
lle_result_t lle_history_append_entry(
    const lle_history_entry_t *entry,
    const char *file_path
);

/* ============================================================================
 * LUSUSH INTEGRATION BRIDGE (Phase 2 Day 5)
 * ============================================================================ */

/* Forward declarations */
typedef struct posix_history_manager posix_history_manager_t;

/**
 * Initialize Lusush history bridge
 * 
 * Establishes bidirectional synchronization between LLE history core
 * and existing Lusush history systems (GNU Readline, POSIX history).
 * 
 * @param lle_core LLE history core instance
 * @param posix_manager POSIX history manager (can be NULL)
 * @param memory_pool Memory pool for allocations
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_init(
    lle_history_core_t *lle_core,
    posix_history_manager_t *posix_manager,
    lle_memory_pool_t *memory_pool
);

/**
 * Shutdown and cleanup bridge
 * 
 * Performs final synchronization and frees bridge resources.
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_shutdown(void);

/**
 * Check if bridge is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool lle_history_bridge_is_initialized(void);

/**
 * Import history from GNU Readline
 * 
 * Imports all entries from GNU Readline history into LLE core.
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_import_from_readline(void);

/**
 * Export history to GNU Readline
 * 
 * Exports all LLE entries to GNU Readline history.
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_export_to_readline(void);

/**
 * Sync single entry to readline
 * 
 * @param entry Entry to sync
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_sync_entry_to_readline(const lle_history_entry_t *entry);

/**
 * Clear readline history
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_clear_readline(void);

/**
 * Import history from POSIX manager
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_import_from_posix(void);

/**
 * Export history to POSIX manager
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_export_to_posix(void);

/**
 * Sync single entry to POSIX manager
 * 
 * @param entry Entry to sync
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_sync_entry_to_posix(const lle_history_entry_t *entry);

/**
 * Add entry to LLE and sync to all systems
 * 
 * High-level API that adds an entry and automatically syncs
 * to GNU Readline and POSIX if enabled.
 * 
 * @param command Command text
 * @param exit_code Command exit code
 * @param entry_id Output pointer for assigned entry ID (can be NULL)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_add_entry(
    const char *command,
    int exit_code,
    uint64_t *entry_id
);

/**
 * Synchronize all systems (full bidirectional sync)
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_sync_all(void);

/**
 * Handle history builtin command
 * 
 * Provides compatibility with existing 'history' command behavior.
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @param output Output buffer (allocated by function, caller must free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_handle_builtin(
    int argc,
    char **argv,
    char **output
);

/**
 * Get entry by history number (for history expansion like !123)
 * 
 * @param number History entry number
 * @param entry Output pointer to entry (do not free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_get_by_number(
    uint64_t number,
    lle_history_entry_t **entry
);

/**
 * Get entry by reverse index (0 = most recent, for !! and !-N)
 * 
 * @param reverse_index Reverse index (0 = newest)
 * @param entry Output pointer to entry (do not free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_get_by_reverse_index(
    size_t reverse_index,
    lle_history_entry_t **entry
);

/**
 * Enable/disable readline synchronization
 * 
 * @param enabled true to enable, false to disable
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_set_readline_sync(bool enabled);

/**
 * Enable/disable POSIX synchronization
 * 
 * @param enabled true to enable, false to disable
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_set_posix_sync(bool enabled);

/**
 * Enable/disable automatic synchronization
 * 
 * @param enabled true to enable, false to disable
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_set_auto_sync(bool enabled);

/**
 * Enable/disable bidirectional synchronization
 * 
 * @param enabled true to enable, false to disable
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_set_bidirectional_sync(bool enabled);

/**
 * Get bridge statistics
 * 
 * @param readline_imports Number of entries imported from readline
 * @param readline_exports Number of entries exported to readline
 * @param posix_imports Number of entries imported from POSIX
 * @param posix_exports Number of entries exported to POSIX
 * @param sync_errors Number of synchronization errors
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_get_stats(
    size_t *readline_imports,
    size_t *readline_exports,
    size_t *posix_imports,
    size_t *posix_exports,
    size_t *sync_errors
);

/**
 * Print bridge diagnostics
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_bridge_print_diagnostics(void);

/* ============================================================================
 * EVENT SYSTEM INTEGRATION (Phase 2 Day 6)
 * ============================================================================ */

/* Event handler function type (matches event_system.h) */
typedef lle_result_t (*lle_event_handler_fn)(struct lle_event *event, void *user_data);

/**
 * Initialize history event integration
 * 
 * Connects history system to Spec 04 event system for real-time notifications.
 * 
 * @param event_system Event system instance
 * @param history_core History core instance
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_events_init(
    struct lle_event_system *event_system,
    lle_history_core_t *history_core
);

/**
 * Shutdown history event integration
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_events_shutdown(void);

/**
 * Check if event system is initialized
 * 
 * @return true if initialized, false otherwise
 */
bool lle_history_events_is_initialized(void);

/**
 * Emit entry added event
 * 
 * @param entry_id Entry ID
 * @param command Command text
 * @param exit_code Exit code
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_emit_entry_added(
    uint64_t entry_id,
    const char *command,
    int exit_code
);

/**
 * Emit entry accessed event
 * 
 * @param entry_id Entry ID
 * @param command Command text
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_emit_entry_accessed(
    uint64_t entry_id,
    const char *command
);

/**
 * Emit history loaded event
 * 
 * @param file_path File path
 * @param entry_count Number of entries loaded
 * @param duration_us Operation duration in microseconds
 * @param success Operation success
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_emit_history_loaded(
    const char *file_path,
    size_t entry_count,
    uint64_t duration_us,
    bool success
);

/**
 * Emit history saved event
 * 
 * @param file_path File path
 * @param entry_count Number of entries saved
 * @param duration_us Operation duration in microseconds
 * @param success Operation success
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_emit_history_saved(
    const char *file_path,
    size_t entry_count,
    uint64_t duration_us,
    bool success
);

/**
 * Emit history search event
 * 
 * @param search_query Search query
 * @param result_count Number of results
 * @param duration_us Search duration in microseconds
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_emit_history_search(
    const char *search_query,
    size_t result_count,
    uint64_t duration_us
);

/**
 * Register handler for history change events
 * 
 * @param handler Handler function
 * @param user_data User data passed to handler
 * @param handler_name Handler name (for debugging)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_register_change_handler(
    lle_event_handler_fn handler,
    void *user_data,
    const char *handler_name
);

/**
 * Register handler for history navigation events
 * 
 * @param handler Handler function
 * @param user_data User data passed to handler
 * @param handler_name Handler name (for debugging)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_register_navigate_handler(
    lle_event_handler_fn handler,
    void *user_data,
    const char *handler_name
);

/**
 * Register handler for history search events
 * 
 * @param handler Handler function
 * @param user_data User data passed to handler
 * @param handler_name Handler name (for debugging)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_register_search_handler(
    lle_event_handler_fn handler,
    void *user_data,
    const char *handler_name
);

/**
 * Enable or disable event emission
 * 
 * @param enabled true to enable, false to disable
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_events_set_enabled(bool enabled);

/**
 * Enable or disable access event emission
 * 
 * Access events can be noisy, so they're disabled by default.
 * 
 * @param enabled true to enable, false to disable
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_events_set_emit_access(bool enabled);

/**
 * Get event emission statistics
 * 
 * @param total_events Total events emitted (can be NULL)
 * @param entry_added Entry added events (can be NULL)
 * @param entry_accessed Entry accessed events (can be NULL)
 * @param history_loaded History loaded events (can be NULL)
 * @param history_saved History saved events (can be NULL)
 * @param history_searched History searched events (can be NULL)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_events_get_stats(
    uint64_t *total_events,
    uint64_t *entry_added,
    uint64_t *entry_accessed,
    uint64_t *history_loaded,
    uint64_t *history_saved,
    uint64_t *history_searched
);

/**
 * Print event statistics
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_events_print_stats(void);

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

/* ============================================================================
 * SEARCH ENGINE API (Phase 3 Day 8)
 * ============================================================================ */

/**
 * Search result types
 */
typedef enum {
    LLE_SEARCH_TYPE_EXACT,      /* Exact command match */
    LLE_SEARCH_TYPE_PREFIX,     /* Command starts with query */
    LLE_SEARCH_TYPE_SUBSTRING,  /* Command contains query */
    LLE_SEARCH_TYPE_FUZZY       /* Approximate match (Levenshtein) */
} lle_search_type_t;

/**
 * Single search result
 */
typedef struct {
    uint64_t entry_id;          /* History entry ID */
    size_t entry_index;         /* Index in history */
    const char *command;        /* Command string (reference) */
    uint64_t timestamp;         /* Command timestamp */
    int score;                  /* Relevance score (higher = better) */
    size_t match_position;      /* Position of match in command */
    lle_search_type_t match_type; /* Type of match */
} lle_search_result_t;

/**
 * Search results container (opaque)
 */
typedef struct lle_history_search_results lle_history_search_results_t;

/**
 * Create search results container
 * 
 * @param max_results Maximum results (0 = default 100)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t* lle_history_search_results_create(size_t max_results);

/**
 * Destroy search results
 * 
 * @param results Search results to destroy
 */
void lle_history_search_results_destroy(lle_history_search_results_t *results);

/**
 * Sort search results by score (descending)
 * 
 * @param results Search results to sort
 */
void lle_history_search_results_sort(lle_history_search_results_t *results);

/**
 * Get number of results
 * 
 * @param results Search results
 * @return Number of results
 */
size_t lle_history_search_results_get_count(const lle_history_search_results_t *results);

/**
 * Get specific result
 * 
 * @param results Search results
 * @param index Result index
 * @return Search result or NULL if invalid index
 */
const lle_search_result_t* lle_history_search_results_get(
    const lle_history_search_results_t *results,
    size_t index
);

/**
 * Get search duration in microseconds
 * 
 * @param results Search results
 * @return Search time in microseconds
 */
uint64_t lle_history_search_results_get_time_us(
    const lle_history_search_results_t *results
);

/**
 * Print search results (for debugging)
 * 
 * @param results Search results to print
 */
void lle_history_search_results_print(const lle_history_search_results_t *results);

/**
 * Search history for exact command match
 * 
 * Performance target: <500μs for 10K entries
 * 
 * @param history_core History core engine
 * @param query Search query
 * @param max_results Maximum results (0 = default 100)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t* lle_history_search_exact(
    lle_history_core_t *history_core,
    const char *query,
    size_t max_results
);

/**
 * Search history for commands starting with prefix
 * 
 * Performance target: <500μs for 10K entries
 * 
 * @param history_core History core engine
 * @param prefix Prefix to search for
 * @param max_results Maximum results (0 = default 100)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t* lle_history_search_prefix(
    lle_history_core_t *history_core,
    const char *prefix,
    size_t max_results
);

/**
 * Search history for commands containing substring
 * 
 * Performance target: <5ms for 10K entries
 * 
 * @param history_core History core engine
 * @param substring Substring to search for
 * @param max_results Maximum results (0 = default 100)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t* lle_history_search_substring(
    lle_history_core_t *history_core,
    const char *substring,
    size_t max_results
);

/**
 * Search history for commands with fuzzy matching
 * 
 * Uses Levenshtein distance with max distance of 3.
 * Performance target: <10ms for 10K entries
 * 
 * @param history_core History core engine
 * @param query Query string
 * @param max_results Maximum results (0 = default 100)
 * @return Search results or NULL on failure
 */
lle_history_search_results_t* lle_history_search_fuzzy(
    lle_history_core_t *history_core,
    const char *query,
    size_t max_results
);

/* ============================================================================
 * INTERACTIVE SEARCH API (Phase 3 Day 9) - Ctrl+R Reverse Incremental Search
 * ============================================================================ */

/**
 * Interactive search state
 */
typedef enum {
    LLE_SEARCH_STATE_INACTIVE,      /* No active search */
    LLE_SEARCH_STATE_ACTIVE,        /* Search active, have results */
    LLE_SEARCH_STATE_NO_RESULTS,    /* Search active, no matches found */
    LLE_SEARCH_STATE_FAILED         /* Search failed (error condition) */
} lle_interactive_search_state_t;

/**
 * Initialize interactive search session
 * 
 * Starts a new Ctrl+R search session. Saves current line for cancel operation.
 * 
 * @param history_core History core engine
 * @param current_line Current line buffer (saved for cancel)
 * @param cursor_pos Current cursor position
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_interactive_search_init(
    lle_history_core_t *history_core,
    const char *current_line,
    size_t cursor_pos
);

/**
 * Update search query with new character
 * 
 * Appends character to query and re-runs search. Called for each keypress.
 * 
 * @param c Character to append
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_interactive_search_update_query(char c);

/**
 * Remove last character from search query (backspace)
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_interactive_search_backspace(void);

/**
 * Move to next (older) search result
 * 
 * Called when user presses Ctrl+R during search.
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_interactive_search_next(void);

/**
 * Move to previous (newer) search result
 * 
 * Called when user presses Ctrl+S during search.
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_interactive_search_prev(void);

/**
 * Accept current search result and exit search mode
 * 
 * Returns selected command. Caller must copy string before next operation.
 * 
 * @return Command string (read-only, must copy) or NULL
 */
const char* lle_history_interactive_search_accept(void);

/**
 * Cancel search and restore original line
 * 
 * Returns original line. Caller must copy string before next operation.
 * 
 * @return Original line (read-only, must copy) or NULL
 */
const char* lle_history_interactive_search_cancel(void);

/**
 * Check if search is currently active
 * 
 * @return true if search session is active
 */
bool lle_history_interactive_search_is_active(void);

/**
 * Get current search query
 * 
 * @return Current query string (read-only)
 */
const char* lle_history_interactive_search_get_query(void);

/**
 * Get current search prompt string
 * 
 * Returns prompt like "(reverse-i-search)`query': "
 * 
 * @return Prompt string (read-only)
 */
const char* lle_history_interactive_search_get_prompt(void);

/**
 * Get currently selected command
 * 
 * Returns command highlighted in search results.
 * 
 * @return Current command (read-only) or NULL
 */
const char* lle_history_interactive_search_get_current_command(void);

/**
 * Get search state
 * 
 * @return Current search state
 */
lle_interactive_search_state_t lle_history_interactive_search_get_state(void);

/**
 * Get search statistics
 * 
 * @param searches_performed Number of searches (output, can be NULL)
 * @param total_time_us Total time in microseconds (output, can be NULL)
 * @param avg_time_us Average time in microseconds (output, can be NULL)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_interactive_search_get_stats(
    uint64_t *searches_performed,
    uint64_t *total_time_us,
    uint64_t *avg_time_us
);

/**
 * Print search statistics (for debugging)
 */
void lle_history_interactive_search_print_stats(void);

/**
 * Reset search statistics
 */
void lle_history_interactive_search_reset_stats(void);

/* ============================================================================
 * HISTORY EXPANSION API (Phase 3 Day 10)
 * ============================================================================ */

/**
 * Initialize history expansion system
 * 
 * Sets up the expansion engine to process bash-compatible history expansions
 * including !!, !n, !-n, !string, !?string, and ^old^new.
 * 
 * @param history_core History core engine for lookups
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_init(lle_history_core_t *history_core);

/**
 * Shutdown history expansion system
 * 
 * Cleans up expansion engine resources.
 * 
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_shutdown(void);

/**
 * Check if command contains history expansion
 * 
 * Detects presence of history expansion markers (!, ^) that need processing.
 * 
 * @param command Command line to check
 * @return true if expansion needed, false otherwise
 */
bool lle_history_expansion_needed(const char *command);

/**
 * Expand history references in command line
 * 
 * Performs bash-compatible history expansion:
 * - !! - Repeat last command
 * - !n - Repeat command number n
 * - !-n - Repeat command n positions back  
 * - !string - Most recent command starting with string
 * - !?string - Most recent command containing string
 * - ^old^new - Quick substitution in last command
 * 
 * The expanded result is allocated using the memory pool and must be
 * freed by the caller using lle_pool_free().
 * 
 * @param command Original command with history references
 * @param expanded Output pointer for expanded command (caller must free)
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expand_line(const char *command, char **expanded);

/**
 * Set whether leading space disables expansion
 * 
 * When enabled (default), commands starting with space are not expanded.
 * This matches bash behavior for privacy (HISTCONTROL=ignorespace).
 * 
 * @param enabled true to enable space-disables-expansion
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_set_space_disables(bool enabled);

/**
 * Get whether leading space disables expansion
 * 
 * @return true if enabled, false otherwise
 */
bool lle_history_expansion_get_space_disables(void);

/**
 * Set whether to verify expansion before execution
 * 
 * When enabled, expanded commands are displayed to user before execution.
 * This matches bash 'shopt -s histverify' option.
 * 
 * @param enabled true to enable verification
 * @return LLE_SUCCESS or error code
 */
lle_result_t lle_history_expansion_set_verify(bool enabled);

/**
 * Get whether verification is enabled
 * 
 * @return true if enabled, false otherwise
 */
bool lle_history_expansion_get_verify(void);

#endif /* LLE_HISTORY_H */

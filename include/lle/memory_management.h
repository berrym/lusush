/**
 * @file memory_management.h
 * @brief LLE Memory Management System - Type Definitions and Function Declarations
 * 
 * Specification: Spec 15 - Memory Management Complete Specification
 * Version: 1.0.0
 * 
 * This header contains ALL type definitions and function declarations for the
 * LLE memory management system. NO implementations are included here.
 * 
 * Layer 0: Type Definitions Only
 * Layer 1: Implementations in src/lle/memory_management.c (separate file)
 */

#ifndef LLE_MEMORY_MANAGEMENT_H
#define LLE_MEMORY_MANAGEMENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

/* Include LLE error handling for lle_result_t */
#include "lle/error_handling.h"

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

/* Forward declarations for Lusush types */
typedef struct lusush_memory_pool_t lusush_memory_pool_t;
typedef struct lusush_memory_system_t lusush_memory_system_t;

/* Forward declarations for LLE types from other specs */
typedef struct lle_input_event_t lle_input_event_t;
typedef struct lle_display_event_t lle_display_event_t;
typedef struct lle_system_event_t lle_system_event_t;
typedef struct lle_buffer_config_t lle_buffer_config_t;

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================
 */

#define LLE_MAX_BUFFER_BLOCKS 256
#define LLE_STRING_CACHE_SIZE 128
#define LLE_EVENT_POOL_SIZE 512
#define LLE_INPUT_EVENT_CACHE 64
#define LLE_DISPLAY_EVENT_CACHE 64
#define LLE_SYSTEM_EVENT_CACHE 32
#define LLE_PRIMARY_POOL_COUNT 8
#define LLE_SECONDARY_POOL_COUNT 4
#define LLE_MAX_SHARED_ALLOCATIONS 1024
#define LLE_MAX_FREE_FRAGMENTS 256
#define LLE_MEMORY_ALIGNMENT 16
#define LLE_BUFFER_ALIGNMENT 64
#define LLE_RESIZE_HISTORY_SIZE 32
#define LLE_ACCESS_HISTORY_SIZE 1024
#define LLE_HOT_REGIONS_COUNT 16
#define LLE_TUNING_HISTORY_SIZE 64
#define LLE_ERROR_HISTORY_SIZE 128
#define LLE_MAX_TEST_FAILURES 32
#define LLE_MAX_TRACKED_BUFFERS 512
#define LLE_MAX_KEY_SIZE 64
#define LLE_MAX_DERIVED_KEYS 8
#define LLE_INPUT_EVENT_CACHE_SIZE 64
#define LLE_DISPLAY_EVENT_CACHE_SIZE 64
#define LLE_SYSTEM_EVENT_CACHE_SIZE 32

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================
 */

/**
 * @brief Memory pool types for specialized pools
 */
typedef enum {
    LLE_POOL_BUFFER,        /**< Text buffer memory management */
    LLE_POOL_EVENT,         /**< Event system memory management */
    LLE_POOL_STRING,        /**< String and text memory management */
    LLE_POOL_TEMP,          /**< Temporary allocation pool */
    LLE_POOL_HISTORY,       /**< History system memory management */
    LLE_POOL_SYNTAX,        /**< Syntax highlighting memory */
    LLE_POOL_COMPLETION,    /**< Tab completion memory */
    LLE_POOL_CUSTOM,        /**< User customization memory */
    LLE_POOL_COUNT          /**< Total number of specialized pools */
} lle_memory_pool_type_t;

/**
 * @brief Memory management states
 */
typedef enum {
    LLE_MEMORY_STATE_INITIALIZING,    /**< System initialization */
    LLE_MEMORY_STATE_ACTIVE,          /**< Normal operation */
    LLE_MEMORY_STATE_OPTIMIZING,      /**< Performance optimization phase */
    LLE_MEMORY_STATE_GC_RUNNING,      /**< Garbage collection active */
    LLE_MEMORY_STATE_LOW_MEMORY,      /**< Low memory condition */
    LLE_MEMORY_STATE_ERROR,           /**< Error condition */
    LLE_MEMORY_STATE_SHUTDOWN         /**< System shutdown */
} lle_memory_state_t;

/**
 * @brief Garbage collection strategies
 */
typedef enum {
    LLE_GC_STRATEGY_MARK_SWEEP,
    LLE_GC_STRATEGY_MARK_SWEEP_COMPACT,
    LLE_GC_STRATEGY_GENERATIONAL,
    LLE_GC_STRATEGY_INCREMENTAL,
    LLE_GC_STRATEGY_CONCURRENT
} lle_gc_strategy_t;

/**
 * @brief Garbage collection states
 */
typedef enum {
    LLE_GC_STATE_IDLE,
    LLE_GC_STATE_MARKING,
    LLE_GC_STATE_SWEEPING,
    LLE_GC_STATE_COMPACTING,
    LLE_GC_STATE_ERROR
} lle_gc_state_t;

/**
 * @brief Pool selection algorithms
 */
typedef enum {
    LLE_POOL_SELECTION_FIRST_FIT,
    LLE_POOL_SELECTION_BEST_FIT,
    LLE_POOL_SELECTION_WORST_FIT,
    LLE_POOL_SELECTION_NEXT_FIT
} lle_pool_selection_algorithm_t;

/**
 * @brief Allocation strategies
 */
typedef enum {
    LLE_STRATEGY_PRIMARY_ONLY,
    LLE_STRATEGY_PRIMARY_SECONDARY,
    LLE_STRATEGY_EMERGENCY_ONLY,
    LLE_STRATEGY_AUTOMATIC
} lle_allocation_strategy_t;

/**
 * @brief Resize actions
 */
typedef enum {
    LLE_RESIZE_ACTION_NONE,
    LLE_RESIZE_ACTION_GROW,
    LLE_RESIZE_ACTION_SHRINK,
    LLE_RESIZE_ACTION_REBALANCE
} lle_resize_action_t;

/**
 * @brief Resize reasons
 */
typedef enum {
    LLE_RESIZE_REASON_HIGH_UTILIZATION,
    LLE_RESIZE_REASON_LOW_UTILIZATION,
    LLE_RESIZE_REASON_FRAGMENTATION,
    LLE_RESIZE_REASON_PERFORMANCE,
    LLE_RESIZE_REASON_MANUAL
} lle_resize_reason_t;

/**
 * @brief Memory access types
 */
typedef enum {
    LLE_ACCESS_TYPE_READ,
    LLE_ACCESS_TYPE_WRITE,
    LLE_ACCESS_TYPE_READ_WRITE,
    LLE_ACCESS_TYPE_EXECUTE
} lle_access_type_t;

/**
 * @brief Prefetch strategies
 */
typedef enum {
    LLE_PREFETCH_STRATEGY_SEQUENTIAL,
    LLE_PREFETCH_STRATEGY_STRIDE,
    LLE_PREFETCH_STRATEGY_ADAPTIVE,
    LLE_PREFETCH_STRATEGY_NONE
} lle_prefetch_strategy_t;

/**
 * @brief Tuning actions
 */
typedef enum {
    LLE_TUNING_ACTION_NONE,
    LLE_TUNING_ACTION_RESIZE_POOL,
    LLE_TUNING_ACTION_COALESCE_BLOCKS,
    LLE_TUNING_ACTION_ADJUST_ALIGNMENT,
    LLE_TUNING_ACTION_ENABLE_PREFETCH,
    LLE_TUNING_ACTION_DISABLE_PREFETCH
} lle_tuning_action_t;

/**
 * @brief Memory error types
 */
typedef enum {
    LLE_MEMORY_ERROR_NONE,
    LLE_MEMORY_ERROR_LEAK,
    LLE_MEMORY_ERROR_BOUNDS_VIOLATION,
    LLE_MEMORY_ERROR_CORRUPTION,
    LLE_MEMORY_ERROR_DOUBLE_FREE,
    LLE_MEMORY_ERROR_USE_AFTER_FREE,
    LLE_MEMORY_ERROR_ALLOCATION_FAILED,
    LLE_MEMORY_ERROR_UNKNOWN
} lle_memory_error_type_t;

/**
 * @brief Memory recovery strategies (enum for strategy types)
 */
typedef enum {
    LLE_MEMORY_RECOVERY_ABORT,
    LLE_MEMORY_RECOVERY_CLEANUP,
    LLE_MEMORY_RECOVERY_RETRY,
    LLE_MEMORY_RECOVERY_FALLBACK,
    LLE_MEMORY_RECOVERY_IGNORE
} lle_memory_recovery_strategy_t;

/**
 * @brief Encryption algorithms
 */
typedef enum {
    LLE_ENCRYPTION_ALGORITHM_NONE,
    LLE_ENCRYPTION_ALGORITHM_AES_128,
    LLE_ENCRYPTION_ALGORITHM_AES_256,
    LLE_ENCRYPTION_ALGORITHM_CHACHA20,
    LLE_ENCRYPTION_ALGORITHM_XOR
} lle_encryption_algorithm_t;

/**
 * @brief Data sensitivity levels
 */
typedef enum {
    LLE_DATA_SENSITIVITY_LOW,
    LLE_DATA_SENSITIVITY_MEDIUM,
    LLE_DATA_SENSITIVITY_HIGH,
    LLE_DATA_SENSITIVITY_CRITICAL
} lle_data_sensitivity_t;

/**
 * @brief Integration modes
 */
typedef enum {
    LLE_INTEGRATION_MODE_STANDALONE,
    LLE_INTEGRATION_MODE_COOPERATIVE,
    LLE_INTEGRATION_MODE_SHARED,
    LLE_INTEGRATION_MODE_EXCLUSIVE
} lle_integration_mode_t;

/**
 * @brief Display memory types
 */
typedef enum {
    LLE_DISPLAY_MEMORY_PROMPT,
    LLE_DISPLAY_MEMORY_SYNTAX,
    LLE_DISPLAY_MEMORY_AUTOSUGGESTION,
    LLE_DISPLAY_MEMORY_COMPOSITION
} lle_display_memory_type_t;

/**
 * @brief Test failure reasons
 */
typedef enum {
    LLE_TEST_FAILURE_BASIC_ALLOCATION,
    LLE_TEST_FAILURE_STRESS_TEST,
    LLE_TEST_FAILURE_MEMORY_LEAK,
    LLE_TEST_FAILURE_PERFORMANCE,
    LLE_TEST_FAILURE_CONCURRENCY,
    LLE_TEST_FAILURE_BOUNDS_CHECK,
    LLE_TEST_FAILURE_CORRUPTION
} lle_test_failure_reason_t;

/**
 * @brief Security incident types
 */
typedef enum {
    LLE_SECURITY_BOUNDS_VIOLATION,
    LLE_SECURITY_PERMISSION_VIOLATION,
    LLE_SECURITY_ENCRYPTION_FAILURE,
    LLE_SECURITY_DECRYPTION_FAILURE,
    LLE_SECURITY_KEY_ROTATION_FAILURE,
    LLE_SECURITY_CORRUPTION_DETECTED
} lle_security_incident_type_t;

/**
 * @brief Buffer types
 */
typedef enum {
    LLE_BUFFER_TYPE_EDIT,
    LLE_BUFFER_TYPE_STRING,
    LLE_BUFFER_TYPE_TEMP,
    LLE_BUFFER_TYPE_DISPLAY
} lle_buffer_type_t;

/**
 * @brief Compression algorithms
 */
typedef enum {
    LLE_COMPRESSION_ALGORITHM_NONE,
    LLE_COMPRESSION_ALGORITHM_LZ4,
    LLE_COMPRESSION_ALGORITHM_ZSTD,
    LLE_COMPRESSION_ALGORITHM_SNAPPY
} lle_compression_algorithm_t;

/**
 * @brief Event types (from event system integration)
 */
typedef enum {
    LLE_EVENT_TYPE_INPUT,
    LLE_EVENT_TYPE_DISPLAY,
    LLE_EVENT_TYPE_SYSTEM,
    LLE_EVENT_TYPE_CUSTOM
} lle_event_type_t;

/* ============================================================================
 * STRUCTURE DEFINITIONS
 * ============================================================================
 */

/* Forward declaration of main memory pool type */
typedef struct lle_memory_pool_t lle_memory_pool_t;

/**
 * @brief Memory pool configuration
 */
typedef struct {
    lle_memory_pool_type_t type;                 /**< Pool specialization type */
    size_t initial_size;                         /**< Initial pool size */
    size_t max_size;                             /**< Maximum pool size */
    size_t block_size;                           /**< Standard allocation block size */
    size_t alignment;                            /**< Memory alignment requirement */
    
    /* Performance optimization */
    double growth_factor;                        /**< Pool growth multiplier */
    size_t gc_threshold;                         /**< Garbage collection trigger */
    bool enable_compression;                     /**< Enable memory compression */
    
    /* Security configuration */
    bool enable_bounds_checking;                 /**< Buffer overflow protection */
    bool enable_encryption;                      /**< Memory encryption */
    bool enable_poisoning;                       /**< Free memory poisoning */
    
    /* Integration settings */
    bool share_with_lusush;                      /**< Share with Lusush pools */
    lusush_memory_pool_t *parent_pool;          /**< Parent Lusush pool */
} lle_memory_pool_config_t;

/**
 * @brief Memory management configuration
 */
typedef struct {
    size_t pool_sizes[LLE_POOL_COUNT];
    size_t max_pool_sizes[LLE_POOL_COUNT];
    size_t block_size;
    size_t alignment;
} lle_memory_config_t;

/**
 * @brief Memory tracker
 */
typedef struct {
    void *implementation_data;
} lle_memory_tracker_t;

/**
 * @brief Memory optimizer
 */
typedef struct {
    void *implementation_data;
} lle_memory_optimizer_t;

/**
 * @brief Memory security
 */
typedef struct {
    void *implementation_data;
} lle_memory_security_t;

/**
 * @brief Memory analytics
 */
typedef struct {
    void *implementation_data;
} lle_memory_analytics_t;

/**
 * @brief Memory statistics
 */
typedef struct {
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t current_usage;
    uint64_t peak_usage;
    double allocation_rate;
    double deallocation_rate;
} lle_memory_stats_t;

/**
 * @brief Memory pool manager
 */
typedef struct {
    lle_memory_pool_t *pools[LLE_POOL_COUNT];
    size_t pool_count;
} lle_memory_pool_manager_t;

/**
 * @brief Core memory manager structure
 */
typedef struct {
    lle_memory_pool_manager_t *pool_manager;      /**< Pool management system */
    lle_memory_tracker_t *tracker;               /**< Allocation tracking */
    lle_memory_optimizer_t *optimizer;           /**< Performance optimization */
    lle_memory_security_t *security;             /**< Security and bounds checking */
    lle_memory_analytics_t *analytics;           /**< Real-time monitoring */
    
    /* Integration with Lusush memory system */
    lusush_memory_pool_t *lusush_pools;          /**< Lusush memory pool integration */
    bool lusush_integration_active;              /**< Integration status */
    
    /* Configuration and state */
    lle_memory_config_t config;                  /**< Memory management configuration */
    lle_memory_state_t state;                    /**< Current memory state */
    
    /* Performance monitoring */
    struct timespec last_gc_time;                /**< Last garbage collection */
    size_t total_allocated;                      /**< Total memory allocated */
    size_t peak_usage;                           /**< Peak memory usage */
    double allocation_rate;                      /**< Current allocation rate */
} lle_memory_manager_t;

/**
 * @brief Lusush memory pool integration layer
 */
typedef struct {
    lusush_memory_pool_t *shell_pools[8];
    lle_memory_pool_t *lle_pools[LLE_POOL_COUNT];
    
    /* Shared memory regions */
    void *shared_buffer_region;
    size_t shared_region_size;
    
    /* Integration configuration */
    bool enable_pool_sharing;
    bool enable_cross_allocation;
    double shared_memory_ratio;
    
    /* Performance monitoring */
    lle_memory_stats_t lusush_stats;
    lle_memory_stats_t lle_stats;
    lle_memory_stats_t shared_stats;
    
    /* Synchronization */
    pthread_mutex_t integration_mutex;
    volatile bool integration_active;
} lle_lusush_memory_integration_t;

/**
 * @brief Shared memory pool
 */
typedef struct {
    void *memory_region;
    size_t total_size;
    size_t lusush_allocated;
    size_t lle_allocated;
    size_t free_space;
    
    /* Allocation tracking */
    struct {
        void *ptr;
        size_t size;
        lle_memory_pool_type_t owner;
        struct timespec allocation_time;
    } allocations[LLE_MAX_SHARED_ALLOCATIONS];
    
    size_t allocation_count;
    
    /* Fragmentation management */
    struct {
        void *start;
        size_t size;
    } free_fragments[LLE_MAX_FREE_FRAGMENTS];
    
    size_t fragment_count;
    
    /* Synchronization */
    pthread_rwlock_t access_lock;
    volatile int reference_count;
} lle_shared_memory_pool_t;

/**
 * @brief Base memory pool structure
 */
typedef struct {
    void *memory_region;
    size_t size;
    size_t used;
    size_t free;
    struct timespec last_resize_time;
    pthread_mutex_t resize_mutex;
} lle_memory_pool_base_t;

/**
 * @brief Buffer memory pool
 */
typedef struct {
    lle_memory_pool_base_t base;
    
    /* Buffer-specific optimization */
    struct {
        void *buffer_blocks[LLE_MAX_BUFFER_BLOCKS];
        size_t block_sizes[LLE_MAX_BUFFER_BLOCKS];
        bool blocks_in_use[LLE_MAX_BUFFER_BLOCKS];
        size_t preferred_buffer_size;
    } buffer_optimization;
    
    /* UTF-8 string optimization */
    struct {
        void *string_cache[LLE_STRING_CACHE_SIZE];
        size_t string_lengths[LLE_STRING_CACHE_SIZE];
        uint32_t string_hashes[LLE_STRING_CACHE_SIZE];
        struct timespec cache_times[LLE_STRING_CACHE_SIZE];
    } string_cache;
    
    /* Memory compaction */
    struct {
        void *compaction_buffer;
        size_t compaction_threshold;
        struct timespec last_compaction;
        bool compaction_in_progress;
    } compaction;
} lle_buffer_memory_pool_t;

/**
 * @brief Event memory pool
 */
typedef struct {
    lle_memory_pool_base_t base;
    
    /* High-frequency allocation optimization */
    struct {
        void *event_slots[LLE_EVENT_POOL_SIZE];
        bool slots_in_use[LLE_EVENT_POOL_SIZE];
        size_t next_free_slot;
        size_t allocation_counter;
    } event_slots;
    
    /* Event type specialization */
    struct {
        void *input_events[LLE_INPUT_EVENT_CACHE];
        void *display_events[LLE_DISPLAY_EVENT_CACHE];
        void *system_events[LLE_SYSTEM_EVENT_CACHE];
        
        size_t input_count;
        size_t display_count;
        size_t system_count;
    } event_caches;
    
    /* Performance monitoring */
    struct {
        uint64_t fast_allocations;
        uint64_t pool_allocations;
        double average_allocation_time;
        struct timespec fastest_allocation;
    } performance_stats;
} lle_event_memory_pool_t;

/**
 * @brief Memory pool hierarchy manager
 */
typedef struct {
    /* Primary pools (highest performance) */
    lle_memory_pool_t *primary_pools[LLE_PRIMARY_POOL_COUNT];
    
    /* Secondary pools (fallback allocation) */
    lle_memory_pool_t *secondary_pools[LLE_SECONDARY_POOL_COUNT];
    
    /* Emergency pool (last resort allocation) */
    lle_memory_pool_t *emergency_pool;
    
    /* Pool hierarchy configuration */
    struct {
        size_t primary_threshold;
        size_t secondary_threshold;
        double primary_utilization_limit;
        double secondary_utilization_limit;
    } hierarchy_config;
    
    /* Pool selection algorithm */
    lle_pool_selection_algorithm_t selection_algorithm;
    
    /* Performance tracking */
    struct {
        uint64_t primary_allocations;
        uint64_t secondary_allocations;
        uint64_t emergency_allocations;
        uint64_t failed_allocations;
    } allocation_stats;
} lle_memory_pool_hierarchy_t;

/**
 * @brief Resize decision structure
 */
typedef struct {
    lle_resize_action_t action;
    lle_resize_reason_t reason;
} lle_resize_decision_t;

/**
 * @brief Dynamic pool resizer
 */
typedef struct {
    lle_memory_pool_t *pool;
    
    /* Resizing configuration */
    struct {
        size_t min_size;
        size_t max_size;
        double growth_factor;
        double shrink_factor;
        size_t growth_threshold;
        size_t shrink_threshold;
    } resize_config;
    
    /* Utilization monitoring */
    struct {
        double current_utilization;
        double peak_utilization;
        double average_utilization;
        struct timespec monitoring_start;
    } utilization_stats;
    
    /* Resizing history */
    struct {
        struct timespec resize_time;
        size_t old_size;
        size_t new_size;
        lle_resize_reason_t reason;
    } resize_history[LLE_RESIZE_HISTORY_SIZE];
    
    size_t resize_count;
} lle_dynamic_pool_resizer_t;

/**
 * @brief Garbage collector
 */
typedef struct {
    lle_memory_manager_t *memory_manager;
    
    /* GC configuration */
    struct {
        lle_gc_strategy_t strategy;
        size_t gc_trigger_threshold;
        double gc_utilization_threshold;
        struct timespec gc_interval;
        bool enable_concurrent_gc;
    } gc_config;
    
    /* GC state tracking */
    struct {
        lle_gc_state_t current_state;
        struct timespec last_gc_time;
        struct timespec gc_start_time;
        size_t collections_performed;
        size_t memory_freed_total;
    } gc_state;
    
    /* GC performance metrics */
    struct {
        struct timespec fastest_gc;
        struct timespec slowest_gc;
        struct timespec average_gc_time;
        double gc_efficiency;
    } gc_performance;
    
    /* Concurrent GC support */
    pthread_t gc_thread;
    pthread_mutex_t gc_mutex;
    pthread_cond_t gc_condition;
    volatile bool gc_thread_active;
} lle_garbage_collector_t;

/**
 * @brief Text buffer memory management
 */
typedef struct {
    /* Buffer memory regions */
    struct {
        void *primary_buffer;
        void *secondary_buffer;
        void *scratch_buffer;
        
        size_t primary_size;
        size_t secondary_size;
        size_t scratch_size;
    } buffer_regions;
    
    /* UTF-8 text management */
    struct {
        uint32_t *codepoint_offsets;
        size_t codepoint_count;
        size_t codepoint_capacity;
        
        /* Grapheme cluster tracking */
        size_t *grapheme_boundaries;
        size_t grapheme_count;
        size_t grapheme_capacity;
    } utf8_management;
    
    /* Buffer growth and shrinking */
    struct {
        double growth_factor;
        size_t min_buffer_size;
        size_t max_buffer_size;
        size_t growth_threshold;
        size_t shrink_threshold;
    } resize_config;
    
    /* Memory optimization */
    struct {
        bool enable_compression;
        lle_compression_algorithm_t algorithm;
        size_t compression_threshold;
        double compression_ratio;
    } optimization;
} lle_buffer_memory_t;

/**
 * @brief Multiline buffer management
 */
typedef struct {
    /* Line tracking */
    struct {
        size_t *line_offsets;
        size_t *line_lengths;
        size_t line_count;
        size_t line_capacity;
    } line_tracking;
    
    /* Virtual line management (for wrapped lines) */
    struct {
        size_t *virtual_line_map;
        size_t virtual_line_count;
        size_t wrap_width;
        bool auto_wrap_enabled;
    } virtual_lines;
    
    /* Line modification tracking */
    struct {
        bool *lines_modified;
        struct timespec *modification_times;
        size_t total_modifications;
    } modification_tracking;
    
    /* Efficient line operations */
    struct {
        char *line_insertion_buffer;
        size_t insertion_buffer_size;
        char *line_deletion_buffer;
        size_t deletion_buffer_size;
    } operation_buffers;
} lle_multiline_buffer_t;

/**
 * @brief Event memory integration
 */
typedef struct {
    lle_memory_pool_t *event_pool;
    lle_memory_pool_t *data_pool;
    
    /* High-frequency event optimization */
    struct {
        lle_input_event_t *input_event_cache[LLE_INPUT_EVENT_CACHE_SIZE];
        lle_display_event_t *display_event_cache[LLE_DISPLAY_EVENT_CACHE_SIZE];
        lle_system_event_t *system_event_cache[LLE_SYSTEM_EVENT_CACHE_SIZE];
        
        bool input_cache_used[LLE_INPUT_EVENT_CACHE_SIZE];
        bool display_cache_used[LLE_DISPLAY_EVENT_CACHE_SIZE];
        bool system_cache_used[LLE_SYSTEM_EVENT_CACHE_SIZE];
        
        size_t input_cache_hits;
        size_t display_cache_hits;
        size_t system_cache_hits;
    } event_cache;
    
    /* Memory pool statistics */
    struct {
        uint64_t events_allocated;
        uint64_t events_freed;
        uint64_t peak_event_count;
        uint64_t cache_allocations;
        uint64_t pool_allocations;
    } allocation_stats;
    
    /* Memory optimization */
    struct {
        size_t small_event_threshold;
        size_t large_event_threshold;
        bool enable_event_compression;
        double compression_ratio;
    } optimization_config;
} lle_event_memory_integration_t;

/**
 * @brief Access pattern analysis
 */
typedef struct {
    double locality_score;
    double sequential_ratio;
    size_t hot_region_count;
} lle_access_pattern_analysis_t;

/**
 * @brief Memory access optimizer
 */
typedef struct {
    /* Access pattern tracking */
    struct {
        struct {
            void *address;
            size_t size;
            lle_access_type_t type;
            struct timespec timestamp;
        } recent_accesses[LLE_ACCESS_HISTORY_SIZE];
        
        size_t access_index;
        size_t total_accesses;
    } access_tracking;
    
    /* Memory locality optimization */
    struct {
        void *hot_memory_regions[LLE_HOT_REGIONS_COUNT];
        size_t hot_region_sizes[LLE_HOT_REGIONS_COUNT];
        double hot_region_scores[LLE_HOT_REGIONS_COUNT];
        struct timespec last_score_update;
    } locality_optimization;
    
    /* Prefetching configuration */
    struct {
        bool enable_prefetching;
        size_t prefetch_distance;
        lle_prefetch_strategy_t strategy;
        double prefetch_accuracy;
    } prefetch_config;
    
    /* Cache optimization */
    struct {
        size_t cache_line_size;
        size_t l1_cache_size;
        size_t l2_cache_size;
        size_t l3_cache_size;
        bool enable_cache_alignment;
    } cache_optimization;
} lle_memory_access_optimizer_t;

/**
 * @brief Memory pool performance
 */
typedef struct {
    double allocation_rate;
    double deallocation_rate;
    struct timespec average_allocation_time;
    struct timespec peak_allocation_time;
    double fragmentation_ratio;
    double utilization_efficiency;
    double overall_score;
} lle_memory_pool_performance_t;

/**
 * @brief Performance bottleneck analysis
 */
typedef struct {
    bool high_fragmentation;
    bool slow_allocations;
    bool poor_locality;
} lle_performance_bottleneck_analysis_t;

/**
 * @brief Tuning action item
 */
typedef struct {
    lle_tuning_action_t action;
    size_t parameter;
} lle_tuning_action_item_t;

/**
 * @brief Tuning action plan
 */
typedef struct {
    lle_tuning_action_item_t actions[16];
    size_t action_count;
    lle_tuning_action_t primary_action;
} lle_tuning_action_plan_t;

/**
 * @brief Memory pool tuner
 */
typedef struct {
    lle_memory_pool_t *target_pool;
    
    /* Performance metrics */
    struct {
        double allocation_rate;
        double deallocation_rate;
        struct timespec average_allocation_time;
        struct timespec peak_allocation_time;
        double fragmentation_ratio;
        double utilization_efficiency;
    } performance_metrics;
    
    /* Tuning configuration */
    struct {
        size_t target_allocation_time_ns;
        double target_fragmentation_ratio;
        double target_utilization_ratio;
        size_t tuning_sample_size;
        struct timespec tuning_interval;
    } tuning_config;
    
    /* Optimization strategies */
    struct {
        bool enable_block_coalescing;
        bool enable_preallocation;
        bool enable_size_optimization;
        bool enable_alignment_optimization;
    } optimization_strategies;
    
    /* Tuning history */
    struct {
        struct {
            struct timespec tuning_time;
            lle_tuning_action_t action;
            double performance_before;
            double performance_after;
            double improvement_ratio;
        } tuning_history[LLE_TUNING_HISTORY_SIZE];
        
        size_t history_count;
        double cumulative_improvement;
    } tuning_history;
} lle_memory_pool_tuner_t;

/**
 * @brief Memory error structure
 */
typedef struct {
    lle_memory_error_type_t error_type;
    void *error_address;
    size_t error_size;
    struct timespec error_time;
    char error_description[256];
} lle_memory_error_t;

/**
 * @brief Integration error
 */
typedef struct {
    int error_code;
    char error_message[256];
} lle_integration_error_t;

/**
 * @brief Memory error handler
 */
typedef struct {
    /* Error detection configuration */
    struct {
        bool enable_bounds_checking;
        bool enable_leak_detection;
        bool enable_corruption_detection;
        bool enable_double_free_detection;
        bool enable_use_after_free_detection;
    } detection_config;
    
    /* Error tracking */
    struct {
        lle_memory_error_t recent_errors[LLE_ERROR_HISTORY_SIZE];
        size_t error_count;
        size_t critical_error_count;
        struct timespec last_error_time;
    } error_tracking;
    
    /* Recovery strategies */
    struct {
        lle_recovery_strategy_t default_strategy;
        lle_recovery_strategy_t leak_recovery_strategy;
        lle_recovery_strategy_t corruption_recovery_strategy;
        bool enable_automatic_recovery;
        size_t max_recovery_attempts;
    } recovery_config;
    
    /* Error statistics */
    struct {
        uint64_t bounds_violations;
        uint64_t memory_leaks;
        uint64_t corruption_events;
        uint64_t double_frees;
        uint64_t use_after_frees;
        uint64_t successful_recoveries;
        uint64_t failed_recoveries;
    } error_statistics;
} lle_memory_error_handler_t;

/**
 * @brief Buffer overflow protection
 */
typedef struct {
    /* Protection configuration */
    struct {
        bool enable_canary_protection;
        bool enable_guard_pages;
        bool enable_bounds_checking;
        bool enable_fortification;
        size_t guard_page_size;
    } protection_config;
    
    /* Canary management */
    struct {
        uint64_t canary_value;
        struct timespec canary_generation_time;
        size_t canary_violations_detected;
        bool rotate_canaries;
        struct timespec rotation_interval;
    } canary_system;
    
    /* Guard page management */
    struct {
        void **guard_pages;
        size_t guard_page_count;
        size_t guard_page_capacity;
        size_t guard_page_violations;
    } guard_page_system;
    
    /* Bounds checking system */
    struct {
        struct {
            void *buffer_start;
            void *buffer_end;
            size_t buffer_size;
            uint32_t access_permissions;
        } tracked_buffers[LLE_MAX_TRACKED_BUFFERS];
        
        size_t tracked_buffer_count;
        size_t bounds_violations_detected;
    } bounds_checking;
} lle_buffer_overflow_protection_t;

/**
 * @brief Memory encryption
 */
typedef struct {
    /* Encryption configuration */
    struct {
        lle_encryption_algorithm_t algorithm;
        size_t key_size;
        size_t block_size;
        bool encrypt_sensitive_data;
        bool encrypt_all_allocations;
    } encryption_config;
    
    /* Key management */
    struct {
        uint8_t master_key[LLE_MAX_KEY_SIZE];
        uint8_t derived_keys[LLE_MAX_DERIVED_KEYS][LLE_MAX_KEY_SIZE];
        size_t active_key_index;
        struct timespec key_generation_time;
        struct timespec key_rotation_interval;
    } key_management;
    
    /* Encryption state */
    struct {
        size_t encrypted_allocations;
        size_t total_encrypted_bytes;
        double encryption_overhead;
        struct timespec average_encryption_time;
        struct timespec average_decryption_time;
    } encryption_state;
    
    /* Security monitoring */
    struct {
        size_t encryption_failures;
        size_t decryption_failures;
        size_t key_rotation_count;
        size_t security_violations;
        struct timespec last_security_event;
    } security_monitoring;
} lle_memory_encryption_t;

/**
 * @brief Complete LLE-Lusush memory integration
 */
typedef struct {
    /* Core integration components */
    lle_memory_manager_t *lle_memory_manager;
    lusush_memory_system_t *lusush_memory_system;
    
    /* Integration state */
    struct {
        bool integration_active;
        lle_integration_mode_t mode;
        double memory_sharing_ratio;
        size_t shared_memory_regions;
        struct timespec integration_start_time;
    } integration_state;
    
    /* Performance monitoring */
    struct {
        uint64_t cross_system_allocations;
        uint64_t shared_memory_hits;
        uint64_t shared_memory_misses;
        double integration_overhead;
        struct timespec average_cross_allocation_time;
    } integration_performance;
    
    /* Synchronization and coordination */
    struct {
        pthread_mutex_t integration_mutex;
        pthread_rwlock_t shared_memory_lock;
        sem_t resource_semaphore;
        volatile bool coordination_active;
    } synchronization;
    
    /* Error handling and recovery */
    struct {
        size_t integration_errors;
        size_t sync_failures;
        lle_integration_error_t last_error;
        struct timespec last_error_time;
        bool automatic_recovery_enabled;
    } error_handling;
} lle_lusush_memory_integration_complete_t;

/**
 * @brief Display memory coordination
 */
typedef struct {
    /* Display memory pools */
    lle_memory_pool_t *prompt_memory_pool;
    lle_memory_pool_t *syntax_highlight_pool;
    lle_memory_pool_t *autosuggestion_pool;
    lle_memory_pool_t *composition_pool;
    
    /* Buffer coordination */
    struct {
        void *display_buffer;
        void *scratch_buffer;
        void *backup_buffer;
        size_t buffer_size;
        size_t buffer_alignment;
    } buffer_coordination;
    
    /* Rendering optimization */
    struct {
        size_t frame_memory_budget;
        double memory_pressure_threshold;
        bool enable_memory_recycling;
        size_t recycling_pool_size;
    } rendering_optimization;
    
    /* Performance tracking */
    struct {
        uint64_t display_allocations;
        uint64_t recycled_buffers;
        struct timespec average_allocation_time;
        double memory_efficiency;
    } performance_tracking;
} lle_display_memory_coordination_t;

/**
 * @brief Memory test framework
 */
typedef struct {
    /* Test configuration */
    struct {
        bool enable_stress_testing;
        bool enable_leak_testing;
        bool enable_performance_testing;
        bool enable_concurrency_testing;
        size_t test_duration_seconds;
        size_t concurrent_thread_count;
    } test_config;
    
    /* Test statistics */
    struct {
        uint64_t total_test_allocations;
        uint64_t successful_allocations;
        uint64_t failed_allocations;
        uint64_t memory_leaks_detected;
        uint64_t corruption_events_detected;
    } test_statistics;
    
    /* Performance benchmarks */
    struct {
        struct timespec fastest_allocation;
        struct timespec slowest_allocation;
        struct timespec average_allocation_time;
        double allocations_per_second;
        size_t peak_memory_usage;
    } performance_benchmarks;
    
    /* Test results */
    struct {
        bool all_tests_passed;
        size_t passed_test_count;
        size_t failed_test_count;
        lle_test_failure_reason_t failure_reasons[LLE_MAX_TEST_FAILURES];
        size_t failure_count;
    } test_results;
} lle_memory_test_framework_t;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================
 * All implementations are in src/lle/memory_management.c (Layer 1)
 */

/* Memory State Management */
lle_result_t lle_memory_transition_state(lle_memory_manager_t *manager, lle_memory_state_t new_state);
bool lle_memory_is_valid_transition(lle_memory_state_t old_state, lle_memory_state_t new_state);
lle_result_t lle_memory_initialize_pools(lle_memory_manager_t *manager);
void lle_memory_start_monitoring(lle_memory_manager_t *manager);
lle_result_t lle_memory_start_optimization(lle_memory_manager_t *manager);
lle_result_t lle_memory_start_garbage_collection(lle_memory_manager_t *manager);
lle_result_t lle_memory_handle_low_memory(lle_memory_manager_t *manager);
lle_result_t lle_memory_handle_error_state(lle_memory_manager_t *manager);
lle_result_t lle_memory_shutdown_pools(lle_memory_manager_t *manager);

/* Lusush Integration */
lle_result_t lle_integrate_with_lusush_memory(lle_memory_manager_t *manager);
lusush_memory_pool_t* lusush_get_memory_pools(void);
lle_result_t lle_analyze_lusush_memory_config(lusush_memory_pool_t *lusush_pools, lle_memory_config_t *lusush_config);
lle_result_t lle_create_specialized_pool(lle_memory_manager_t *manager, const lle_memory_pool_config_t *pool_config);
void lle_cleanup_partial_integration(lle_memory_manager_t *manager, size_t pool_index);
lle_result_t lle_create_shared_memory_regions(lle_memory_manager_t *manager, const lle_memory_config_t *lusush_config);
lle_result_t lle_initialize_cross_allocation_tables(lle_memory_manager_t *manager);
lle_result_t lle_start_integration_monitoring(lle_memory_manager_t *manager);

/* Shared Memory Pool Management */
void* lle_shared_memory_allocate(lle_shared_memory_pool_t *pool, size_t size, lle_memory_pool_type_t owner);
size_t lle_align_memory_size(size_t size, size_t alignment);
int lle_find_suitable_fragment(lle_shared_memory_pool_t *pool, size_t size);
void lle_remove_fragment(lle_shared_memory_pool_t *pool, int fragment_index);
struct timespec lle_get_current_time(void);

/* Buffer Memory Pool */
void* lle_buffer_memory_allocate(lle_buffer_memory_pool_t *pool, size_t size, lle_buffer_type_t buffer_type);
void* lle_check_string_cache(lle_buffer_memory_pool_t *pool, size_t size);
void* lle_allocate_buffer_block(lle_buffer_memory_pool_t *pool, size_t size);
size_t lle_optimize_buffer_allocation_size(size_t size, lle_buffer_type_t buffer_type);
void* lle_pool_allocate(lle_memory_pool_base_t *pool, size_t size);
void lle_initialize_edit_buffer(void *buffer, size_t size);

/* Hierarchical Memory Allocation */
void* lle_hierarchical_allocate(lle_memory_pool_hierarchy_t *hierarchy, size_t size, lle_memory_pool_type_t preferred_type);
lle_allocation_strategy_t lle_determine_allocation_strategy(size_t size);
void* lle_try_primary_allocation(lle_memory_pool_hierarchy_t *hierarchy, size_t size, lle_memory_pool_type_t preferred_type);
void* lle_try_secondary_allocation(lle_memory_pool_hierarchy_t *hierarchy, size_t size, lle_memory_pool_type_t preferred_type);
void* lle_try_emergency_allocation(lle_memory_pool_hierarchy_t *hierarchy, size_t size);
void lle_log_emergency_allocation(size_t size, lle_memory_pool_type_t preferred_type);
void lle_handle_allocation_failure(lle_memory_pool_hierarchy_t *hierarchy, size_t size, lle_memory_pool_type_t preferred_type);

/* Dynamic Pool Resizing */
lle_result_t lle_dynamic_pool_resize(lle_dynamic_pool_resizer_t *resizer);
double lle_calculate_pool_utilization(lle_memory_pool_t *pool);
lle_resize_decision_t lle_evaluate_resize_need(lle_dynamic_pool_resizer_t *resizer, double utilization);
size_t lle_get_pool_size(lle_memory_pool_t *pool);
size_t lle_clamp_size(size_t value, size_t min, size_t max);
lle_result_t lle_atomic_pool_resize(lle_memory_pool_t *pool, size_t old_size, size_t new_size);
void lle_update_utilization_stats(lle_dynamic_pool_resizer_t *resizer, double utilization);
lle_result_t lle_expand_pool_memory(lle_memory_pool_t *pool, size_t additional_size);
lle_result_t lle_compact_pool_memory(lle_memory_pool_t *pool, size_t reduction_size);
void lle_recalculate_free_space(lle_memory_pool_t *pool);
void lle_notify_pool_resize_listeners(lle_memory_pool_t *pool, size_t old_size, size_t new_size);

/* Garbage Collection */
lle_result_t lle_perform_garbage_collection(lle_garbage_collector_t *gc);
lle_result_t lle_gc_transition_state(lle_garbage_collector_t *gc, lle_gc_state_t new_state);
lle_result_t lle_gc_mark_phase(lle_garbage_collector_t *gc, size_t *objects_marked);
lle_result_t lle_gc_sweep_phase(lle_garbage_collector_t *gc, size_t *memory_freed);
lle_result_t lle_gc_compact_phase(lle_garbage_collector_t *gc);
struct timespec lle_timespec_diff(struct timespec start, struct timespec end);
void lle_update_gc_performance_stats(lle_garbage_collector_t *gc, struct timespec gc_duration, size_t memory_freed);

/* Buffer Memory Management */
lle_result_t lle_initialize_buffer_memory(lle_buffer_memory_t *buffer_mem, const lle_buffer_config_t *config);
void* lle_pool_allocate_aligned(lle_memory_pool_t *pool, size_t size, size_t alignment);
void lle_cleanup_buffer_regions(lle_buffer_memory_t *buffer_mem, lle_memory_pool_t *pool);
lle_result_t lle_initialize_utf8_management(lle_buffer_memory_t *buffer_mem, const lle_buffer_config_t *config);
size_t lle_calculate_scratch_buffer_size(const lle_buffer_config_t *config);
void lle_pool_free(lle_memory_pool_t *pool, void *ptr);

/* Multiline Buffer Management */
lle_result_t lle_insert_line(lle_multiline_buffer_t *multiline_buffer, lle_buffer_memory_t *buffer_memory, size_t line_index, const char *line_text, size_t line_length);
lle_result_t lle_expand_line_tracking_arrays(lle_multiline_buffer_t *multiline_buffer);
bool lle_buffer_has_space(lle_buffer_memory_t *buffer_memory, size_t required_space);
lle_result_t lle_expand_primary_buffer(lle_buffer_memory_t *buffer_memory, size_t additional_space);
size_t lle_calculate_buffer_tail_size(lle_buffer_memory_t *buffer_memory, size_t offset);
void lle_mark_line_modified(lle_multiline_buffer_t *multiline_buffer, size_t line_index);
void lle_update_utf8_tracking_after_insertion(lle_buffer_memory_t *buffer_memory, size_t offset, size_t size);

/* Event System Memory Integration */
void* lle_allocate_event_fast(lle_event_memory_integration_t *integration, lle_event_type_t event_type, size_t event_size);
void* lle_allocate_from_input_cache(lle_event_memory_integration_t *integration);
void* lle_allocate_from_display_cache(lle_event_memory_integration_t *integration);
void* lle_allocate_from_system_cache(lle_event_memory_integration_t *integration);
void* lle_pool_allocate_fast(lle_memory_pool_t *pool, size_t size);
void lle_free_event_fast(lle_event_memory_integration_t *integration, void *event_ptr, lle_event_type_t event_type, size_t event_size);
bool lle_return_to_input_cache(lle_event_memory_integration_t *integration, void *event_ptr);
bool lle_return_to_display_cache(lle_event_memory_integration_t *integration, void *event_ptr);
bool lle_return_to_system_cache(lle_event_memory_integration_t *integration, void *event_ptr);
void lle_pool_free_fast(lle_memory_pool_t *pool, void *ptr);

/* Memory Access Pattern Optimization */
lle_result_t lle_analyze_memory_access_patterns(lle_memory_access_optimizer_t *optimizer);
lle_result_t lle_analyze_recent_accesses(lle_memory_access_optimizer_t *optimizer, lle_access_pattern_analysis_t *pattern_analysis);
lle_result_t lle_identify_hot_regions(lle_memory_access_optimizer_t *optimizer, lle_access_pattern_analysis_t *pattern_analysis);
lle_result_t lle_calculate_locality_scores(lle_memory_access_optimizer_t *optimizer);
lle_result_t lle_update_prefetch_strategy(lle_memory_access_optimizer_t *optimizer, lle_access_pattern_analysis_t *pattern_analysis);
bool lle_should_optimize_layout(lle_access_pattern_analysis_t *pattern_analysis);
lle_result_t lle_optimize_memory_layout(lle_memory_access_optimizer_t *optimizer);
void lle_log_memory_analysis_performance(struct timespec duration, lle_access_pattern_analysis_t *pattern_analysis);

/* Memory Pool Performance Tuning */
lle_result_t lle_tune_memory_pool_performance(lle_memory_pool_tuner_t *tuner);
lle_result_t lle_measure_pool_performance(lle_memory_pool_t *pool, size_t sample_size, lle_memory_pool_performance_t *performance);
lle_result_t lle_analyze_performance_bottlenecks(lle_memory_pool_tuner_t *tuner, lle_memory_pool_performance_t *current_performance, lle_performance_bottleneck_analysis_t *bottleneck_analysis);
lle_result_t lle_create_tuning_action_plan(lle_memory_pool_tuner_t *tuner, lle_performance_bottleneck_analysis_t *bottleneck_analysis, lle_tuning_action_plan_t *action_plan);
lle_result_t lle_execute_tuning_action(lle_memory_pool_tuner_t *tuner, lle_tuning_action_item_t *action);
void lle_rollback_tuning_actions(lle_memory_pool_tuner_t *tuner, lle_tuning_action_plan_t *action_plan, size_t action_index);
void lle_update_average_time(struct timespec *average, struct timespec new_sample, size_t sample_count);

/* Error Detection and Recovery */
lle_result_t lle_detect_memory_errors(lle_memory_error_handler_t *error_handler, lle_memory_manager_t *memory_manager);
lle_result_t lle_detect_memory_leaks(lle_memory_error_handler_t *error_handler, lle_memory_manager_t *memory_manager);
lle_result_t lle_detect_bounds_violations(lle_memory_error_handler_t *error_handler, lle_memory_manager_t *memory_manager);
lle_result_t lle_detect_memory_corruption(lle_memory_error_handler_t *error_handler, lle_memory_manager_t *memory_manager);
lle_result_t lle_detect_double_free_attempts(lle_memory_error_handler_t *error_handler, lle_memory_manager_t *memory_manager);
lle_result_t lle_detect_use_after_free(lle_memory_error_handler_t *error_handler, lle_memory_manager_t *memory_manager);
void lle_record_memory_error(lle_memory_error_handler_t *error_handler, lle_result_t error_result);
lle_result_t lle_recover_from_memory_error(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error, lle_memory_manager_t *memory_manager);
lle_recovery_strategy_t lle_determine_recovery_strategy(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error);
lle_result_t lle_recover_from_memory_leak(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error, lle_memory_manager_t *memory_manager);
lle_result_t lle_recover_from_bounds_violation(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error, lle_memory_manager_t *memory_manager);
lle_result_t lle_recover_from_corruption(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error, lle_memory_manager_t *memory_manager);
lle_result_t lle_recover_from_double_free(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error, lle_memory_manager_t *memory_manager);
lle_result_t lle_recover_from_use_after_free(lle_memory_error_handler_t *error_handler, lle_memory_error_t *error, lle_memory_manager_t *memory_manager);

/* Buffer Overflow Protection */
lle_result_t lle_check_buffer_bounds(lle_buffer_overflow_protection_t *protection, void *buffer_ptr, size_t access_size, lle_access_type_t access_type);
uint32_t lle_access_type_to_permissions(lle_access_type_t access_type);
void lle_log_security_incident(lle_security_incident_type_t incident_type, void *address, size_t size);

/* Memory Encryption */
lle_result_t lle_encrypt_memory_allocation(lle_memory_encryption_t *encryption, void *memory_ptr, size_t memory_size, lle_data_sensitivity_t sensitivity);
lle_result_t lle_encrypt_data_in_place(void *data, size_t size, uint8_t *key, size_t key_size, lle_encryption_algorithm_t algorithm);

/* Complete Integration */
lle_result_t lle_initialize_complete_memory_integration(lle_lusush_memory_integration_complete_t *integration, lle_memory_manager_t *lle_manager, lusush_memory_system_t *lusush_system);
void lle_cleanup_integration_sync(lle_lusush_memory_integration_complete_t *integration);
lle_result_t lle_establish_shared_memory_regions(lle_lusush_memory_integration_complete_t *integration);
lle_result_t lle_configure_integration_mode(lle_lusush_memory_integration_complete_t *integration, lle_integration_mode_t mode);
void lle_cleanup_shared_memory_regions(lle_lusush_memory_integration_complete_t *integration);

/* Display Memory Coordination */
void* lle_allocate_display_memory_optimized(lle_display_memory_coordination_t *coord, lle_display_memory_type_t type, size_t size);
void* lle_try_recycle_display_buffer(lle_display_memory_coordination_t *coord, lle_display_memory_type_t type, size_t size);
double lle_calculate_memory_pressure(lle_display_memory_coordination_t *coord);
void lle_apply_memory_pressure_relief(lle_display_memory_coordination_t *coord);

/* Testing and Validation */
lle_result_t lle_run_comprehensive_memory_tests(lle_memory_test_framework_t *test_framework, lle_memory_manager_t *memory_manager);
lle_result_t lle_run_basic_memory_tests(lle_memory_test_framework_t *test_framework, lle_memory_manager_t *memory_manager);
void lle_record_test_failure(lle_memory_test_framework_t *test_framework, lle_test_failure_reason_t reason, lle_result_t result);
lle_result_t lle_run_memory_stress_tests(lle_memory_test_framework_t *test_framework, lle_memory_manager_t *memory_manager);
lle_result_t lle_run_memory_leak_tests(lle_memory_test_framework_t *test_framework, lle_memory_manager_t *memory_manager);
lle_result_t lle_run_performance_benchmarks(lle_memory_test_framework_t *test_framework, lle_memory_manager_t *memory_manager);
lle_result_t lle_run_concurrency_tests(lle_memory_test_framework_t *test_framework, lle_memory_manager_t *memory_manager);
void lle_generate_memory_test_report(lle_memory_test_framework_t *test_framework, struct timespec test_duration, lle_result_t overall_result);

#endif /* LLE_MEMORY_MANAGEMENT_H */

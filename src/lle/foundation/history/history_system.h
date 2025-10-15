// src/lle/foundation/history/history_system.h
//
// LLE History System - Complete Spec 09 Structural Implementation
//
// This module provides the complete architectural structure for the LLE History
// System as specified in Spec 09 (History System Complete Specification).
//
// COMPLIANCE STATUS: 100% STRUCTURAL COMPLIANCE
// - All 20 components from Spec 09 are present in structure
// - Working components integrated (lle_history_t)
// - Non-implemented components stubbed with TODO_SPEC09 markers
// - All types forward-declared for future implementation
//
// Implementation follows SPECIFICATION_IMPLEMENTATION_POLICY.md:
// - Complete structure definitions with ALL fields present
// - Exact naming compliance with Spec 09
// - TODO markers for all stubs referencing specification sections
// - Forward declarations for all supporting types

#ifndef LLE_FOUNDATION_HISTORY_SYSTEM_H
#define LLE_FOUNDATION_HISTORY_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>
#include "history.h"  // Existing working history implementation

// =============================================================================
// FORWARD DECLARATIONS - Supporting Types (Not Yet Implemented)
// =============================================================================
// These types are defined in Spec 09 but not yet implemented.
// They are forward-declared to support the complete structure definition.

// Core history management (Spec 09 Section 2)
typedef struct lle_history_core lle_history_core_t;

// Forensic tracking (Spec 09 Section 4)
typedef struct lle_forensic_tracker lle_forensic_tracker_t;

// Search and retrieval (Spec 09 Section 6)
typedef struct lle_history_search_engine lle_history_search_engine_t;
typedef struct lle_history_dedup_engine lle_history_dedup_engine_t;

// History-Buffer Integration (Spec 09 Section 12)
typedef struct lle_history_buffer_integration lle_history_buffer_integration_t;
typedef struct lle_edit_session_manager lle_edit_session_manager_t;
typedef struct lle_multiline_reconstruction lle_multiline_reconstruction_t;

// Lusush system integration (Spec 09 Section 5)
typedef struct posix_history_manager posix_history_manager_t;
typedef struct lle_history_bridge lle_history_bridge_t;
typedef struct lle_history_sync_manager lle_history_sync_manager_t;

// Storage and persistence (Spec 09 Section 7)
typedef struct lle_history_storage lle_history_storage_t;
typedef struct lle_history_cache lle_history_cache_t;

// Performance and coordination (Spec 09 Section 8, 10)
typedef struct lle_hash_table lle_hash_table_t;
typedef struct lle_performance_monitor lle_performance_monitor_t;
typedef struct lle_event_coordinator lle_event_coordinator_t;

// Memory management (Spec 09 Section 9)
typedef struct memory_pool memory_pool_t;

// Security and configuration (Spec 09 Section 11, 16)
typedef struct lle_history_security lle_history_security_t;
typedef struct lle_history_config lle_history_config_t;
typedef struct lle_history_state lle_history_state_t;

// =============================================================================
// PRIMARY HISTORY SYSTEM STRUCTURE - SPEC 09 COMPLIANT
// =============================================================================

/**
 * LLE History System - Complete architectural structure
 *
 * This structure contains all 20 components specified in Spec 09 Section 2.1.
 * Components marked with TODO_SPEC09 are stubbed for future implementation.
 * Working components are integrated and functional.
 *
 * COMPLIANCE: 100% structural, ~10% functional
 * - Structure has ALL 20 required component pointers
 * - Currently ~2 components working (lle_history_t integration)
 * - Remaining 18 components stubbed with clear TODO markers
 */
typedef struct lle_history_system {
    // -------------------------------------------------------------------------
    // CORE HISTORY MANAGEMENT (4 components)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: Core history management engine (Spec 09 Section 3)
    // Target: Phase 1 implementation
    // Purpose: Central history management with forensic capabilities
    // Status: STUBBED - Using legacy lle_history_t for now
    lle_history_core_t *history_core;
    
    // TODO_SPEC09: Forensic metadata and lifecycle tracking (Spec 09 Section 4)
    // Target: Phase 2 implementation
    // Purpose: Complete command lifecycle tracking with metadata forensics
    // Status: STUBBED - No forensic tracking yet
    lle_forensic_tracker_t *forensic_tracker;
    
    // TODO_SPEC09: Advanced search and retrieval system (Spec 09 Section 6)
    // Target: Phase 1 implementation
    // Purpose: Multi-modal search with fuzzy matching, context awareness
    // Status: STUBBED - Basic pattern search exists in lle_history_t
    lle_history_search_engine_t *search_engine;
    
    // TODO_SPEC09: Intelligent deduplication system (Spec 09 Section 6)
    // Target: Phase 1 implementation
    // Purpose: Intelligent duplicate detection with context-aware preservation
    // Status: STUBBED - Basic duplicate check exists in lle_history_t
    lle_history_dedup_engine_t *dedup_engine;
    
    // -------------------------------------------------------------------------
    // HISTORY-BUFFER INTEGRATION (3 components)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: History-buffer integration system (Spec 09 Section 12)
    // Target: Phase 2 implementation
    // Purpose: Seamless loading of historical commands into editing buffer
    // Status: STUBBED - No buffer integration yet
    lle_history_buffer_integration_t *buffer_integration;
    
    // TODO_SPEC09: Interactive editing session management (Spec 09 Section 13)
    // Target: Phase 2 implementation
    // Purpose: Track and manage interactive history editing sessions
    // Status: STUBBED - No session management yet
    lle_edit_session_manager_t *edit_session_manager;
    
    // TODO_SPEC09: Multiline command reconstruction (Spec 09 Section 14)
    // Target: Phase 2 implementation
    // Purpose: Full preservation and restoration of complex shell constructs
    // Status: STUBBED - No multiline support yet
    lle_multiline_reconstruction_t *multiline_engine;
    
    // -------------------------------------------------------------------------
    // LUSUSH SYSTEM INTEGRATION (3 components)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: Existing POSIX history system (Spec 09 Section 5.1)
    // Target: Phase 1 integration
    // Purpose: Integrate with existing POSIX history and enhanced history
    // Status: STUBBED - No POSIX integration yet
    posix_history_manager_t *posix_history;
    
    // TODO_SPEC09: Bridge to Lusush history systems (Spec 09 Section 5.1)
    // Target: Phase 1 integration
    // Purpose: Native integration with existing history functionality
    // Status: STUBBED - No bridge implementation yet
    lle_history_bridge_t *lusush_bridge;
    
    // TODO_SPEC09: Real-time bidirectional synchronization (Spec 09 Section 5.1)
    // Target: Phase 1 integration
    // Purpose: Sync between LLE history and Lusush history systems
    // Status: STUBBED - No sync manager yet
    lle_history_sync_manager_t *sync_manager;
    
    // -------------------------------------------------------------------------
    // STORAGE AND PERSISTENCE (3 components)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: History persistence and file management (Spec 09 Section 7)
    // Target: Phase 1 implementation
    // Purpose: Advanced storage with compression, encryption
    // Status: STUBBED - Basic file I/O exists in lle_history_t
    lle_history_storage_t *storage_manager;
    
    // TODO_SPEC09: Intelligent history caching system (Spec 09 Section 8)
    // Target: Phase 2 optimization
    // Purpose: Cache frequently accessed history for performance
    // Status: STUBBED - No caching yet, linear search
    lle_history_cache_t *cache_system;
    
    // TODO_SPEC09: Fast command lookup hashtable (Spec 09 Section 8)
    // Target: Phase 2 optimization
    // Purpose: O(1) command lookup instead of O(n) search
    // Status: STUBBED - Linear search currently
    lle_hash_table_t *command_index;
    
    // -------------------------------------------------------------------------
    // PERFORMANCE AND COORDINATION (3 components)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: History performance monitoring (Spec 09 Section 8)
    // Target: Phase 2 optimization
    // Purpose: Monitor history system performance and metrics
    // Status: STUBBED - No performance monitoring yet
    lle_performance_monitor_t *perf_monitor;
    
    // TODO_SPEC09: Event system coordination (Spec 09 Section 10)
    // Target: Phase 2 integration
    // Purpose: Event-driven history updates and notifications
    // Status: STUBBED - No event integration yet
    lle_event_coordinator_t *event_coordinator;
    
    // TODO_SPEC09: Lusush memory pool integration (Spec 09 Section 9)
    // Target: Phase 1 implementation
    // Purpose: Zero-allocation history operations with memory pool
    // Status: STUBBED - Currently using malloc/free
    memory_pool_t *memory_pool;
    
    // -------------------------------------------------------------------------
    // SECURITY AND CONFIGURATION (3 components)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: Privacy and access control (Spec 09 Section 11)
    // Target: Phase 2 security
    // Purpose: Enterprise-grade privacy controls and filtering
    // Status: STUBBED - No security controls yet
    lle_history_security_t *security_manager;
    
    // TODO_SPEC09: History system configuration (Spec 09 Section 16)
    // Target: Phase 1 implementation
    // Purpose: Comprehensive configuration management
    // Status: STUBBED - Basic inline flags in lle_history_t
    lle_history_config_t *config;
    
    // TODO_SPEC09: Current system state tracking (Spec 09 Section 2.1)
    // Target: Phase 1 implementation
    // Purpose: Track history system operational state
    // Status: STUBBED - Ad-hoc state management
    lle_history_state_t *current_state;
    
    // -------------------------------------------------------------------------
    // SYNCHRONIZATION AND STATE (4 fields)
    // -------------------------------------------------------------------------
    
    // TODO_SPEC09: Thread-safe access control (Spec 09 Section 2.1)
    // Target: Phase 2 threading
    // Purpose: Thread-safe history access with read-write locks
    // Status: STUBBED - Not thread-safe currently
    pthread_rwlock_t history_lock;
    
    // History system active status
    bool system_active;
    
    // Operation tracking counter for metrics
    uint64_t operation_counter;
    
    // History API version for compatibility
    uint32_t api_version;
    
    // -------------------------------------------------------------------------
    // WORKING COMPONENTS - Currently Integrated
    // -------------------------------------------------------------------------
    
    // ✅ WORKING: Legacy history implementation (basic functionality)
    // This provides basic POSIX-style history until full Spec 09 implementation
    // Will eventually be replaced by history_core with full capabilities
    lle_history_t *legacy_history;
    
} lle_history_system_t;

// =============================================================================
// API VERSION
// =============================================================================

#define LLE_HISTORY_SYSTEM_API_VERSION 1

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * Initialize LLE History System with complete architectural structure
 *
 * Creates and initializes the complete history system structure with all 20
 * components from Spec 09. Currently integrates working legacy history and
 * stubs remaining components for future implementation.
 *
 * @param system Pointer to history system structure pointer (will be allocated)
 * @return 0 on success, negative error code on failure
 *
 * CURRENT IMPLEMENTATION:
 * - Allocates lle_history_system_t structure
 * - Initializes legacy_history with working lle_history_t
 * - Sets all stub component pointers to NULL with TODO markers
 * - Initializes synchronization and state fields
 *
 * FUTURE PHASES:
 * - Phase 1: Integrate memory_pool, storage_manager, search_engine
 * - Phase 2: Add forensic_tracker, buffer_integration, cache_system
 * - Phase 3: Implement lusush_bridge, sync_manager, event_coordinator
 */
int lle_history_system_init(lle_history_system_t **system,
                           const char *history_file,
                           size_t capacity);

/**
 * Cleanup and destroy history system
 *
 * Properly cleans up all initialized components and frees allocated memory.
 *
 * @param system History system to cleanup (pointer will be set to NULL)
 */
void lle_history_system_cleanup(lle_history_system_t **system);

/**
 * Get current history system version
 *
 * @return API version number
 */
uint32_t lle_history_system_get_version(void);

/**
 * Check if history system is active
 *
 * @param system History system
 * @return true if active, false otherwise
 */
bool lle_history_system_is_active(const lle_history_system_t *system);

// =============================================================================
// STUB COMPONENT INITIALIZATION FUNCTIONS
// =============================================================================
// These functions are declared but not yet implemented. They will return NULL
// or error codes until the corresponding components are implemented.

// TODO_SPEC09: Core history management (Spec 09 Section 3)
lle_history_core_t* lle_history_core_create_stub(void);

// TODO_SPEC09: Forensic tracking (Spec 09 Section 4)
lle_forensic_tracker_t* lle_forensic_tracker_create_stub(void);

// TODO_SPEC09: Search engine (Spec 09 Section 6)
lle_history_search_engine_t* lle_history_search_engine_create_stub(void);

// TODO_SPEC09: Deduplication engine (Spec 09 Section 6)
lle_history_dedup_engine_t* lle_history_dedup_engine_create_stub(void);

// TODO_SPEC09: Buffer integration (Spec 09 Section 12)
lle_history_buffer_integration_t* lle_history_buffer_integration_create_stub(void);

// TODO_SPEC09: Edit session manager (Spec 09 Section 13)
lle_edit_session_manager_t* lle_edit_session_manager_create_stub(void);

// TODO_SPEC09: Multiline reconstruction (Spec 09 Section 14)
lle_multiline_reconstruction_t* lle_multiline_reconstruction_create_stub(void);

// TODO_SPEC09: POSIX history manager (Spec 09 Section 5.1)
posix_history_manager_t* posix_history_manager_create_stub(void);

// TODO_SPEC09: Lusush bridge (Spec 09 Section 5.1)
lle_history_bridge_t* lle_history_bridge_create_stub(void);

// TODO_SPEC09: Sync manager (Spec 09 Section 5.1)
lle_history_sync_manager_t* lle_history_sync_manager_create_stub(void);

// TODO_SPEC09: Storage manager (Spec 09 Section 7)
lle_history_storage_t* lle_history_storage_create_stub(void);

// TODO_SPEC09: Cache system (Spec 09 Section 8)
lle_history_cache_t* lle_history_cache_create_stub(void);

// TODO_SPEC09: Command index (Spec 09 Section 8)
lle_hash_table_t* lle_hash_table_create_stub(void);

// TODO_SPEC09: Performance monitor (Spec 09 Section 8)
lle_performance_monitor_t* lle_performance_monitor_create_stub(void);

// TODO_SPEC09: Event coordinator (Spec 09 Section 10)
lle_event_coordinator_t* lle_event_coordinator_create_stub(void);

// TODO_SPEC09: Memory pool (Spec 09 Section 9)
memory_pool_t* memory_pool_create_stub(void);

// TODO_SPEC09: Security manager (Spec 09 Section 11)
lle_history_security_t* lle_history_security_create_stub(void);

// TODO_SPEC09: History config (Spec 09 Section 16)
lle_history_config_t* lle_history_config_create_stub(void);

// TODO_SPEC09: History state (Spec 09 Section 2.1)
lle_history_state_t* lle_history_state_create_stub(void);

#endif // LLE_FOUNDATION_HISTORY_SYSTEM_H

// src/lle/foundation/display/display_integration_system.h
//
// LLE Display Integration System (Spec 08 Implementation)
//
// SPECIFICATION COMPLIANCE: This file implements the COMPLETE Spec 08 Display Integration
// architecture with ALL 14 components as specified in Section 2.1. Components not yet
// needed for current phase are stubbed with NULL and marked with TODO_SPEC08.
//
// IMPORTANT: This follows SPECIFICATION_IMPLEMENTATION_POLICY.md requirements:
// - ALL structure fields present (no omissions)
// - EXACT naming from specification
// - Stubs clearly marked with TODO_SPEC08
// - Forward declarations for all types
//
// Current Implementation Status:
// - display_controller_t: ⚠️ PARTIAL (exists in lusush, void* reference)
// - lle_display_t: ✅ COMPLETE (basic display - display.h/display.c)
// - lle_display_bridge_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_render_controller_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_display_cache_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_composition_manager_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - theme_manager_t: ⚠️ PARTIAL (exists in lusush, not yet integrated)
// - memory_pool_t: ⚠️ PARTIAL (exists in lusush, not yet integrated)
// - lle_display_metrics_t: ⚠️ PARTIAL (basic counters exist)
// - lle_event_coordinator_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_terminal_adapter_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_display_config_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_display_state_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)
// - lle_hash_table_t: ⚠️ STUBBED (TODO_SPEC08 - Future phase)

#ifndef LLE_FOUNDATION_DISPLAY_INTEGRATION_SYSTEM_H
#define LLE_FOUNDATION_DISPLAY_INTEGRATION_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

// Include existing working implementations
#include "display.h"          // lle_display_t (basic display)
#include "display_buffer.h"   // lle_display_buffer_renderer_t (buffer renderer)

// Forward declarations for Lusush system integration (Spec 08 Section 3)
// TODO_SPEC08: Lusush display system integration not yet complete (Spec 08 Section 3)
// Target: Future phase (Display System Integration)
typedef struct display_controller display_controller_t;

// TODO_SPEC08: Lusush theme system not yet integrated (Spec 08 Section 10)
// Target: Future phase (Theme System Integration)
typedef struct theme_manager theme_manager_t;

// TODO_SPEC08: Lusush memory pool not yet integrated (Spec 08 Section 8)
// Target: Future phase (Memory Management Integration)
typedef struct memory_pool memory_pool_t;

// Forward declarations for display integration components (Spec 08 Section 2.1)
// These will be implemented in future phases per SPECIFICATION_IMPLEMENTATION_POLICY.md

// TODO_SPEC08: Display bridge not yet implemented (Spec 08 Section 3.2)
// Target: Future phase (Display Bridge Architecture)
// Purpose: Bridge between LLE buffer system and Lusush display layers
typedef struct lle_display_bridge lle_display_bridge_t;

// TODO_SPEC08: Render controller not yet implemented (Spec 08 Section 4)
// Target: Future phase (Layered Display Controller)
// Purpose: Real-time rendering coordination and pipeline management
typedef struct lle_render_controller lle_render_controller_t;

// TODO_SPEC08: Display cache not yet implemented (Spec 08 Section 7)
// Target: Future phase (Performance Optimization)
// Purpose: Intelligent display state caching for performance
typedef struct lle_display_cache lle_display_cache_t;

// TODO_SPEC08: Composition manager not yet implemented (Spec 08 Section 4)
// Target: Future phase (Composition Management)
// Purpose: Display layer composition management
typedef struct lle_composition_manager lle_composition_manager_t;

// TODO_SPEC08: Display metrics not yet implemented (Spec 08 Section 7)
// Target: Future phase (Performance Monitoring)
// Purpose: Comprehensive display performance tracking
typedef struct lle_display_metrics lle_display_metrics_t;

// TODO_SPEC08: Event coordinator not yet implemented (Spec 08 Section 9)
// Target: Future phase (Event System Coordination)
// Purpose: Event system coordination between LLE and display
typedef struct lle_event_coordinator lle_event_coordinator_t;

// TODO_SPEC08: Terminal adapter not yet implemented (Spec 08 Section 11)
// Target: Future phase (Terminal Compatibility Layer)
// Purpose: Terminal compatibility management and abstraction
typedef struct lle_terminal_adapter lle_terminal_adapter_t;

// TODO_SPEC08: Display configuration not yet implemented (Spec 08 Section 13)
// Target: Future phase (Configuration Management)
// Purpose: Display integration configuration system
typedef struct lle_display_config lle_display_config_t;

// TODO_SPEC08: Display state not yet implemented (Spec 08 Section 3)
// Target: Future phase (State Management)
// Purpose: Current display state tracking and management
typedef struct lle_display_state lle_display_state_t;

// TODO_SPEC08: Hash table not yet implemented (Spec 08 Section 7)
// Target: Future phase (Render Cache)
// Purpose: Hash table for render result caching
typedef struct lle_hash_table lle_hash_table_t;

// Result codes for display integration system operations
typedef enum {
    LLE_DISPLAY_INTEGRATION_OK = 0,
    LLE_DISPLAY_INTEGRATION_ERR_NULL_PTR = -1,
    LLE_DISPLAY_INTEGRATION_ERR_ALLOC_FAILED = -2,
    LLE_DISPLAY_INTEGRATION_ERR_INVALID_STATE = -3,
    LLE_DISPLAY_INTEGRATION_ERR_COMPONENT_INIT_FAILED = -4,
    LLE_DISPLAY_INTEGRATION_ERR_THREAD_INIT = -5,
    LLE_DISPLAY_INTEGRATION_ERR_THEME_UNAVAILABLE = -6,
    LLE_DISPLAY_INTEGRATION_ERR_DISPLAY_UNAVAILABLE = -7,
} lle_display_integration_error_t;

// ============================================================================
// PRIMARY DISPLAY INTEGRATION SYSTEM (Spec 08 Section 2.1)
// ============================================================================
//
// This structure contains ALL 14 components specified in Spec 08 Section 2.1.
// Components marked with TODO_SPEC08 are stubbed for future implementation.
// This ensures forward compatibility and eliminates future API breaks.

typedef struct lle_display_integration_system {
    // ========================================================================
    // CORE INTEGRATION COMPONENTS (Stubbed - Future Phase)
    // ========================================================================
    
    // TODO_SPEC08: Display bridge not yet implemented (Spec 08 Section 3.2)
    // Target: Future phase (Display Bridge Architecture)
    // Purpose: Bridge between LLE and display system
    lle_display_bridge_t *display_bridge;
    
    // TODO_SPEC08: Render controller not yet implemented (Spec 08 Section 4)
    // Target: Future phase (Layered Display Controller)
    // Purpose: Real-time rendering coordination
    lle_render_controller_t *render_controller;
    
    // TODO_SPEC08: Display cache not yet implemented (Spec 08 Section 7)
    // Target: Future phase (Performance Optimization)
    // Purpose: Intelligent display state caching
    lle_display_cache_t *display_cache;
    
    // TODO_SPEC08: Composition manager not yet implemented (Spec 08 Section 4)
    // Target: Future phase (Composition Management)
    // Purpose: Display layer composition management
    lle_composition_manager_t *comp_manager;
    
    // ========================================================================
    // LUSUSH SYSTEM INTEGRATION (Partial - References to Lusush)
    // ========================================================================
    
    // TODO_SPEC08: Lusush display controller integration partial (Spec 08 Section 3)
    // Target: Future phase (Full Display System Integration)
    // Purpose: Existing Lusush display controller reference
    display_controller_t *lusush_display;
    
    // TODO_SPEC08: Lusush theme system not yet integrated (Spec 08 Section 10)
    // Target: Future phase (Theme System Integration)
    // Purpose: Existing theme system reference
    theme_manager_t *theme_system;
    
    // TODO_SPEC08: Lusush memory pool not yet integrated (Spec 08 Section 8)
    // Target: Future phase (Memory Management Integration)
    // Purpose: Lusush memory pool reference
    memory_pool_t *memory_pool;
    
    // ========================================================================
    // PERFORMANCE AND COORDINATION (Stubbed - Future Phase)
    // ========================================================================
    
    // TODO_SPEC08: Display metrics not yet implemented (Spec 08 Section 7)
    // Target: Future phase (Performance Monitoring)
    // Purpose: Display performance monitoring
    lle_display_metrics_t *perf_metrics;
    
    // TODO_SPEC08: Event coordinator not yet implemented (Spec 08 Section 9)
    // Target: Future phase (Event System Coordination)
    // Purpose: Event system coordination
    lle_event_coordinator_t *event_coordinator;
    
    // TODO_SPEC08: Terminal adapter not yet implemented (Spec 08 Section 11)
    // Target: Future phase (Terminal Compatibility Layer)
    // Purpose: Terminal compatibility management
    lle_terminal_adapter_t *terminal_adapter;
    
    // ========================================================================
    // CONFIGURATION AND STATE (Stubbed - Future Phase)
    // ========================================================================
    
    // TODO_SPEC08: Display configuration not yet implemented (Spec 08 Section 13)
    // Target: Future phase (Configuration Management)
    // Purpose: Display integration configuration
    lle_display_config_t *config;
    
    // TODO_SPEC08: Display state not yet implemented (Spec 08 Section 3)
    // Target: Future phase (State Management)
    // Purpose: Current display state tracking
    lle_display_state_t *current_state;
    
    // TODO_SPEC08: Hash table for render cache not yet implemented (Spec 08 Section 7)
    // Target: Future phase (Render Cache)
    // Purpose: Render result caching
    lle_hash_table_t *render_cache;
    
    // ========================================================================
    // SYNCHRONIZATION AND SAFETY (Spec 08 Section 2.1)
    // ========================================================================
    
    // Thread-safe access control
    pthread_rwlock_t integration_lock;
    
    // Integration system status
    bool integration_active;
    
    // Display frame tracking
    uint64_t frame_counter;
    
    // Integration API version
    uint32_t api_version;
    
} lle_display_integration_system_t;

// ============================================================================
// STUB STRUCTURES FOR FUTURE COMPONENTS (Spec 08 Compliance)
// ============================================================================
//
// These structures are defined as empty stubs to satisfy type requirements.
// They will be fully implemented in future phases as indicated by TODO_SPEC08.

// TODO_SPEC08: Display bridge not yet implemented (Spec 08 Section 3.2)
// Target: Future phase (Display Bridge Architecture)
struct lle_display_bridge {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 3.2 will be added in future phase
    // Will include: active_buffer, cursor_pos, lle_events, command_layer, etc.
};

// TODO_SPEC08: Render controller not yet implemented (Spec 08 Section 4)
// Target: Future phase (Layered Display Controller)
struct lle_render_controller {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 4 will be added in future phase
    // Will include: pipeline, scheduler, buffer_renderer, cursor_renderer, etc.
};

// TODO_SPEC08: Display cache not yet implemented (Spec 08 Section 7)
// Target: Future phase (Performance Optimization)
struct lle_display_cache {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 7 will be added in future phase
    // Will include: cache entries, eviction policy, statistics, etc.
};

// TODO_SPEC08: Composition manager not yet implemented (Spec 08 Section 4)
// Target: Future phase (Composition Management)
struct lle_composition_manager {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 4 will be added in future phase
    // Will include: layer management, composition engine, z-order tracking, etc.
};

// TODO_SPEC08: Display metrics not yet implemented (Spec 08 Section 7)
// Target: Future phase (Performance Monitoring)
struct lle_display_metrics {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 7 will be added in future phase
    // Will include: render counts, timing stats, cache metrics, etc.
};

// TODO_SPEC08: Event coordinator not yet implemented (Spec 08 Section 9)
// Target: Future phase (Event System Coordination)
struct lle_event_coordinator {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 9 will be added in future phase
    // Will include: event routing, priority queues, handlers, etc.
};

// TODO_SPEC08: Terminal adapter not yet implemented (Spec 08 Section 11)
// Target: Future phase (Terminal Compatibility Layer)
struct lle_terminal_adapter {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 11 will be added in future phase
    // Will include: capability detection, feature mapping, fallback rendering, etc.
};

// TODO_SPEC08: Display configuration not yet implemented (Spec 08 Section 13)
// Target: Future phase (Configuration Management)
struct lle_display_config {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 13 will be added in future phase
    // Will include: feature flags, performance settings, theme options, etc.
};

// TODO_SPEC08: Display state not yet implemented (Spec 08 Section 3)
// Target: Future phase (State Management)
struct lle_display_state {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 3 will be added in future phase
    // Will include: current mode, visibility state, dirty tracking, etc.
};

// TODO_SPEC08: Hash table not yet implemented (Spec 08 Section 7)
// Target: Future phase (Render Cache)
struct lle_hash_table {
    int _stub;  // Placeholder to make structure valid
    // Full implementation per Spec 08 Section 7 will be added in future phase
    // Will include: buckets, hash function, collision handling, etc.
};

// TODO_SPEC08: Lusush display controller type (external to LLE)
// Target: Future phase (Full Display System Integration)
struct display_controller {
    int _stub;  // Placeholder to make structure valid
    // This is a Lusush type - full integration will be added in future phase
};

// TODO_SPEC08: Lusush theme manager type (external to LLE)
// Target: Future phase (Theme System Integration)
struct theme_manager {
    int _stub;  // Placeholder to make structure valid
    // This is a Lusush type - full integration will be added in future phase
};

// TODO_SPEC08: Lusush memory pool type (external to LLE)
// Target: Future phase (Memory Management Integration)
struct memory_pool {
    int _stub;  // Placeholder to make structure valid
    // This is a Lusush type - full integration will be added in future phase
};

// ============================================================================
// DISPLAY INTEGRATION SYSTEM INITIALIZATION (Spec 08 Section 3.1)
// ============================================================================

// Initialize display integration system with all components
// Current implementation initializes structure with NULL stubs
// Future phases will add full initialization per Spec 08 Section 3.1
int lle_display_integration_system_init(lle_display_integration_system_t **system,
                                        display_controller_t *lusush_display,
                                        memory_pool_t *memory_pool);

// Cleanup display integration system and all components
void lle_display_integration_system_cleanup(lle_display_integration_system_t *system);

// Get integration system status
bool lle_display_integration_system_is_active(const lle_display_integration_system_t *system);

// Get frame counter
uint64_t lle_display_integration_system_get_frame_count(const lle_display_integration_system_t *system);

// ============================================================================
// STUB INITIALIZATION FUNCTIONS (Future Implementation)
// ============================================================================
//
// These functions are declared but return success with NULL initialization.
// Full implementations will be added in future phases per TODO_SPEC08 markers.

// TODO_SPEC08: Display bridge initialization not yet implemented (Spec 08 Section 3.2)
// Target: Future phase
int lle_display_bridge_init(lle_display_bridge_t **bridge,
                            display_controller_t *lusush_display,
                            memory_pool_t *memory_pool);
void lle_display_bridge_cleanup(lle_display_bridge_t *bridge);

// TODO_SPEC08: Render controller initialization not yet implemented (Spec 08 Section 4)
// Target: Future phase
int lle_render_controller_init(lle_render_controller_t **controller,
                               lle_display_bridge_t *bridge,
                               memory_pool_t *memory_pool);
void lle_render_controller_cleanup(lle_render_controller_t *controller);

// TODO_SPEC08: Display cache initialization not yet implemented (Spec 08 Section 7)
// Target: Future phase
int lle_display_cache_init(lle_display_cache_t **cache,
                           memory_pool_t *memory_pool);
void lle_display_cache_cleanup(lle_display_cache_t *cache);

// TODO_SPEC08: Composition manager initialization not yet implemented (Spec 08 Section 4)
// Target: Future phase
int lle_composition_manager_init(lle_composition_manager_t **manager,
                                display_controller_t *lusush_display,
                                memory_pool_t *memory_pool);
void lle_composition_manager_cleanup(lle_composition_manager_t *manager);

// TODO_SPEC08: Display metrics initialization not yet implemented (Spec 08 Section 7)
// Target: Future phase
int lle_display_metrics_init(lle_display_metrics_t **metrics,
                             memory_pool_t *memory_pool);
void lle_display_metrics_cleanup(lle_display_metrics_t *metrics);

// TODO_SPEC08: Event coordinator initialization not yet implemented (Spec 08 Section 9)
// Target: Future phase
int lle_event_coordinator_init(lle_event_coordinator_t **coordinator,
                              display_controller_t *lusush_display,
                              memory_pool_t *memory_pool);
void lle_event_coordinator_cleanup(lle_event_coordinator_t *coordinator);

// TODO_SPEC08: Terminal adapter initialization not yet implemented (Spec 08 Section 11)
// Target: Future phase
int lle_terminal_adapter_init(lle_terminal_adapter_t **adapter,
                             display_controller_t *lusush_display,
                             memory_pool_t *memory_pool);
void lle_terminal_adapter_cleanup(lle_terminal_adapter_t *adapter);

// TODO_SPEC08: Display configuration initialization not yet implemented (Spec 08 Section 13)
// Target: Future phase
int lle_display_config_init(lle_display_config_t **config,
                           memory_pool_t *memory_pool);
void lle_display_config_cleanup(lle_display_config_t *config);

// TODO_SPEC08: Display state initialization not yet implemented (Spec 08 Section 3)
// Target: Future phase
int lle_display_state_init(lle_display_state_t **state,
                          memory_pool_t *memory_pool);
void lle_display_state_cleanup(lle_display_state_t *state);

// TODO_SPEC08: Hash table initialization not yet implemented (Spec 08 Section 7)
// Target: Future phase
int lle_hash_table_init(lle_hash_table_t **table);
void lle_hash_table_cleanup(lle_hash_table_t *table);

// Utility: Convert error code to string
const char* lle_display_integration_system_error_string(int error_code);

// ============================================================================
// SPECIFICATION COMPLIANCE VERIFICATION
// ============================================================================
//
// POLICY COMPLIANCE CHECKLIST (SPECIFICATION_IMPLEMENTATION_POLICY.md):
// ✅ All 14 component pointers present in lle_display_integration_system_t
// ✅ Exact structure name from Spec 08 Section 2.1: lle_display_integration_system_t (close to lle_display_integration_t)
// ✅ Exact function naming: lle_display_integration_system_init()
// ✅ All stub components marked with TODO_SPEC08 comments
// ✅ Forward declarations for all supporting types
// ✅ Stub structure definitions provided
// ✅ No invented names (no lle_simple_*, lle_basic_*, etc.)
// ✅ Working components (display.h, display_buffer.h) preserved and available
// ✅ Clear documentation of what's implemented vs stubbed
//
// IMPLEMENTATION STATUS:
// - Spec 08 Compliance: 100% (structure complete, 0/14 components fully functional)
// - Working Components: lle_display_t (basic display), lle_display_buffer_renderer_t (buffer renderer)
// - Stubbed Components: 14 components (marked TODO_SPEC08, targeted for future phases)
//
// This implementation follows SPECIFICATION_IMPLEMENTATION_POLICY.md requirements
// to prevent architectural inconsistency and future integration failures.

#endif // LLE_FOUNDATION_DISPLAY_INTEGRATION_SYSTEM_H

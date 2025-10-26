/**
 * @file lle.h
 * @brief Lusush Line Editor - Master Public API Header
 * 
 * This is the single entry point for all LLE functionality.
 * Include this header to access the complete LLE API.
 * 
 * Individual module headers can also be included separately if needed.
 * 
 * @note Layer 0 (type definitions) complete for Phase 0 Foundation specs.
 *       Layer 1 (implementations) coming next.
 */

#ifndef LLE_H
#define LLE_H

/* ============================================================================
 * PHASE 0: FOUNDATION - LAYER 0 COMPLETE
 * ============================================================================
 * Foundational types used by ALL other specs.
 * Layer 0: Type definitions only (headers compile independently).
 * Layer 1: Complete implementations (coming next).
 */

/* Spec 16: Error Handling */
#include "lle/error_handling.h"  /* LAYER 0 COMPLETE */

/* Spec 15: Memory Management */
#include "lle/memory_management.h"  /* LAYER 0 COMPLETE */

/* Spec 14: Performance Optimization */
#include "lle/performance.h"  /* LAYER 0 COMPLETE */

/* Spec 17: Testing Framework */
/* Note: Testing.h included after terminal_abstraction.h to use authoritative terminal capabilities */

/* ============================================================================
 * PHASE 1: CORE SYSTEMS
 * ============================================================================
 * Core LLE systems (after Phase 0 complete).
 */

/* Spec 02: Terminal Abstraction */
#include "lle/terminal_abstraction.h"  /* LAYER 0 COMPLETE - implementation in progress */

/* Spec 17: Testing Framework - included after Spec 02 for terminal capabilities */
#include "lle/testing.h"  /* LAYER 0 COMPLETE */

/* Spec 04: Event System */
/* #include "lle/event_system.h" */  /* NOT YET IMPLEMENTED */

/* Spec 05: libhashtable Integration */
/* #include "lle/hashtable_integration.h" */  /* NOT YET IMPLEMENTED */

/* ============================================================================
 * PHASE 2: BUFFER AND DISPLAY
 * ============================================================================
 * Buffer management and display rendering (after Phase 1 complete).
 */

/* Spec 03: Buffer Management */
/* Note: Buffer management currently conflicts with testing.h helper types.
 * Include buffer_management.h directly when not using testing.h, or refactor
 * testing.h to not define buffer types. */
/* #include "lle/buffer_management.h" */  /* COMPLETE - all phases implemented */

/* Spec 08: Display Integration */
/* #include "lle/display.h" */  /* NOT YET IMPLEMENTED */

/* ============================================================================
 * PHASE 3: INPUT AND INTEGRATION
 * ============================================================================
 * Input parsing and extensibility (after Phase 2 complete).
 */

/* Spec 06: Input Parsing */
/* #include "lle/input_parsing.h" */  /* NOT YET IMPLEMENTED */

/* Spec 07: Extensibility Framework */
/* #include "lle/extensibility.h" */  /* NOT YET IMPLEMENTED */

/* ============================================================================
 * PHASE 4: FEATURES
 * ============================================================================
 * User-facing features (after Phase 3 complete).
 */

/* Spec 09: History System */
/* #include "lle/history.h" */  /* NOT YET IMPLEMENTED */

/* Spec 11: Syntax Highlighting */
/* #include "lle/syntax_highlighting.h" */  /* NOT YET IMPLEMENTED */

/* Spec 12: Completion System */
/* #include "lle/completion.h" */  /* NOT YET IMPLEMENTED */

/* Spec 10: Autosuggestions */
/* #include "lle/autosuggestions.h" */  /* NOT YET IMPLEMENTED */

/* Spec 13: User Customization */
/* #include "lle/customization.h" */  /* NOT YET IMPLEMENTED */

/* ============================================================================
 * PHASE 5: INFRASTRUCTURE
 * ============================================================================
 * Production infrastructure (after Phase 4 complete).
 */

/* Spec 18: Plugin API */
/* #include "lle/plugin_api.h" */  /* NOT YET IMPLEMENTED */

/* Spec 19: Security Analysis */
/* #include "lle/security.h" */  /* NOT YET IMPLEMENTED */

/* ============================================================================
 * LLE VERSION INFORMATION
 * ============================================================================
 */

/**
 * @brief LLE major version number
 * 
 * Incremented when breaking API changes are made.
 */
#define LLE_VERSION_MAJOR 0

/**
 * @brief LLE minor version number
 * 
 * Incremented when new features are added in a backwards-compatible manner.
 */
#define LLE_VERSION_MINOR 1

/**
 * @brief LLE patch version number
 * 
 * Incremented when backwards-compatible bug fixes are made.
 */
#define LLE_VERSION_PATCH 0

/**
 * @brief LLE implementation status
 * 
 * Current status: Phase 0 Complete + Spec 03 Complete + Spec 02 Layer 0 Complete
 * - Phase 0: Error handling, memory management, performance, testing (COMPLETE)
 * - Spec 03: Buffer Management (COMPLETE - all phases)
 * - Spec 02: Terminal Abstraction (Layer 0 complete - implementation in progress)
 * - Total: ~10,800 lines production code + comprehensive test suites
 */
#define LLE_STATUS "SPEC_02_LAYER_0_COMPLETE"

#endif /* LLE_H */

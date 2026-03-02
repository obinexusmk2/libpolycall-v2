// odts_integration.h
// OBINexus Computational Cognition Gating + ODTS Integration
// Implements X/Y/Z axis gating methodology with derivative tracing
#ifndef ODTS_INTEGRATION_H
#define ODTS_INTEGRATION_H

#include "agha_dozie.h"
#include "odts_core.h"

/**
 * X-Axis Gate States: Workflow Management
 * todo → doing → done
 */
typedef enum {
    WORKFLOW_TODO,      // Gate Open: Requirements defined, ready to start
    WORKFLOW_DOING,     // Gate Active: Development in progress
    WORKFLOW_DONE       // Gate Closed: All tests pass, coverage >= 95%
} XAxisGateState;

/**
 * Y-Axis Gate States: Validation Management
 * open → validate → close
 */
typedef enum {
    VALIDATION_OPEN,        // Gate Open: Ready for integration testing
    VALIDATION_VALIDATE,    // Gate Active: Integration tests + policy checks
    VALIDATION_CLOSE        // Gate Closed: All validations passed
} YAxisGateState;

/**
 * Z-Axis Gate States: Deployment Management  
 * stage → deploy → monitor
 */
typedef enum {
    DEPLOYMENT_STAGE,      // Gate Open: Staging environment
    DEPLOYMENT_DEPLOY,     // Gate Active: Production deployment
    DEPLOYMENT_MONITOR     // Gate Closed: Post-deployment monitoring
} ZAxisGateState;

/**
 * Pattern Structure (generalized from Polygon)
 * Can represent polygons, state machines, or any gatable entity
 */
typedef struct {
    void* data;                     // Generic pattern data
    size_t feature_count;           // Number of features (edges, states, etc.)
    XAxisGateState workflow_state;  // Current X-gate position
    YAxisGateState validation_state; // Current Y-gate position
    ZAxisGateState deployment_state; // Current Z-gate position
    double singpashes_cost;         // Cost metric for policy validation
    bool hacc_decorators_present;   // HACC decorator requirement
} Pattern;

/**
 * Gating Context: Multi-pattern analysis state
 * Replaces GatogiContext with proper terminology
 */
typedef struct {
    Pattern* patterns;                  // Array of patterns to analyze
    size_t count;                       // Number of patterns
    ODTSDerivativeTrace* derivative_traces;  // ODTS trace results
    PatternCoherence coherence_state;   // Agha-Dozie coherence result
    FaultTolerance fault_model;         // Fault tolerance state
    XAxisGateState x_gate_state;        // Global X-gate state
    YAxisGateState y_gate_state;        // Global Y-gate state
    ZAxisGateState z_gate_state;        // Global Z-gate state
} GatingContext;

/**
 * Gating Results: Overall analysis outcome
 * Replaces GatogiResult with proper terminology
 */
typedef enum {
    GATING_VALID,                   // All gates passed, patterns coherent
    GATING_DERIVATIVE_TERMINATED,   // ODTS verification successful
    GATING_PATTERN_UNCERTAIN,       // Agha-Dozie: insufficient data
    GATING_FAULT_DETECTED          // Mathematical or policy fault
} GatingResult;

/**
 * Policy Validation Structure
 * Enforces OBINexus constitutional compliance
 */
typedef struct {
    bool cost_check;            // singpashes_cost <= 0.55
    bool decorator_check;       // #sorrynotsorry, #hacc, #noghosting present
    bool trace_chain_valid;     // UUID trace integrity verified
    bool all_checks_passed;     // All requirements met
} PolicyValidation;

/**
 * Core Gating Functions
 */

// Main gating analysis function
GatingResult gating_analyze_patterns(GatingContext* ctx);

// ODTS derivative tracing
ODTSResult odts_trace_pattern_derivatives(Pattern* pattern);

// Axis gate transitions
XAxisGateState x_gate_transition(Pattern* pattern);
YAxisGateState y_gate_validation(Pattern* pattern);
ZAxisGateState z_gate_deployment(Pattern* pattern);

// Cross-verification with all gates
GatingResult gating_cross_verify(
    GatingContext* ctx,
    ODTSResult* odts_results,
    PatternCoherence pattern_coherence
);

// Helper functions for state name conversion
const char* x_gate_state_name(XAxisGateState state);
const char* y_gate_state_name(YAxisGateState state);
const char* z_gate_state_name(ZAxisGateState state);

#endif // ODTS_INTEGRATION_H

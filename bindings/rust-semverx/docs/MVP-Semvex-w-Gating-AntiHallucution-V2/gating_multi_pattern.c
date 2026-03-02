// examples/gating_multi_pattern.c
// OBINexus Computational Cognition Gating Example
// Demonstrates X/Y/Z axis gating for multi-pattern analysis
#include "odts_integration.h"

/**
 * Example: Multi-Pattern Analysis Using OBINexus Gating
 * 
 * Demonstrates computational cognition gates:
 * - X-Axis: Workflow progression (todo → doing → done)
 * - Y-Axis: Validation gates (open → validate → close)  
 * - Z-Axis: Deployment pipeline (stage → deploy → monitor)
 */
void example_gating_multi_pattern() {
    printf("=== OBINexus Gating: Multi-Pattern Analysis ===\n");
    
    // Create test patterns (can be polygons, state machines, etc.)
    Pattern patterns[3] = {
        create_regular_polygon(6, 10.0),  // Hexagon
        create_regular_polygon(4, 7.5),   // Square  
        create_regular_polygon(8, 12.0)   // Octagon
    };
    
    GatingContext ctx = {
        .patterns = patterns,
        .count = 3,
        .coherence_state = PATTERN_UNKNOWN,
        .x_gate_state = WORKFLOW_TODO,
        .y_gate_state = VALIDATION_OPEN,
        .z_gate_state = DEPLOYMENT_STAGE
    };
    
    printf("\n--- Phase 1: X-Axis Gate (Workflow Management) ---\n");
    for (size_t i = 0; i < ctx.count; i++) {
        XAxisGateState x_state = x_gate_transition(&patterns[i]);
        printf("Pattern %zu: X-Gate = %s\n", i, x_gate_state_name(x_state));
    }
    
    printf("\n--- Phase 2: Y-Axis Gate (Validation) ---\n");
    for (size_t i = 0; i < ctx.count; i++) {
        YAxisGateState y_state = y_gate_validation(&patterns[i]);
        printf("Pattern %zu: Y-Gate = %s\n", i, y_gate_state_name(y_state));
    }
    
    printf("\n--- Phase 3: Run Gating Analysis ---\n");
    GatingResult result = gating_analyze_patterns(&ctx);
    
    // ODTS Audit Trail
    printf("\n--- ODTS Derivative Traces ---\n");
    for (size_t i = 0; i < ctx.count; i++) {
        printf("Pattern %zu: Termination at order %d\n", 
               i, ctx.derivative_traces[i].termination_order);
        
        // Agha-Dozie: Verify pattern coherence
        if (ctx.derivative_traces[i].fault_state == ODTS_CLEAN) {
            printf("  ✓ Mathematical soundness verified\n");
        }
    }
    
    // Results with ethical uncertainty (Agha-Dozie principle)
    printf("\n--- Gating Results ---\n");
    switch (result) {
        case GATING_VALID:
            printf("✓ Gating analysis: All patterns coherent\n");
            printf("  X-Gate: DONE | Y-Gate: CLOSE | Z-Gate: DEPLOY\n");
            break;
            
        case GATING_DERIVATIVE_TERMINATED:
            printf("✓ Gating analysis: Derivatives properly terminated\n");
            printf("  ODTS verification: PASSED\n");
            break;
            
        case GATING_PATTERN_UNCERTAIN:
            printf("⚠ Gating analysis: Pattern uncertainty detected\n");
            printf("  Manual review recommended per Agha-Dozie principles\n");
            printf("  Current State: X=%s | Y=%s | Z=%s\n",
                   x_gate_state_name(ctx.x_gate_state),
                   y_gate_state_name(ctx.y_gate_state),
                   z_gate_state_name(ctx.z_gate_state));
            break;
            
        case GATING_FAULT_DETECTED:
            printf("✗ Gating analysis: Mathematical fault detected\n");
            printf("  Fault: %s\n", ctx.fault_model.error_message);
            printf("  Fault Tolerance State: ");
            switch (ctx.fault_model.state) {
                case FAULT_ERROR:
                    printf("ERROR (Recoverable)\n");
                    break;
                case FAULT_EXCEPTION:
                    printf("EXCEPTION (Handled)\n");
                    break;
                case FAULT_PANIC:
                    printf("PANIC (System Reset Required)\n");
                    break;
            }
            break;
    }
    
    // Z-Axis Gate Deployment Status
    if (result == GATING_VALID) {
        printf("\n--- Z-Axis Gate (Deployment Pipeline) ---\n");
        for (size_t i = 0; i < ctx.count; i++) {
            ZAxisGateState z_state = z_gate_deployment(&patterns[i]);
            printf("Pattern %zu: Z-Gate = %s\n", i, z_gate_state_name(z_state));
        }
    }
    
    printf("\n=== OBINexus Gating Analysis Complete ===\n");
}

/**
 * Helper: X-Gate State Name
 */
const char* x_gate_state_name(XAxisGateState state) {
    switch (state) {
        case WORKFLOW_TODO: return "TODO (Open Gate)";
        case WORKFLOW_DOING: return "DOING (Active)";
        case WORKFLOW_DONE: return "DONE (Closed Gate)";
        default: return "UNKNOWN";
    }
}

/**
 * Helper: Y-Gate State Name
 */
const char* y_gate_state_name(YAxisGateState state) {
    switch (state) {
        case VALIDATION_OPEN: return "OPEN (Awaiting Validation)";
        case VALIDATION_VALIDATE: return "VALIDATE (In Progress)";
        case VALIDATION_CLOSE: return "CLOSE (Validation Complete)";
        default: return "UNKNOWN";
    }
}

/**
 * Helper: Z-Gate State Name
 */
const char* z_gate_state_name(ZAxisGateState state) {
    switch (state) {
        case DEPLOYMENT_STAGE: return "STAGE (Pre-Deployment)";
        case DEPLOYMENT_DEPLOY: return "DEPLOY (Active Deployment)";
        case DEPLOYMENT_MONITOR: return "MONITOR (Post-Deployment)";
        default: return "UNKNOWN";
    }
}

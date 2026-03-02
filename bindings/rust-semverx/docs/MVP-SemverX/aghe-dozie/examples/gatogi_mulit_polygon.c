// examples/gatogi_multi_polygon.c
#include "odts_integration.h"

void example_gatogi_multi_polygon() {
    printf("=== Gatogi Algorithm: Multi-Polygon Analysis ===\n");
    
    // Create test polygons
    Polygon polygons[3] = {
        create_regular_polygon(6, 10.0),  // Hexagon
        create_regular_polygon(4, 7.5),   // Square  
        create_regular_polygon(8, 12.0)   // Octagon
    };
    
    GatogiContext ctx = {
        .polygons = polygons,
        .count = 3,
        .coherence_state = PATTERN_UNKNOWN
    };
    
    // Run Gatogi analysis
    GatogiResult result = gatogi_analyze_polygons(&ctx);
    
    // ODTS Audit Trail
    printf("ODTS Derivative Traces:\n");
    for (size_t i = 0; i < ctx.count; i++) {
        printf("Polygon %zu: Termination at order %d\n", 
               i, ctx.derivative_traces[i].termination_order);
        
        // Agha-Dozie: Verify pattern coherence
        if (ctx.derivative_traces[i].fault_state == ODTS_CLEAN) {
            printf("  ✓ Mathematical soundness verified\n");
        }
    }
    
    // Results with ethical uncertainty
    switch (result) {
        case GATOGI_VALID:
            printf("✓ Gatogi analysis: All polygons coherent\n");
            break;
        case GATOGI_DERIVATIVE_TERMINATED:
            printf("✓ Gatogi analysis: Derivatives properly terminated\n");
            break;
        case GATOGI_PATTERN_UNCERTAIN:
            printf("⚠ Gatogi analysis: Pattern uncertainty - manual review recommended\n");
            break;
        case GATOGI_FAULT_DETECTED:
            printf("✗ Gatogi analysis: Mathematical fault detected\n");
            printf("  Fault: %s\n", ctx.fault_model.error_message);
            break;
    }
}

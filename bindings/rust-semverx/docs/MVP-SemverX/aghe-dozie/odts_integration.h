// odts_integration.h
#include "agha_dozie.h"
#include "odts_core.h"

typedef struct {
    Polygon* polygons;
    size_t count;
    ODTSDerivativeTrace* derivative_traces;
    PatternCoherence coherence_state;
    FaultTolerance fault_model;
} GatogiContext;

typedef enum {
    GATOGI_VALID,
    GATOGI_DERIVATIVE_TERMINATED,
    GATOGI_PATTERN_UNCERTAIN,
    GATOGI_FAULT_DETECTED
} GatogiResult;

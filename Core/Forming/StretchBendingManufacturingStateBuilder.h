#pragma once

#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Forming/StretchBendingManufacturingState.h"

// =====================================================
// STRETCH-BENDING MANUFACTURING-STATE BUILDER
//
// Creates the initial fixed-zone manufacturing state.
//
// Phase 10J does not calculate time-dependent playback.
// It only prepares a valid Ready state.
// =====================================================

class StretchBendingManufacturingStateBuilder
{
public:
    static StretchBendingManufacturingState buildReadyState(
        const StretchBendingProcessInput& input,
        const StretchBendingEvaluationResult& evaluation,
        const StretchBendingActiveZone& activeZone
    );
};

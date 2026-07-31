#pragma once

#include "StretchBendingManufacturingState.h"
#include "StretchBendingEvaluationResult.h"
#include "StretchBendingCurrentProfileParameters.h"

// =====================================================
// STRETCH-BENDING CURRENT PROFILE PARAMETER RESOLVER
//
// Converts:
//
//     manufacturing process state
//     +
//     evaluated loaded/final process values
//
// into:
//
//     instantaneous curvature and torsion
//
// This class does not construct a complete arc-length
// profile yet. That will be Phase 10L.
// =====================================================

class StretchBendingCurrentProfileParameterResolver
{
public:
    static StretchBendingCurrentProfileParameters resolve(
        const StretchBendingManufacturingState& state,
        const StretchBendingEvaluationResult& evaluation
    );

private:
    static double interpolate(
        double startValue,
        double endValue,
        double fraction
    );

    static double clamp01(
        double value
    );
};

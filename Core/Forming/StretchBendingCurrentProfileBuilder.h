#pragma once

#include "Core/Geometry/CurvatureTorsionProfile.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Forming/StretchBendingManufacturingState.h"

// =====================================================
// STRETCH-BENDING CURRENT PROFILE BUILDER
//
// Converts the current manufacturing state into a
// spatial curvature/torsion profile.
//
// This class does not:
// - advance manufacturing time,
// - evaluate material feasibility,
// - integrate geometry,
// - modify the manufacturing state.
//
// It only determines where the instantaneous curvature
// and torsion apply along the pipe.
//
// Spatial layout:
//
//     0             activeStart
//     |------------------|
//          straight
//
//     activeStart       activeEnd
//     |====================|
//          active bending
//
//     activeEnd          totalLength
//     |-----------------------|
//          straight
// =====================================================

class StretchBendingCurrentProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        const StretchBendingManufacturingState& state,
        const StretchBendingEvaluationResult& evaluation
    );
};
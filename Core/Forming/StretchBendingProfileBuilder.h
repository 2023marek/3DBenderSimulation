#pragma once

#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"

// =====================================================
// STRETCH-BENDING PROFILE BUILDER
//
// Converts an accepted stretch-bending input/evaluation
// into a process-independent curvature/torsion profile.
//
// The builder:
//
//     does not generate PipeNodes
//     does not modify manufacturing state
//     does not perform material evaluation
//     does not render anything
//
// The first implementation produces a constant profile:
//
//     kappa(s) = targetCurvature
//     tau(s)   = targetTorsion
// =====================================================

class StretchBendingProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        const StretchBendingProcessInput& input,
        const StretchBendingEvaluationResult& evaluation
    );
};

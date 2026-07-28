#pragma once

#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"

// =====================================================
// FINAL UNLOADED STRETCH-BENDING PROFILE
//
// Creates the expected centerline profile after unloading.
//
// Loaded profile:
//     uses loadedCurvatureCommand
//
// Final profile:
//     uses predictedFinalCurvature
// =====================================================

class StretchBendingFinalProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        const StretchBendingProcessInput& input,
        const StretchBendingEvaluationResult& evaluation
    );
};
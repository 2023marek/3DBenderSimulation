#pragma once

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"

// =====================================================
// SPATIAL CURVE INTEGRATOR
//
// Process-independent geometry engine.
//
// Input:
//     start frame
//     curvature/torsion profile ?(s), ?(s)
//     sampling distance
//
// Output:
//     generated PipeNodes
//     cumulative arc lengths
//     final frame
//     integration diagnostics
//
// This class does not own manufacturing state.
// It does not consume stock.
// It does not update active zones.
// It only integrates spatial centerline geometry.
// =====================================================

#pragma once

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"

class SpatialCurveIntegrator
{
public:
    SpatialCurveIntegrationResult integrate(
        const Frame& startFrame,
        const CurvatureTorsionProfile& profile,
        double sampleStep
    ) const;

private:
    bool isValidFrame(
        const Frame& frame
    ) const;

    bool isValidProfile(
        const CurvatureTorsionProfile& profile
    ) const;

    void orthonormalizeFrame(
        Frame& frame
    ) const;

    PipeNode makeNodeFromFrame(
        const Frame& frame
    ) const;

    bool sampleProfileAtArcLength(
        const CurvatureTorsionProfile& profile,
        double arcLength,
        double& outCurvature,
        double& outTorsion
    ) const;

    bool isFiniteFrame(
        const Frame& frame
    ) const;
};
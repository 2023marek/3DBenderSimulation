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
bool sampleProfileForDebug(
        const CurvatureTorsionProfile& profile,
        double arcLength,
        double& outCurvature,
        double& outTorsion
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
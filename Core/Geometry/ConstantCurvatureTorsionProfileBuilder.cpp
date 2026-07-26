#include "Core/Geometry/ConstantCurvatureTorsionProfileBuilder.h"

#include <cmath>

CurvatureTorsionProfile
ConstantCurvatureTorsionProfileBuilder::build(
    double totalArcLength,
    double curvature,
    double torsion
)
{
    CurvatureTorsionProfile profile;

    // =====================================================
    // VALIDATION
    // =====================================================

    if (totalArcLength <= 0.0)
        return profile;

    if (!std::isfinite(totalArcLength)
        || !std::isfinite(curvature)
        || !std::isfinite(torsion))
    {
        return profile;
    }

    // =====================================================
    // CONSTANT PROFILE
    //
    // Two samples are sufficient because the integrator
    // linearly interpolates between samples.
    //
    // Both samples carry identical ? and ? values.
    // =====================================================

    CurvatureTorsionSample startSample;

    startSample.arcLength =
        0.0;

    startSample.curvature =
        curvature;

    startSample.torsion =
        torsion;

    CurvatureTorsionSample endSample;

    endSample.arcLength =
        totalArcLength;

    endSample.curvature =
        curvature;

    endSample.torsion =
        torsion;

    profile.samples.push_back(
        startSample
    );

    profile.samples.push_back(
        endSample
    );

    profile.totalArcLength =
        totalArcLength;

    profile.valid =
        true;

    return profile;
}
#include "Core/Forming/StretchBendingProfileBuilder.h"

#include <cmath>

CurvatureTorsionProfile
StretchBendingProfileBuilder::build(
    const StretchBendingProcessInput& input,
    const StretchBendingEvaluationResult& evaluation
)
{
    CurvatureTorsionProfile profile;

    // =====================================================
    // ACCEPTED PROCESS ONLY
    //
    // Do not generate a geometric command for an invalid,
    // unsafe, disabled, or below-yield process.
    // =====================================================

    if (!input.isValid())
        return profile;

    if (!evaluation.valid)
        return profile;

    if (evaluation.status
        != StretchBendingEvaluationStatus::Valid)
    {
        return profile;
    }

    if (!evaluation.inputValid)
        return profile;

    if (!evaluation.geometryFeasible)
        return profile;

    if (!evaluation.innerWallSafe)
        return profile;

    if (!evaluation.outerWallSafe)
        return profile;

    if (!evaluation.aboveYield)
        return profile;

    // =====================================================
    // NUMERICAL VALIDATION
    // =====================================================

    const double totalArcLength =
        input.geometry.targetArcLength;

    const double curvature =
        evaluation.loadedCurvatureCommand;

    const double torsion =
        input.geometry.targetTorsion;

    if (!std::isfinite(totalArcLength)
        || !std::isfinite(curvature)
        || !std::isfinite(torsion))
    {
        return profile;
    }

    if (totalArcLength <= 0.0)
        return profile;

    if (curvature < 0.0)
        return profile;

    // =====================================================
    // CONSTANT STRETCH-BENDING PROFILE
    //
    // Two samples are sufficient because the shared
    // integrator linearly interpolates kappa and tau.
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

    profile.samples.reserve(
        2
    );

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
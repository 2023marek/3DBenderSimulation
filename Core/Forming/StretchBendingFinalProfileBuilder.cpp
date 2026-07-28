#include "Core/Forming/StretchBendingFinalProfileBuilder.h"

#include <cmath>

CurvatureTorsionProfile
StretchBendingFinalProfileBuilder::build(
    const StretchBendingProcessInput& input,
    const StretchBendingEvaluationResult& evaluation
)
{
    CurvatureTorsionProfile profile;

    if (!input.isValid())
        return profile;

    if (!evaluation.valid)
        return profile;

    if (evaluation.status
        != StretchBendingEvaluationStatus::Valid)
    {
        return profile;
    }

    if (!evaluation.springbackPredictionValid)
        return profile;

    const double totalArcLength =
        input.geometry.targetArcLength;

    const double finalCurvature =
        evaluation.predictedFinalCurvature;

    const double finalTorsion =
        input.geometry.targetTorsion;

    if (!std::isfinite(totalArcLength)
        || !std::isfinite(finalCurvature)
        || !std::isfinite(finalTorsion))
    {
        return profile;
    }

    if (totalArcLength <= 0.0)
        return profile;

    if (finalCurvature < 0.0)
        return profile;

    CurvatureTorsionSample startSample;

    startSample.arcLength =
        0.0;

    startSample.curvature =
        finalCurvature;

    startSample.torsion =
        finalTorsion;

    CurvatureTorsionSample endSample;

    endSample.arcLength =
        totalArcLength;

    endSample.curvature =
        finalCurvature;

    endSample.torsion =
        finalTorsion;

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
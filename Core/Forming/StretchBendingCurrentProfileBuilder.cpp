#include "Core/Forming/StretchBendingCurrentProfileBuilder.h"

#include "Core/Forming/StretchBendingCurrentProfileParameterResolver.h"
#include <iostream>
#include <cmath>

CurvatureTorsionProfile
StretchBendingCurrentProfileBuilder::build(
    const StretchBendingManufacturingState& state,
    const StretchBendingEvaluationResult& evaluation)
{
    CurvatureTorsionProfile profile;

    // Always begin from a known empty result.
    profile.clear();

    // =================================================
    // 1. VALIDATE PROCESS STATE
    // =================================================

    if (!state.isValid())
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=InvalidState"
            << std::endl;

        return profile;
    }

    // =================================================
    // 2. VALIDATE EVALUATION RESULT
    // =================================================

    if (!evaluation.valid)
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=InvalidEvaluation"
            << std::endl;

        return profile;
    }

    if (!evaluation.springbackPredictionValid)
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=InvalidSpringbackPrediction"
            << std::endl;

        return profile;
    }

    // =================================================
    // 3. VALIDATE TOTAL PIPE LENGTH
    // =================================================

    const double totalLength =
        evaluation.targetArcLength;

    if (!std::isfinite(totalLength)
        || totalLength <= 0.0)
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=InvalidTotalLength"
            << " totalLength="
            << totalLength
            << std::endl;

        return profile;
    }

    // =================================================
    // 4. VALIDATE ACTIVE ZONE
    // =================================================

    if (!state.activeZone.isValidForLength(
        totalLength
    ))
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=InvalidActiveZone"
            << " startS="
            << state.activeZone.startS
            << " endS="
            << state.activeZone.endS
            << " totalLength="
            << totalLength
            << std::endl;

        return profile;
    }

    // =================================================
    // 5. RESOLVE INSTANTANEOUS MATERIAL PARAMETERS
    //
    // This gives one pair:
    //
    //     current curvature
    //     current torsion
    //
    // It does not yet determine their spatial location.
    // =================================================

    const StretchBendingCurrentProfileParameters
        currentParameters =
        StretchBendingCurrentProfileParameterResolver::
        resolve(
            state,
            evaluation
        );

    if (!currentParameters.isValid())
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=InvalidCurrentParameters"
            << " stage="
            << stretchBendingManufacturingStageToString(
                state.stage
            )
            << std::endl;

        return profile;
    }

  

    const double activeStart =
        state.activeZone.startS;

    const double activeEnd =
        state.activeZone.endS;

    const double currentCurvature =
        currentParameters.curvature;

    const double currentTorsion =
        currentParameters.torsion;

    // The profile construction is completed below using
    // the exact CurvatureTorsionProfile sample API used
    // =================================================
// 6. BUILD PIECEWISE-CONSTANT PROFILE
//
// Desired spatial distribution:
//
//     s=0       activeStart       activeEnd       totalLength
//      |------------|================|----------------|
//          ?=0          ?=current           ?=0
//          ?=0          ?=current           ?=0
//
// Duplicate boundary arc lengths express an immediate
// parameter change at activeStart and activeEnd.
//
// IMPORTANT:
// SpatialCurveIntegrator's profile-sampling logic must
// explicitly support duplicate arc lengths before this
// profile is integrated.
// =================================================

    CurvatureTorsionSample pipeStartSample;

    pipeStartSample.arcLength =
        0.0;

    pipeStartSample.curvature =
        0.0;

    pipeStartSample.torsion =
        0.0;


    // Straight value immediately before the active zone.
    CurvatureTorsionSample activeStartOutsideSample;

    activeStartOutsideSample.arcLength =
        activeStart;

    activeStartOutsideSample.curvature =
        0.0;

    activeStartOutsideSample.torsion =
        0.0;


    // Active value beginning at the same spatial boundary.
    CurvatureTorsionSample activeStartInsideSample;

    activeStartInsideSample.arcLength =
        activeStart;

    activeStartInsideSample.curvature =
        currentCurvature;

    activeStartInsideSample.torsion =
        currentTorsion;


    // Active value up to the end of the active zone.
    CurvatureTorsionSample activeEndInsideSample;

    activeEndInsideSample.arcLength =
        activeEnd;

    activeEndInsideSample.curvature =
        currentCurvature;

    activeEndInsideSample.torsion =
        currentTorsion;


    // Straight value beginning at the same end boundary.
    CurvatureTorsionSample activeEndOutsideSample;

    activeEndOutsideSample.arcLength =
        activeEnd;

    activeEndOutsideSample.curvature =
        0.0;

    activeEndOutsideSample.torsion =
        0.0;


    // Straight value at the end of the pipe.
    CurvatureTorsionSample pipeEndSample;

    pipeEndSample.arcLength =
        totalLength;

    pipeEndSample.curvature =
        0.0;

    pipeEndSample.torsion =
        0.0;


    // Preserve increasing spatial order.
    //
    // Duplicate positions occur only at the two deliberate
    // discontinuities:
    //     activeStart
    //     activeEnd
    profile.samples.push_back(
        pipeStartSample
    );

    profile.samples.push_back(
        activeStartOutsideSample
    );

    profile.samples.push_back(
        activeStartInsideSample
    );

    profile.samples.push_back(
        activeEndInsideSample
    );

    profile.samples.push_back(
        activeEndOutsideSample
    );

    profile.samples.push_back(
        pipeEndSample
    );
    // by the project.


    if (profile.samples.empty())
    {
        std::cout
            << "[STRETCH CURRENT PROFILE BUILDER REJECTION]"
            << " reason=NoSamplesGenerated"
            << std::endl;

        return profile;
    }

    // =================================================
    // 8. FINALIZE SUCCESSFUL PROFILE
    // =================================================

    profile.totalArcLength =
        totalLength;

    profile.valid =
        true;

    std::cout
        << "[STRETCH CURRENT PROFILE BUILDER SUCCESS]"
        << " samples="
        << profile.samples.size()
        << " totalArcLength="
        << profile.totalArcLength
        << " valid="
        << profile.valid
        << std::endl;

    return profile;
}
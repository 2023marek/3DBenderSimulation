#include "Core/Forming/StretchHelixCurrentProfileBuilder.h"

#include <cmath>

CurvatureTorsionProfile
StretchHelixCurrentProfileBuilder::build(
    const StretchHelixWrappingInput& input,
    const StretchHelixWrappingKinematics& kinematics,
    const StretchHelixWrappingState& state)
{
    CurvatureTorsionProfile profile;

    profile.clear();

    // =================================================
    // 1. VALIDATION
    // =================================================

    if (!input.isValid())
        return profile;

    if (!kinematics.valid)
        return profile;

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        return profile;
    }

    const double totalLength =
        input.pipeArcLength;

    const double frontS =
        state.contactFrontS;

    const double helixKappa =
        kinematics.curvature;

    const double helixTau =
        kinematics.torsion;

    // =================================================
    // 2. NO CONTACT YET
    //
    // Entire pipe remains straight.
    // =================================================

    if (frontS <= 1e-12)
    {
        CurvatureTorsionSample start;
        start.arcLength = 0.0;
        start.curvature = 0.0;
        start.torsion = 0.0;

        CurvatureTorsionSample end;
        end.arcLength = totalLength;
        end.curvature = 0.0;
        end.torsion = 0.0;

        profile.samples.push_back(
            start
        );

        profile.samples.push_back(
            end
        );

        profile.totalArcLength =
            totalLength;

        profile.valid =
            true;

        return profile;
    }

    // =================================================
    // 3. FULLY WRAPPED
    // =================================================

    if (frontS >= totalLength - 1e-12)
    {
        CurvatureTorsionSample start;
        start.arcLength = 0.0;
        start.curvature = helixKappa;
        start.torsion = helixTau;

        CurvatureTorsionSample end;
        end.arcLength = totalLength;
        end.curvature = helixKappa;
        end.torsion = helixTau;

        profile.samples.push_back(
            start
        );

        profile.samples.push_back(
            end
        );

        profile.totalArcLength =
            totalLength;

        profile.valid =
            true;

        return profile;
    }

    // =================================================
    // 4. PARTIALLY WRAPPED PIPE
    //
    // Duplicate samples at contactFrontS create the
    // sharp transition:
    //
    //     helix --> free straight pipe
    //
    // Your SpatialCurveIntegrator profile sampler has
    // already been tested for this duplicate-S behavior.
    // =================================================

    CurvatureTorsionSample wrappedStart;

    wrappedStart.arcLength =
        0.0;

    wrappedStart.curvature =
        helixKappa;

    wrappedStart.torsion =
        helixTau;


    CurvatureTorsionSample wrappedEnd;

    wrappedEnd.arcLength =
        frontS;

    wrappedEnd.curvature =
        helixKappa;

    wrappedEnd.torsion =
        helixTau;


    CurvatureTorsionSample freeStart;

    freeStart.arcLength =
        frontS;

    freeStart.curvature =
        0.0;

    freeStart.torsion =
        0.0;


    CurvatureTorsionSample freeEnd;

    freeEnd.arcLength =
        totalLength;

    freeEnd.curvature =
        0.0;

    freeEnd.torsion =
        0.0;


    profile.samples.push_back(
        wrappedStart
    );

    profile.samples.push_back(
        wrappedEnd
    );

    profile.samples.push_back(
        freeStart
    );

    profile.samples.push_back(
        freeEnd
    );

    profile.totalArcLength =
        totalLength;

    profile.valid =
        true;

    return profile;
}
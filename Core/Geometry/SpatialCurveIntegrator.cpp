#include "Core/Geometry/SpatialCurveIntegrator.h"
#include <algorithm>
#include <cmath>
#include <cstddef>



SpatialCurveIntegrationResult
SpatialCurveIntegrator::integrate(
    const Frame& startFrame,
    const CurvatureTorsionProfile& profile,
    double sampleStep
) const
{
    SpatialCurveIntegrationResult result;

    result.startFrame =
        startFrame;

    result.requestedArcLength =
        profile.totalArcLength;

    result.sampleStep =
        sampleStep;

    // =====================================================
    // INPUT VALIDATION
    // =====================================================

    if (!isValidFrame(
        startFrame
    ))
    {
        result.status =
            SpatialCurveIntegrationStatus::
            InvalidStartFrame;

        return result;
    }

    if (!isValidProfile(
        profile
    ))
    {
        result.status =
            SpatialCurveIntegrationStatus::
            InvalidProfile;

        return result;
    }

    if (sampleStep <= 1e-9
        || !std::isfinite(sampleStep))
    {
        result.status =
            SpatialCurveIntegrationStatus::
            InvalidSampleStep;

        return result;
    }

    // =====================================================
    // INITIAL FRAME
    // =====================================================

    Frame currentFrame =
        startFrame;

    orthonormalizeFrame(
        currentFrame
    );

    if (!isFiniteFrame(
        currentFrame
    ))
    {
        result.status =
            SpatialCurveIntegrationStatus::
            NumericalFailure;

        return result;
    }

    // One initial node plus one node for each step.
    size_t estimatedStepCount =
        static_cast<size_t>(
            std::ceil(
                profile.totalArcLength
                / sampleStep
            )
            );

    result.requestedStepCount =
        estimatedStepCount;

    result.nodes.reserve(
        estimatedStepCount + 1
    );

    result.arcLengths.reserve(
        estimatedStepCount + 1
    );

    result.nodes.push_back(
        makeNodeFromFrame(
            currentFrame
        )
    );

    result.arcLengths.push_back(
        0.0
    );

    // =====================================================
    // CONSTANT-STEP FRENET INTEGRATION
    // =====================================================

    double currentArcLength =
        0.0;

    size_t completedSteps =
        0;

    while (currentArcLength
        < profile.totalArcLength - 1e-12)
    {
        // Shorten only the final step so integration ends
        // exactly at the requested arc length.
        double stepLength =
            std::min(
                sampleStep,
                profile.totalArcLength
                - currentArcLength
            );

        double curvature =
            0.0;

        double torsion =
            0.0;

        // Sample at the beginning of the current interval.
        if (!sampleProfileAtArcLength(
            profile,
            currentArcLength,
            curvature,
            torsion
        ))
        {
            result.status =
                SpatialCurveIntegrationStatus::
                InvalidProfile;

            result.completedStepCount =
                completedSteps;

            result.integratedArcLength =
                currentArcLength;

            result.endFrame =
                currentFrame;

            return result;
        }

        if (!std::isfinite(curvature)
            || !std::isfinite(torsion))
        {
            result.status =
                SpatialCurveIntegrationStatus::
                NumericalFailure;

            result.completedStepCount =
                completedSteps;

            result.integratedArcLength =
                currentArcLength;

            result.endFrame =
                currentFrame;

            return result;
        }

        const Vec3D oldT =
            currentFrame.T;

        const Vec3D oldN =
            currentFrame.N;

        const Vec3D oldB =
            currentFrame.B;

        // =================================================
        // FRENET-SERRET EXPLICIT EULER STEP
        // =================================================

        Vec3D nextT =
            oldT
            + oldN
            * (
                stepLength
                * curvature
                );

        Vec3D nextN =
            oldN
            + (
                oldT
                * (-curvature)
                + oldB
                * torsion
                )
            * stepLength;

        Vec3D nextB =
            oldB
            + oldN
            * (
                -stepLength
                * torsion
                );

        // Use average tangent for a slightly better
        // position update than oldT alone.
        Vec3D movementDirection =
            oldT + nextT;

        if (movementDirection.lengthSquared()
            <= 1e-12)
        {
            movementDirection =
                oldT;
        }

        movementDirection =
            movementDirection.normalized();

        currentFrame.P +=
            movementDirection
            * stepLength;

        currentFrame.T =
            nextT;

        currentFrame.N =
            nextN;

        currentFrame.B =
            nextB;

        orthonormalizeFrame(
            currentFrame
        );

        if (!isFiniteFrame(
            currentFrame
        )
            || !isValidFrame(
                currentFrame
            ))
        {
            result.status =
                SpatialCurveIntegrationStatus::
                NumericalFailure;

            result.completedStepCount =
                completedSteps;

            result.integratedArcLength =
                currentArcLength;

            result.endFrame =
                currentFrame;

            return result;
        }

        currentArcLength +=
            stepLength;

        ++completedSteps;

        result.nodes.push_back(
            makeNodeFromFrame(
                currentFrame
            )
        );

        result.arcLengths.push_back(
            currentArcLength
        );
    }

    // =====================================================
    // SUCCESS RESULT
    // =====================================================

    result.endFrame =
        currentFrame;

    result.integratedArcLength =
        currentArcLength;

    result.completedStepCount =
        completedSteps;

    result.status =
        SpatialCurveIntegrationStatus::
        Completed;

    result.valid =
        result.hasConsistentNodeData();

    return result;
}

bool SpatialCurveIntegrator::isValidFrame(
    const Frame& frame
) const
{
    return frame.T.lengthSquared() > 1e-12
        && frame.N.lengthSquared() > 1e-12
        && frame.B.lengthSquared() > 1e-12;
}



bool SpatialCurveIntegrator::isValidProfile(
    const CurvatureTorsionProfile& profile
) const
{
    if (!profile.valid)
        return false;

    if (profile.totalArcLength <= 0.0)
        return false;

    if (profile.samples.empty())
        return false;

    double previousArcLength =
        -1.0;

    for (const CurvatureTorsionSample& sample :
        profile.samples)
    {
        if (!sample.isValid())
            return false;

        if (!std::isfinite(
            sample.arcLength
        )
            || !std::isfinite(
                sample.curvature
            )
            || !std::isfinite(
                sample.torsion
            ))
        {
            return false;
        }

        if (sample.arcLength
            < previousArcLength)
        {
            return false;
        }

        previousArcLength =
            sample.arcLength;
    }

    return true;
}



void SpatialCurveIntegrator::orthonormalizeFrame(
    Frame& frame
) const
{
    frame.T =
        frame.T.normalized();

    frame.N =
        (
            frame.N
            - frame.T * dot(
                frame.N,
                frame.T
            )
            ).normalized();

    frame.B =
        cross(
            frame.T,
            frame.N
        ).normalized();

    frame.N =
        cross(
            frame.B,
            frame.T
        ).normalized();
}
PipeNode SpatialCurveIntegrator::makeNodeFromFrame(
    const Frame& frame
) const
{
    PipeNode node;

    node.pos =
        frame.P;

    node.T =
        frame.T;

    node.N =
        frame.N;

    node.B =
        frame.B;

    return node;
}

bool SpatialCurveIntegrator::sampleProfileAtArcLength(
    const CurvatureTorsionProfile& profile,
    double arcLength,
    double& outCurvature,
    double& outTorsion) const
{
    if (!profile.valid)
        return false;

    if (profile.samples.empty())
        return false;

    if (!std::isfinite(arcLength))
        return false;

    const auto& samples =
        profile.samples;

    if (arcLength < samples.front().arcLength)
    {
        outCurvature =
            samples.front().curvature;

        outTorsion =
            samples.front().torsion;

        return true;
    }

    if (arcLength >= samples.back().arcLength)
    {
        outCurvature =
            samples.back().curvature;

        outTorsion =
            samples.back().torsion;

        return true;
    }

    // Find the rightmost sample whose arc length is <=
    // the requested position.
    //
    // With duplicate boundaries:
    //
    //     s=40, ?=0
    //     s=40, ?=active
    //
    // arcLength=40 selects the second value.
    std::size_t leftIndex =
        0;

    for (std::size_t i = 0;
        i < samples.size();
        ++i)
    {
        if (samples[i].arcLength <= arcLength)
        {
            leftIndex =
                i;
        }
        else
        {
            break;
        }
    }

    const CurvatureTorsionSample& left =
        samples[leftIndex];

    if (leftIndex + 1 >= samples.size())
    {
        outCurvature =
            left.curvature;

        outTorsion =
            left.torsion;

        return true;
    }

    const CurvatureTorsionSample& right =
        samples[leftIndex + 1];

    const double interval =
        right.arcLength
        - left.arcLength;

    if (interval <= 1e-12)
    {
        outCurvature =
            right.curvature;

        outTorsion =
            right.torsion;

        return true;
    }

    double interpolation =
        (
            arcLength
            - left.arcLength
            )
        / interval;

    interpolation =
        std::clamp(
            interpolation,
            0.0,
            1.0
        );

    outCurvature =
        left.curvature
        + interpolation
        * (
            right.curvature
            - left.curvature
            );

    outTorsion =
        left.torsion
        + interpolation
        * (
            right.torsion
            - left.torsion
            );

    return
        std::isfinite(outCurvature)
        && std::isfinite(outTorsion);
}

bool SpatialCurveIntegrator::isFiniteFrame(
    const Frame& frame
) const
{
    return std::isfinite(frame.P.x)
        && std::isfinite(frame.P.y)
        && std::isfinite(frame.P.z)

        && std::isfinite(frame.T.x)
        && std::isfinite(frame.T.y)
        && std::isfinite(frame.T.z)

        && std::isfinite(frame.N.x)
        && std::isfinite(frame.N.y)
        && std::isfinite(frame.N.z)

        && std::isfinite(frame.B.x)
        && std::isfinite(frame.B.y)
        && std::isfinite(frame.B.z);
}

bool SpatialCurveIntegrator::sampleProfileForDebug(
    const CurvatureTorsionProfile& profile,
    double arcLength,
    double& outCurvature,
    double& outTorsion) const
{
    return sampleProfileAtArcLength(
        profile,
        arcLength,
        outCurvature,
        outTorsion
    );
}


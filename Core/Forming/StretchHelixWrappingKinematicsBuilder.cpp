#include "Core/Forming/StretchHelixWrappingKinematicsBuilder.h"

#include <cmath>

StretchHelixWrappingKinematics
StretchHelixWrappingKinematicsBuilder::build(
    const StretchHelixWrappingInput& input)
{
    StretchHelixWrappingKinematics result;

    result.clear();

    // =================================================
    // 1. VALIDATE INPUT
    // =================================================

    if (!input.isValid())
        return result;

    constexpr double PI =
        3.14159265358979323846;

    // =================================================
    // 2. WORKPIECE CENTERLINE RADIUS
    //
    // The workpiece contacts the cylindrical support by
    // its inner surface.
    //
    // Therefore:
    //
    // centerlineRadius =
    //     supportOuterRadius + pipeOuterRadius
    // =================================================

    const double pipeOuterRadius =
        input.pipeSection.outerDiameter
        * 0.5;

    result.centerlineRadius =
        input.supportOuterRadius
        + pipeOuterRadius;

    // =================================================
    // 3. PITCH FROM MACHINE MOTION
    //
    // One revolution requires:
    //
    //     revolutionTime = 2*pi / |omega|
    //
    // During that time, the axial motion advances:
    //
    //     P = vz * revolutionTime
    //
    // Therefore:
    //
    //     P = 2*pi*vz / |omega|
    // =================================================

    const double absoluteRotationSpeed =
        std::abs(
            input.rotationSpeed
        );

    result.pitch =
        2.0
        * PI
        * input.axialSpeed
        / absoluteRotationSpeed;

    // =================================================
    // 4. HELIX RISE PER RADIAN
    //
    //     b = P / (2*pi)
    //
    // Since P came from machine motion:
    //
    //     b = vz / |omega|
    // =================================================

    result.helixRisePerRadian =
        result.pitch
        / (
            2.0
            * PI
            );

    // =================================================
    // 5. CURVATURE AND TORSION
    //
    // Circular helix:
    //
    //              r
    // kappa = -----------
    //          r^2 + b^2
    //
    //              b
    // tau   = -----------
    //          r^2 + b^2
    //
    // Torsion receives the commanded handedness sign.
    // =================================================

    const double r =
        result.centerlineRadius;

    const double b =
        result.helixRisePerRadian;
    const double alternativeSpeed =
        std::sqrt(
            (
                r
                * absoluteRotationSpeed
                )
            * (
                r
                * absoluteRotationSpeed
                )
            +
            input.axialSpeed
            * input.axialSpeed
        );

    result.centerlineSpeed =
        absoluteRotationSpeed
        * std::sqrt(
            r * r
            + b * b
        );


   

    const double denominator =
        r * r
        + b * b;

    if (!std::isfinite(denominator)
        || denominator <= 1e-12)
    {
        return result;
    }

    result.curvature =
        r
        / denominator;

    result.torsion =
        static_cast<double>(
            input.rotationDirection
            )
        * b
        / denominator;

    // =================================================
    // 6. HELIX ANGLE
    //
    // Measured relative to the circumferential direction:
    //
    //     tan(alpha) = b / r
    // =================================================

    result.helixAngle =
        std::atan2(
            b,
            r
        );

    // =================================================
    // 7. ARC LENGTH PER REVOLUTION
    //
    //     Lrev = 2*pi*sqrt(r^2 + b^2)
    // =================================================

    result.arcLengthPerRevolution =
        2.0
        * PI
        * std::sqrt(
            denominator
        );

    // =================================================
    // 8. FINAL VALIDATION
    // =================================================

    if (!std::isfinite(result.centerlineRadius)
        || !std::isfinite(result.pitch)
        || !std::isfinite(result.helixRisePerRadian)
        || !std::isfinite(result.curvature)
        || !std::isfinite(result.torsion)
        || !std::isfinite(result.helixAngle)
        || !std::isfinite(result.arcLengthPerRevolution))
    {
        result.clear();
        return result;
    }

    if (result.centerlineRadius <= 0.0
        || result.curvature <= 0.0
        || result.arcLengthPerRevolution <= 0.0)
    {
        result.clear();
        return result;
    }

    result.valid =
        true;

    return result;

}
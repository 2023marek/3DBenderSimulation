#include "Core/Forming/StretchHelixWrappingStateAdvancer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

void StretchHelixWrappingStateAdvancer::advance(
    StretchHelixWrappingState& state,
    double dt,
    const StretchHelixWrappingInput& input,
    const StretchHelixWrappingKinematics& kinematics,
    double formingRisePerRadian)
{
    if (!state.valid)
        return;

    if (!input.isValid())
        return;

    if (!kinematics.valid)
        return;

    if (!std::isfinite(formingRisePerRadian))
    {
        return;
    }
    if (!std::isfinite(dt)
        || dt <= 0.0)
    {
        return;
    }

    if (state.complete)
        return;

    if (!std::isfinite(kinematics.centerlineSpeed)
        || kinematics.centerlineSpeed <= 0.0)
    {
        return;
    }

    const double totalLength =
        input.pipeArcLength;

    const double remainingLength =
        std::max(
            0.0,
            totalLength
            - state.wrappedLength
        );

    const double requestedAdvance =
        kinematics.centerlineSpeed
        * dt;

    const double actualAdvance =
        std::min(
            remainingLength,
            requestedAdvance
        );

    const double actualDt =
        actualAdvance
        / kinematics.centerlineSpeed;

    // =====================================================
    // ADVANCE WRAPPED PIPE
    // =====================================================

    state.wrappedLength +=
        actualAdvance;

    state.wrappedLength =
        std::min(
            state.wrappedLength,
            totalLength
        );

    state.contactFrontS =
        state.wrappedLength;

    state.progress =
        totalLength > 0.0
        ? state.wrappedLength / totalLength
        : 0.0;

    state.progress =
        std::clamp(
            state.progress,
            0.0,
            1.0
        );

    // =====================================================
    // ADVANCE MACHINE MOTION
    //
    // IMPORTANT:
    // Use actualDt, not requested dt.
    //
    // If only 5 mm of pipe remains, machine rotation and
    // axial motion should advance only for the time needed
    // to consume those final 5 mm.
    // =====================================================

    // =====================================================
// ADVANCE MACHINE MOTION
// =====================================================

    const double deltaAngle =
        static_cast<double>(
            input.rotationDirection
            )
        * input.rotationSpeed
        * actualDt;

    const double deltaAxial =
        formingRisePerRadian
        * deltaAngle;

    const double measuredRisePerRadian =
        std::abs(deltaAngle) > 1e-12
        ? deltaAxial / deltaAngle
        : 0.0;

    std::cout
        << "[MH1.20.8B FORMING PITCH]"
        << " deltaAngle="
        << deltaAngle
        << " deltaAxial="
        << deltaAxial
        << " risePerRadian="
        << measuredRisePerRadian
        << " formingRisePerRadian="
        << formingRisePerRadian
        << " inputAxialSpeed="
        << input.axialSpeed
        << " inputRotationSpeed="
        << input.rotationSpeed
        << std::endl;

    state.supportRotationAngle +=
        deltaAngle;

    state.supportAxialPosition +=
        deltaAxial;

   
    state.elapsedTime +=
        actualDt;

    // =====================================================
    // COMPLETION
    // =====================================================

    state.complete =
        state.wrappedLength
        >= totalLength - 1e-12;

    if (state.complete)
    {
        state.wrappedLength =
            totalLength;

        state.contactFrontS =
            totalLength;

        state.progress =
            1.0;
    }

    std::cout
        << "[STRETCH HELIX MACHINE STATE]"
        << " dtRequested="
        << dt
        << " dtActual="
        << actualDt
        << " wrappedLength="
        << state.wrappedLength
        << " rotationAngle="
        << state.supportRotationAngle
        << " axialPosition="
        << state.supportAxialPosition
        << " progress="
        << state.progress
        << " complete="
        << state.complete
        << std::endl;
}
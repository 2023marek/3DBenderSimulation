#include "Core/Forming/StretchHelixWrappingStateAdvancer.h"

#include <algorithm>
#include <cmath>

void StretchHelixWrappingStateAdvancer::advance(
    StretchHelixWrappingState& state,
    double dt,
    const StretchHelixWrappingInput& input,
    const StretchHelixWrappingKinematics& kinematics)
{
    if (!state.valid)
        return;

    if (!input.isValid())
        return;

    if (!kinematics.valid)
        return;

    if (!std::isfinite(dt)
        || dt <= 0.0)
    {
        return;
    }

    if (state.complete)
        return;

    const double totalLength =
        input.pipeArcLength;

    const double deltaWrappedLength =
        kinematics.centerlineSpeed
        * dt;

    state.elapsedTime +=
        dt;

    state.wrappedLength =
        std::min(
            totalLength,
            state.wrappedLength
            + deltaWrappedLength
        );

    state.contactFrontS =
        state.wrappedLength;

    state.progress =
        state.wrappedLength
        / totalLength;

    state.complete =
        state.wrappedLength
        >= totalLength - 1e-12;
}
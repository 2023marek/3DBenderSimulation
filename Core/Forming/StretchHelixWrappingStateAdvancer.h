#pragma once

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"
#include "Core/Forming/StretchHelixWrappingState.h"

class StretchHelixWrappingStateAdvancer
{
public:
    static void advance(
        StretchHelixWrappingState& state,
        double dt,
        const StretchHelixWrappingInput& input,
        const StretchHelixWrappingKinematics& kinematics,
        double formingCenterlineRadius,
        double formingRisePerRadian
    );
};
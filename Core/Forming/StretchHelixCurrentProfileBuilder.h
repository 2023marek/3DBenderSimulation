#pragma once

#include "Core/Geometry/CurvatureTorsionProfile.h"

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"
#include "Core/Forming/StretchHelixWrappingState.h"

class StretchHelixCurrentProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        const StretchHelixWrappingInput& input,
        const StretchHelixWrappingKinematics& kinematics,
        const StretchHelixWrappingState& state
    );
};
#pragma once

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingState.h"

class StretchHelixWrappingStateBuilder
{
public:
    static StretchHelixWrappingState buildInitial(
        const StretchHelixWrappingInput& input
    );
};
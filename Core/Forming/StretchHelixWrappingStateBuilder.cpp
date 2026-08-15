#include "Core/Forming/StretchHelixWrappingStateBuilder.h"

StretchHelixWrappingState
StretchHelixWrappingStateBuilder::buildInitial(
    const StretchHelixWrappingInput& input)
{
    StretchHelixWrappingState state;

    state.clear();

    if (!input.isValid())
        return state;

    state.wrappedLength =
        0.0;

    state.contactFrontS =
        0.0;

    state.progress =
        0.0;

    state.complete =
        false;

    state.valid =
        true;

    state.supportRotationAngle =
        0.0;

    state.supportAxialPosition =
        0.0;

   

    return state;
}
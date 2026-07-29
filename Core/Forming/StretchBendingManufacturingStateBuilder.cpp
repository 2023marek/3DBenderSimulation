#include "Core/Forming/StretchBendingManufacturingStateBuilder.h"

StretchBendingManufacturingState
StretchBendingManufacturingStateBuilder::buildReadyState(
    const StretchBendingProcessInput& input,
    const StretchBendingEvaluationResult& evaluation,
    const StretchBendingActiveZone& activeZone
)
{
    StretchBendingManufacturingState state;

    // -------------------------------------------------
    // A manufacturing state can only be prepared from
    // accepted stretch-bending input.
    // -------------------------------------------------

    if (!input.isValid())
        return state;

    if (!evaluation.valid)
        return state;

    if (evaluation.status
        != StretchBendingEvaluationStatus::Valid)
    {
        return state;
    }

    const double totalLength =
        input.geometry.targetArcLength;

    if (!activeZone.isValidForLength(
        totalLength
    ))
    {
        return state;
    }

    // -------------------------------------------------
    // Initial ready state.
    //
    // No tension or bending load has been applied yet.
    // -------------------------------------------------

    state.stage =
        StretchBendingManufacturingStage::Ready;

    state.processProgress =
        0.0;

    state.activeZone =
        activeZone;

    state.tensionFraction =
        0.0;

    state.bendingFraction =
        0.0;

    state.unloadingFraction =
        0.0;

    return state;
}
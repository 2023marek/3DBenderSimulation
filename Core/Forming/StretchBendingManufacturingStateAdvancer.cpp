#include "StretchBendingManufacturingStateAdvancer.h"

#include <algorithm>

// =====================================================
// LOCAL CLAMP
// =====================================================

double StretchBendingManufacturingStateAdvancer::clamp01(
    double value)
{
    return std::max(
        0.0,
        std::min(
            1.0,
            value
        )
    );
}

// =====================================================
// ADVANCE MANUFACTURING STATE
// =====================================================

void StretchBendingManufacturingStateAdvancer::advance(
    StretchBendingManufacturingState& state,
    double dt,
    const StretchBendingManufacturingTiming& timing)
{
    // -------------------------------------------------
    // Reject invalid input without attempting to advance.
    // -------------------------------------------------

    if (!state.valid)
    {
        state.stage =
            StretchBendingManufacturingStage::Invalid;

        return;
    }

    if (!timing.isValid())
    {
        state.stage =
            StretchBendingManufacturingStage::Invalid;

        state.valid =
            false;

        return;
    }

    // Negative or zero time must never run the process
    // backwards.
    if (dt <= 0.0)
    {
        return;
    }

    // A completed state remains complete.
    if (state.stage
        == StretchBendingManufacturingStage::Complete)
    {
        return;
    }

    state.elapsedTime +=
        dt;

    const double tensionEnd =
        timing.tensionDuration;

    const double formingEnd =
        tensionEnd
        + timing.formingDuration;

    const double holdEnd =
        formingEnd
        + timing.loadedHoldDuration;

    const double processEnd =
        holdEnd
        + timing.unloadingDuration;

    // Overall normalized manufacturing progress.
    state.processProgress =
        clamp01(
            state.elapsedTime
            / processEnd
        );

    // =================================================
    // STAGE 1 — APPLYING TENSION
    // =================================================

    if (state.elapsedTime < tensionEnd)
    {
        state.stage =
            StretchBendingManufacturingStage::
            ApplyingTension;

        state.tensionFraction =
            clamp01(
                state.elapsedTime
                / timing.tensionDuration
            );

        state.bendingFraction =
            0.0;

        state.unloadingFraction =
            0.0;

        return;
    }

    // =================================================
    // STAGE 2 — FORMING
    // =================================================

    if (state.elapsedTime < formingEnd)
    {
        state.stage =
            StretchBendingManufacturingStage::Forming;

        const double localFormingTime =
            state.elapsedTime
            - tensionEnd;

        state.tensionFraction =
            1.0;

        state.bendingFraction =
            clamp01(
                localFormingTime
                / timing.formingDuration
            );

        state.unloadingFraction =
            0.0;

        return;
    }

    // =================================================
    // STAGE 3 — LOADED HOLD
    // =================================================

    if (state.elapsedTime < holdEnd)
    {
        state.stage =
            StretchBendingManufacturingStage::LoadedHold;

        state.tensionFraction =
            1.0;

        state.bendingFraction =
            1.0;

        state.unloadingFraction =
            0.0;

        return;
    }

    // =================================================
    // STAGE 4 — UNLOADING
    // =================================================

    if (state.elapsedTime < processEnd)
    {
        state.stage =
            StretchBendingManufacturingStage::Unloading;

        const double localUnloadingTime =
            state.elapsedTime
            - holdEnd;

        state.unloadingFraction =
            clamp01(
                localUnloadingTime
                / timing.unloadingDuration
            );

        // Axial tension is gradually released.
        state.tensionFraction =
            1.0
            - state.unloadingFraction;

        // The loaded bend has already been achieved.
        //
        // Geometry interpolation between loaded and final
        // shapes will be introduced in a later phase.
        state.bendingFraction =
            1.0;

        return;
    }

    // =================================================
    // STAGE 5 — COMPLETE
    // =================================================

    state.elapsedTime =
        processEnd;

    state.processProgress =
        1.0;

    state.tensionFraction =
        0.0;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        1.0;

    state.stage =
        StretchBendingManufacturingStage::Complete;
}
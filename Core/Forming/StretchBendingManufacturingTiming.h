#pragma once

// =====================================================
// STRETCH-BENDING MANUFACTURING TIMING
//
// Defines how long each independent manufacturing stage
// lasts.
//
// This structure contains only process timing data.
// It does not contain geometry or material calculations.
// =====================================================

struct StretchBendingManufacturingTiming
{
    // Time used to increase axial tension from zero
    // to the full commanded value.
    double tensionDuration =
        1.0;

    // Time used to increase bending from zero
    // to the full loaded curvature command.
    double formingDuration =
        2.0;

    // Optional time during which the fully loaded shape
    // and full tension are maintained.
    double loadedHoldDuration =
        0.5;

    // Time used to release the forming load and tension.
    double unloadingDuration =
        1.0;

    bool isValid() const
    {
        return tensionDuration > 0.0
            && formingDuration > 0.0
            && loadedHoldDuration >= 0.0
            && unloadingDuration > 0.0;
    }

    double totalDuration() const
    {
        return tensionDuration
            + formingDuration
            + loadedHoldDuration
            + unloadingDuration;
    }
};
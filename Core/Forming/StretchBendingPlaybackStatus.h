#pragma once

#include "Core/Forming/StretchBendingManufacturingStage.h"

// =====================================================
// STRETCH-BENDING PLAYBACK STATUS
//
// Compact read-only snapshot for UI and HUD display.
//
// It contains no geometry and cannot advance playback.
// =====================================================

struct StretchBendingPlaybackStatus
{
    StretchBendingManufacturingStage stage =
        StretchBendingManufacturingStage::Invalid;

    double elapsedTime =
        0.0;

    double totalDuration =
        0.0;

    double progress =
        0.0;

    double playbackSpeed =
        1.0;

    double tensionFraction =
        0.0;

    double bendingFraction =
        0.0;

    double unloadingFraction =
        0.0;

    double currentCurvature =
        0.0;

    double currentTorsion =
        0.0;

    bool prepared =
        false;

    bool playing =
        false;

    bool previewVisible =
        false;

    bool geometryValid =
        false;

    bool complete =
        false;
};
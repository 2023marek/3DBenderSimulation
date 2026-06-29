#pragma once

#include "Core/Forming/PassPlacement.h"
#include "Core/Curve/PipeCurve.h"
#include "Core/Geometry/Frame.h"

// =====================================================
// PASS PLACEMENT RESOLUTION
//
// Result of converting a PassPlacement into:
// - concrete start Frame
// - resolved arc length on base curve
// - validity flag
//
// ExplicitStartFrame:
//     frame = placement.startFrame
//     arcLength = 0.0
//
// InsertAtArcLength:
//     arcLength = placement.arcLength
//     frame = sampled frame at that arc length
//
// InsertAtNodeIndex:
//     nodeIndex -> arcLength -> frame
// =====================================================

struct PassPlacementResolution
{
    bool valid = false;
    Frame frame;
    double arcLength = 0.0;
};

PassPlacementResolution resolvePassPlacementFrame(
    const PassPlacement& placement,
    const PipeCurve& baseCurve,
    double sampleStep
);
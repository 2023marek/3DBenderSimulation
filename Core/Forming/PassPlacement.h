#pragma once

#include <cstddef>

#include "Core/Geometry/Frame.h"

// =====================================================
// PASS PLACEMENT
//
// Describes where a ManufacturingPass should be placed
// relative to previous pipe geometry.
//
// Current phase:
// - metadata only
//
// Later phase:
// - ManufacturingPlan will use this to insert / append
//   pass output curves.
// =====================================================

enum class PassPlacementMode
{
    // Start this pass at the end frame of the previous pass.
    AppendToPrevious,

    // Start this pass at arc length s along the already-built curve.
    InsertAtArcLength,

    // Start this pass at sampled node index.
    InsertAtNodeIndex,

    // Start this pass from an explicit frame.
    ExplicitStartFrame
};
// Helper for debugging.
inline const char* passPlacementModeToString(
    PassPlacementMode mode)
{
    switch (mode)
    {
    case PassPlacementMode::AppendToPrevious:
        return "AppendToPrevious";

    case PassPlacementMode::InsertAtArcLength:
        return "InsertAtArcLength";

    case PassPlacementMode::InsertAtNodeIndex:
        return "InsertAtNodeIndex";

    case PassPlacementMode::ExplicitStartFrame:
        return "ExplicitStartFrame";

    default:
        return "Unknown";
    }
}
struct PassPlacement
{
    PassPlacementMode mode =
        PassPlacementMode::AppendToPrevious;

    // Used by InsertAtArcLength.
    double arcLength = 0.0;

    // Used by InsertAtNodeIndex.
    size_t nodeIndex = 0;

    // Used by ExplicitStartFrame.
    Frame startFrame;

    static PassPlacement append()
    {
        PassPlacement p;
        p.mode = PassPlacementMode::AppendToPrevious;
        return p;
    }

    static PassPlacement atArcLength(double s)
    {
        PassPlacement p;
        p.mode = PassPlacementMode::InsertAtArcLength;
        p.arcLength = s;
        return p;
    }

    static PassPlacement atNodeIndex(size_t index)
    {
        PassPlacement p;
        p.mode = PassPlacementMode::InsertAtNodeIndex;
        p.nodeIndex = index;
        return p;
    }

    static PassPlacement atFrame(const Frame& frame)
    {
        PassPlacement p;
        p.mode = PassPlacementMode::ExplicitStartFrame;
        p.startFrame = frame;
        return p;
    }
	
};
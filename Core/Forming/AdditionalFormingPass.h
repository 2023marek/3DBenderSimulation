#pragma once

#include <string>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Geometry/Frame.h"
#include "Core/Forming/PassPlacement.h"
#include "Core/Forming/DeformableRegion.h"

// =====================================================
// ADDITIONAL FORMING PASS
//
// Real manufacturing concept.
//
// Used when a pipe is already partially or fully formed,
// then another operation is performed on that same physical pipe.
//
// This is NOT planned-preview insertion.
// This belongs to manufacturing history / process simulation.
//
// Examples:
// - manually add one bend after previous bending
// - add helix after rotary draw bending
// - continue forming in another machine
// =====================================================

struct AdditionalFormingPass
{
    std::string name;

    // The forming operation/pass to apply next.
    ManufacturingPass pass;

    // Frame where the already formed pipe enters the next process.
    Frame entryFrame;

    // Future:
    // - selected deformable region
    // - clamp points
    // - machine type
    // - process constraints
    // - collision validation
    bool enabled = true;
    // Debug / trace info.
// Explains how entryFrame was resolved.
    PassPlacementMode resolvedPlacementMode =
    PassPlacementMode::AppendToPrevious;

    size_t resolvedNodeIndex = 0;

    double requestedArcLength = 0.0;
    double resolvedArcLength = 0.0;

    bool hasResolvedPlacement = false;
    DeformableRegion deformableRegion;
};
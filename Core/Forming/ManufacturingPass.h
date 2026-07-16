#pragma once

#include <string>
#include <vector>

#include "Core/Operations.h"
#include "Core/Curve/PipeCurve.h"
#include "Core/Forming/TubeFormingProcessType.h"
#include "Core/Forming/PassPlacement.h"
#include "Core/Geometry/Frame.h"
#include "Core/Forming/DeformableRegion.h"

// =====================================================
// MANUFACTURING PASS
//
// One manufacturing stage applied to the pipe.
//
// Examples:
// - rotary draw bending pass
// - manual rework pass
// - helix forming pass
// - stretch bending pass
// - roller forming pass
//
// This class does NOT execute simulation yet.
// It only describes a stage and stores its curve result.
// =====================================================

struct ManufacturingPass
{
    // =====================================================
    // ID / DESCRIPTION
    // =====================================================

    std::string name;

    TubeFormingProcessType processType =
        TubeFormingProcessType::RotaryDrawBending;
    PassPlacement placement =
        PassPlacement::append();

    // =====================================================
    // INPUT COMMANDS
    //
    // For now, this uses the current Operation type.
    // This fits RotaryDrawBending.
    //
    // Later we can replace or extend this with:
    // FormingOperation
    // HelixOperation
    // StretchBendOperation
    // =====================================================

    std::vector<Operation> operations;



    // =====================================================
    // OUTPUT GEOMETRY
    //
    // Curvature-driven result of this manufacturing pass.
    //
    // Important rule:
    // outputCurve is the model.
    // nodes are only sampled later.
    // =====================================================

    PipeCurve outputCurve;

    DeformableRegion deformableRegion;

    // =====================================================
// RESOLVED START FRAME
//
// Used after placement resolution.
//
// Example:
// InsertAtArcLength(202)
//      ?
// find frame on existing curve at s=202
//      ?
// store that frame here
//
// Phase 7Q:
// metadata only.
// Later:
// inserted outputCurve will be transformed/aligned to this frame.
// =====================================================

    bool hasResolvedStartFrame = false;
    Frame resolvedStartFrame;

    // =====================================================
    // STATE FLAGS
    // =====================================================

    bool enabled = true;
    bool completed = false;

    void clear()
    {
        name.clear();

        processType =
            TubeFormingProcessType::RotaryDrawBending;

        placement =
            PassPlacement::append();

        operations.clear();
        outputCurve.clear();

        enabled = true;
        completed = false;

        hasResolvedStartFrame = false;
        resolvedStartFrame = Frame();
    }
};
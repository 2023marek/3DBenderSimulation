#pragma once

#include <string>
#include <vector>

#include "Core/Operations.h"
#include "Core/Curve/PipeCurve.h"
#include "Core/Forming/TubeFormingProcessType.h"

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

        operations.clear();
        outputCurve.clear();

        enabled = true;
        completed = false;
    }
};
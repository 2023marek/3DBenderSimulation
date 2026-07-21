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
    std::vector<Operation> operations;

    TubeFormingProcessType processType =
        TubeFormingProcessType::RotaryDrawBending;

    PassPlacement placement =
        PassPlacement::append();

    PipeCurve outputCurve;
        
    DeformableRegion deformableRegion;

 Frame resolvedStartFrame;

    bool hasResolvedStartFrame = false;
    
    // Helix-forming parameters.
    double helixLength =
        0.0;

    double helixRadius =
        0.0;

    double helixPitch =
        0.0;

    double helixFeedSpeed =
        0.0;

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
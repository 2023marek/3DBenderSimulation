#pragma once

#include <cstddef>
#include <vector>

#include "Core/Geometry/PipeNode.h"

// =====================================================
// DEFORMABLE REGION SELECTION
//
// Result of splitting manufactured geometry by an
// arc-length range.
//
// No geometry is modified here.
//
// beforeNodes:
//     geometry before the selected range.
//
// selectedNodes:
//     nodes inside the deformable range.
//
// afterNodes:
//     geometry after the selected range.
// =====================================================

struct DeformableRegionSelection
{
    std::vector<PipeNode> beforeNodes;
    std::vector<PipeNode> selectedNodes;
    std::vector<PipeNode> afterNodes;

    double sourceArcLength =
        0.0;

    double selectedStartArcLength =
        0.0;

    double selectedEndArcLength =
        0.0;

    bool valid =
        false;
    double availableSourceLength =
        0.0;

    double requiredEndArcLength =
        0.0;

    double missingLength =
        0.0;

    bool fitsAvailableLength =
        false;

    double actualSelectedStartArcLength =
        0.0;

    double actualSelectedEndArcLength =
        0.0;

    double startBoundaryError =
        0.0;

    double endBoundaryError =
        0.0;

    void clear()
    {
        beforeNodes.clear();
        selectedNodes.clear();
        afterNodes.clear();

        sourceArcLength = 0.0;
        selectedStartArcLength = 0.0;
        selectedEndArcLength = 0.0;

        valid = false;

        availableSourceLength = 0.0;
        requiredEndArcLength = 0.0;
        missingLength = 0.0;
        fitsAvailableLength = false;

        actualSelectedStartArcLength = 0.0;
        actualSelectedEndArcLength = 0.0;
        startBoundaryError = 0.0;
        endBoundaryError = 0.0;
    }
};
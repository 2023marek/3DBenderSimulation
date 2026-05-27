#pragma once

#include <vector>

#include "Core/Manufacturing/ManufacturingTypes.h"
#include "Core/Geometry/PipeNode.h"

// =====================================================
// MANUFACTURING STATE
//
// Owns all dynamic pipe manufacturing state.
//
// Phase 5B-1:
//     PipeAxis3D still owns this bundle.
//
// Phase 5B-2:
//     ManufacturingPipeSimulator will own this bundle.
//
// This state represents:
//     IncomingStock
//     PositionedStraight
//     ActiveZone
//     CurrentBendTrace
//     FrozenGeometry
//     RenderData
// =====================================================

struct ManufacturingState
{
    ManufacturingIncomingStock incomingStock;
    ManufacturingPositionedStraight positionedStraight;
    ManufacturingActiveZone activeZone;

    std::vector<PipeNode> currentBendTraceNodes;
    std::vector<PipeNode> frozenNodes;

    ManufacturingRenderData renderData;

    void clear()
    {
        incomingStock = ManufacturingIncomingStock{};
        positionedStraight = ManufacturingPositionedStraight{};
        activeZone = ManufacturingActiveZone{};

        currentBendTraceNodes.clear();
        frozenNodes.clear();

        renderData.clear();
    }
};

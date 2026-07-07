#pragma once

#include <vector>

#include "Core/BendDirection.h"
#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"

// =====================================================
// MANUFACTURING INCOMING STOCK
// =====================================================

struct ManufacturingIncomingStock
{
    double totalLength = 300.0;      // full raw stock length
    double remainingLength = 300.0;  // stock not yet consumed by machine

    // Material already fed into machine     
    double consumedLength = 0.0;

    bool visible = true;
    bool exhausted =        false;
};

// =====================================================
// MANUFACTURING POSITIONED STRAIGHT
//
// Material already fed through machine,
// still straight, not yet bent/frozen.
// =====================================================

struct ManufacturingPositionedStraight
{
    // =====================================================
    // SOURCE OF TRUTH
    //
    // Remaining straight pipe currently positioned in the
    // machine before entering the active bend.
    // =====================================================
    double length = 0.0;

    // =====================================================
    // GENERATED CACHE
    //
    // Rebuilt from:
    //   - length
    //   - current attachment frame
    //
    // This is render/generated geometry.
    // Do not treat this as simulation state.
    // =====================================================
    std::vector<PipeNode> nodes;

	bool visible = true;
};

// =====================================================
// MANUFACTURING ACTIVE ZONE
//
// Local deformation window during bending.
// This is NOT the full visible bend trace.
// =====================================================

struct ManufacturingActiveZone
{
    Frame frame;

    double curvature = 0.0;
    double accumulatedAngle = 0.0;
    double targetAngle = 0.0;

    BendDirection direction = BendDirection::CCW;

    double activeLength = 5.0;

    std::vector<PipeNode> localNodes;

    bool active = false;

    size_t frozenCountAtBendStart = 0;
};

// =====================================================
// MANUFACTURING RENDER DATA
//
// Render-ready separated zone data.
// GLView should draw these as separate line strips / tubes.
// =====================================================

struct ManufacturingRenderData
{
    std::vector<PipeNode> incomingStockNodes;
    std::vector<PipeNode> positionedStraightNodes;
    std::vector<PipeNode> activeZoneNodes;
    std::vector<PipeNode> currentBendTraceNodes;
    std::vector<PipeNode> frozenNodes;

    void clear()
    {
        incomingStockNodes.clear();
        positionedStraightNodes.clear();
        activeZoneNodes.clear();
        currentBendTraceNodes.clear();
        frozenNodes.clear();
    }
};

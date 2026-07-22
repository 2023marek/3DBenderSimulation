#pragma once

#include <vector>

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Math/Vec3D.h"

// =====================================================
// LOCAL DEFORMABLE REGION
//
// Copy of selected manufacturing nodes transformed into
// the local coordinate system of the additional-pass
// entry frame.
//
// No source manufacturing geometry is modified.
// =====================================================

struct LocalDeformableRegion
{
    Frame worldEntryFrame;

    std::vector<PipeNode> localNodes;
    std::vector<double> localArcLengths;
    std::vector<double> helixPhases;
    std::vector<Vec3D> helixOffsets;
    

    // Offset from the existing centerline to the future
    // helical centerline for each selected node.
    //
    // The offsets are expressed in the local entry-frame
    // coordinate system.
   
    std::vector<PipeNode> previewHelixNodes;

    double totalArcLength =
        0.0;

    bool valid =
        false;

    void clear()
    {
        worldEntryFrame =
            Frame{};

        localNodes.clear();
        localArcLengths.clear();
        helixPhases.clear();
        helixOffsets.clear();
        previewHelixNodes.clear();

        totalArcLength =
            0.0;

        valid =
            false;
    }
};
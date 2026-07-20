#pragma once

#include <vector>

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"

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

    bool valid =
        false;

    void clear()
    {
        worldEntryFrame =
            Frame{};

        localNodes.clear();

        valid =
            false;
    }
};

#pragma once

#include "Core/Geometry/Frame.h"
#include "Core/Math/Vec3D.h"

// =====================================================
// MACHINE MODEL
//
// Static machine/tooling data.
//
// Later this will contain:
// - bend die geometry
// - pressure die geometry
// - clamp geometry
// - mandrel geometry
// - full machine coordinate frames
// =====================================================

struct MachineModel
{
    Frame machineEntryFrame;

    Vec3D bendDieCenter = { 0.0, 0.0, 0.0 };

    double defaultBendRadius = 20.0;
    double pipeOuterRadius = 5.0;

    void reset()
    {
        machineEntryFrame.P = { 0.0, 0.0, 0.0 };
        machineEntryFrame.T = { 1.0, 0.0, 0.0 };
        machineEntryFrame.N = { 0.0, 1.0, 0.0 };
        machineEntryFrame.B = { 0.0, 0.0, 1.0 };

        bendDieCenter = { 0.0, 0.0, 0.0 };

        defaultBendRadius = 20.0;
        pipeOuterRadius = 5.0;
    }
};
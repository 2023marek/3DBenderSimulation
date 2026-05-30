#pragma once

// =====================================================
// MACHINE MODEL
//
// Static machine/tooling data.
//
// Later this will contain:
// - bend die radius
// - pressure die geometry
// - clamp geometry
// - mandrel geometry
// - machine coordinate frames
//
// For now it is intentionally minimal.
// =====================================================

struct MachineModel
{
    double defaultBendRadius = 20.0;
    double pipeOuterRadius = 5.0;

    void reset()
    {
        defaultBendRadius = 20.0;
        pipeOuterRadius = 5.0;
    }
};
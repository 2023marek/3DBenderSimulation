#pragma once

#include "Core/Geometry/Frame.h"
#include "Core/Math/Vec3D.h"
#include "Core/Machine/MachinePart.h"
#include <vector>
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
    std::vector<MachinePart> parts;
    void reset()
    {
        machineEntryFrame.P = { 0.0, 0.0, 0.0 };
        machineEntryFrame.T = { 1.0, 0.0, 0.0 };
        machineEntryFrame.N = { 0.0, 1.0, 0.0 };
        machineEntryFrame.B = { 0.0, 0.0, 1.0 };

        bendDieCenter = { 0.0, 0.0, 0.0 };

        defaultBendRadius = 20.0;
        pipeOuterRadius = 5.0;

        parts.clear();

        // Bend die
        MachinePart bendDie;
        bendDie.type = MachinePartType::BendDie;
        bendDie.geometryType = MachinePartGeometryType::MeshAsset;
        bendDie.frame = machineEntryFrame;
        bendDie.frame.P = bendDieCenter;
        bendDie.meshPath = "Assets/Machine/bend_die.stl";
        bendDie.meshScale = 1.0;
        bendDie.radius = defaultBendRadius;
        bendDie.visible = true;
        parts.push_back(bendDie);

        // Clamp die
        MachinePart clamp;
        clamp.type = MachinePartType::ClampDie;
        clamp.geometryType = MachinePartGeometryType::MeshAsset;
        clamp.frame = machineEntryFrame;
        clamp.frame.P =
            machineEntryFrame.P
            + machineEntryFrame.T * 35.0
            - machineEntryFrame.N * 12.0;
        clamp.meshPath = "Assets/Machine/clamp_die.stl";
        clamp.meshScale = 1.0;
        clamp.visible = true;
        parts.push_back(clamp);

        // Pressure die
        MachinePart pressure;
        pressure.type = MachinePartType::PressureDie;
        pressure.geometryType = MachinePartGeometryType::MeshAsset;
        pressure.frame = machineEntryFrame;
        pressure.frame.P =
            machineEntryFrame.P
            - machineEntryFrame.T * 25.0
            - machineEntryFrame.N * 12.0;
        pressure.meshPath = "Assets/Machine/pressure_die.stl";
        pressure.meshScale = 1.0;
        pressure.visible = true;
        parts.push_back(pressure);

    }
};
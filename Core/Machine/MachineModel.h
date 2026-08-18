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
    Frame supportAxisFrame;
    Vec3D bendDieCenter = { 0.0, 0.0, 0.0 };

    double defaultBendRadius = 20.0;
    double pipeOuterRadius = 5.0;
    std::vector<MachinePart> parts;


    // helix
    double supportOuterRadius =
        50.0;

    Vec3D supportAxisPoint =
    { 0.0, 0.0, 0.0 };

    Vec3D supportAxisDirection =
    { 0.0, 0.0, 1.0 };

    void reset()
    {
        machineEntryFrame.P = { 0.0, 0.0, 0.0 };
        machineEntryFrame.T = { 1.0, 0.0, 0.0 };
        machineEntryFrame.N = { 0.0, 1.0, 0.0 };
        machineEntryFrame.B = { 0.0, 0.0, 1.0 };

        bendDieCenter = { 0.0, 0.0, 0.0 };

        defaultBendRadius = 20.0;
        pipeOuterRadius = 5.0;

		// Helix support axis

        supportAxisFrame.P =
        { 0.0, -500.0, 0.0 };

        supportAxisFrame.T =
        { 0.0, 0.0, 1.0 };

        supportAxisFrame.N =
        { 1.0, 0.0, 0.0 };

        supportAxisFrame.B =
        { 0.0, 1.0, 0.0 };

        supportOuterRadius =
            50.0;

        parts.clear();

        // Bend die
        MachinePart bendDie;
        bendDie.type = MachinePartType::BendDie;
        bendDie.geometryType = MachinePartGeometryType::MeshAsset;
        bendDie.frame = machineEntryFrame;
        bendDie.frame.P = bendDieCenter;
        bendDie.meshPath ="Assets/Machine/bend_die.stl";
        bendDie.meshScale = 1.0;
        bendDie.radius = defaultBendRadius;
        bendDie.visible = false;
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
        clamp.visible = false;
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
        pressure.visible = false;
        parts.push_back(pressure);

    }
};
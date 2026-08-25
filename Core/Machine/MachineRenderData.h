#pragma once

#include <vector>

#include "Core/Math/Vec3D.h"
#include "Core/Geometry/Frame.h"
#include "Core/BendDirection.h"
#include "Core/Manufacturing/RotationKinematicMode.h"
#include "Core/Machine/MachinePart.h"

// =====================================================
// MACHINE RENDER DATA
//
// Read-only snapshot for renderers.
// =====================================================

struct MachineRenderData
{
    Frame machineEntryFrame;

    Frame supportAxisFrame;

    double supportOuterRadius =
        0.0;

    bool supportVisible =
        false;

    Vec3D bendDieCenter = { 0.0, 0.0, 0.0 };

    double bendDieRadius = 20.0;
    double pipeOuterRadius = 5.0;

    double feedPosition = 0.0;
    double rotationAngle = 0.0;
    double bendAngle = 0.0;

    BendDirection bendDirection = BendDirection::CCW;

    RotationKinematicMode rotationMode =
        RotationKinematicMode::PipeRoll;

    bool feeding = false;
    bool rotating = false;
    bool bending = false;

    std::vector<MachinePart> parts;
};
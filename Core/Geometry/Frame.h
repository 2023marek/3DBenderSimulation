#pragma once

#include "Core/Math/Vec3D.h"

// =====================================================
// FRAME
//
// Shared geometric local coordinate frame.
//
// Meaning:
// P = origin / position
// T = tangent / forward axis
// N = normal
// B = binormal
//
// Used by:
// - PipeAxis3D
// - future GeometricPipeModel
// - future ManufacturingPipeSimulator
// - MachineModel
// - DimensionSystem
// - CollisionSystem
// =====================================================

struct Frame
{
    Vec3D P;
    Vec3D T;
    Vec3D N;
    Vec3D B;
};

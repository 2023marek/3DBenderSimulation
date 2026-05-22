#pragma once

// =====================================================
// ROTATION DIRECTION
//
// OWNER:
// Shared CNC semantic layer.
//
// Used by:
// - Operation
// - SimulationController
// - PipeAxis3D::processRotate()
//
// Meaning:
// Direction of pipe roll around machineEntryFrame.T
//
// Convention:
// Looking along +T direction:
//     CCW = positive
//     CW  = negative
// =====================================================

enum class RotationDirection
{
    CCW = 1,
    CW = -1
};

inline double rotationDirectionSign(RotationDirection dir)
{
    return static_cast<int>(dir);
}

inline const char* rotationDirectionToString(RotationDirection dir)
{
    return dir == RotationDirection::CCW ? "CCW" : "CW";
}

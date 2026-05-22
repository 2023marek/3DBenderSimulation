#pragma once

// =====================================================
// BEND DIRECTION
//
// OWNER:
// Shared CNC semantic layer.
//
// Used by:
// - Operation
// - PipeAxis3D
// - SimulationController
// - HUD
// - future import/export
//
// Rule:
// Bend angle stays positive.
// BendDirection controls geometric sign.
// =====================================================

enum class BendDirection
{
    CCW = 1,
    CW = -1
};

inline double bendDirectionSign(BendDirection dir)
{
    return static_cast<int>(dir);
}

inline const char* bendDirectionToString(BendDirection dir)
{
    return dir == BendDirection::CCW ? "CCW" : "CW";
}
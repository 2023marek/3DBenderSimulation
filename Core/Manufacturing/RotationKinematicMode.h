#pragma once

// =====================================================
// ROTATION KINEMATIC MODE
//
// Defines how ROTATE command is physically interpreted.
//
// PipeRoll:
//     Pipe rotates around machine entry axis.
//     Machine bend plane stays fixed.
//
// ToolHeadRotate:
//     Pipe stays fixed.
//     Tool / bend plane rotates around pipe axis.
// =====================================================

enum class RotationKinematicMode
{
    PipeRoll,
    ToolHeadRotate
};
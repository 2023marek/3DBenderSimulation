#pragma once
#include "Core/Math/Vec3D.h"

// =========================================================================
// MACHINE STATE - Current 3D configuration of pipe bender
// =========================================================================
//
// Tracks the complete state of the machine at any moment:
//   • Position in 3D space
//   • Orientation (rotation angles)
//   • Twist (longitudinal rotation)
//   • Time elapsed
//   • Current status
//
// This is updated by SimulationController as operations execute
//
// =========================================================================

struct MachineState
{
    enum class Status
    {
        IDLE,
        RUNNING,
        PAUSED,
        COMPLETED
    };

    // =====================================================================
    // POSITION & ORIENTATION (3D Coordinates)
    // =====================================================================

    Vec3D position = { 0, 0, 0 };      // Current XYZ position (mm)
    Vec3D orientation = { 0, 0, 0 };   // Roll, Pitch, Yaw angles (radians)

    // =====================================================================
    // ROTATION TRACKING (NEW FOR PHASE 3B)
    // =====================================================================
    //
    // PURPOSE: Track total twist rotation around longitudinal axis
    //
    // HOW IT WORKS:
    //   • Starts at 0 radians
    //   • Increases when ROTATE operation executes
    //   • Represents "twist angle" of the entire pipe
    //   • Used for G-Code generation
    //
    // EXAMPLE:
    //   After ROTATE 90°: rotation = ?/2 radians
    //   After ROTATE 180°: rotation = ? radians
    //   After ROTATE 270°: rotation = 3?/2 radians
    //
    double rotation = 0.0;            // Twist around Z-axis (radians)

    // =====================================================================
    // SIMULATION TIME
    // =====================================================================

    double currentTime = 0.0;         // Elapsed time in seconds
    double feedPosition = 0.0; // how much pipe passed through machine 
    // =====================================================================
    // STATUS & CONTROL
    // =====================================================================

    Status status = Status::IDLE;

    // =====================================================================
    // RESET & UPDATE METHODS
    // =====================================================================

    void reset()
    {
        position = { 0, 0, 0 };
        orientation = { 0, 0, 0 };
        rotation = 0.0;               // ? Reset rotation
        currentTime = 0.0;
        status = Status::IDLE;
    }

    void setStatus(Status s)
    {
        status = s;
    }

    // Move forward in direction of orientation
    void feedForward(double distance)
    {
        position.x += distance;  // Move along X-axis
    }

    // Bend - change pitch orientation
    void bendAngle(double angle)
    {
        orientation.y += angle;  // Pitch rotation
    }

    // NOTE: Rotation is updated directly in SimulationController::executeRotate()
};
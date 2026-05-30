#pragma once

#include "Core/BendDirection.h"
#include "Core/Manufacturing/RotationKinematicMode.h"

// =====================================================
// MACHINE RUNTIME STATE
//
// Dynamic machine state.
//
// This represents moving machine/tooling values.
// It does NOT own pipe geometry.
// =====================================================

struct MachineRuntimeState
{
    enum class Status
    {
        IDLE,
        RUNNING,
        PAUSED,
        COMPLETE
    };

    Status status = Status::IDLE;

    double currentTime = 0.0;

    double feedPosition = 0.0;
    double rotationAngle = 0.0;
    double bendAngle = 0.0;

    double currentBendRadius = 0.0;

    BendDirection bendDirection = BendDirection::CCW;

    RotationKinematicMode rotationMode =
        RotationKinematicMode::PipeRoll;

    bool feeding = false;
    bool rotating = false;
    bool bending = false;

    void reset()
    {
        status = Status::IDLE;

        currentTime = 0.0;

        feedPosition = 0.0;
        rotationAngle = 0.0;
        bendAngle = 0.0;

        currentBendRadius = 0.0;

        bendDirection = BendDirection::CCW;
        rotationMode = RotationKinematicMode::PipeRoll;

        feeding = false;
        rotating = false;
        bending = false;
    }
};
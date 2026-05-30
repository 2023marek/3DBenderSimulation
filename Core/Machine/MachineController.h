#pragma once

#include "Core/Machine/MachineModel.h"
#include "Core/Machine/MachineRuntimeState.h"

// =====================================================
// MACHINE CONTROLLER
//
// Owns machine-level state changes.
//
// This class does NOT modify pipe geometry directly.
// It only updates machine runtime state.
// =====================================================

class MachineController
{
public:
    MachineController(
        MachineModel& modelRef,
        MachineRuntimeState& stateRef)
        : model(modelRef),
        state(stateRef)
    {
    }

    void reset()
    {
        state.reset();
    }
    //==========================
    //Methods
    void setStatus(MachineRuntimeState::Status status)
    {
        state.status = status;
    }

    void advanceTime(double dt)
    {
        if (dt > 0.0)
            state.currentTime += dt;
    }
    // =========================
    // FEED
    // =========================

    void beginFeed()
    {
        if (state.feeding)
            return;

        state.feeding = true;
        state.rotating = false;
        state.bending = false;
    }

    void addFeed(double distance)
    {
        state.feedPosition += distance;
    }

    void endFeed()
    {
        state.feeding = false;
    }

    // =========================
    // ROTATE
    // =========================

    void beginRotate()
    {
        if (state.rotating)
            return;

        state.feeding = false;
        state.rotating = true;
        state.bending = false;
    }

    void addRotation(double signedAngle)
    {
        state.rotationAngle += signedAngle;
    }

    void endRotate()
    {
        state.rotating = false;
    }

    // =========================
    // BEND
    // =========================

    void beginBend(
        double radius,
        BendDirection bendDirection)
    {
        if (state.bending)
            return;

        state.feeding = false;
        state.rotating = false;
        state.bending = true;

        state.currentBendRadius = radius;
        state.bendDirection = bendDirection;

        // Bend angle is local to the current bend operation.
        state.bendAngle = 0.0;
    }

    void addBendAngle(double angleIncrement)
    {
        state.bendAngle += angleIncrement;
    }

    void endBend()
    {
        state.bending = false;
    }

private:
    MachineModel& model;
    MachineRuntimeState& state;
};
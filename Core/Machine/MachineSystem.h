#pragma once

#include "Core/Machine/MachineModel.h"
#include "Core/Machine/MachineRuntimeState.h"
#include "Core/Machine/MachineController.h"

// =====================================================
// MACHINE SYSTEM
//
// Aggregates machine model, runtime state, and controller.
//
// SimulationController owns MachineSystem.
// Renderers may read MachineSystem later.
// Pipe simulators should not own machine state long-term.
// =====================================================

class MachineSystem
{
public:
    MachineSystem()
        : controller(model, runtimeState)
    {
    }

    MachineModel& getModel()
    {
        return model;
    }

    const MachineModel& getModel() const
    {
        return model;
    }

    MachineRuntimeState& getRuntimeState()
    {
        return runtimeState;
    }

    const MachineRuntimeState& getRuntimeState() const
    {
        return runtimeState;
    }

    MachineController& getController()
    {
        return controller;
    }

    const MachineController& getController() const
    {
        return controller;
    }

    void reset()
    {
        model.reset();
        controller.reset();
    }

private:
    MachineModel model;
    MachineRuntimeState runtimeState;
    MachineController controller;
};
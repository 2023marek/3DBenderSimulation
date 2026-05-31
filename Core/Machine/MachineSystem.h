#pragma once

#include "Core/Machine/MachineModel.h"
#include "Core/Machine/MachineRuntimeState.h"
#include "Core/Machine/MachineController.h"
#include "Core/Machine/MachineRenderData.h"

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
        reset();
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

    //=========================================
	// RENDER DATA
    MachineRenderData getRenderData() const
    {
        MachineRenderData data;

        data.machineEntryFrame =
            model.machineEntryFrame;

        data.bendDieRadius =
            runtimeState.currentBendRadius > 0.0
            ? runtimeState.currentBendRadius
            : model.defaultBendRadius;

        data.pipeOuterRadius =
            model.pipeOuterRadius;

        data.feedPosition =
            runtimeState.feedPosition;

        data.rotationAngle =
            runtimeState.rotationAngle;

        data.bendAngle =
            runtimeState.bendAngle;

        data.bendDirection =
            runtimeState.bendDirection;

        data.rotationMode =
            runtimeState.rotationMode;

        data.feeding =
            runtimeState.feeding;

        data.rotating =
            runtimeState.rotating;

        data.bending =
            runtimeState.bending;

        double bendSign =
            bendDirectionSign(data.bendDirection);

        data.bendDieCenter =
            model.machineEntryFrame.P
            + model.machineEntryFrame.N * (data.bendDieRadius * bendSign);

        return data;
    }

private:
    MachineModel model;
    MachineRuntimeState runtimeState;
    MachineController controller;
};
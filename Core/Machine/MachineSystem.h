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

        // Copy static machine parts into render snapshot.
        data.parts =
            model.parts;

        // =====================================================
        // MH1.19C — HELIX SUPPORT TOOL
        // =====================================================

        data.supportAxisFrame =
            model.supportAxisFrame;

        data.supportOuterRadius =
            model.supportOuterRadius;

        data.supportVisible =
            true;

      


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

        // Copy static machine parts into render snapshot.
       

      //  std::cout
      //      << "[MH1.19C RENDER SUPPORT DATA]"
       //     << " radius="
     //       << data.supportOuterRadius
      //      << " axisPoint=("
     //       << data.supportAxisFrame.P.x
     //       << ", "
     //       << data.supportAxisFrame.P.y
     //       << ", "
    //        << data.supportAxisFrame.P.z
    //        << ")"
    //        << " axisDir=("
    //        << data.supportAxisFrame.T.x
    //        << ", "
    //        << data.supportAxisFrame.T.y
    //        << ", "
    //        << data.supportAxisFrame.T.z
     //       << ")"
    //        << std::endl;
        return data;
    }

    void setHelixSupportGeometry(
        const Frame& axisFrame,
        double outerRadius)
    {
        if (outerRadius <= 0.0)
            return;

        model.supportAxisFrame =
            axisFrame;

        model.supportOuterRadius =
            outerRadius;
    }


private:
    MachineModel model;
    MachineRuntimeState runtimeState;
    MachineController controller;
};
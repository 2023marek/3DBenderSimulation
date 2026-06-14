#include <sstream>
#include <iostream>
#include <vector>

#include "AppController.h"

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/ManufacturingPlan.h"
#include "Core/Forming/HelixOperation.h"
#include "Core/Forming/HelixFormingPassBuilder.h"
#include "Core/Forming/RotaryDrawPassBuilder.h"

// =====================================
// CONSTRUCTOR
// =====================================
AppController::AppController()
{
    std::vector<Operation> ops =
        buildTestOperations();

    sim.loadProgram(
        ops
    );

    sim.getManufacturingPipe().setIncomingStockLength(
        300.0
    );

    ManufacturingPlan plan =
        buildTestManufacturingPlan(
            ops
        );

    sim.getManufacturingPlanPreview().setPlan(
        plan
    );

    auto& preview =
        sim.getManufacturingPlanPreview();
    preview.setShowInsertionMarker(true);
    preview.setShowInsertionFrame(false);
    preview.setShowTransformedInsertOverlay(true);
   
    configureInitialMode();
}


void AppController::update(double dt)
{
    sim.update(dt);
}

HUDData AppController::buildHUDData() const
{
    HUDData data;

    data.isPlaying = sim.isPlaying();
    data.isPaused = sim.isPaused();
    data.speed = sim.getSpeed();

    const MachineRuntimeState& state = sim.getMachineRuntimeState();

    data.time = state.currentTime;
    data.feedPosition = state.feedPosition;
    data.rotationDeg = state.rotationAngle * 180.0 / PI;
    data.bendDeg = state.bendAngle * 180.0 / PI;

    data.feeding = state.feeding;
    data.rotating = state.rotating;
    data.bending = state.bending;

    data.machineStateName = "IDLE";

    if (state.feeding)
        data.machineStateName = "FEED";
    else if (state.rotating)
        data.machineStateName = "ROTATE";
    else if (state.bending)
        data.machineStateName = "BEND";

    if (data.isPlaying)
        data.status = "PLAYING";
    else if (data.isPaused)
        data.status = "PAUSED";
    else
        data.status = "IDLE";

    auto mode =
        sim.getMode();

    if (mode == SimulationController::SimulationMode::CADPreview)
    {
        data.simulationModeName =
            "CAD PREVIEW";
    }
    else if (mode == SimulationController::SimulationMode::PlannedShapePreview)
    {
        data.simulationModeName =
            "PLANNED SHAPE";
    }
    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        data.simulationModeName =
            "MFG PLAYBACK";
    }
    else
    {
        data.simulationModeName =
            "UNKNOWN";
    }

    data.currentOpIndex = sim.getCurrentOperationIndex();
    data.totalOperations = sim.getTotalOperations();

    data.currentOpProgress = sim.getCurrentOperationProgress();
    data.overallProgress = sim.getOverallProgress();

    if (sim.getMode() == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        data.nodeCount = sim.getManufacturingPipe().getNodes().size();
    }
    else if (sim.getMode() == SimulationController::SimulationMode::CADPreview)
    {
        data.nodeCount = sim.getCadPipeGeometry().getNodes().size();
    }
    else
    {
        data.nodeCount = 0;
    }





    std::ostringstream oss;

    const OperationQueue& queue = sim.getQueue();
    const Operation* currentOp = queue.getCurrent();

    if (currentOp)
    {
        if (currentOp->type == Operation::FEED)
        {
            oss << "FEED " << currentOp->length << " mm";
        }
        else if (currentOp->type == Operation::BEND)
        {
            double angleDeg = currentOp->angle * 180.0 / PI;
            oss << "BEND R=" << currentOp->R << " mm, angle=" << angleDeg << " deg";
        }
        else if (currentOp->type == Operation::ROTATE)
        {
            double angleDeg = currentOp->angle * 180.0 / PI;
            oss << "ROTATE " << angleDeg << " deg";
        }
        else
        {
            oss << "UNKNOWN OPERATION";
        }
    }
    else
    {
        oss << "NO OPERATION";
    }

    data.currentOpName = oss.str();

    return data;
}

void AppController::handleAction(UserAction action)
{
    switch (action)
    {
    case UserAction::Play:
        sim.play();
        break;

    case UserAction::Pause:
        sim.pause();
        break;

    case UserAction::Reset:
        sim.reset();
        break;

    case UserAction::Step:
        sim.step();
        break;

    case UserAction::ToggleRenderMode:
        toggleRenderMode();
        break;
    case UserAction::ToggleSimulationMode:
        toggleSimulationMode();
        break;
    }
}

void AppController::useCADPreview()
{
    sim.setMode(
        SimulationController::SimulationMode::CADPreview
    );
   
}

void AppController::usePlannedShapePreview()
{
    sim.setMode(
        SimulationController::SimulationMode::PlannedShapePreview
    );
    
}

void AppController::useManufacturingPlayback()
{
    sim.setMode(
        SimulationController::SimulationMode::ManufacturingPlayback
    );
    
}

void AppController::toggleSimulationMode()
{
    auto mode =
        sim.getMode();

    if (mode == SimulationController::SimulationMode::CADPreview)
    {
        usePlannedShapePreview();
    }
    else if (mode == SimulationController::SimulationMode::PlannedShapePreview)
    {
        useManufacturingPlayback();
    }
    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        useCADPreview();
    }
}
// Helpers
    std::vector<Operation> AppController::buildTestOperations() const
    {
        std::vector<Operation> ops;

        Operation op1;
        op1.type = Operation::FEED;
        op1.length = 198.0;

        Operation op2;
        op2.type = Operation::BEND;
        op2.R = 20.0;
        op2.angle = PI / 2.0;
        op2.bendDirection = BendDirection::CCW;

        Operation op3;
        op3.type = Operation::ROTATE;
        op3.angle = PI / 2.0;
        op3.rotationDirection = RotationDirection::CCW;

        Operation op4;
        op4.type = Operation::FEED;
        op4.length = 110.0;

        ops.push_back(op1);
        ops.push_back(op2);
        ops.push_back(op3);
        ops.push_back(op4);

        return ops;
    }

    ManufacturingPlan AppController::buildTestManufacturingPlan(
        const std::vector<Operation>& ops) const
    {
        ManufacturingPass rotaryPass =
            RotaryDrawPassBuilder::buildPass(
                ops,
                "Rotary draw bending pass"
            );

        HelixOperation helixPassOp;

        helixPassOp.inputMode =
            HelixOperation::InputMode::RadiusPitch;

        helixPassOp.length = 200.0;
        helixPassOp.helixRadius = 10.0;
        helixPassOp.pitch = 15.0;
        helixPassOp.feedSpeed = 40.0;

        ManufacturingPass helixPass =
            HelixFormingPassBuilder::buildPass(
                helixPassOp,
                "Heating element helix pass"
            );

        helixPass.placement =
            PassPlacement::atArcLength(
                202.0
            );

        ManufacturingPlan plan;

        plan.addPass(
            rotaryPass
        );

        plan.addPass(
            helixPass
        );

        return plan;
    }

    void AppController::configureInitialMode()
    {
        usePlannedShapePreview();

        // Alternatives for testing:
        //
        // useManufacturingPlayback();
        // useCADPreview();
    }

   



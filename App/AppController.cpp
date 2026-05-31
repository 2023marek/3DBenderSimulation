#include <sstream>
#include "AppController.h"


// =====================================
// CONSTRUCTOR
// =====================================
AppController::AppController()
{
    std::vector<Operation> ops;

    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 120;

    Operation op2;
    op2.type = Operation::BEND;
    op2.R = 20;
    op2.angle = PI / 2.0;
    op2.bendDirection = BendDirection::CCW;

    Operation op3;
    op3.type = Operation::ROTATE;
    op3.angle = PI / 2.0;
    op3.rotationDirection = RotationDirection::CW;

    Operation op4;
    op4.type = Operation::BEND;
    op4.R = 20;
    op4.angle = PI / 2.0;
    op4.bendDirection = BendDirection::CW;

    ops.push_back(op1);
    ops.push_back(op2);
    ops.push_back(op3);
    ops.push_back(op4);
    sim.loadProgram(ops);


   // sim.setMode(
   //SimulationController::SimulationMode::ManufacturingPlayback
   // );

    // Or for CAD preview:
    //
     sim.setMode(
     SimulationController::SimulationMode::CADPreview
     );

    

    std::cout << "Playing: " << sim.isPlaying() << std::endl;
    std::cout << "Progress: " << sim.getOverallProgress() << std::endl;
    std::cout << "nodes: "
        << sim.getManufacturingPipe().getNodes().size()
        << std::endl;
    std::cout << "CurrentIdx: " << sim.getCurrentOperationIndex() << std::endl;
}
// =====================================
// UPDATE (called every frame)
// =====================================
void AppController::update(double dt)
{
    sim.update(dt);
    std::cout << "Playing: " << sim.isPlaying() << std::endl;
    std::cout << "Ops: " << sim.getTotalOperations() << std::endl;
    std::cout << "CurrentIdx: " << sim.getCurrentOperationIndex() << std::endl;
}

// =====================================
// GEOMETRY ACCESS
// =====================================


// =====================================
// HUD DATA BUILDER (translator layer)
// =====================================
HUDData AppController::buildHUDData() const
{
    HUDData data;

    // ===== BASIC STATE =====
    data.isPlaying = sim.isPlaying();
    data.isPaused = sim.isPaused();

    data.speed = sim.getSpeed();

    const MachineRuntimeState& state =
        sim.getMachineRuntimeState();

    data.time =
        state.currentTime;

    data.feedPosition =
        state.feedPosition;

    data.rotationDeg =
        state.rotationAngle * 180.0 / 3.141592653589793;

    data.bendDeg =
        state.bendAngle * 180.0 / 3.141592653589793;

    data.feeding =
        state.feeding;

    data.rotating =
        state.rotating;

    data.bending =
        state.bending;

    // ===== MACHINE STATE NAME =====
    data.machineStateName = "IDLE";

    if (state.feeding)
    {
        data.machineStateName = "FEED";
    }
    else if (state.rotating)
    {
        data.machineStateName = "ROTATE";
    }
    else if (state.bending)
    {
        data.machineStateName = "BEND";
    }

    // ===== DISPLAY STATUS =====
    if (data.isPlaying)
    {
        data.status = "PLAYING";
    }
    else if (data.isPaused)
    {
        data.status = "PAUSED";
    }
    else
    {
        data.status = "IDLE";
    }

    // ===== OPERATIONS =====
    data.currentOpIndex =
        sim.getCurrentOperationIndex();

    data.totalOperations =
        sim.getTotalOperations();

    // ===== PROGRESS =====
    data.currentOpProgress =
        sim.getCurrentOperationProgress();

    data.overallProgress =
        sim.getOverallProgress();

    // ===== GEOMETRY =====
    if (sim.getMode()
        == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        data.nodeCount =
            sim.getManufacturingPipe().getNodes().size();
    }
    else if (sim.getMode()
        == SimulationController::SimulationMode::CADPreview)
    {
        data.nodeCount =
            sim.getCadPipeGeometry().getNodes().size();
    }
    else
    {
        data.nodeCount = 0;
    }
    //

    std::ostringstream oss; 

    const OperationQueue& queue = sim.getQueue();
    const Operation* currentOp = queue.getCurrent();

    if (currentOp)
    {
        if (currentOp->type == Operation::FEED)
        {
            oss << "FEED " << currentOp->length << "mm";
        }
        else if (currentOp->type == Operation::BEND)
        {
            double angleDeg = currentOp->angle * 180.0 / 3.141592653589793;
            oss << "BEND R=" << currentOp->R << "mm, angle=" << angleDeg << "deg";
        }
        else if (currentOp->type == Operation::ROTATE)
        {
            double angleDeg = currentOp->angle * 180.0 / 3.141592653589793;
            oss << "ROTATE " << angleDeg << "deg";
        }
    }

    data.currentOpName = oss.str();




    // ===== STATUS =====
    if (data.isPlaying)
        data.status = "PLAYING";
    else if (data.isPaused)
        data.status = "PAUSED";
    else
        data.status = "IDLE";

   

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
    }
}
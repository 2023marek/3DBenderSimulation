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
    op1.length = 100;

    Operation op2;
    op2.type = Operation::BEND;
    op2.R = 5;
    op2.angle = 3.1415 / 2;

    Operation op3;
	op3.type = Operation::ROTATE;
	op3.angle = 3.1415 / 2; // Rotate 90 degrees

    Operation op4;
    op4.type = Operation::FEED;
    op4.length = 50;

    Operation op5;
    op5.type = Operation::BEND;
    op5.R = 1500;
    op5.angle = 3.1415 /1;

    ops.push_back(op1);
    ops.push_back(op2);
    ops.push_back(op3);
    ops.push_back(op4);
	ops.push_back(op5);

    sim.loadProgram(ops);

    

    std::cout << "Playing: " << sim.isPlaying() << std::endl;
    std::cout << "Progress: " << sim.getOverallProgress() << std::endl;
    std::cout << "nodes: " << sim.getPipeGeometry().getNodes().size() << std::endl;
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
const PipeAxis3D& AppController::getPipeGeometry() const
{
    return sim.getPipeGeometry();
    

}

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

    const MachineState& state = sim.getState();
    data.time = state.currentTime;
    data.rotationDeg = state.rotation * 180.0 / 3.141592653589793;

    // ===== OPERATIONS =====
    data.currentOpIndex = sim.getCurrentOperationIndex();
    data.totalOperations = sim.getTotalOperations();
    

    // ===== PROGRESS =====
    data.currentOpProgress = sim.getCurrentOperationProgress();
    data.overallProgress = sim.getOverallProgress();

    // ===== GEOMETRY =====
    data.nodeCount = sim.getPipeGeometry().getNodes().size();
    
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
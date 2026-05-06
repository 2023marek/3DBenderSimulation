#include <sstream>
#include "AppController.h"
#include "Core/PipeUtils.h"


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
    op2.R = 50;
    op2.angle = 3.1415 / 2;

    Operation op3;
    op3.type = Operation::FEED;
    op3.length = 100;

    ops.push_back(op1);
    ops.push_back(op2);
    ops.push_back(op3);

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
    std::cout << "[SIM UPDATE] dt=" << dt << std::endl;
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

void AppController::buildRenderData(
    std::vector<Vec3D>& points,
    std::vector<Vec3D>& tangents
) const
{
    points.clear();
    tangents.clear();

    // 1. Get full nodes
    const auto& nodes = sim.getPipeGeometry().getNodes();

    if (nodes.empty()) return;

    // 2. Clip by current feed length ??
    auto clippedNodes = clipByLength(nodes, sim.getTotalFedLength());

    // 3. Convert to rendering format
    for (const auto& n : clippedNodes)
    {
        points.push_back(n.pos);
        tangents.push_back(n.T);
    }
}
double AppController::getTotalFedLength() const
{
    return sim.getTotalFedLength();
}
#include <sstream>
#include "AppController.h"

// =====================================
// CONSTRUCTOR
// =====================================
AppController::AppController()
{
    // For now empty (later: load program, init state)
}

// =====================================
// UPDATE (called every frame)
// =====================================
void AppController::update(double dt)
{
    sim.update(dt);
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
#include "AppController.h"
#include <iostream>

AppController::AppController()
{
    std::cout << "AppController constructed\n";
}


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

    // ===== STATUS STRING =====
    if (data.isPlaying)
        data.status = "PLAYING";
    else if (data.isPaused)
        data.status = "PAUSED";
    else
        data.status = "IDLE";

    return data;
}
#pragma once

#include "OperationQueue.h"
#include "PipeAxis3D.h"
#include "Machine/MachineState.h"
#include <vector>

class SimulationController
{
public:
    // =========================================
    // PUBLIC INTERFACE (unchanged from Phase 3A)
    // =========================================

    SimulationController();
    void loadProgram(const std::vector<Operation>& ops);
    void play();
    void pause();
    void step();
    void reset();
    void update(double deltaTime);

    const MachineState& getState() const { return machineState; }
    MachineState& getState() { return machineState; }
    const PipeAxis3D& getPipeGeometry() const { return pipeGeometry; }
    const OperationQueue& getQueue() const { return operationQueue; }
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    double getSpeed() const { return speed; }
    void setSpeed(double speedMmPerSec) { speed = speedMmPerSec; }

    double getCurrentOperationProgress() const;
    double getOverallProgress() const;
    size_t getCurrentOperationIndex() const { return operationQueue.getCurrentIndex(); }
    size_t getTotalOperations() const { return operationQueue.getTotalOperations(); }

private:
    // =========================================================================
    // PLAYBACK STATE
    // =========================================================================
    bool playing;              // Currently executing
    bool paused;               // Paused mid-execution
    double speed;              // mm/second

    // =========================================================================
    // PROGRESS TRACKING
    // =========================================================================
    //
    // These variables track progress within the CURRENT operation
    // They are reset to 0 when moving to the next operation
    //

    double accumulatedDistance;  // mm done in current FEED
    double accumulatedAngle;     // radians done in current BEND

    // =========================================================================
    // ROTATION TRACKING (NEW FOR PHASE 3B)
    // =========================================================================
    //
    // PURPOSE: Track twist rotation during ROTATE operations
    //
    // HOW IT WORKS:
    //   When executing ROTATE operation:
    //   • accumulatedRotation starts at 0.0
    //   • Increases by angleIncrement each frame
    //   • When >= operation.angle, operation completes
    //   • Reset to 0.0 when advancing to next operation
    //
    // EXAMPLE:
    //   Operation: ROTATE 90° (?/2 radians)
    //   Frame 1: accumulatedRotation = 0.5 rad (32%)
    //   Frame 2: accumulatedRotation = 1.0 rad (64%)
    //   Frame 3: accumulatedRotation = 1.57 rad (100% - DONE!)
    //
    double accumulatedRotation = 0.0;  // ? NEW: radians

    // =========================================================================
    // CORE COMPONENTS
    // =========================================================================
    OperationQueue operationQueue;    // Queue of operations to execute
    MachineState machineState;        // Current machine state
    PipeAxis3D pipeGeometry;          // Pipe geometry (segments + nodes)

    // =========================================================================
    // LOCAL OPERATION STORAGE (for geometry accumulation)
    // =========================================================================
    std::vector<Operation> loadedOperations;  // Copy of all operations

    // =========================================================================
    // INTERNAL EXECUTION METHODS
    // =========================================================================

    void executeOperation(double distance);
    void executeFeed(double distance);
    void executeBend(double angle);

    // =========================================================================
    // ROTATION EXECUTION (NEW FOR PHASE 3B)
    // =========================================================================
    //
    // Executes a ROTATE operation - twists pipe around longitudinal axis
    //
    // CONCEPT: Imagine twisting a rubber tube around its centerline
    //
    //   Before:        During:        After:
    //   ????           /|\           ????
    //   ????    ?      / \    ?      ????
    //   ????           \ /           ????
    //                   \|/
    //
    // KEY INSIGHT:
    //   • Position (P) doesn't change
    //   • Only frame orientation changes
    //   • Normal and Binormal vectors rotate
    //   • Tangent stays the same
    //
    void executeRotate(double angleIncrement);

    void advanceToNextOperation();
    void updatePipeGeometry();
};
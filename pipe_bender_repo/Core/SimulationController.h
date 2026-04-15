#pragma once

#include "OperationQueue.h"
#include "PipeAxis3D.h"
#include "../Machine/MachineState.h"
#include <vector>

/*
================================================================================
  SIMULATION CONTROLLER - Orchestrates pipe bending simulation
================================================================================

  This class manages the overall simulation state and coordinates:
    • Operation execution (FEED/BEND operations)
    • Machine state updates
    • Simulation playback (play, pause, step, reset)
    • Pipe geometry updates
    • Progress tracking

  ARCHITECTURE:

  ???????????????????????????????????????
  ?   SimulationController              ?  (THIS CLASS)
  ?   - Manages overall sim state       ?
  ?   - Orchestrates execution flow     ?
  ???????????????????????????????????????
           ?         ?         ?
    ???????????? ???????????? ????????????
    ?Operation ? ?Operation ? ?  Pipe    ?
    ?  Queue   ? ?  Exec    ? ? Geometry ?
    ???????????? ???????????? ????????????
           ?         ?         ?
    ????????????????????????????????????
    ?     MachineState                 ?
    ?  (position, orientation, time)   ?
    ????????????????????????????????????

================================================================================
*/

class SimulationController
{
public:
    // =========================================
    // INITIALIZATION
    // =========================================

    /// Constructor - Initialize with default values
    SimulationController();

    // =========================================
    // PROGRAM MANAGEMENT
    // =========================================

    /// Load a program (list of FEED/BEND operations)
    /// Resets all progress and state
    /// @param ops Vector of Operation structs to execute
    void loadProgram(const std::vector<Operation>& ops);

    // =========================================
    // PLAYBACK CONTROL
    // =========================================

    /// Start simulation playback
    /// Sets status to RUNNING if not already complete
    void play();

    /// Pause simulation (can resume with play())
    void pause();

    /// Execute one small step of current operation
    /// Useful for manual debugging
    void step();

    /// Reset all progress, state, and geometry
    /// Allows re-running program from beginning
    void reset();

    // =========================================
    // MAIN UPDATE LOOP (called from render loop)
    // =========================================

    /// Update simulation state based on elapsed time
    /// @param deltaTime Time since last frame (seconds)
    /// 
    /// This is the PRIMARY update function:
    ///   1. Calculates distance to move this frame (speed * deltaTime)
    ///   2. Executes current operation by that amount
    ///   3. Checks if current operation is complete
    ///   4. Advances to next operation if needed
    ///   5. Updates pipe geometry for rendering
    void update(double deltaTime);

    // =========================================
    // STATE QUERIES
    // =========================================

    /// Get current machine state (position, orientation, status, etc.)
    const MachineState& getState() const { return machineState; }

    /// Get mutable access to machine state
    MachineState& getState() { return machineState; }

    /// Get pipe geometry (nodes for rendering)
    const PipeAxis3D& getPipeGeometry() const { return pipeGeometry; }

    /// Get operation queue
    const OperationQueue& getQueue() const { return operationQueue; }

    /// Check if simulation is currently playing
    bool isPlaying() const { return playing; }

    /// Check if simulation is paused
    bool isPaused() const { return paused; }

    /// Get simulation playback speed (mm/s)
    double getSpeed() const { return speed; }

    /// Set simulation playback speed
    /// @param speedMmPerSec Speed in mm/second
    void setSpeed(double speedMmPerSec) { speed = speedMmPerSec; }

    // =========================================
    // PROGRESS TRACKING
    // =========================================

    /// Get progress of current operation (0.0 to 1.0)
    double getCurrentOperationProgress() const;

    /// Get overall program progress (0.0 to 1.0)
    double getOverallProgress() const;

    /// Get current operation index
    size_t getCurrentOperationIndex() const { return operationQueue.getCurrentIndex(); }

    /// Get total number of operations
    size_t getTotalOperations() const { return operationQueue.getTotalOperations(); }

private:
    // =========================================
    // INTERNAL STATE
    // =========================================

    // --- Playback State ---
    bool playing;              // Currently executing
    bool paused;               // Paused mid-execution
    double speed;              // mm/second

    // --- Progress Tracking ---
    double accumulatedDistance;  // mm done in current FEED operation
    double accumulatedAngle;     // radians done in current BEND operation

    // --- Core Components ---
    OperationQueue operationQueue;    // Queue of operations to execute
    MachineState machineState;        // Current machine state
    PipeAxis3D pipeGeometry;          // Pipe geometry (segments + nodes)

    // =========================================
    // INTERNAL EXECUTION (called by update())
    // =========================================

    /// Execute current operation by the specified distance/angle
    /// @param distance mm to advance in this frame
    void executeOperation(double distance);

    /// Execute FEED operation (linear motion)
    /// @param distance mm to move forward
    void executeFeed(double distance);

    /// Execute BEND operation (curved motion)
    /// @param angle radians to bend in this frame
    void executeBend(double angle);

    /// Advance to the next operation in queue
    /// Updates status and progress
    void advanceToNextOperation();

    /// Update pipe geometry for rendering
    /// Regenerates nodes from current state
    void updatePipeGeometry();
};


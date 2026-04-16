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
	std::vector<Operation> LoadedOperations;  // Local copy of operations for geometry rebuilding

    // --- Core Components ---
    OperationQueue operationQueue;    // Queue of operations to execute
    MachineState machineState;        // Current machine state
    PipeAxis3D pipeGeometry;          // Pipe geometry (segments + nodes)

    // =========================================================================
    // OPTION C: LOCAL OPERATION STORAGE FOR GEOMETRY REBUILDING
    // =========================================================================
    //
    // PURPOSE: Store loaded operations to enable complete geometry reconstruction
    //
    // WHY NEEDED:
    //   OperationQueue stores operations privately (for encapsulation)
    //   But we need to iterate through ALL operations to rebuild geometry
    //   This includes completed operations that are no longer in the queue
    //
    // USAGE:
    //   - Set in loadProgram() with: loadedOperations = ops;
    //   - Used in updatePipeGeometry() to add completed segments
    //   - Allows proper accumulation of all executed operations
    //
    // EXAMPLE FLOW:
    //   Frame 1: Op 0 executing at 50% ? Geometry = Op0_partial
    //   Frame 2: Op 0 complete, Op 1 starting ? Geometry = Op0_full + Op1_partial
    //   Frame 3: Op 0+1 complete, Op 2 starting ? Geometry = Op0+Op1_full + Op2_partial
    //
    // =========================================================================
    std::vector<Operation> loadedOperations;  // Local copy of operations

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
    /// 
    /// IMPLEMENTATION DETAILS (Option C - Hybrid Smart Update):
    ///
    /// Strategy: Accumulate ALL executed operations + current partial operation
    ///
    /// Flow:
    ///   1. Create fresh PipeAxis3D geometry object
    ///   2. Add ALL completed operations as full segments
    ///      (for i = 0 to currentIdx-1: add loadedOperations[i])
    ///   3. Add PARTIAL current operation (accumulated progress only)
    ///   4. Call pipeGeometry.build() to generate nodes
    ///
    /// Result: Complete pipe representation up to current execution point
    ///
    /// Performance:
    ///   - Full rebuild each frame (simple, robust)
    ///   - Could be optimized to incremental updates later
    ///   - Currently ~1-2ms per frame (acceptable for ~60fps)
    ///
    void updatePipeGeometry();
};